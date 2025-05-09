#include "sockets.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/socket.h>

#define SERVER_PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

volatile bool server_running = true;

typedef struct {
    Socket *socket;
    pthread_t thread_id;
    int client_id;
} Client;

Client clients[MAX_CLIENTS];

void handle_signal(int sig) {
    printf("\nShutting down server...\n");
    server_running = false;
}

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    Socket *socket = client->socket;
    int client_id = client->client_id;
    
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    printf("Client #%d connected from %s:%d\n", client_id, socket->ip, socket->port);
    
    snprintf(response, BUFFER_SIZE, "Welcome client #%d! Type 'exit' to disconnect.\n", client_id);
    socket_send(socket, response, strlen(response));
    
    while (server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t recv_size = socket_recv(socket, buffer, BUFFER_SIZE - 1);
        
        if (recv_size <= 0) {
            break;
        }
        
        buffer[recv_size] = '\0';
        
        if (strncmp(buffer, "exit", 4) == 0) {
            printf("Client #%d requested to disconnect\n", client_id);
            break;
        }
        
        printf("Client #%d says: %s", client_id, buffer);
        
        snprintf(response, BUFFER_SIZE, "Server received: %s", buffer);
        socket_send(socket, response, strlen(response));
    }
    
    printf("Client #%d disconnected\n", client_id);
    socket_free(socket);
    clients[client_id].socket = NULL;
    
    return NULL;
}

int find_empty_client_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == NULL) {
            return i;
        }
    }
    return -1;
}

int main() {
    signal(SIGINT, handle_signal);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = NULL;
    }
    
    char server_addr[32];
    snprintf(server_addr, sizeof(server_addr), "0.0.0.0:%d", SERVER_PORT);
    
    Socket *server_socket = socket_create(server_addr, SOCK_STREAM, 1);
    
    if (socket_listen(server_socket, MAX_CLIENTS) < 0) {
        socket_fault("Failed to listen on server socket\n");
    }
    
    printf("Server started on port %d. Press Ctrl+C to stop.\n", SERVER_PORT);
    
    while (server_running) {
        Socket *client_socket = socket_accept(server_socket);
        if (client_socket == NULL) {
            if (!server_running) {
                break;
            }
            
            perror("Failed to accept client connection");
            continue;
        }
        
        int client_slot = find_empty_client_slot();
        if (client_slot < 0) {
            printf("Server full. Rejecting client.\n");
            socket_send(client_socket, "Server is full. Try again later.\n", 33);
            socket_free(client_socket);
            continue;
        }
        
        clients[client_slot].socket = client_socket;
        clients[client_slot].client_id = client_slot;
        
        if (pthread_create(&clients[client_slot].thread_id, NULL, handle_client, &clients[client_slot]) != 0) {
            perror("Failed to create client thread");
            socket_free(client_socket);
            clients[client_slot].socket = NULL;
            continue;
        }
        
        pthread_detach(clients[client_slot].thread_id);
    }
    
    printf("Closing server socket...\n");
    socket_free(server_socket);
    
    printf("Waiting for client threads to finish...\n");
    sleep(1);
    
    printf("Server shutdown complete.\n");
    return 0;
}
