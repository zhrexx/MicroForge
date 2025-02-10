#include <stdio.h>
#include <stdlib.h>


#define CHECK_IFNF(f) {                                 \
    if (f == NULL) {                                    \
        fprintf(stderr, "ERROR: Cannot create FP\n");   \
        exit(1);                                        \
    }                                                   \
}                                                       


int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        fprintf(stderr, "ERROR: No input and output file provided.\n");
        exit(1);
    }

    char *input = argv[1];
    char *output = argv[2];

    FILE *_input = fopen(input, "rb");
    CHECK_IFNF(_input);
    FILE *_output = fopen(output, "wb");
    CHECK_IFNF(_output);
   
    fseek(_input, 0, SEEK_END);
    long file_size = ftell(_input);
    fseek(_input, 0, SEEK_SET);

    char *input_content = malloc(file_size);
    if (input_content == NULL) {
        fprintf(stderr, "Failed to allocate memory!\n");
    }

    fread(input_content, 1, file_size, _input);
    fwrite(input_content, 1, file_size, _output);

    return 0;
}



