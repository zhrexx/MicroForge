#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define LOG_IP_ENABLED 1

#ifdef SSL_ENABLE
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

typedef enum {
    HM_GET,
    HM_POST,
} HTTP_Method;

typedef struct {
    char *key;
    char *value;
} HTTP_Parameter;

typedef struct {
    HTTP_Method method;
    char *route;
    HTTP_Parameter *parameters;
    int param_count;
    char *host;
    char *body;
} HTTP_Request;

void log_msg(const char *prefix, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[%s] ", prefix);
    vprintf(format, args);
    va_end(args);
}

#ifdef SSL_ENABLE
void init_ssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX *create_ssl_context() {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("Unable to create SSL context");
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void configure_ssl_context(SSL_CTX *ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        perror("SSL certificate error");
        exit(EXIT_FAILURE);
    }
}
#endif

char *str_dup_until(const char *start, char stop) {
    const char *end = strchr(start, stop);
    if (!end) end = start + strlen(start);
    
    size_t len = end - start;
    char *copy = malloc(len + 1);
    strncpy(copy, start, len);
    copy[len] = '\0';
    return copy;
}

HTTP_Request parse_http_request(const char *request) {
    HTTP_Request result = {0};
    
    if (strncmp(request, "GET ", 4) == 0) {
        result.method = HM_GET;
    } else if (strncmp(request, "POST ", 5) == 0) {
        result.method = HM_POST;
    } else {
        log_msg("ERROR", "Unsupported HTTP method\n");
        return result;
    }

    const char *route_start = request + (result.method == HM_POST ? 5 : 4);
    result.route = str_dup_until(route_start, ' '); 

    char *query = strchr(result.route, '?');
    if (query) {
        *query = '\0'; 
        query++;
        int count = 0;
        for (char *p = query; *p; p++) {
            if (*p == '&') count++;
        }
        result.param_count = count + 1;
        result.parameters = malloc(sizeof(HTTP_Parameter) * result.param_count);

        char *pair = strtok(query, "&");
        int i = 0;
        while (pair && i < result.param_count) {
            result.parameters[i].key = str_dup_until(pair, '=');
            result.parameters[i].value = strdup(strchr(pair, '=') + 1);
            pair = strtok(NULL, "&");
            i++;
        }
    }

    const char *host_header = strstr(request, "Host: ");
    if (host_header) {
        result.host = str_dup_until(host_header + 6, '\n');
    }

    if (result.method == HM_POST) {
        const char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4;
            result.body = strdup(body);
            
            const char *content_type = strstr(request, "Content-Type: ");
            if (content_type && strstr(content_type, "application/x-www-form-urlencoded")) {
                int count = 0;
                for (const char *p = body; *p; p++) {
                    if (*p == '&') count++;
                }
                result.param_count = count + 1;
                result.parameters = malloc(sizeof(HTTP_Parameter) * result.param_count);

                char *body_copy = strdup(body);
                char *pair = strtok(body_copy, "&");
                int i = 0;
                while (pair && i < result.param_count) {
                    result.parameters[i].key = str_dup_until(pair, '=');
                    result.parameters[i].value = strdup(strchr(pair, '=') + 1);
                    pair = strtok(NULL, "&");
                    i++;
                }
                free(body_copy);
            }
        }
    }

    return result;
}

#ifdef SSL_ENABLE
void send_response(int client_socket, char *status, char *content, SSL *ssl) {
#else
void send_response(int client_socket, char *status, char *content) {
#endif
    size_t len = strlen(content);
    char *response = malloc(len + 50);
    snprintf(response, len + 50, "HTTP/1.1 %s\r\nContent-Length: %zu\r\n\r\n%s", status, len, content);
    
#ifdef SSL_ENABLE
    SSL_write(ssl, response, strlen(response));
#else
    send(client_socket, response, strlen(response), 0);
#endif

    free(response);
}

int check_route(char *route, char *exroute) {
    int x = strncmp(route, exroute, strlen(route));
    if (x == 0) {
        return 1;
    } else {
        return 0;
    }
}

char *method_to_str(HTTP_Method method) {
    if (method == 0) return "GET";
    if (method == 1) return "POST";
    else return "Unknown";
}

#ifdef SSL_ENABLE
void handle_client(int client_socket, SSL_CTX *ctx) {
#else
void handle_client(int client_socket) {
#endif
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    if (getpeername(client_socket, (struct sockaddr *)&client_addr, &client_len) < 0) {
        perror("getpeername failed");
        close(client_socket);
        exit(1);
    }
    
    char *client_ip_address = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

#ifdef SSL_ENABLE 
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);
    if (SSL_accept(ssl) <= 0) {
        SSL_free(ssl);
        close(client_socket);
        return;
    }
    bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
#else
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
#endif 

    if (bytes_received <= 0) {
        perror("recv");
        #ifdef SSL_ENABLE
        SSL_free(ssl);
        #endif
        close(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    time_t rawtime;
    struct tm *timeinfo;
    char time_str[20];
    
    time(&rawtime); 
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%d-%m %H:%M", timeinfo);

    HTTP_Request req = parse_http_request(buffer);
    if (LOG_IP_ENABLED) {
        printf("[%s:%d %s] %s %s\n", client_ip_address, client_port, time_str, method_to_str(req.method), req.route);
    } else {
        printf("[%s] %s %s\n", time_str, method_to_str(req.method), req.route);
    }

    if (check_route(req.route, "/")) {
        FILE *fp = fopen("index.html", "r");
        if (!fp) {
            #ifdef SSL_ENABLE
            send_response(client_socket, "404 NOT_FOUND", "File not found!", ssl);
            #else
            send_response(client_socket, "404 NOT_FOUND", "File not found!");
            #endif
        } else {
            char content[1024];
            size_t len = fread(content, 1, sizeof(content) - 1, fp);
            content[len] = '\0';
            fclose(fp);
            #ifdef SSL_ENABLE
            send_response(client_socket, "200 OK", content, ssl);
            #else
            send_response(client_socket, "200 OK", content);
            #endif
        }
    } else if (check_route(req.route, "/server_info/") || check_route(req.route, "/server_info")) {
        #ifdef SSL_ENABLE
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>", ssl);
        #else
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>");
        #endif
    } else {
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s.html", req.route + 1);  
        FILE *fp = fopen(file_path, "r");
        if (!fp) {
            #ifdef SSL_ENABLE
            send_response(client_socket, "404 NOT_FOUND", "File not found!", ssl);
            #else
            send_response(client_socket, "404 NOT_FOUND", "File not found!");
            #endif
        } else {
            char content[1024];
            size_t len = fread(content, 1, sizeof(content) - 1, fp);
            content[len] = '\0';
            fclose(fp);
            #ifdef SSL_ENABLE
            send_response(client_socket, "200 OK", content, ssl);
            #else
            send_response(client_socket, "200 OK", content);
            #endif
        }
    }

    free(req.host);
    free(req.route);
    if (req.parameters) {
        for (int i = 0; i < req.param_count; i++) {
            free(req.parameters[i].key);
            free(req.parameters[i].value);
        }
        free(req.parameters);
    }
    if (req.body) {
        free(req.body);
    }

#ifdef SSL_ENABLE 
    SSL_shutdown(ssl);
    SSL_free(ssl);
#endif
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr;
    
#ifdef SSL_ENABLE 
    SSL_CTX *ctx;
    init_ssl();
    ctx = create_ssl_context();
    configure_ssl_context(ctx);
#endif 

    double VERSION = 1.0;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("-------------------------------------------------------------------------------------\n");
    printf("MicroForgeHTTP\n");
    char *server_ip_address = inet_ntoa(server_addr.sin_addr);
    int server_port = ntohs(server_addr.sin_port);
    printf("- Version: %.1f\n", VERSION);
    printf("- IP: %s:%d\n", server_ip_address, server_port);
    printf("-------------------------------------------------------------------------------------\n");
    printf(" LOGS:\n");
    printf("-------------------------------------------------------------------------------------\n");

    while (1) {
        if ((client_socket = accept(server_fd, NULL, NULL)) < 0) {
            perror("accept");
            continue;
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            #ifdef SSL_ENABLE
            handle_client(client_socket, ctx);
            #else
            handle_client(client_socket);
            #endif
            exit(0);
        } else if (pid > 0) {
            close(client_socket);
        } else {
            perror("fork");
        }
    }

#ifdef SSL_ENABLE 
    SSL_CTX_free(ctx);
#endif 
    close(server_fd);
    return 0;
}
