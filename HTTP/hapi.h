/*
 *  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
 *  ┃ Project:  MicroForgeHTTP                   ┃
 *  ┃ File:     hapi.h                           ┃
 *  ┃ Author:   zhrexx                           ┃
 *  ┃ License:  NovaLicense                      ┃
 *  ┃━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┃     
 *  ┃ MFH is a WebAPI and HTTP server library.   ┃
 *  ┃ Its Fast and simple to use.                ┃
 *  ┃ It has it own features which other servers ┃
 *  ┃ don't have, Were trying to create our own  ┃
 *  ┃ API which would add some features but same ┃
 *  ┃ times be compatible with all Browsers      ┃
 *  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
 */

#ifndef HAPI_H
#define HAPI_H
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
#include <signal.h> 
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include "htengine.h"
#ifdef SSL_ENABLE
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif


#define TOKEN_LENGTH 32
#define R_BUFFER_SIZE (1 * 1024 * 1024)
#define BLOCKLIST_MAX_LINES 1024
#define BLOCKLIST_MAX_TOKENS 256
#define BLOCKLIST_MAX_LENGTH 1024

#define SERVER_API_NAME "mfh"
#define SERVER_API_VERSION 1.0

typedef enum {
    HM_GET,
    HM_POST,
    HM_UNKNOWN,
} HTTP_Method;

typedef struct {
    char *key;
    char *value;
} HTTP_Parameter;
typedef struct {
    char *name;
    char *value;
} HTTP_Cookie;

typedef struct {
    HTTP_Cookie *cookies;
    int cookie_count;
} HTTP_CookieJar;

typedef struct {
    HTTP_Method method;
    char *route;
    HTTP_Parameter *parameters;
    int param_count;
    char *host;
    char *body;
    char *extracted_ip;
    HTTP_CookieJar cookie_jar;
} HTTP_Request;

static char **blocklist = NULL;
static int block_count = 0;

#ifdef SSL_ENABLE
typedef void (*handle_client_f)(int, SSL *);
void http_send_response(int client_socket, const char *status, const char *content, SSL *ssl);
void http_send_file_response(int client_socket, char *status, const char *filepath, HtmlTemplate *tmpl, SSL *ssl);
#else
typedef void (*handle_client_f)(int);
void http_send_file_response(int client_socket, char *status, const char *filepath, HtmlTemplate *tmpl);
void http_send_response(int client_socket, const char *status, const char *content);
#endif
int blocklist_load(const char *filename);
void blocklist_free();


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
char *str_format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len < 0) return NULL;
    char *buffer = malloc(len + 1);
    if (!buffer) return NULL;
    va_start(args, fmt);
    vsnprintf(buffer, len + 1, fmt, args);
    va_end(args);
    return buffer;
}

// NOTE: ex = Expected Route 
int http_check_route(const char *route, const char *exroute) {
    if (!route || !exroute) return 0;
    return strcmp(route, exroute) == 0;
}

char *http_method_to_str(HTTP_Method method) {
    if (method == HM_GET) return "GET";
    if (method == HM_POST) return "POST";
    else return "UNKNOWN";
}

int http_check_ip_address(char *ip) {
    for (int i = 0; i < block_count; i++) {
        if (strncmp(ip, blocklist[i], strlen(blocklist[i])) == 0) {
                return 1;
        }
    }
    return 0;
}

char *token_generate() {
    static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t charset_size = sizeof(charset) - 1;

    char *token = malloc(TOKEN_LENGTH + 1);
    if (!token) {
        return NULL;
    }

    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    for (size_t i = 0; i < TOKEN_LENGTH; i++) {
        token[i] = charset[rand() % charset_size];
    }
    token[TOKEN_LENGTH] = '\0';

    return token;
}

const char* mime_type_get(const char *filename) {
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

/**
 * Cookie management functions for HTTP API
 * Handles setting, removing, retrieving, and cleaning up HTTP cookies
 */

char* hapi_format_cookie(const char *name, const char *value, int max_age) {
    if (!name || !value || max_age < 0) {
        return NULL;
    }

    for (const char *p = name; *p; p++) {
        if (*p <= 32 || *p >= 127 || strchr("()<>@,;:\\\"/[]?={}", *p)) {
            return NULL;
        }
    }
    
    for (const char *p = value; *p; p++) {
        if (*p <= 31 || *p >= 127) {
            return NULL;
        }
    }

    time_t now = time(NULL);
    now += max_age;
    struct tm *tm_info = gmtime(&now);
    char expires[32];
    strftime(expires, sizeof(expires), "%a, %d %b %Y %H:%M:%S GMT", tm_info);

    char *cookie_header = malloc(512);
    if (!cookie_header) {
        return NULL;
    }

    snprintf(cookie_header, 512,
        "Set-Cookie: %s=%s; Path=/; HttpOnly; SameSite=Strict%s; Max-Age=%d; Expires=%s\r\n",
        name, value,
#ifdef SSL_ENABLE
        "; Secure",
#else
        "",
#endif
        max_age, expires);

    return cookie_header;
}

bool hapi_set_cookie(int client_socket, const char *name, const char *value, int max_age
#ifdef SSL_ENABLE
    , SSL *ssl
#endif
) {
    if (!name || !value || max_age < 0) {
        return false;
    }

    for (const char *p = name; *p; p++) {
        if (*p <= 32 || *p >= 127 || strchr("()<>@,;:\\\"/[]?={}", *p)) {
            return false;
        }
    }
    
    for (const char *p = value; *p; p++) {
        if (*p <= 31 || *p >= 127) {
            return false;
        }
    }

    size_t name_len = strlen(name);
    size_t value_len = strlen(value);
    size_t required_size = name_len + value_len + 128;

    if (name_len > 256 || value_len > 4096 || required_size > 8192) {
        return false;
    }

    time_t now = time(NULL);
    now += max_age;
    struct tm *tm_info = gmtime(&now);
    char expires[32];
    strftime(expires, sizeof(expires), "%a, %d %b %Y %H:%M:%S GMT", tm_info);

    char *header = calloc(1, required_size);
    if (!header) {
        return false;
    }

    int written = snprintf(header, required_size,
        "HTTP/1.1 200 OK\r\n"
        "Set-Cookie: %s=%s; Path=/; HttpOnly; SameSite=Strict%s; Max-Age=%d; Expires=%s\r\n"
        "Content-Type: text/html\r\n"
        "Connection: keep-alive\r\n"
        "\r\n",
        name, value,
#ifdef SSL_ENABLE
        "; Secure",
#else
        "",
#endif
        max_age, expires);

    if (written < 0 || (size_t)written >= required_size) {
        free(header);
        return false;
    }

    bool success = false;
#ifdef SSL_ENABLE
    int ssl_result = SSL_write(ssl, header, written);
    success = (ssl_result > 0);
    if (!success) {
        ERR_clear_error();
    }
#else
    ssize_t send_result = send(client_socket, header, written, 0);
    success = (send_result == written);
#endif

    free(header);
    return success;
}


bool hapi_remove_cookie(int client_socket, const char *name
#ifdef SSL_ENABLE
    , SSL *ssl
#endif
) {
    if (!name) {
        return false;
    }

    for (const char *p = name; *p; p++) {
        if (*p <= 32 || *p >= 127 || strchr("()<>@,;:\\\"/[]?={}", *p)) {
            return false;
        }
    }

    size_t name_len = strlen(name);
    size_t required_size = name_len + 128; 

    if (name_len > 256 || required_size > 8192) {
        return false;
    }

    char *response = calloc(1, required_size);
    if (!response) {
        return false;
    }

    int written = snprintf(response, required_size,
        "Set-Cookie: %s=; Path=/; HttpOnly; SameSite=Strict%s; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n",
        name,
#ifdef SSL_ENABLE
        "; Secure"
#else
        ""
#endif
    );

    if (written < 0 || (size_t)written >= required_size) {
        free(response);
        return false;
    }

    bool success = false;
#ifdef SSL_ENABLE
    int ssl_result = SSL_write(ssl, response, written);
    success = (ssl_result > 0);
    if (!success) {
        ERR_clear_error();
    }
#else
    ssize_t send_result = send(client_socket, response, written, 0);
    success = (send_result == written);
#endif

    free(response);
    return success;
}

char *hapi_get_cookie(const HTTP_Request *req, const char *name) {
    if (!req || !name || !req->cookie_jar.cookies || 
        req->cookie_jar.cookie_count <= 0) {
        return NULL;
    }

    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > 256) {
        return NULL;
    }

    for (int i = 0; i < req->cookie_jar.cookie_count; i++) {
        if (req->cookie_jar.cookies[i].name && 
            req->cookie_jar.cookies[i].value &&
            strcmp(req->cookie_jar.cookies[i].name, name) == 0) {
            char *value_copy = strdup(req->cookie_jar.cookies[i].value);
            if (!value_copy) {
                return NULL;
            }
            return value_copy;
        }
    }
    return NULL;
}

void hapi_free_cookies(HTTP_Request *req) {
    if (!req || !req->cookie_jar.cookies) {
        return;
    }

    for (int i = 0; i < req->cookie_jar.cookie_count; i++) {
        free(req->cookie_jar.cookies[i].name);
        free(req->cookie_jar.cookies[i].value);
        req->cookie_jar.cookies[i].name = NULL;
        req->cookie_jar.cookies[i].value = NULL;
    }
    
    free(req->cookie_jar.cookies);
    req->cookie_jar.cookies = NULL;
    req->cookie_jar.cookie_count = 0;
}

// Feature section
// f = feature 

int hapi_f_time(HTTP_Request *req, int client_socket) {
    if (http_check_route(req->route, str_format("/%s/f/time", SERVER_API_NAME)) || http_check_route(req->route, str_format("/%s/f/time/", SERVER_API_NAME)) ) {
        http_send_response(client_socket, "200 OK", str_format("%d", time(NULL)));
        return 1;
    } else {
        return 0;
    }
}

int hapi_f_token(HTTP_Request *req, int client_socket) {
    if (http_check_route(req->route, str_format("/%s/f/token", SERVER_API_NAME)) || http_check_route(req->route, str_format("/%s/f/token/", SERVER_API_NAME)) ) {
        http_send_response(client_socket, "200 OK", str_format("%s", token_generate()));
        return 1;
    } else {
        return 0;
    }
}


typedef int (*hapi_f_function)(HTTP_Request *, int);
int hapi_f(HTTP_Request *req, int client_socket) {
    hapi_f_function functions[2] = {
        hapi_f_token,
        hapi_f_time
    };
    for (size_t i = 0; i < 2; i++) {
        if (functions[i](req, client_socket)) {
            return 1;
        }
    }
    return 0;
}


/*
 * Parsing Cookies
 */
void http_parse_cookies(HTTP_Request *req, const char *header) {
    const char *cookie_header = strstr(header, "Cookie: ");
    if (!cookie_header) return;
    
    cookie_header += 8;
    char *cookies_str = strdup(cookie_header);
    char *token = strtok(cookies_str, "; ");
    
    while (token) {
        char *eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            req->cookie_jar.cookies = realloc(req->cookie_jar.cookies, 
                (req->cookie_jar.cookie_count + 1) * sizeof(HTTP_Cookie));
            req->cookie_jar.cookies[req->cookie_jar.cookie_count].name = strdup(token);
            req->cookie_jar.cookies[req->cookie_jar.cookie_count].value = strdup(eq + 1);
            req->cookie_jar.cookie_count++;
        }
        token = strtok(NULL, "; ");
    }
    
    free(cookies_str);
}


/*
 * Calls helper functions and parses request 
 */
HTTP_Request http_parse_request(const char *request) {
    HTTP_Request result = {0};
    
    if (strncmp(request, "GET ", 4) == 0) {
        result.method = HM_GET;
    } else if (strncmp(request, "POST ", 5) == 0) {
        result.method = HM_POST;
    } else {
        result.method = HM_UNKNOWN;
        log_msg("ERROR", "Unsupported HTTP method: %s\n", str_dup_until(request, ' '));
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
    http_parse_cookies(&result, request);

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
void ssl_init() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

SSL_CTX *ssl_create_context() {
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        perror("Unable to create SSL context");
        exit(EXIT_FAILURE);
    }
    return ctx;
}

void ssl_configure_context(SSL_CTX *ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        perror("SSL certificate error");
        exit(EXIT_FAILURE);
    }
}
#endif

#ifdef SSL_ENABLE
void http_send_response(int client_socket, const char *status, const char *content, SSL *ssl) {
#else
void http_send_response(int client_socket, const char *status, const char *content) {
#endif
    char *cookie_header = NULL;
    char *session_token = token_generate();
    if (session_token) {
        cookie_header = hapi_format_cookie("mfh_session_token", session_token, 3600);
        free(session_token);
    }

    size_t len = strlen(content);
    size_t header_size = len + 256 + (cookie_header ? strlen(cookie_header) : 0);
    
    char *response = malloc(header_size);
    if (!response) {
        free(cookie_header);
        return;
    }

    int written;
    if (cookie_header) {
        written = snprintf(response, header_size,
            "HTTP/1.1 %s\r\n"
            "Server: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "%s"
            "\r\n%s",
            status, SERVER_API_NAME, len, cookie_header, content);
        free(cookie_header);
    } else {
        written = snprintf(response, header_size,
            "HTTP/1.1 %s\r\n"
            "Server: %s\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n%s",
            status, SERVER_API_NAME, len, content);
    }

    if (written > 0 && written < (int)header_size) {
#ifdef SSL_ENABLE
        SSL_write(ssl, response, written);
#else
        send(client_socket, response, written, 0);
#endif
    }

    free(response);
}

#ifdef SSL_ENABLE
void http_send_file_response(int client_socket, char *status, const char *filepath, HtmlTemplate *tmpl, SSL *ssl) {
#else 
void http_send_file_response(int client_socket, char *status, const char *filepath, HtmlTemplate *tmpl) {
#endif
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        char *not_found = "404 Not Found";
#ifdef SSL_ENABLE
        http_send_response(client_socket, "404 Not Found", not_found, ssl);
#else
        http_send_response(client_socket, "404 Not Found", not_found);
#endif
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    const char *mime_type = mime_type_get(filepath);
    
    char *cookie_header = NULL;
    char *session_token = token_generate();
    if (session_token) {
        cookie_header = hapi_format_cookie("mfh_session_token", session_token, 3600);
        free(session_token);
    }

    size_t header_size = 512 + (cookie_header ? strlen(cookie_header) : 0);
    char *header = malloc(header_size);
    if (!header) {
        free(cookie_header);
        fclose(fp);
        return;
    }

    int written;
    if (cookie_header) {
        written = snprintf(header, header_size, 
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Cache-Control: public, max-age=31536000\r\n"
            "%s"
            "\r\n", 
            status, mime_type, file_size, cookie_header);
        free(cookie_header);
    } else {
        written = snprintf(header, header_size, 
            "HTTP/1.1 %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Cache-Control: public, max-age=31536000\r\n"
            "\r\n", 
            status, mime_type, file_size);
    }
        
    if (written > 0 && written < (int)header_size) {
#ifdef SSL_ENABLE
        SSL_write(ssl, header, written);
#else
        send(client_socket, header, written, 0);
#endif
    }
    
    free(header);

    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(fp);
        return;
    }
    
    fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    char *rendered = ht_render(tmpl, buffer);
    ht_destroy(tmpl);
    free(buffer);

    if (rendered) {
#ifdef SSL_ENABLE
        SSL_write(ssl, rendered, strlen(rendered));
#else
        send(client_socket, rendered, strlen(rendered), 0);
#endif
        ht_free_rendered(rendered);
    }
}

extern void handle_signal(int);
int http_run_server(int port, int *sfdG, handle_client_f f) {
    if (!sfdG || !f || port <= 0 || port > 65535) {
        return -1;
    }
    
    int server_fd;
    struct sockaddr_in server_addr;

    if (blocklist_load("BLOCKLIST") < 0) {
        fprintf(stderr, "Warning: Failed to load blocklist\n");
    }
    
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

#ifdef SSL_ENABLE 
    SSL_CTX *ctx;
    ssl_init();
    ctx = ssl_create_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create SSL context\n");
        return -1;
    }
    ssl_configure_context(ctx);
#endif 

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
        perror("setsockopt (keepalive)");
        close(server_fd);
        return -1;
    }
    
    *sfdG = server_fd;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    printf("-------------------------------------------------------------------------------------\n");
    printf("MicroForgeHTTP\n");
    char *server_ip_address = inet_ntoa(server_addr.sin_addr);
    int server_port = ntohs(server_addr.sin_port);
    printf("- Version: %.1f\n", SERVER_API_VERSION);
    printf("- IP: %s:%d\n", server_ip_address, server_port);
#ifdef SSL_ENABLE
    printf("- SSL: Enabled\n");
#else
    printf("- SSL: Disabled\n");
#endif
    printf("-------------------------------------------------------------------------------------\n");
    printf(" LOGS:\n");
    printf("-------------------------------------------------------------------------------------\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }
        
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        
        if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("setsockopt (timeout)");
            close(client_socket);
            continue;
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            
#ifdef SSL_ENABLE
            f(client_socket, ctx);
#else
            f(client_socket);
#endif
            exit(0);
        } 
        else if (pid > 0) {
            close(client_socket);
            
            while (waitpid(-1, NULL, WNOHANG) > 0);
        } 
        else {
            perror("fork");
            close(client_socket);
        }
    }

#ifdef SSL_ENABLE 
    SSL_CTX_free(ctx);
#endif 
    close(server_fd);
    blocklist_free();
    return 0;
}


/* 
 * Loads BLOCKLIST (= List with all blocked IP addresses
 */
int blocklist_load(const char *filename) {
        FILE *file = fopen(filename, "r");
        if (!file) {
            perror("Error opening file");
            return -1;
        }

    blocklist = malloc(BLOCKLIST_MAX_LINES * sizeof(char *));
    if (!blocklist) {
        perror("Memory allocation failed");
        fclose(file);
        return -1;
    }

    char buffer[BLOCKLIST_MAX_LENGTH];
    while (fgets(buffer, sizeof(buffer), file) && block_count < BLOCKLIST_MAX_LINES) {
        buffer[strcspn(buffer, "\n")] = 0;

        char *token = strtok(buffer, " ");
        while (token && block_count < BLOCKLIST_MAX_LINES) {
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

void blocklist_free() {
        for (int i = 0; i < block_count; i++) {
            free(blocklist[i]);
        }
        free(blocklist);
}

#endif // HAPI_H
