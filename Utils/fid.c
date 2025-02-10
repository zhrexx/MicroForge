#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __WIN32
    #include "dirent.h"
    #include <windows.h>
    #include <tchar.h>
    #define stat _stat
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif 

int main(int argc, char *argv[])
{
    char *directory;
    if (argc == 1) {
        char *pwd = getenv("PWD");
        if (pwd == NULL) {
            directory = strdup(".");
        }
        else {
            directory = strdup(pwd);
        }
    }
    else {
        directory = strdup(argv[1]);
    }
    struct dirent *dp;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "ERROR: Opening of directory '%s' failed.\n", directory);
        exit(1);
    }
    size_t file_count = 0;
    while ((dp=readdir(dir))) {
        file_count++;
    }
    
    printf("Directory '%s' contains %zu elements\n", directory, file_count);
    closedir(dir);
    free(directory);
    return EXIT_SUCCESS;
}
