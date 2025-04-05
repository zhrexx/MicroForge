#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "xdb.h"

unsigned int hash_function(const char* key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + (*key++);
    }
    return hash % HASH_TABLE_SIZE;
}

void init_hash_table(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        ht->buckets[i].entries = NULL;
        ht->buckets[i].size = 0;
        ht->buckets[i].capacity = 0;
        mutex_init(&ht->locks[i]);
    }
}

void free_hash_table(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        mutex_lock(&ht->locks[i]);
        free(ht->buckets[i].entries);
        mutex_unlock(&ht->locks[i]);
        mutex_destroy(&ht->locks[i]);
    }
}

int set_key(HashTable* ht, const char* key, const char* value, int expire_seconds) {
    unsigned int index = hash_function(key);
    Bucket* bucket = &ht->buckets[index];
    
    mutex_lock(&ht->locks[index]);
    
    for (int i = 0; i < bucket->size; i++) {
        if (strcmp(bucket->entries[i].key, key) == 0) {
            strncpy(bucket->entries[i].value, value, MAX_VALUE_SIZE - 1);
            bucket->entries[i].value[MAX_VALUE_SIZE - 1] = '\0';
            
            if (expire_seconds > 0) {
                bucket->entries[i].expiry = time(NULL) + expire_seconds;
            } else {
                bucket->entries[i].expiry = 0;
            }
            
            mutex_unlock(&ht->locks[index]);
            return 1;
        }
    }
    
    if (bucket->size >= bucket->capacity) {
        int new_capacity = bucket->capacity == 0 ? 2 : bucket->capacity * 2;
        KeyValue* new_entries = realloc(bucket->entries, sizeof(KeyValue) * new_capacity);
        if (!new_entries) {
            mutex_unlock(&ht->locks[index]);
            return 0;
        }
        bucket->entries = new_entries;
        bucket->capacity = new_capacity;
    }
    
    KeyValue* entry = &bucket->entries[bucket->size++];
    strncpy(entry->key, key, MAX_KEY_SIZE - 1);
    entry->key[MAX_KEY_SIZE - 1] = '\0';
    strncpy(entry->value, value, MAX_VALUE_SIZE - 1);
    entry->value[MAX_VALUE_SIZE - 1] = '\0';
    
    if (expire_seconds > 0) {
        entry->expiry = time(NULL) + expire_seconds;
    } else {
        entry->expiry = 0;
    }
    
    mutex_unlock(&ht->locks[index]);
    return 1;
}

int get_key(HashTable* ht, const char* key, char* value_buf, size_t buf_size) {
    unsigned int index = hash_function(key);
    Bucket* bucket = &ht->buckets[index];
    
    mutex_lock(&ht->locks[index]);
    
    for (int i = 0; i < bucket->size; i++) {
        if (strcmp(bucket->entries[i].key, key) == 0) {
            if (bucket->entries[i].expiry > 0 && bucket->entries[i].expiry < time(NULL)) {
                memmove(&bucket->entries[i], &bucket->entries[i + 1], 
                        sizeof(KeyValue) * (bucket->size - i - 1));
                bucket->size--;
                mutex_unlock(&ht->locks[index]);
                return 0;
            }
            
            strncpy(value_buf, bucket->entries[i].value, buf_size - 1);
            value_buf[buf_size - 1] = '\0';
            mutex_unlock(&ht->locks[index]);
            return 1;
        }
    }
    
    mutex_unlock(&ht->locks[index]);
    return 0;
}

int delete_key(HashTable* ht, const char* key) {
    unsigned int index = hash_function(key);
    Bucket* bucket = &ht->buckets[index];
    
    mutex_lock(&ht->locks[index]);
    
    for (int i = 0; i < bucket->size; i++) {
        if (strcmp(bucket->entries[i].key, key) == 0) {
            memmove(&bucket->entries[i], &bucket->entries[i + 1], 
                    sizeof(KeyValue) * (bucket->size - i - 1));
            bucket->size--;
            mutex_unlock(&ht->locks[index]);
            return 1;
        }
    }
    
    mutex_unlock(&ht->locks[index]);
    return 0;
}

void save_to_file(HashTable* ht, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return;
    }
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        mutex_lock(&ht->locks[i]);
        Bucket* bucket = &ht->buckets[i];
        
        for (int j = 0; j < bucket->size; j++) {
            KeyValue* entry = &bucket->entries[j];
            if (entry->expiry == 0 || entry->expiry > time(NULL)) {
                size_t key_len = strlen(entry->key);
                size_t value_len = strlen(entry->value);
                
                fwrite(&key_len, sizeof(size_t), 1, file);
                fwrite(entry->key, 1, key_len, file);
                fwrite(&value_len, sizeof(size_t), 1, file);
                fwrite(entry->value, 1, value_len, file);
                fwrite(&entry->expiry, sizeof(time_t), 1, file);
            }
        }
        
        mutex_unlock(&ht->locks[i]);
    }
    
    fclose(file);
}

void load_from_file(HashTable* ht, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return;
    }
    
    while (!feof(file)) {
        size_t key_len, value_len;
        time_t expiry;
        char key[MAX_KEY_SIZE];
        char value[MAX_VALUE_SIZE];
        
        if (fread(&key_len, sizeof(size_t), 1, file) != 1) {
            break;
        }
        
        if (key_len >= MAX_KEY_SIZE) {
            break;
        }
        
        if (fread(key, 1, key_len, file) != key_len) {
            break;
        }
        key[key_len] = '\0';
        
        if (fread(&value_len, sizeof(size_t), 1, file) != 1) {
            break;
        }
        
        if (value_len >= MAX_VALUE_SIZE) {
            break;
        }
        
        if (fread(value, 1, value_len, file) != value_len) {
            break;
        }
        value[value_len] = '\0';
        
        if (fread(&expiry, sizeof(time_t), 1, file) != 1) {
            break;
        }
        
        if (expiry == 0 || expiry > time(NULL)) {
            unsigned int index = hash_function(key);
            Bucket* bucket = &ht->buckets[index];
            
            mutex_lock(&ht->locks[index]);
            
            if (bucket->size >= bucket->capacity) {
                int new_capacity = bucket->capacity == 0 ? 2 : bucket->capacity * 2;
                KeyValue* new_entries = realloc(bucket->entries, sizeof(KeyValue) * new_capacity);
                if (!new_entries) {
                    mutex_unlock(&ht->locks[index]);
                    continue;
                }
                bucket->entries = new_entries;
                bucket->capacity = new_capacity;
            }
            
            KeyValue* entry = &bucket->entries[bucket->size++];
            strncpy(entry->key, key, MAX_KEY_SIZE - 1);
            entry->key[MAX_KEY_SIZE - 1] = '\0';
            strncpy(entry->value, value, MAX_VALUE_SIZE - 1);
            entry->value[MAX_VALUE_SIZE - 1] = '\0';
            entry->expiry = expiry;
            
            mutex_unlock(&ht->locks[index]);
        }
    }
    
    fclose(file);
}

Database* create_database(const char* name, const char* db_path) {
    Database* db = (Database*)malloc(sizeof(Database));
    if (!db) return NULL;
    
    strncpy(db->name, name, MAX_DB_NAME_SIZE - 1);
    db->name[MAX_DB_NAME_SIZE - 1] = '\0';
    
    db->store = (HashTable*)malloc(sizeof(HashTable));
    if (!db->store) {
        free(db);
        return NULL;
    }
    
    db->db_path = strdup(db_path);
    init_hash_table(db->store);
    load_from_file(db->store, db->db_path);
    
    return db;
}

void destroy_database(Database* db) {
    if (!db) return;
    
    save_to_file(db->store, db->db_path);
    free_hash_table(db->store);
    free(db->store);
    free(db->db_path);
    free(db);
}

#ifdef _WIN32
DWORD WINAPI handle_client(LPVOID arg) {
#else
void* handle_client(void* arg) {
#endif
    ClientInfo* info = (ClientInfo*)arg;
    XDBServer* server = (XDBServer*)info->server;
    socket_t client_sock = info->client_sock;
    char buffer[MAX_COMMAND_SIZE];
    char response[MAX_VALUE_SIZE + 128];
    
    info->current_db_index = 0;
    
    while (server->server_running) {
        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        char* cmd = strtok(buffer, " \r\n");
        if (!cmd) continue;
        
        if (strcmp(cmd, "SET") == 0) {
            char* key = strtok(NULL, " \r\n");
            char* value = strtok(NULL, " \r\n");
            char* expire_str = strtok(NULL, " \r\n");
            
            if (!key || !value) {
                strcpy(response, "-ERR invalid syntax\r\n");
            } else {
                int expire_seconds = expire_str ? atoi(expire_str) : 0;
                mutex_lock(&server->db_mutex);
                if (info->current_db_index < server->db_count && 
                    set_key(server->databases[info->current_db_index].store, key, value, expire_seconds)) {
                    strcpy(response, "+OK\r\n");
                } else {
                    strcpy(response, "-ERR failed to set key\r\n");
                }
                mutex_unlock(&server->db_mutex);
            }
        } else if (strcmp(cmd, "GET") == 0) {
            char* key = strtok(NULL, " \r\n");
            
            if (!key) {
                strcpy(response, "-ERR invalid syntax\r\n");
            } else {
                char value[MAX_VALUE_SIZE];
                mutex_lock(&server->db_mutex);
                if (info->current_db_index < server->db_count && 
                    get_key(server->databases[info->current_db_index].store, key, value, sizeof(value))) {
                    sprintf(response, "$%zu\r\n%s\r\n", strlen(value), value);
                } else {
                    strcpy(response, "$-1\r\n");
                }
                mutex_unlock(&server->db_mutex);
            }
        } else if (strcmp(cmd, "DEL") == 0) {
            char* key = strtok(NULL, " \r\n");
            
            if (!key) {
                strcpy(response, "-ERR invalid syntax\r\n");
            } else {
                mutex_lock(&server->db_mutex);
                if (info->current_db_index < server->db_count && 
                    delete_key(server->databases[info->current_db_index].store, key)) {
                    strcpy(response, ":1\r\n");
                } else {
                    strcpy(response, ":0\r\n");
                }
                mutex_unlock(&server->db_mutex);
            }
        } else if (strcmp(cmd, "SELECTDB") == 0) {
            char* db_index_str = strtok(NULL, " \r\n");
            
            if (!db_index_str) {
                strcpy(response, "-ERR invalid syntax\r\n");
            } else {
                int db_index = atoi(db_index_str);
                mutex_lock(&server->db_mutex);
                if (db_index >= 0 && db_index < server->db_count) {
                    info->current_db_index = db_index;
                    sprintf(response, "+OK switched to DB %d (%s)\r\n", 
                            db_index, server->databases[db_index].name);
                } else {
                    strcpy(response, "-ERR invalid database index\r\n");
                }
                mutex_unlock(&server->db_mutex);
            }
        } else if (strcmp(cmd, "LISTDBS") == 0) {
            mutex_lock(&server->db_mutex);
            sprintf(response, "*%d\r\n", server->db_count);
            char temp[MAX_DB_NAME_SIZE + 32];
            
            for (int i = 0; i < server->db_count; i++) {
                sprintf(temp, "$%zu\r\n%d:%s\r\n", strlen(server->databases[i].name) + 2, 
                        i, server->databases[i].name);
                strcat(response, temp);
            }
            mutex_unlock(&server->db_mutex);
        } else if (strcmp(cmd, "SAVE") == 0) {
            mutex_lock(&server->db_mutex);
            if (info->current_db_index < server->db_count) {
                save_to_file(server->databases[info->current_db_index].store, 
                             server->databases[info->current_db_index].db_path);
                strcpy(response, "+OK\r\n");
            } else {
                strcpy(response, "-ERR invalid database\r\n");
            }
            mutex_unlock(&server->db_mutex);
        } else if (strcmp(cmd, "SAVEALL") == 0) {
            mutex_lock(&server->db_mutex);
            for (int i = 0; i < server->db_count; i++) {
                save_to_file(server->databases[i].store, server->databases[i].db_path);
            }
            strcpy(response, "+OK all databases saved\r\n");
            mutex_unlock(&server->db_mutex);
        } else if (strcmp(cmd, "PING") == 0) {
            strcpy(response, "+PONG\r\n");
        } else {
            strcpy(response, "-ERR unknown command\r\n");
        }
        
        send(client_sock, response, strlen(response), 0);
    }
    
    close_socket(client_sock);
    free(info);
    
    mutex_lock(&server->client_mutex);
    server->client_count--;
    mutex_unlock(&server->client_mutex);
    
    return 0;
}

#ifdef _WIN32
DWORD WINAPI server_autosave(LPVOID arg) {
#else
void* server_autosave(void* arg) {
#endif
    XDBServer* server = (XDBServer*)arg;
    
    while (server->server_running) {
        sleep_ms(30000);
        
        mutex_lock(&server->db_mutex);
        for (int i = 0; i < server->db_count; i++) {
            save_to_file(server->databases[i].store, server->databases[i].db_path);
        }
        mutex_unlock(&server->db_mutex);
    }
    
    return 0;
}

XDBServer* xdb_server_create(int port) {
    XDBServer* server = (XDBServer*)malloc(sizeof(XDBServer));
    if (!server) return NULL;
    
    server->port = port > 0 ? port : DEFAULT_PORT;
    server->server_running = 0;
    server->client_count = 0;
    server->db_count = 0;
    server->databases = NULL;
    
    mutex_init(&server->client_mutex);
    mutex_init(&server->db_mutex);
    
    return server;
}

int xdb_server_add_database(XDBServer* server, const char* name, const char* db_path) {
    if (!server || server->db_count >= MAX_DB_COUNT) return 0;
    
    mutex_lock(&server->db_mutex);
    
    for (int i = 0; i < server->db_count; i++) {
        if (strcmp(server->databases[i].name, name) == 0) {
            mutex_unlock(&server->db_mutex);
            return 0;
        }
    }
    
    Database* new_dbs = (Database*)realloc(server->databases, 
                                         sizeof(Database) * (server->db_count + 1));
    if (!new_dbs) {
        mutex_unlock(&server->db_mutex);
        return 0;
    }
    server->databases = new_dbs;
    
    Database* db = create_database(name, db_path);
    if (!db) {
        mutex_unlock(&server->db_mutex);
        return 0;
    }
    
    server->databases[server->db_count++] = *db;
    free(db);
    
    mutex_unlock(&server->db_mutex);
    return 1;
}

int xdb_server_start(XDBServer* server) {
    if (server->server_running || server->db_count == 0) {
        return 0;
    }
    
    #ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return 0;
    }
    #endif
    
    server->server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_sock == INVALID_SOCKET) {
        return 0;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server->port);
    
    #ifdef _WIN32
    char opt = 1;
    #else
    int opt = 1;
    #endif
    setsockopt(server->server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(server->server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        close_socket(server->server_sock);
        return 0;
    }
    
    if (listen(server->server_sock, 10) == SOCKET_ERROR) {
        close_socket(server->server_sock);
        return 0;
    }
    
    server->server_running = 1;
    
    thread_create(&server->autosave_thread, server_autosave, server);
    
    while (server->server_running) {
        struct sockaddr_in client_addr;
        #ifdef _WIN32
        int addr_len = sizeof(client_addr);
        #else
        socklen_t addr_len = sizeof(client_addr);
        #endif
        
        socket_t client_sock = accept(server->server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            continue;
        }
        
        mutex_lock(&server->client_mutex);
        if (server->client_count >= MAX_CLIENTS) {
            close_socket(client_sock);
            mutex_unlock(&server->client_mutex);
            continue;
        }
        
        ClientInfo* client_info = (ClientInfo*)malloc(sizeof(ClientInfo));
        if (!client_info) {
            close_socket(client_sock);
            mutex_unlock(&server->client_mutex);
            continue;
        }
        
        client_info->client_sock = client_sock;
        client_info->client_addr = client_addr;
        client_info->server = server;
        
        thread_create(&server->client_threads[server->client_count], handle_client, client_info);
        server->client_count++;
        mutex_unlock(&server->client_mutex);
    }
    
    return 1;
}

void xdb_server_stop(XDBServer* server) {
    if (!server->server_running) {
        return;
    }
    
    server->server_running = 0;
    close_socket(server->server_sock);
    
    sleep_ms(1000);
    
    mutex_lock(&server->db_mutex);
    for (int i = 0; i < server->db_count; i++) {
        save_to_file(server->databases[i].store, server->databases[i].db_path);
    }
    mutex_unlock(&server->db_mutex);
    
    #ifdef _WIN32
    WSACleanup();
    #endif
}

void xdb_server_destroy(XDBServer* server) {
    if (server->server_running) {
        xdb_server_stop(server);
    }
    
    mutex_lock(&server->db_mutex);
    for (int i = 0; i < server->db_count; i++) {
        free_hash_table(server->databases[i].store);
        free(server->databases[i].store);
        free(server->databases[i].db_path);
    }
    free(server->databases);
    mutex_unlock(&server->db_mutex);
    
    mutex_destroy(&server->client_mutex);
    mutex_destroy(&server->db_mutex);
    free(server);
}


XDBInstance* xdb_instance_create(const char* name, const char* db_path) {
    XDBInstance* instance = (XDBInstance*)malloc(sizeof(XDBInstance));
    if (!instance) return NULL;
    
    instance->db = create_database(name, db_path);
    if (!instance->db) {
        free(instance);
        return NULL;
    }
    
    return instance;
}

int xdb_instance_set(XDBInstance* instance, const char* key, const char* value, int expire_seconds) {
    if (!instance || !instance->db) return 0;
    return set_key(instance->db->store, key, value, expire_seconds);
}

int xdb_instance_get(XDBInstance* instance, const char* key, char* value_buf, size_t buf_size) {
    if (!instance || !instance->db) return 0;
    return get_key(instance->db->store, key, value_buf, buf_size);
}

int xdb_instance_delete(XDBInstance* instance, const char* key) {
    if (!instance || !instance->db) return 0;
    return delete_key(instance->db->store, key);
}

void xdb_instance_save(XDBInstance* instance) {
    if (!instance || !instance->db) return;
    save_to_file(instance->db->store, instance->db->db_path);
}

void xdb_instance_destroy(XDBInstance* instance) {
    if (!instance) return;
    
    if (instance->db) {
        xdb_instance_save(instance);
        destroy_database(instance->db);
    }
    
    free(instance);
}


