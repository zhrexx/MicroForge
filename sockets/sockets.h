#ifndef SOCKETS_H
#define SOCKETS_H
#include <stdio.h> 
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET_BUFFER_SIZE 4096
#define MAX_CONNECTIONS 10

typedef struct {
    int fd;
    int domain;
    int type;
    
    char *ip;
    uint16_t port;

    char *message;
    size_t message_size;
} Socket;

void socket_fault(char *msg, ...);
Socket *socket_create(char *ip_, int type, int auto_bind);
int socket_bind(Socket *socket);
int socket_listen(Socket *socket, int backlog);
Socket *socket_accept(Socket *server_socket);
int socket_connect(Socket *socket);
ssize_t socket_send(Socket *socket, const void *buffer, size_t length);
ssize_t socket_recv(Socket *socket, void *buffer, size_t length);
int socket_close(Socket *socket);
void socket_free(Socket *socket);
char *socket_get_peer_ip(Socket *socket);
void socket_set_blocking(Socket *socket, int blocking);
int socket_set_option(Socket *socket, int level, int option_name, const void *option_value, socklen_t option_len);
int socket_get_error(Socket *socket);

#endif
