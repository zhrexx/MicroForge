#include "hapi.h"

#ifndef S_PORT 
#define S_PORT 8080
#endif
#define LOG_IP_ENABLED 1

int server_fdG = 0;

void handle_signal(int sig) {
    (void) sig;
    if (close(server_fdG) < 0) {
        fprintf(stderr, "ERROR: Could not close server socket!\n");
    }
    blocklist_free();
    exit(0);
}

int handle_routes(int client_socket, HTTP_Request req
#ifdef SSL_ENABLE
    SSL *ssl
#endif 
        ) {
    char file_path[512];
    if (strcmp(req.route, "/") == 0) {
        strcpy(file_path, "index.html");
    } else if (strcmp(req.route, "/server_info") == 0 || strcmp(req.route, "/server_info/") == 0) {
#ifdef SSL_ENABLE
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>", ssl);
#else
        send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>");
#endif
        return 1;
    } else {
        snprintf(file_path, sizeof(file_path), "%s", req.route + 1);
    }

#ifdef SSL_ENABLE
    send_file_response(client_socket, "200 OK", file_path, ssl);
#else
    send_file_response(client_socket, "200 OK", file_path);
#endif

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

    char buffer[R_BUFFER_SIZE];
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
    if (req.method == HM_UNKNOWN) return;
    if (LOG_IP_ENABLED) {
        printf("[%s:%d %s] %s %s\n", client_ip_address, client_port, time_str, method_to_str(req.method), req.route);
    } else {
        printf("[%s] %s %s\n", time_str, method_to_str(req.method), req.route);
    }

    char *session_token = token_generate();
    if (session_token) {
        bool cookie_set = hapi_set_cookie(client_socket, "mfh_session_token", session_token, 3600
#ifdef SSL_ENABLE
            , ssl
#endif
        );
        
        //if (!cookie_set) {
        //    log_msg("ERROR", "Failed to set session cookie\n");
        //}
        (void) cookie_set;
        free(session_token);
    }


#ifdef SSL_ENABLE 
    if (handle_routes(client_socket, req, ssl)) {
#else 
    if (handle_routes(client_socket, req)) {
#endif
        goto cleanup;
    }

cleanup:
    hapi_free_cookies(&req);
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
    if (blocklist_load("BLOCKLIST") < 0) {
        return 1;
    } 
    
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

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
    server_fdG = server_fd;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(S_PORT);

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
    blocklist_free();
    return 0;
}
