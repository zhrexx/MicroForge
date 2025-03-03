#include <stdio.h>
#include "zhah.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <string1> [string2] ...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        uint64_t hash = zhah_hash(argv[i]);
        printf("%lu\n", hash);
    }

    return 0;
}

