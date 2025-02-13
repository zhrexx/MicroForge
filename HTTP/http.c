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
#include <ctype.h> 

#define PORT 8080
#define LOG_IP_ENABLED 1
#define BUFFER_SIZE (6 * 1024 * 1024)
#define MAX_LINES 1024
#define MAX_TOKENS 256
#define MAX_LENGTH 1024

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
    char *extracted_ip;
    char *content_type;
} HTTP_Request;

char **blocklist = NULL;
int block_count = 0;

int load_blocklist(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    blocklist = malloc(MAX_LINES * sizeof(char *));
    if (!blocklist) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    char buffer[MAX_LENGTH];
    while (fgets(buffer, sizeof(buffer), file) && block_count < MAX_LINES) {
        buffer[strcspn(buffer, "\n")] = 0;

        char *token = strtok(buffer, " ");
        while (token && block_count < MAX_LINES) {
            blocklist[block_count] = strdup(token);
            if (!blocklist[block_count]) {
                perror("Memory allocation failed");
                fclose(file);
                return -1;
            }
            block_count++;
            token = strtok(NULL, " ");
        }
    }

    fclose(file);
    return block_count;
}

void free_blocklist() {
    for (int i = 0; i < block_count; i++) {
        free(blocklist[i]);
    }
    free(blocklist);
}


const char* get_mime_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "application/octet-stream";
    
    char ext[32];
    size_t i = 0;
    dot++;
    while (*dot && i < sizeof(ext) - 1) {
        ext[i++] = tolower(*dot++);
    }
    ext[i] = '\0';
    
    if (strcmp(ext, "html") == 0) return "text/html";
    if (strcmp(ext, "css") == 0) return "text/css";
    if (strcmp(ext, "js") == 0) return "application/javascript";
    if (strcmp(ext, "png") == 0) return "image/png";
    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, "gif") == 0) return "image/gif";
    if (strcmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcmp(ext, "ico") == 0) return "image/x-icon";
    if (strcmp(ext, "pdf") == 0) return "application/pdf";
    if (strcmp(ext, "txt") == 0) return "text/plain";
    if (strcmp(ext, "xml") == 0) return "application/xml";
    if (strcmp(ext, "json") == 0) return "application/json";
    
    return "application/octet-stream";
}


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
    const char *xff_header = strstr(request, "X-Forwarded-For: ");
    if (xff_header) {
        result.extracted_ip = str_dup_until(xff_header + 17, '\r');
    }
    else {
        result.extracted_ip = strdup("NOTPROVIDED");
    }

    const char *content_type = strstr(request, "Content-Type: ");
    result.content_type = strdup(content_type);

    if (result.method == HM_POST) {
        const char *body = strstr(request, "\r\n\r\n");
        if (body) {
            body += 4;
            result.body = strdup(body);
            
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

void send_file_response(int client_socket, char *status, const char *filepath
#ifdef SSL_ENABLE
    , SSL *ssl
#endif
) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        char *not_found = "404 Not Found";
#ifdef SSL_ENABLE
        send_response(client_socket, "404 Not Found", not_found, ssl);
#else
        send_response(client_socket, "404 Not Found", not_found);
#endif
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    const char *mime_type = get_mime_type(filepath);
    
    char header[512];
    snprintf(header, sizeof(header), 
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Cache-Control: public, max-age=31536000\r\n"
        "\r\n", 
        status, mime_type, file_size);
        
#ifdef SSL_ENABLE
    SSL_write(ssl, header, strlen(header));
#else
    send(client_socket, header, strlen(header), 0);
#endif

    char *buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        fclose(fp);
        return;
    }

    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
#ifdef SSL_ENABLE
        SSL_write(ssl, buffer, bytes_read);
#else
        send(client_socket, buffer, bytes_read, 0);
#endif
    }
    
    free(buffer);
    fclose(fp);
}

int check_ip_address(char *ip) {
    for (int i = 0; i < block_count; i++) {
        if (strncmp(ip, blocklist[i], strlen(blocklist[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

void handle_client(int client_socket
#ifdef SSL_ENABLE
    , SSL_CTX *ctx
#endif
) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    

    if (getpeername(client_socket, (struct sockaddr *)&client_addr, &client_len) < 0) {
        perror("getpeername failed");
        close(client_socket);
        return;
    }
    
    char *client_ip_address = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);

    time_t rawtime;
    struct tm *timeinfo;
    char time_str[20];
    
    time(&rawtime); 
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%d-%m %H:%M", timeinfo);
    
    if (check_ip_address(client_ip_address)) {
        printf("[%s:%d %s] Blocked Connection: Banned IP address\n", client_ip_address, client_port, time_str);
        send_response(client_socket, "404 BLOCKED_IP_ADDRESS", "Your IP address is blocked!");
        close(client_socket);
        return;
    }

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
    

    HTTP_Request req = parse_http_request(buffer);
    if (LOG_IP_ENABLED) {
        printf("[%s:%d %s] %s %s\n", client_ip_address, client_port, time_str, method_to_str(req.method), req.route);
    } else {
        printf("[%s] %s %s\n", time_str, method_to_str(req.method), req.route);
    }

    char file_path[512];
    if (strcmp(req.route, "/") == 0) {
        strcpy(file_path, "index.html");
    } else if (strcmp(req.route, "/server_info") == 0 || strcmp(req.route, "/server_info/") == 0) {
#ifdef SSL_ENABLE
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>", ssl);
#else
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>");
#endif
        goto cleanup;
    } else {
        snprintf(file_path, sizeof(file_path), "%s", req.route + 1);
    }

#ifdef SSL_ENABLE
    send_file_response(client_socket, "200 OK", file_path, ssl);
#else
    send_file_response(client_socket, "200 OK", file_path);
#endif

cleanup:
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
    if (load_blocklist("BLOCKLIST") < 0) {
        return 1;
    } 

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
    free_blocklist();
    return 0;
}
