#include "xwbin.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        printf("Failed to open file: %s\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    uint8_t* bytes = (uint8_t*)malloc(file_size);
    fread(bytes, 1, file_size, file);
    fclose(file);
    
    XWBModule* module = xwb_parse_module(bytes, file_size);
    free(bytes);
    
    if (module->start_func_idx != 0) {
        XWBExecutionContext* ctx = xwb_create_context(module);
        xwb_execute_function(ctx, module->start_func_idx);
        xwb_free_context(ctx);
    }
    
    xwb_free_module(module);
    
    return 0;
}

