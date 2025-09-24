#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hmap.h"

// Example 1: Simple key-value configuration storage
void example_configuration() {
    printf("Example 1: Configuration Storage\n");
    printf("================================\n");
    
    HashMap config = hmap_create(0);
    
    // Load configuration from "file" (simulated)
    hmap_put_string(&config, "app_name", "MicroForge App");
    hmap_put_string(&config, "version", "1.0.0");
    hmap_put_int(&config, "port", 3000);
    hmap_put_int(&config, "worker_threads", 4);
    hmap_put_double(&config, "cache_timeout", 300.0);
    hmap_put_string(&config, "log_file", "/var/log/app.log");
    
    // Access configuration values
    printf("Application: %s v%s\n", 
           hmap_get_string(&config, "app_name"),
           hmap_get_string(&config, "version"));
    printf("Running on port %d with %d worker threads\n",
           *hmap_get_int(&config, "port"),
           *hmap_get_int(&config, "worker_threads"));
    printf("Cache timeout: %.1f seconds\n", 
           *hmap_get_double(&config, "cache_timeout"));
    printf("Logging to: %s\n", hmap_get_string(&config, "log_file"));
    
    hmap_destroy(&config);
    printf("\n");
}

// Example 2: User database simulation
void example_user_database() {
    printf("Example 2: User Database\n");
    printf("========================\n");
    
    typedef struct {
        int user_id;
        char email[64];
        int age;
        bool active;
    } User;
    
    HashMap users = hmap_create(0);
    
    // Add some users
    User john = {1001, "john@example.com", 30, true};
    User jane = {1002, "jane@example.com", 28, true};
    User bob = {1003, "bob@example.com", 35, false};
    
    hmap_put(&users, "john_doe", &john, sizeof(User));
    hmap_put(&users, "jane_smith", &jane, sizeof(User));
    hmap_put(&users, "bob_wilson", &bob, sizeof(User));
    
    // Look up users
    User* user = (User*)hmap_get(&users, "jane_smith", NULL);
    if (user) {
        printf("Found user: %s (ID: %d, Age: %d, Active: %s)\n",
               user->email, user->user_id, user->age,
               user->active ? "Yes" : "No");
    }
    
    // List all users
    size_t count;
    char** usernames = hmap_keys(&users, &count);
    printf("All users (%zu total):\n", count);
    
    for (size_t i = 0; i < count; i++) {
        User* u = (User*)hmap_get(&users, usernames[i], NULL);
        printf("  %s: %s (%s)\n", usernames[i], u->email,
               u->active ? "Active" : "Inactive");
    }
    
    hmap_free_keys(usernames, count);
    hmap_destroy(&users);
    printf("\n");
}

// Example 3: Word frequency counter
void example_word_counter() {
    printf("Example 3: Word Frequency Counter\n");
    printf("=================================\n");
    
    const char* text = "the quick brown fox jumps over the lazy dog the fox is quick";
    HashMap word_count = hmap_create(0);
    
    // Split text into words and count frequencies
    char* text_copy = strdup(text);
    char* word = strtok(text_copy, " ");
    
    while (word) {
        int* count = hmap_get_int(&word_count, word);
        if (count) {
            // Word exists, increment count
            int new_count = *count + 1;
            hmap_put_int(&word_count, word, new_count);
        } else {
            // New word, initialize count to 1
            hmap_put_int(&word_count, word, 1);
        }
        word = strtok(NULL, " ");
    }
    
    // Display word frequencies
    printf("Text: \"%s\"\n\n", text);
    printf("Word frequencies:\n");
    
    size_t count;
    char** words = hmap_keys(&word_count, &count);
    
    for (size_t i = 0; i < count; i++) {
        int* freq = hmap_get_int(&word_count, words[i]);
        printf("  %-8s: %d\n", words[i], *freq);
    }
    
    hmap_free_keys(words, count);
    free(text_copy);
    hmap_destroy(&word_count);
    printf("\n");
}

// Global cache for fibonacci example
static HashMap* fib_cache = NULL;

// Helper function for fibonacci with caching
int get_fibonacci(int n) {
    char key[32];
    sprintf(key, "fib_%d", n);
    
    int* cached = hmap_get_int(fib_cache, key);
    if (cached) {
        printf("  fib(%d) = %d (from cache)\n", n, *cached);
        return *cached;
    }
    
    // Base cases
    if (n <= 1) {
        hmap_put_int(fib_cache, key, n);
        printf("  fib(%d) = %d (computed - base case)\n", n, n);
        return n;
    }
    
    // Recursive computation
    int result = get_fibonacci(n-1) + get_fibonacci(n-2);
    hmap_put_int(fib_cache, key, result);
    printf("  fib(%d) = %d (computed)\n", n, result);
    return result;
}

// Example 4: Caching system simulation
void example_cache_system() {
    printf("Example 4: Simple Cache System\n");
    printf("==============================\n");
    
    HashMap cache = hmap_create(0);
    fib_cache = &cache;  // Set global reference for helper function
    
    // Simulate expensive computations with caching
    printf("Computing fibonacci numbers with caching...\n");
    
    // Pre-populate cache with some values
    hmap_put_int(&cache, "fib_0", 0);
    hmap_put_int(&cache, "fib_1", 1);
    hmap_put_int(&cache, "fib_2", 1);
    hmap_put_int(&cache, "fib_3", 2);
    hmap_put_int(&cache, "fib_4", 3);
    hmap_put_int(&cache, "fib_5", 5);
    
    // Test cache hits and misses
    get_fibonacci(3);  // Should be cache hit
    get_fibonacci(7);  // Should compute and cache fib(6), fib(7)
    get_fibonacci(6);  // Should be cache hit
    
    printf("Cache now contains %zu entries\n", hmap_size(&cache));
    
    fib_cache = NULL;  // Clear global reference
    hmap_destroy(&cache);
    printf("\n");
}

int main() {
    printf("Hash Map Library - Practical Examples\n");
    printf("=====================================\n\n");
    
    example_configuration();
    example_user_database();
    example_word_counter();
    example_cache_system();
    
    printf("All examples completed successfully!\n");
    return 0;
}