#include <stdio.h>
#include <stdlib.h>



int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "ERROR: No file provided\n");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR: could no open file '%s'.\n", argv[1]);
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buffer = malloc(file_size);
    fread(buffer, sizeof(char), file_size, fp);
    printf("%s\n", buffer);
    return EXIT_SUCCESS;
}
