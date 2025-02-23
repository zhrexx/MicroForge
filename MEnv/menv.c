#include "menv.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    MEnv menv;
    menv_create_file_if_not_exists();
    menv_init(&menv);
    menv_load(&menv);
    if (!menv_exists(&menv, "HOME") && !menv_exists(&menv, "USERNAME")) {
#ifdef _WIN32
        char *home_path = getenv("USERPROFILE");
        char *username = getenv("USERNAME");
#else
        char *home_path = getenv("HOME");
        char *username = getenv("USER");
#endif
        menv_set(&menv, "HOME", home_path);
        menv_set(&menv, "USERNAME", username);
    }    
    if (argc < 2) {
        fprintf(stderr, "MEnv %.1f\n", MENV_VERSION);
        fprintf(stderr, "- get <key>\n");
        fprintf(stderr, "- set <key> <value>\n");
        fprintf(stderr, "- list\n");
        fprintf(stderr, "ERROR: No arguments providen\n");
        exit(1);
    }
    if (argc == 4 && strcmp(argv[1], "set") == 0) {
        menv_set(&menv, argv[2], argv[3]);
    } else if (argc == 3 && strcmp(argv[1], "get") == 0) {
        printf("%s=%s", argv[2], menv_get(&menv, argv[2]));
    } else if (argc == 2 && strcmp(argv[1], "list") == 0) { 
        print_all_vars(&menv);
    } else {
        fprintf(stderr, "ERROR: Invalid Argument: %s\n", argv[1]);
        exit(1);
    }
    
    menv_save(&menv);
    return EXIT_SUCCESS;
}
