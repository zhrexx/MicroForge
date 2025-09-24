#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hmap.h"

void test_basic_operations() {
    printf("Testing basic hash map operations...\n");
    
    HashMap map = hmap_create(0);  // Use default initial capacity
    
    // Test putting and getting string values
    assert(hmap_put_string(&map, "name", "John Doe"));
    assert(hmap_put_string(&map, "city", "New York"));
    assert(hmap_put_string(&map, "country", "USA"));
    
    assert(hmap_size(&map) == 3);
    assert(!hmap_is_empty(&map));
    
    // Test getting values
    const char* name = hmap_get_string(&map, "name");
    const char* city = hmap_get_string(&map, "city");
    const char* country = hmap_get_string(&map, "country");
    
    assert(name && strcmp(name, "John Doe") == 0);
    assert(city && strcmp(city, "New York") == 0);
    assert(country && strcmp(country, "USA") == 0);
    
    // Test contains
    assert(hmap_contains(&map, "name"));
    assert(hmap_contains(&map, "city"));
    assert(!hmap_contains(&map, "nonexistent"));
    
    // Test updating existing key
    assert(hmap_put_string(&map, "name", "Jane Smith"));
    name = hmap_get_string(&map, "name");
    assert(name && strcmp(name, "Jane Smith") == 0);
    assert(hmap_size(&map) == 3); // Size shouldn't change
    
    hmap_destroy(&map);
    printf("✓ Basic operations test passed\n");
}

void test_numeric_types() {
    printf("Testing numeric type operations...\n");
    
    HashMap map = hmap_create(0);
    
    // Test integer values
    assert(hmap_put_int(&map, "age", 30));
    assert(hmap_put_int(&map, "year", 2024));
    
    int* age = hmap_get_int(&map, "age");
    int* year = hmap_get_int(&map, "year");
    
    assert(age && *age == 30);
    assert(year && *year == 2024);
    
    // Test double values
    assert(hmap_put_double(&map, "pi", 3.14159));
    assert(hmap_put_double(&map, "e", 2.71828));
    
    double* pi = hmap_get_double(&map, "pi");
    double* e = hmap_get_double(&map, "e");
    
    assert(pi && *pi == 3.14159);
    assert(e && *e == 2.71828);
    
    assert(hmap_size(&map) == 4);
    
    hmap_destroy(&map);
    printf("✓ Numeric types test passed\n");
}

void test_removal_and_clearing() {
    printf("Testing removal and clearing operations...\n");
    
    HashMap map = hmap_create(0);
    
    // Add some entries
    assert(hmap_put_string(&map, "key1", "value1"));
    assert(hmap_put_string(&map, "key2", "value2"));
    assert(hmap_put_string(&map, "key3", "value3"));
    assert(hmap_size(&map) == 3);
    
    // Test removal
    assert(hmap_remove(&map, "key2"));
    assert(!hmap_contains(&map, "key2"));
    assert(hmap_size(&map) == 2);
    
    // Try to remove non-existent key
    assert(!hmap_remove(&map, "nonexistent"));
    assert(hmap_size(&map) == 2);
    
    // Test clearing
    hmap_clear(&map);
    assert(hmap_size(&map) == 0);
    assert(hmap_is_empty(&map));
    assert(!hmap_contains(&map, "key1"));
    
    hmap_destroy(&map);
    printf("✓ Removal and clearing test passed\n");
}

void test_key_enumeration() {
    printf("Testing key enumeration...\n");
    
    HashMap map = hmap_create(0);
    
    // Add entries with known keys
    const char* expected_keys[] = {"apple", "banana", "cherry", "date"};
    const char* expected_values[] = {"red", "yellow", "red", "brown"};
    size_t num_entries = sizeof(expected_keys) / sizeof(expected_keys[0]);
    
    for (size_t i = 0; i < num_entries; i++) {
        assert(hmap_put_string(&map, expected_keys[i], expected_values[i]));
    }
    
    // Get all keys
    size_t count;
    char** keys = hmap_keys(&map, &count);
    
    assert(count == num_entries);
    assert(keys != NULL);
    
    // Verify all expected keys are present
    for (size_t i = 0; i < num_entries; i++) {
        bool found = false;
        for (size_t j = 0; j < count; j++) {
            if (strcmp(keys[j], expected_keys[i]) == 0) {
                found = true;
                break;
            }
        }
        assert(found);
    }
    
    hmap_free_keys(keys, count);
    hmap_destroy(&map);
    printf("✓ Key enumeration test passed\n");
}

void test_collision_handling() {
    printf("Testing collision handling...\n");
    
    HashMap map = hmap_create(4);  // Small capacity to force collisions
    
    // Add many entries to test collision resolution
    char key[32], value[32];
    for (int i = 0; i < 20; i++) {
        sprintf(key, "key%d", i);
        sprintf(value, "value%d", i);
        assert(hmap_put_string(&map, key, value));
    }
    
    assert(hmap_size(&map) == 20);
    
    // Verify all entries can be retrieved correctly
    for (int i = 0; i < 20; i++) {
        sprintf(key, "key%d", i);
        sprintf(value, "value%d", i);
        const char* retrieved = hmap_get_string(&map, key);
        assert(retrieved && strcmp(retrieved, value) == 0);
    }
    
    hmap_destroy(&map);
    printf("✓ Collision handling test passed\n");
}

void test_resize_behavior() {
    printf("Testing resize behavior...\n");
    
    HashMap map = hmap_create(2);  // Very small initial capacity
    
    // Add entries beyond initial capacity to trigger resize
    char key[32], value[32];
    for (int i = 0; i < 50; i++) {
        sprintf(key, "item%d", i);
        sprintf(value, "data%d", i);
        assert(hmap_put_string(&map, key, value));
    }
    
    assert(hmap_size(&map) == 50);
    
    // Verify all entries are still accessible after resizing
    for (int i = 0; i < 50; i++) {
        sprintf(key, "item%d", i);
        sprintf(value, "data%d", i);
        const char* retrieved = hmap_get_string(&map, key);
        assert(retrieved && strcmp(retrieved, value) == 0);
    }
    
    hmap_destroy(&map);
    printf("✓ Resize behavior test passed\n");
}

void test_generic_data() {
    printf("Testing generic data storage...\n");
    
    HashMap map = hmap_create(0);
    
    // Test storing custom struct
    typedef struct {
        int x, y;
        char label[16];
    } Point;
    
    Point p1 = {10, 20, "origin"};
    Point p2 = {100, 200, "corner"};
    
    assert(hmap_put(&map, "point1", &p1, sizeof(Point)));
    assert(hmap_put(&map, "point2", &p2, sizeof(Point)));
    
    size_t value_size;
    Point* retrieved_p1 = (Point*)hmap_get(&map, "point1", &value_size);
    Point* retrieved_p2 = (Point*)hmap_get(&map, "point2", &value_size);
    
    assert(retrieved_p1 && value_size == sizeof(Point));
    assert(retrieved_p1->x == 10 && retrieved_p1->y == 20);
    assert(strcmp(retrieved_p1->label, "origin") == 0);
    
    assert(retrieved_p2 && value_size == sizeof(Point));
    assert(retrieved_p2->x == 100 && retrieved_p2->y == 200);
    assert(strcmp(retrieved_p2->label, "corner") == 0);
    
    hmap_destroy(&map);
    printf("✓ Generic data storage test passed\n");
}

void demonstrate_usage() {
    printf("\n--- Hash Map Usage Demonstration ---\n");
    
    HashMap config = hmap_create(0);
    
    // Store configuration values of different types
    hmap_put_string(&config, "server_name", "MicroForge Server");
    hmap_put_string(&config, "database_url", "localhost:5432");
    hmap_put_int(&config, "port", 8080);
    hmap_put_int(&config, "max_connections", 1000);
    hmap_put_double(&config, "timeout", 30.5);
    hmap_put_string(&config, "log_level", "INFO");
    
    printf("Configuration loaded with %zu entries:\n", hmap_size(&config));
    
    // Retrieve and display values
    printf("Server: %s\n", hmap_get_string(&config, "server_name"));
    printf("Database: %s\n", hmap_get_string(&config, "database_url"));
    printf("Port: %d\n", *hmap_get_int(&config, "port"));
    printf("Max Connections: %d\n", *hmap_get_int(&config, "max_connections"));
    printf("Timeout: %.1f seconds\n", *hmap_get_double(&config, "timeout"));
    printf("Log Level: %s\n", hmap_get_string(&config, "log_level"));
    
    // Enumerate all configuration keys
    size_t count;
    char** keys = hmap_keys(&config, &count);
    
    printf("\nAll configuration keys:\n");
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", keys[i]);
    }
    
    hmap_free_keys(keys, count);
    hmap_destroy(&config);
}

int main() {
    printf("Hash Map Library Test Suite\n");
    printf("===========================\n\n");
    
    test_basic_operations();
    test_numeric_types();
    test_removal_and_clearing();
    test_key_enumeration();
    test_collision_handling();
    test_resize_behavior();
    test_generic_data();
    
    demonstrate_usage();
    
    printf("\n✓ All tests passed! Hash map library is working correctly.\n");
    return 0;
}