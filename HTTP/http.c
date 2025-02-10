#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

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
} HTTP_Request;

void log_msg(const char *prefix, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[%s] ", prefix);
    vprintf(format, args);
    va_end(args);
}

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

    const char *route_start = request + 4;
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

    return result;
}

void send_response(int client_socket, char *status, char *content) {
    size_t len = strlen(content);
    char *request;
    
    request = malloc(len + 50); 
    snprintf(request, len + 50, "HTTP/1.1 %s\r\nContent-Length: %zu\r\n\r\n%s", status, len, content);

    send(client_socket, request, strlen(request), 0);
}

void handle_client(int client_socket) {
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
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("recv");
        return;
    }
    
    buffer[bytes_received] = '\0';

    HTTP_Request req = parse_http_request(buffer);
    printf("New Request: %s:%d: %s\n", client_ip_address, client_port, req.route);
    if (strncmp(req.route, "/", strlen(req.route)) == 0) {
        FILE *fp = fopen("index.html", "r");
        if (!fp) {
            send_response(client_socket, "404 NOT_FOUND", "File not found!");
        } else {
            char content[1024];
            size_t len = fread(content, 1, sizeof(content) - 1, fp);
            content[len] = '\0';
            send_response(client_socket, "200 OK", content);
        }
    } else if (strncmp(req.route, "/server_info/", strlen(req.route)) == 0 || strncmp(req.route, "/server_info", strlen(req.route)) == 0) {
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge\">MicroForge/HTTP</a>");
    } else {
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "%s.html", req.route + 1);  
        FILE *fp = fopen(file_path, "r");
        if (!fp) {
            send_response(client_socket, "404 NOT_FOUND", "File not found!");
        } else {
            char content[1024];
            size_t len = fread(content, 1, sizeof(content) - 1, fp);
            content[len] = '\0';
            send_response(client_socket, "404 NOT_FOUND", content);
        }
    }

    free(req.host);
    free(req.route);
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in server_addr;
    
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
    
    char *server_ip_address = inet_ntoa(server_addr.sin_addr);
    int server_port = ntohs(server_addr.sin_port);
    printf("Running at %s:%d...\n", server_ip_address, server_port);

    while (1) {
        if ((client_socket = accept(server_fd, NULL, NULL)) < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(client_socket);
            exit(0);
        } else if (pid > 0) {
            close(client_socket);
        } else {
            perror("fork");
        }

    }

    close(server_fd);
    return 0;
}

