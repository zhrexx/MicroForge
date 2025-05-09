#include "sockets.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>

void socket_fault(char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(1);
}

Socket *socket_create(char *ip_, int type, int auto_bind) {
    Socket *result = malloc(sizeof(Socket));
    char *ip = strdup(ip_);
    int domain = AF_INET;

    char *colon = strchr(ip, ':');
    if (colon == NULL) {
        socket_fault("[SOCKETS] Invalid IP expected 'ip:port' got '%s'\n", ip_);
    }
    
    *colon = '\0';
    
    result->ip = strdup(ip);
    result->port = (uint16_t)atoi(colon + 1);
    if (strcmp(result->ip, "0.0.0.0") == 0) {
        domain = AF_INET;
    } else if (strcmp(result->ip, "127.0.0.1") == 0 || strcmp(result->ip, "localhost") == 0) {
        domain = AF_LOCAL; 
    }

    free(ip);
   
    result->domain = domain;
    result->type = type;
    
    result->fd = socket(domain, type, 0);
    if (result->fd < 0) socket_fault("[SOCKETS] Could not open socket\n");
    
    result->message = NULL;
    result->message_size = 0;
    
    if (auto_bind) {
        int enable = 1;
        if (setsockopt(result->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            socket_fault("[SOCKETS] Failed to set socket options\n");
        }
        
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        
        server_addr.sin_family = AF_INET;
        
        if (strcmp(result->ip, "0.0.0.0") == 0) {
            server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        } else {
            server_addr.sin_addr.s_addr = inet_addr(result->ip);
        }
        
        server_addr.sin_port = htons(result->port);
        
        if (bind(result->fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            socket_fault("[SOCKETS] Failed to bind socket to %s:%d\n", result->ip, result->port);
        }
    }

    return result;
}

int socket_bind(Socket *socket) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    
    if (strcmp(socket->ip, "0.0.0.0") == 0) {
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        server_addr.sin_addr.s_addr = inet_addr(socket->ip);
    }
    
    server_addr.sin_port = htons(socket->port);
    
    int enable = 1;
    if (setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        return -1;
    }
    
    if (bind(socket->fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        return -1;
    }
    
    return 0;
}

int socket_listen(Socket *socket, int backlog) {
    if (backlog <= 0) {
        backlog = MAX_CONNECTIONS;
    }
    
    if (listen(socket->fd, backlog) < 0) {
        return -1;
    }
    
    return 0;
}

Socket *socket_accept(Socket *server_socket) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(server_socket->fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        return NULL;
    }
    
    Socket *client_socket = malloc(sizeof(Socket));
    if (!client_socket) {
        close(client_fd);
        return NULL;
    }
    
    client_socket->fd = client_fd;
    client_socket->domain = server_socket->domain;
    client_socket->type = server_socket->type;
    
    char *client_ip = inet_ntoa(client_addr.sin_addr);
    client_socket->ip = strdup(client_ip);
    client_socket->port = ntohs(client_addr.sin_port);
    
    client_socket->message = NULL;
    client_socket->message_size = 0;
    
    return client_socket;
}

int socket_connect(Socket *socket) {
    struct sockaddr_in server_addr;
    struct hostent *server;
    
    server = gethostbyname(socket->ip);
    if (server == NULL) {
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(socket->port);
    
    if (connect(socket->fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        return -1;
    }
    
    return 0;
}

ssize_t socket_send(Socket *socket, const void *buffer, size_t length) {
    return send(socket->fd, buffer, length, 0);
}

ssize_t socket_recv(Socket *socket, void *buffer, size_t length) {
    return recv(socket->fd, buffer, length, 0);
}

int socket_close(Socket *socket) {
    if (!socket) return -1;
    
    int result = 0;
    if (socket->fd >= 0) {
        result = close(socket->fd);
        socket->fd = -1;
    }
    
    return result;
}

void socket_free(Socket *socket) {
    if (!socket) return;
    
    socket_close(socket);
    
    if (socket->ip) {
        free(socket->ip);
    }
    
    if (socket->message) {
        free(socket->message);
    }
    
    free(socket);
}

char *socket_get_peer_ip(Socket *socket) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    if (getpeername(socket->fd, (struct sockaddr *)&addr, &addr_len) < 0) {
        return NULL;
    }
    
    return inet_ntoa(addr.sin_addr);
}

void socket_set_blocking(Socket *socket, int blocking) {
    int flags = fcntl(socket->fd, F_GETFL, 0);
    
    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }
    
    fcntl(socket->fd, F_SETFL, flags);
}

int socket_set_option(Socket *socket, int level, int option_name, const void *option_value, socklen_t option_len) {
    return setsockopt(socket->fd, level, option_name, option_value, option_len);
}

int socket_get_error(Socket *socket) {
    int error = 0;
    socklen_t len = sizeof(error);
    
    if (getsockopt(socket->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return errno;
    }
    
    return error;
}
