// TODO: Add colors like in ls -lh 
// TODO: Add options via args

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


size_t ls_filesize(char *filepath) {
    size_t size = 0;
    struct stat fileStat;
    if (stat(filepath, &fileStat) == 0) {
        size = (size_t) fileStat.st_size;
    }
    else {
        size = -1;
    }
    return size;
}

char *ls_pathcat(char *str1, char *str2)
{
    size_t strlen1 = strlen(str1);
    size_t strlen2 = strlen(str2);

    char *res = malloc(strlen1+strlen2+2);
    if (res == NULL) {
        fprintf(stderr, "ERROR: could not allocate memory\n");
        exit(1);
    }
    snprintf(res, strlen1 + strlen2+2, "%s/%s", str1, str2);
    return res;
}


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
    char *fullpath;
    while ((dp=readdir(dir))) {
        fullpath = ls_pathcat(directory, dp->d_name);
        size_t file_size = ls_filesize(fullpath);
        if (file_size == (size_t) -1) {
            fprintf(stderr, "ERROR: Getting file size failed.\n");
            exit(1);
        }
        printf("- %s | %zu \n", fullpath, file_size);
        free(fullpath);
    }

    closedir(dir);
    free(directory);
    return EXIT_SUCCESS;
}
