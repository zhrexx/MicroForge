#ifndef ZHAH_H
#define ZHAH_H
#include <stdint.h>

#define HASH_SIZE 2048
#define HASH_KEY 0x1010
#define HASH_ANTI_RECOVER 0x9997
#define HASH_ANTI_RECOVER2 0x1929
#define HASH_ANTI_RECOVER3 82*91

uint64_t reverse(uint64_t n) {
    return (n >> 32) | (n << 32);
}

uint64_t mix(uint64_t n) {
    return reverse(n*HASH_ANTI_RECOVER*10)*HASH_KEY;
}

uint64_t zhah_hash(char *str) {
    uint64_t result_hash = 0;
    uint64_t key_mul = HASH_KEY * HASH_KEY;
    uint64_t key_recover = 0xFFFFFFFFFFFFFFFF*0xFFFFFFFFFFFFFFFF;

    size_t pos = 0;

    while (*str) {
        result_hash += reverse((*str + key_mul) * HASH_ANTI_RECOVER);
        result_hash *= key_recover^HASH_SIZE^key_recover^key_mul;
        result_hash *= mix(mix(key_recover**str));
        pos++;
        if (pos >= HASH_SIZE / 64) {
            pos = 0;
        }
        str++;
    }

    return result_hash;
}
#endif // ZHAH_H
