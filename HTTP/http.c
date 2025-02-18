#include "hapi.h"

#ifndef S_PORT 
#define S_PORT 8080
#endif
#define LOG_IP_ENABLED 1

int server_fdG = 0;

/*
 * Handle SIGINT and others
*/
void handle_signal(int sig) {
    (void) sig;
    if (close(server_fdG) < 0) {
        fprintf(stderr, "ERROR: Could not close server socket!\n");
    }
    blocklist_free();
    exit(0);
}

/*
 * Helper function to handle every request
*/
#ifdef SSL_ENABLE
int handle_routes(int client_socket, HTTP_Request req, SSL *ssl) {
#else 
int handle_routes(int client_socket, HTTP_Request req) {
#endif
    char file_path[512];
    if (hapi_f(&req, client_socket)) {
        return 0;
    }
    if (http_check_route(req.route, "/")) {
        strcpy(file_path, "index.html");
    } else if (http_check_route(req.route, "/server_info") || http_check_route(req.route, "/server_info/")) {
#ifdef SSL_ENABLE
        http_send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>", ssl);
#else
        http_send_response(client_socket, "200 OK", "<!DOCTYPE html>This is running on <a href=\"https://github.com/zhrexx/MicroForge/tree/main/HTTP\">MicroForge/HTTP</a> created by <a href=\"https://github.com/zhrexx\">zhrexx</a>");
#endif
        return 1;
    } else {
        snprintf(file_path, sizeof(file_path), "%s", req.route + 1);
    }

#ifdef SSL_ENABLE
    http_send_file_response(client_socket, "200 OK", file_path, ssl);
#else
    http_send_file_response(client_socket, "200 OK", file_path);
#endif

    return 0;
}

/*
 * Parses Request and calls helper function to handle it
 */
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
#ifdef SSL_ENABLE
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);
    if (SSL_accept(ssl) <= 0) {
        SSL_free(ssl);
        close(client_socket);
        return;
    }
#endif
    if (http_check_ip_address(client_ip_address)) {
        printf("[%s:%d %s] Blocked Connection: Banned IP address\n", client_ip_address, client_port, time_str);
#ifdef SSL_ENABLE
        http_send_response(client_socket, "404 BLOCKED_IP_ADDRESS", "Your IP address is blocked!", ssl);
#else 
        http_send_response(client_socket, "404 BLOCKED_IP_ADDRESS", "Your IP address is blocked!");
#endif

        close(client_socket);
        return;
    }

    char buffer[R_BUFFER_SIZE];
    ssize_t bytes_received;

#ifdef SSL_ENABLE 
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
    

    HTTP_Request req = http_parse_request(buffer);
    if (req.method == HM_UNKNOWN) return;
    if (LOG_IP_ENABLED) {
        printf("[%s:%d %s] %s %s\n", client_ip_address, client_port, time_str, http_method_to_str(req.method), req.route);
    } else {
        printf("[%s] %s %s\n", time_str, http_method_to_str(req.method), req.route);
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
    handle_client_f hcF = handle_client;
    if (http_run_server(S_PORT, &server_fdG, hcF) < 0) {
        fprintf(stderr, "ERROR: Could not run server!\n");
        return 1;
    }
    return 0;
}
