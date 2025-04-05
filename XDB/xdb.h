#include <stdlib.h>
#ifndef XDB_H
#define XDB_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    typedef HANDLE thread_t;
    typedef CRITICAL_SECTION mutex_t;
    #define mutex_init(m) InitializeCriticalSection(m)
    #define mutex_lock(m) EnterCriticalSection(m)
    #define mutex_unlock(m) LeaveCriticalSection(m)
    #define mutex_destroy(m) DeleteCriticalSection(m)
    #define thread_create(t, f, arg) *t = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)f, arg, 0, NULL)
    #define thread_join(t) WaitForSingleObject(t, INFINITE)
    #define close_socket(s) closesocket(s)
    #define sleep_ms(ms) Sleep(ms)
    #define PATH_SEPARATOR "\\"
#else
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <signal.h>
    typedef int socket_t;
    typedef pthread_t thread_t;
    typedef pthread_mutex_t mutex_t;
    #define mutex_init(m) pthread_mutex_init(m, NULL)
    #define mutex_lock(m) pthread_mutex_lock(m)
    #define mutex_unlock(m) pthread_mutex_unlock(m)
    #define mutex_destroy(m) pthread_mutex_destroy(m)
    #define thread_create(t, f, arg) pthread_create(t, NULL, f, arg)
    #define thread_join(t) pthread_join(t, NULL)
    #define close_socket(s) close(s)
    #define sleep_ms(ms) usleep(ms * 1000)
    #define PATH_SEPARATOR "/"
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#define MAX_KEY_SIZE 128
#define MAX_VALUE_SIZE 4096
#define MAX_COMMAND_SIZE 4224
#define MAX_CLIENTS 100
#define DEFAULT_PORT 6379
#define HASH_TABLE_SIZE 1024
#define MAX_DB_COUNT 16
#define MAX_DB_NAME_SIZE 64

typedef struct {
    char key[MAX_KEY_SIZE];
    char value[MAX_VALUE_SIZE];
    time_t expiry;
} KeyValue;

typedef struct {
    KeyValue* entries;
    int size;
    int capacity;
} Bucket;

typedef struct {
    Bucket buckets[HASH_TABLE_SIZE];
    mutex_t locks[HASH_TABLE_SIZE];
} HashTable;

typedef struct {
    char name[MAX_DB_NAME_SIZE];
    HashTable* store;
    char* db_path;
} Database;

typedef struct {
    socket_t client_sock;
    struct sockaddr_in client_addr;
    int current_db_index;
    void* server;
} ClientInfo;

typedef struct {
    Database* databases;
    int db_count;
    socket_t server_sock;
    int port;
    int server_running;
    thread_t client_threads[MAX_CLIENTS];
    thread_t autosave_thread;
    int client_count;
    mutex_t client_mutex;
    mutex_t db_mutex;
} XDBServer;

XDBServer* xdb_server_create(int port);
int xdb_server_add_database(XDBServer* server, const char* name, const char* db_path);
int xdb_server_start(XDBServer* server);
void xdb_server_stop(XDBServer* server);
void xdb_server_destroy(XDBServer* server);

typedef struct {
    Database* db;
} XDBInstance;

XDBInstance* xdb_instance_create(const char* name, const char* db_path);
int xdb_instance_set(XDBInstance* instance, const char* key, const char* value, int expire_seconds);
int xdb_instance_get(XDBInstance* instance, const char* key, char* value_buf, size_t buf_size);
int xdb_instance_delete(XDBInstance* instance, const char* key);
void xdb_instance_save(XDBInstance* instance);
void xdb_instance_destroy(XDBInstance* instance);

#endif
