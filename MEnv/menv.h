#ifndef MENV_H
#define MENV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MENV_VERSION 1.0

#define MAX_ENV_VARS 10000
#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 256

typedef struct {
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} EnvVar;

typedef struct {
    EnvVar vars[MAX_ENV_VARS];
    int count;
} MEnv;

void menv_init(MEnv *menv) {
    menv->count = 0;
}

void menv_create_file_if_not_exists() {
    const char *home = getenv("HOME");
    char menv_file_path[MAX_KEY_LENGTH];
    snprintf(menv_file_path, sizeof(menv_file_path), "%s/.menv", home);

    if (access(menv_file_path, F_OK) == -1) {
        FILE *file = fopen(menv_file_path, "w");
        if (file) {
            fclose(file);
        } else {
            perror("Could not create .menv file");
        }
    }
}

void menv_load(MEnv *menv) {
    const char *home = getenv("HOME");
    char menv_file_path[MAX_KEY_LENGTH];
    snprintf(menv_file_path, sizeof(menv_file_path), "%s/.menv", home);

    FILE *file = fopen(menv_file_path, "r");
    if (!file) {
        perror("Could not open .menv file");
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char *pos = strchr(line, '=');
        if (pos) {
            *pos = '\0';
            const char *key = line;
            const char *value = pos + 1;

            char temp_value[MAX_VALUE_LENGTH];
            strncpy(temp_value, value, MAX_VALUE_LENGTH);
            temp_value[strcspn(temp_value, "\n")] = 0;

            if (menv->count < MAX_ENV_VARS) {
                strncpy(menv->vars[menv->count].key, key, MAX_KEY_LENGTH);
                strncpy(menv->vars[menv->count].value, temp_value, MAX_VALUE_LENGTH);
                menv->count++;
            } else {
                fprintf(stderr, "Maximum environment variables exceeded.\n");
            }
        }
    }
    fclose(file);
}

const char *menv_get(const MEnv *menv, const char *key) {
    for (int i = 0; i < menv->count; i++) {
        if (strcmp(menv->vars[i].key, key) == 0) {
            return menv->vars[i].value;
        }
    }
    return NULL;
}

int menv_exists(MEnv *menv, const char *key) {
    for (int i = 0; i < menv->count; i++) {
        if (strcmp(menv->vars[i].key, key) == 0) {
            return 1;
        }
    }
    return 0;
}

void menv_set(MEnv *menv, const char *key, const char *value) {
    for (int i = 0; i < menv->count; i++) {
        if (strcmp(menv->vars[i].key, key) == 0) {
            strncpy(menv->vars[i].value, value, MAX_VALUE_LENGTH);
            return;
        }
    }
    if (menv->count < MAX_ENV_VARS) {
        strncpy(menv->vars[menv->count].key, key, MAX_KEY_LENGTH);
        strncpy(menv->vars[menv->count].value, value, MAX_VALUE_LENGTH);
        menv->count++;
    } else {
        fprintf(stderr, "Maximum environment variables exceeded.\n");
    }
}

void menv_save(const MEnv *menv) {
    const char *home = getenv("HOME");
    char menv_file_path[MAX_KEY_LENGTH];
    snprintf(menv_file_path, sizeof(menv_file_path), "%s/.menv", home);

    FILE *file = fopen(menv_file_path, "w");
    if (!file) {
        perror("Could not open .menv file for writing");
        return;
    }

    for (int i = 0; i < menv->count; i++) {
        fprintf(file, "%s=%s\n", menv->vars[i].key, menv->vars[i].value);
    }

    fclose(file);
}

void menv_remove(MEnv *menv, const char *key) {
    for (int i = 0; i < menv->count; i++) {
        if (strcmp(menv->vars[i].key, key) == 0) {
            for (int j = i; j < menv->count - 1; j++) {
                menv->vars[j] = menv->vars[j + 1];
            }
            menv->count--;
            return;
        }
    }
    fprintf(stderr, "Variable %s not found.\n", key);
}

void print_all_vars(const MEnv *menv) {
    for (int i = 0; i < menv->count; i++) {
        printf("%s=%s\n", menv->vars[i].key, menv->vars[i].value);
    }
}

#endif
