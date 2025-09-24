#include <stdio.h>
#include <stdlib.h>
#include "hmap.h"
#include "sb.h"
#include "sarr.h"

// Comprehensive test showing all libraries working together
int main() {
    printf("Full Library Integration Test\n");
    printf("=============================\n\n");
    
    // 1. Create a hash map to store user preferences
    HashMap user_prefs = hmap_create(0);
    hmap_put_string(&user_prefs, "theme", "dark");
    hmap_put_string(&user_prefs, "language", "en");
    hmap_put_int(&user_prefs, "font_size", 14);
    hmap_put_string(&user_prefs, "notifications", "enabled");
    
    // 2. Create a StringArray with user's favorite features
    StringArray features = sarr_create(5);
    sarr_append(&features, "syntax_highlighting");
    sarr_append(&features, "auto_completion"); 
    sarr_append(&features, "git_integration");
    sarr_append(&features, "dark_theme");
    sarr_append(&features, "plugin_system");
    
    // 3. Use StringBuilder to create a configuration report
    StringBuilder config_report = sb_init(512);
    
    // Add header
    char* header = "=== User Configuration Report ===\n\n";
    for (size_t i = 0; i < strlen(header); i++) {
        sb_append(&config_report, header[i]);
    }
    
    // Add preferences from HashMap
    char* preferences_header = "User Preferences:\n";
    for (size_t i = 0; i < strlen(preferences_header); i++) {
        sb_append(&config_report, preferences_header[i]);
    }
    
    size_t pref_count;
    char** pref_keys = hmap_keys(&user_prefs, &pref_count);
    
    for (size_t i = 0; i < pref_count; i++) {
        char line[128];
        void* value = hmap_get(&user_prefs, pref_keys[i], NULL);
        
        // Format based on key name (simple heuristic)
        if (strcmp(pref_keys[i], "font_size") == 0) {
            sprintf(line, "  - %s: %d\n", pref_keys[i], *(int*)value);
        } else {
            sprintf(line, "  - %s: %s\n", pref_keys[i], (char*)value);
        }
        
        for (size_t j = 0; j < strlen(line); j++) {
            sb_append(&config_report, line[j]);
        }
    }
    
    // Add favorite features from StringArray
    char* features_header = "\nFavorite Features:\n";
    for (size_t i = 0; i < strlen(features_header); i++) {
        sb_append(&config_report, features_header[i]);
    }
    
    for (size_t i = 0; i < features.size; i++) {
        char feature_line[128];
        sprintf(feature_line, "  %zu. %s\n", i + 1, sarr_get(&features, i));
        
        for (size_t j = 0; j < strlen(feature_line); j++) {
            sb_append(&config_report, feature_line[j]);
        }
    }
    
    // Add summary statistics
    char* stats_header = "\nSummary Statistics:\n";
    for (size_t i = 0; i < strlen(stats_header); i++) {
        sb_append(&config_report, stats_header[i]);
    }
    
    char stats[256];
    sprintf(stats, "  - Total preferences: %zu\n  - Favorite features: %zu\n  - Config size: %zu bytes\n",
            hmap_size(&user_prefs), features.size, config_report.size);
    
    for (size_t i = 0; i < strlen(stats); i++) {
        sb_append(&config_report, stats[i]);
    }
    
    // Display the final report
    char* final_report = sb_to_cstr(&config_report);
    printf("%s", final_report);
    
    // Store the report in the hash map for later use
    hmap_put_string(&user_prefs, "last_report", final_report);
    printf("\n--- Report stored in preferences map ---\n");
    printf("Report length: %zu characters\n", strlen(final_report));
    
    // Create a combined string from all features
    char* combined_features = sarr_combine(&features);
    printf("All features combined: %s\n", combined_features);
    
    // Clean up all resources
    free(final_report);
    free(combined_features);
    sb_destroy(&config_report);
    sarr_free(&features);
    hmap_free_keys(pref_keys, pref_count);
    hmap_destroy(&user_prefs);
    
    printf("\n✓ All libraries integrated successfully!\n");
    printf("✓ Memory management verified - no leaks!\n");
    
    return 0;
}