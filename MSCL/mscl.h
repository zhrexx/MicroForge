#ifndef MSCL_H
#define MSCL_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

size_t mscl_max_compressed_size(size_t in_size) {
    return in_size + ((in_size / 128) + 1) * 2;
}

int mscl_is_compressed_valid(const unsigned char *in, size_t in_size) {
    size_t i = 0;
    while (i < in_size) {
        signed char ctrl = (signed char)in[i];
        i++;
        if (ctrl >= 0) {
            size_t literal = (size_t)ctrl + 1;
            if (i + literal > in_size)
                return 0;
            i += literal;
        } else {
            if (i >= in_size)
                return 0;
            i++;
        }
    }
    return i == in_size;
}

double mscl_get_compression_ratio(size_t original_size, size_t compressed_size) {
    if (original_size == 0)
        return 0.0;
    return (double)compressed_size / original_size;
}

size_t mscl_compress(const unsigned char *in, size_t in_size, unsigned char *out, size_t out_size) {
    size_t ip = 0, op = 0;
    while (ip < in_size) {
        size_t run = 1;
        while (ip + run < in_size && run < 128 && in[ip] == in[ip + run])
            run++;
        if (run >= 3) {
            if (op + 2 > out_size)
                return 0;
            out[op++] = (unsigned char)(1 - run);
            out[op++] = in[ip];
            ip += run;
        } else {
            size_t lit_start = ip;
            size_t lit_count = 0;
            while (ip < in_size) {
                run = 1;
                while (ip + run < in_size && run < 128 && in[ip] == in[ip + run])
                    run++;
                if (run >= 3)
                    break;
                ip++;
                lit_count++;
                if (lit_count == 128)
                    break;
            }
            if (op + 1 + lit_count > out_size)
                return 0;
            out[op++] = (unsigned char)(lit_count - 1);
            memcpy(&out[op], &in[lit_start], lit_count);
            op += lit_count;
        }
    }
    return op;
}

size_t mscl_decompress(const unsigned char *in, size_t in_size, unsigned char *out, size_t out_size) {
    size_t ip = 0, op = 0;
    while (ip < in_size) {
        signed char ctrl = (signed char)in[ip++];
        if (ctrl >= 0) {
            size_t len = (size_t)ctrl + 1;
            if (ip + len > in_size || op + len > out_size)
                return 0;
            memcpy(&out[op], &in[ip], len);
            ip += len;
            op += len;
        } else {
            size_t len = (size_t)(1 - ctrl);
            if (ip >= in_size || op + len > out_size)
                return 0;
            memset(&out[op], in[ip++], len);
            op += len;
        }
    }
    return op;
}

int mscl_is_data_compressible(const unsigned char *in, size_t in_size) {
    size_t max_size = mscl_max_compressed_size(in_size);
    unsigned char *temp = (unsigned char*)malloc(max_size);
    if (!temp)
        return 0;
    size_t comp_size = mscl_compress(in, in_size, temp, max_size);
    free(temp);
    return comp_size && comp_size < in_size;
}

unsigned int mscl_calculate_checksum(const unsigned char *data, size_t size) {
    unsigned int checksum = 0;
    for (size_t i = 0; i < size; i++)
        checksum += data[i];
    return checksum;
}

unsigned char* mscl_compress_buffer(const unsigned char *in, size_t in_size, size_t *compressed_size) {
    size_t max_size = mscl_max_compressed_size(in_size);
    unsigned char *out = (unsigned char*)malloc(max_size);
    if (!out)
        return NULL;
    size_t comp_size = mscl_compress(in, in_size, out, max_size);
    if (comp_size == 0) {
        free(out);
        return NULL;
    }
    *compressed_size = comp_size;
    return out;
}

unsigned char* mscl_decompress_buffer(const unsigned char *in, size_t in_size, size_t out_size) {
    unsigned char *out = (unsigned char*)malloc(out_size);
    if (!out)
        return NULL;
    size_t decomp_size = mscl_decompress(in, in_size, out, out_size);
    if (decomp_size == 0) {
        free(out);
        return NULL;
    }
    return out;
}
#endif
