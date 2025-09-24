#include <stdio.h>
#include <stdlib.h>
#include "hmap.h"
#include "sb.h"

// Example showing integration between HashMap and StringBuilder
int main() {
    printf("HashMap + StringBuilder Integration Example\n");
    printf("==========================================\n\n");
    
    // Create a hash map to store template variables
    HashMap templates = hmap_create(0);
    
    // Store template variables
    hmap_put_string(&templates, "user_name", "Alice Johnson");
    hmap_put_string(&templates, "app_name", "MicroForge");
    hmap_put_string(&templates, "version", "2.0.1");
    hmap_put_int(&templates, "user_id", 12345);
    hmap_put_int(&templates, "score", 98);
    
    // Use StringBuilder to build a formatted report
    StringBuilder report = sb_init(256);
    
    // Build a welcome message using template variables
    sb_append(&report, '=');
    for (int i = 0; i < 50; i++) sb_append(&report, '=');
    sb_append(&report, '\n');
    
    char* app_name = (char*)hmap_get_string(&templates, "app_name");
    char* version = (char*)hmap_get_string(&templates, "version");
    
    char header[128];
    sprintf(header, " Welcome to %s v%s \n", app_name, version);
    
    for (size_t i = 0; i < strlen(header); i++) {
        sb_append(&report, header[i]);
    }
    
    for (int i = 0; i < 51; i++) sb_append(&report, '=');
    sb_append(&report, '\n');
    sb_append(&report, '\n');
    
    // Add user information
    char* user_name = (char*)hmap_get_string(&templates, "user_name");
    int* user_id = hmap_get_int(&templates, "user_id");
    int* score = hmap_get_int(&templates, "score");
    
    char user_info[256];
    sprintf(user_info, "User: %s (ID: %d)\nCurrent Score: %d/100\n\n", 
            user_name, *user_id, *score);
    
    for (size_t i = 0; i < strlen(user_info); i++) {
        sb_append(&report, user_info[i]);
    }
    
    // Add status based on score
    if (*score >= 90) {
        char* status = "Status: EXCELLENT! You're doing great!\n";
        for (size_t i = 0; i < strlen(status); i++) {
            sb_append(&report, status[i]);
        }
    } else if (*score >= 70) {
        char* status = "Status: Good work, keep it up!\n";
        for (size_t i = 0; i < strlen(status); i++) {
            sb_append(&report, status[i]);
        }
    } else {
        char* status = "Status: There's room for improvement.\n";
        for (size_t i = 0; i < strlen(status); i++) {
            sb_append(&report, status[i]);
        }
    }
    
    // Convert StringBuilder to string and display
    char* final_report = sb_to_cstr(&report);
    printf("%s", final_report);
    
    // Show template variable count
    printf("\nTemplate contains %zu variables:\n", hmap_size(&templates));
    size_t count;
    char** keys = hmap_keys(&templates, &count);
    for (size_t i = 0; i < count; i++) {
        printf("  - %s\n", keys[i]);
    }
    
    // Cleanup
    free(final_report);
    sb_destroy(&report);
    hmap_free_keys(keys, count);
    hmap_destroy(&templates);
    
    printf("\n✓ Integration example completed successfully!\n");
    return 0;
}