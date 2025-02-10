#include <stdio.h>
#include <stdlib.h>

#ifndef DEBUG
#define DEBUG 0
#endif

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s files...\n", argv[0]);
        fprintf(stderr, "ERROR: No file to remove provided.\n");
        exit(1);
    }
    
    for (int i = 1; i < argc; i++) {
        int s = remove(argv[i]);
        if (s < 0 && DEBUG) {
            fprintf(stderr, "ERROR: Failed to remove \"%s\".\n", argv[i]);
        }
        else {
            if (DEBUG) printf("Successfully removed \"%s\".\n", argv[i]);
        }
    }


    return EXIT_SUCCESS;
}
