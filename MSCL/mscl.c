
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mscl.h"

int main(void) {
    const char *data = "aaabbbcccdddd";
    size_t data_size = strlen(data);
    size_t comp_size;

    unsigned char *comp_buf = mscl_compress_buffer((const unsigned char *)data, data_size, &comp_size);
    if (!comp_buf) {
        printf("Compression failed\n");
        return 1;
    }

    printf("Original size: %zu, Compressed size: %zu, Ratio: %f\n",
           data_size, comp_size, mscl_get_compression_ratio(data_size, comp_size));

    unsigned char *decomp_buf = mscl_decompress_buffer(comp_buf, comp_size, data_size);
    if (!decomp_buf) {
        printf("Decompression failed\n");
        free(comp_buf);
        return 1;
    }
    printf("Compressed data: %s\n", comp_buf);
    printf("Decompressed data: %s\n", decomp_buf);

    free(comp_buf);
    free(decomp_buf);
    return 0;
}

