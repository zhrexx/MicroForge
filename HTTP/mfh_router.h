/*
 *  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
 *  ┃ Project:  MicroForgeHTTP                   ┃
 *  ┃ File:     mfh_router.h                     ┃
 *  ┃ Author:   zhrexx                           ┃
 *  ┃ License:  NovaLicense                      ┃
 *  ┃━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┃     
 *  ┃ Flask-like router API for MicroForgeHTTP   ┃
 *  ┃ Enables easy route definition and handling ┃
 *  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
 */

#ifndef MFH_ROUTER_H
#define MFH_ROUTER_H

#include "hapi.h"
#include <regex.h>

#define MAX_ROUTES 100
#define MAX_ROUTE_PARAMS 10
#define MAX_PARAM_NAME 32

typedef struct {
    char *name;
    char *value;
} RouteParam;

typedef struct {
    RouteParam params[MAX_ROUTE_PARAMS];
    int count;
} RouteParams;

#ifdef SSL_ENABLE
typedef void (*route_handler_f)(HTTP_Request *req, int client_socket, SSL *ssl, RouteParams *params);
#else
typedef void (*route_handler_f)(HTTP_Request *req, int client_socket, RouteParams *params);
#endif

typedef struct {
    HTTP_Method method;
    char *path;
    route_handler_f handler;
    regex_t regex;
    char *param_names[MAX_ROUTE_PARAMS];
    int param_count;
} Route;

typedef struct {
    Route routes[MAX_ROUTES];
    int count;
} Router;

Router *router = NULL;

Router* router_init() {
    router = (Router*)malloc(sizeof(Router));
    if (router) {
        router->count = 0;
        memset(router->routes, 0, sizeof(Route) * MAX_ROUTES);
    }
    return router;
}

char* convert_path_to_regex(const char *path, char **param_names, int *param_count) {
    char *regex_str = malloc(strlen(path) * 3 + 10);
    if (!regex_str) return NULL;
    
    char *regex_ptr = regex_str;
    const char *path_ptr = path;
    
    *regex_ptr++ = '^';
    
    *param_count = 0;
    
    while (*path_ptr) {
        if (*path_ptr == '<') {
            const char *param_start = path_ptr + 1;
            const char *param_end = strchr(param_start, '>');
            
            if (!param_end) {
                free(regex_str);
                return NULL; 
            }
            
            int param_len = param_end - param_start;
            if (param_len > MAX_PARAM_NAME - 1) param_len = MAX_PARAM_NAME - 1;
            
            param_names[*param_count] = malloc(param_len + 1);
            strncpy(param_names[*param_count], param_start, param_len);
            param_names[*param_count][param_len] = '\0';
            
            strcpy(regex_ptr, "([^/]+)");
            regex_ptr += 7; 
            
            (*param_count)++;
            path_ptr = param_end + 1;
        } else {
            *regex_ptr++ = *path_ptr++;
        }
    }
    
    *regex_ptr++ = '$';
    *regex_ptr = '\0';
    
    return regex_str;
}

int router_add_route(HTTP_Method method, const char *path, route_handler_f handler) {
    if (!router || router->count >= MAX_ROUTES) return -1;
    
    Route *route = &router->routes[router->count];
    route->method = method;
    route->path = strdup(path);
    route->handler = handler;
    route->param_count = 0;

    if (strchr(path, '<') != NULL) {
        char *regex_pattern = convert_path_to_regex(path, route->param_names, &route->param_count);
        if (!regex_pattern) return -1;
        
        int result = regcomp(&route->regex, regex_pattern, REG_EXTENDED);
        free(regex_pattern);
        
        if (result != 0) {
            free(route->path);
            return -1;
        }
    }
    
    router->count++;
    return 0;
}

int router_get(const char *path, route_handler_f handler) {
    return router_add_route(HM_GET, path, handler);
}

int router_post(const char *path, route_handler_f handler) {
    return router_add_route(HM_POST, path, handler);
}

void extract_route_params(const char *path, Route *route, RouteParams *params) {
    regmatch_t matches[MAX_ROUTE_PARAMS + 1]; 
    
    params->count = 0;
    
    if (regexec(&route->regex, path, route->param_count + 1, matches, 0) == 0) {
        for (int i = 0; i < route->param_count; i++) {
            regmatch_t *match = &matches[i + 1]; 
            int len = match->rm_eo - match->rm_so;
            
            params->params[i].name = route->param_names[i];
            params->params[i].value = malloc(len + 1);
            strncpy(params->params[i].value, path + match->rm_so, len);
            params->params[i].value[len] = '\0';
            
            params->count++;
        }
    }
}

#ifdef SSL_ENABLE
int router_handle_request(HTTP_Request *req, int client_socket, SSL *ssl) {
#else
int router_handle_request(HTTP_Request *req, int client_socket) {
#endif
    if (!router) return 0;
    
    for (int i = 0; i < router->count; i++) {
        Route *route = &router->routes[i];
        
        if (route->method != req->method) continue;

        if (route->param_count == 0) {
            if (strcmp(route->path, req->route) == 0) {
                RouteParams params = {0};
#ifdef SSL_ENABLE
                route->handler(req, client_socket, ssl, &params);
#else
                route->handler(req, client_socket, &params);
#endif
                return 1;
            }
        } else {
            regmatch_t matches[1];
            if (regexec(&route->regex, req->route, 1, matches, 0) == 0) {
                RouteParams params = {0};
                extract_route_params(req->route, route, &params);
                
#ifdef SSL_ENABLE
                route->handler(req, client_socket, ssl, &params);
#else
                route->handler(req, client_socket, &params);
#endif
                
                for (int j = 0; j < params.count; j++) {
                    free(params.params[j].value);
                }
                
                return 1;
            }
        }
    }
    
    return 0;
}

const char* get_route_param(RouteParams *params, const char *name) {
    for (int i = 0; i < params->count; i++) {
        if (strcmp(params->params[i].name, name) == 0) {
            return params->params[i].value;
        }
    }
    return NULL;
}

void router_cleanup() {
    if (!router) return;
    
    for (int i = 0; i < router->count; i++) {
        Route *route = &router->routes[i];
        free(route->path);
        
        if (route->param_count > 0) {
            regfree(&route->regex);
            for (int j = 0; j < route->param_count; j++) {
                free(route->param_names[j]);
            }
        }
    }
    
    free(router);
    router = NULL;
}

#ifdef SSL_ENABLE
void handle_client_with_router(int client_socket, SSL_CTX *ctx) {
    char buffer[R_BUFFER_SIZE] = {0};
    int valread;
    SSL *ssl;

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_socket);

    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client_socket);
        return;
    }

    valread = SSL_read(ssl, buffer, R_BUFFER_SIZE - 1);
#else
void handle_client_with_router(int client_socket) {
    char buffer[R_BUFFER_SIZE] = {0};
    int valread;
    
    valread = read(client_socket, buffer, R_BUFFER_SIZE - 1);
#endif

    if (valread > 0) {
        HTTP_Request req = http_parse_request(buffer);
        
        if (http_check_ip_address(req.extracted_ip)) {
            char *blocked_msg = "Your IP address is blocked from accessing this server.";
#ifdef SSL_ENABLE
            http_send_response(client_socket, "403 Forbidden", blocked_msg, ssl);
#else
            http_send_response(client_socket, "403 Forbidden", blocked_msg);
#endif
        } else {
#ifdef SSL_ENABLE
            int feature_handled = hapi_f(&req, client_socket, ssl);
#else
            int feature_handled = hapi_f(&req, client_socket);
#endif

            if (!feature_handled) {
#ifdef SSL_ENABLE
                int route_handled = router_handle_request(&req, client_socket, ssl);
#else
                int route_handled = router_handle_request(&req, client_socket);
#endif

                if (!route_handled) {
#ifdef SSL_ENABLE
                    http_send_response(client_socket, "404 Not Found", "Route not found", ssl);
#else
                    http_send_response(client_socket, "404 Not Found", "Route not found");
#endif
                }
            }
        }

        free(req.route);
        free(req.host);
        free(req.body);
        free(req.extracted_ip);
        
        if (req.param_count > 0) {
            for (int i = 0; i < req.param_count; i++) {
                free(req.parameters[i].key);
                free(req.parameters[i].value);
            }
            free(req.parameters);
        }
        
        hapi_free_cookies(&req);
    }

#ifdef SSL_ENABLE
    SSL_shutdown(ssl);
    SSL_free(ssl);
#endif
    close(client_socket);
}

#define MFH_APP() router_init()
#define MFH_GET(path, handler) router_get(path, handler)
#define MFH_POST(path, handler) router_post(path, handler)
#define MFH_RUN(port) do { \
    int server_fd = 0; \
    signal(SIGCHLD, SIG_IGN); \
    http_run_server(port, &server_fd, handle_client_with_router); \
    router_cleanup(); \
} while(0)

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nShutting down server...\n");
        router_cleanup();
        exit(0);
    }
}

#endif // MFH_ROUTER_H
