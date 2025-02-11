#include <stdio.h>
#include <ctype.h>

void to_uppercase(char *str) {
    while (*str) {
        *str = toupper((unsigned char)*str);
        str++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: upper <strings>\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        to_uppercase(argv[i]);
        printf("%s ", argv[i]);
    }
    printf("\n");

    return 0;
}

