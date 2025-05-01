#ifndef XPROJECT_H
#define XPROJECT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define PLATFORM "win"

#include <process.h>
#define THREAD_HANDLE HANDLE
#define MUTEX_HANDLE CRITICAL_SECTION
#define MUTEX_INIT(mutex) InitializeCriticalSection(&mutex)
#define MUTEX_LOCK(mutex) EnterCriticalSection(&mutex)
#define MUTEX_UNLOCK(mutex) LeaveCriticalSection(&mutex)
#define MUTEX_DESTROY(mutex) DeleteCriticalSection(&mutex)
#define THREAD_CREATE(handle, func, arg) ((handle = (THREAD_HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL)) != 0)
#define THREAD_JOIN(handle) WaitForSingleObject(handle, INFINITE); CloseHandle(handle)
#define THREAD_DETACH(handle) CloseHandle(handle)
#define THREAD_RETURN_TYPE unsigned int WINAPI
#define THREAD_RETURN 0
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <dirent.h>
#include <unistd.h>
#define PLATFORM "linux"

#include <pthread.h>
#define THREAD_HANDLE pthread_t
#define MUTEX_HANDLE pthread_mutex_t
#define MUTEX_INIT(mutex) pthread_mutex_init(&mutex, NULL)
#define MUTEX_LOCK(mutex) pthread_mutex_lock(&mutex)
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&mutex)
#define MUTEX_DESTROY(mutex) pthread_mutex_destroy(&mutex)
#define THREAD_CREATE(handle, func, arg) (pthread_create(&handle, NULL, func, arg) == 0)
#define THREAD_JOIN(handle) pthread_join(handle, NULL)
#define THREAD_DETACH(handle) pthread_detach(handle)
#define THREAD_RETURN_TYPE void*
#define THREAD_RETURN NULL
#define SLEEP_MS(ms) usleep((ms) * 1000)

#endif

#include "dependencies/lua.h"
#include "dependencies/lauxlib.h"
#include "dependencies/lualib.h"

#define MAX_PATH 512
#define MAX_COMMAND 1024
#define MAX_NAME 128
#define MAX_TARGETS 64
#define MAX_SOURCES 128
#define MAX_DEPENDENCIES 64
#define MAX_FLAGS 64
#define MAX_THREADS 8

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#define MKDIR(path) _mkdir(path)
#define RMDIR(path) _rmdir(path)
#else
#define PATH_SEPARATOR "/"
#define MKDIR(path) mkdir(path, 0755)
#define RMDIR(path) rmdir(path)
#endif

typedef enum {
    TARGET_EXECUTABLE,
    TARGET_STATIC_LIB,
    TARGET_SHARED_LIB,
    TARGET_CUSTOM
} TargetType;

typedef struct {
    char name[MAX_NAME];
    char sources[MAX_SOURCES][MAX_PATH];
    char include_paths[MAX_DEPENDENCIES][MAX_PATH];
    char library_paths[MAX_DEPENDENCIES][MAX_PATH];
    char link_libraries[MAX_DEPENDENCIES][MAX_NAME];
    char custom_cflags[MAX_FLAGS][128];
    TargetType type;
    int source_count;
    int include_count;
    int lib_path_count;
    int link_lib_count;
    int custom_cflags_count;
} BuildTarget;

typedef struct {
    BuildTarget* target;
    int result;
} CompileJob;

typedef struct {
    BuildTarget targets[MAX_TARGETS];
    int target_count;
    char compiler[64];
    char cflags[256];
    char ldflags[256];
    char output_dir[MAX_PATH];
} BuildSystem;

BuildSystem build_system = {0};
MUTEX_HANDLE job_mutex;
int active_jobs = 0;
int max_parallel_jobs = 4;

void init_parallel_system() {
    MUTEX_INIT(job_mutex);
}

void cleanup_parallel_system() {
    MUTEX_DESTROY(job_mutex);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

void create_directory(const char* path) {
    #ifdef _WIN32
    struct _stat st = {0};
    if (_stat(path, &st) == -1) {
        MKDIR(path);
    }
    #else
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        MKDIR(path);
    }
    #endif
}

int file_exists(const char* path) {
    #ifdef _WIN32
    return _access(path, 0) != -1;
    #else
    return access(path, F_OK) != -1;
    #endif
}

int directory_exists(const char* path) {
    #ifdef _WIN32
    struct _stat st = {0};
    return _stat(path, &st) == 0 && (st.st_mode & _S_IFDIR);
    #else
    struct stat st = {0};
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
    #endif
}

int gout = 0;

int redirectOutput(const char *filename) {
    int original_stdout;
    FILE *fp;

#ifdef _WIN32
    original_stdout = _dup(_fileno(stdout));
    if (original_stdout == -1) {
        perror("Failed to duplicate stdout");
        return -1;
    }

    fp = freopen(filename, "w", stdout);
#else
    original_stdout = dup(STDOUT_FILENO);
    if (original_stdout == -1) {
        perror("Failed to duplicate stdout");
        return -1;
    }

    fp = freopen(filename, "w", stdout);
#endif

    if (fp == NULL) {
        perror("Failed to redirect stdout");
#ifdef _WIN32
        _dup2(original_stdout, _fileno(stdout));
        _close(original_stdout);
#else
        dup2(original_stdout, STDOUT_FILENO);
        close(original_stdout);
#endif
        return -1;
    }

    return original_stdout;
}

int restoreOutput(int original_stdout) {
    if (original_stdout < 0) {
        fprintf(stderr, "Invalid original stdout descriptor\n");
        return -1;
    }

    fflush(stdout);

#ifdef _WIN32
    fclose(stdout);

    if (_dup2(original_stdout, _fileno(stdout)) == -1) {
        perror("Failed to restore stdout");
        _close(original_stdout);
        return -1;
    }

    FILE *fp = _fdopen(_fileno(stdout), "w");
    if (fp == NULL) {
        perror("Failed to reopen stdout");
        return -1;
    }

    _close(original_stdout);
#else
    fclose(stdout);

    if (dup2(original_stdout, STDOUT_FILENO) == -1) {
        perror("Failed to restore stdout");
        close(original_stdout);
        return -1;
    }

    FILE *fp = fdopen(STDOUT_FILENO, "w");
    if (fp == NULL) {
        perror("Failed to reopen stdout");
        return -1;
    }

    close(original_stdout);
#endif

    return 0;
}

int system_command(const char* command) {
    return system(command);
}

int check_library(const char* library) {
    #ifdef _WIN32
    return 0;
    #else
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "pkg-config --exists %s", library);
    return system(cmd) == 0;
    #endif
}

int check_utility(const char* utility) {
    #ifdef _WIN32
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "where %s > nul 2>&1", utility);
    #else
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "which %s > /dev/null 2>&1", utility);
    #endif
    return system(cmd) == 0;
}

void add_target_flag(BuildTarget* target, const char* flag) {
    if (target->custom_cflags_count >= MAX_FLAGS) {
        log_error("Maximum number of custom flags reached for target %s", target->name);
    }

    char trimmed_flag[128];
    sscanf(flag, "%127s", trimmed_flag);

    for (int i = 0; i < target->custom_cflags_count; i++) {
        if (strcmp(target->custom_cflags[i], trimmed_flag) == 0) {
            return;
        }
    }

    strncpy(target->custom_cflags[target->custom_cflags_count++],
            trimmed_flag, sizeof(trimmed_flag) - 1);
}

int get_file_size(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return -1;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    fclose(file);

    return length;
}

char* read_file_contents(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    return buffer;
}

int write_file_contents(const char* path, const char* content) {
    FILE* file = fopen(path, "wb");
    if (!file) return 0;

    size_t written = fwrite(content, 1, strlen(content), file);
    fclose(file);

    return written == strlen(content);
}

char* list_directory(const char* path) {
    static char result[MAX_COMMAND] = {0};
    result[0] = '\0';

    #ifdef _WIN32
    WIN32_FIND_DATA findFileData;
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return result;
    }

    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            strcat(result, findFileData.cFileName);
            strcat(result, " ");
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
    #else
    DIR* dir;
    struct dirent* entry;

    dir = opendir(path);
    if (!dir) return result;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            strcat(result, entry->d_name);
            strcat(result, " ");
        }
    }
    closedir(dir);
    #endif

    return result;
}

char* get_absolute_path(const char* path) {
    static char absolute_path[MAX_PATH];
    #ifdef _WIN32
    if (_fullpath(absolute_path, path, MAX_PATH) == NULL) {
        return (char*)path;
    }
    #else
    if (realpath(path, absolute_path) == NULL) {
        return (char*)path;
    }
    #endif
    return absolute_path;
}

int hash_file(const char* path, char* hash_result) {
    #ifdef _WIN32
    FILE* pipe;
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "certutil -hashfile \"%s\" SHA256", path);

    pipe = _popen(cmd, "r");
    if (!pipe) return 0;

    char buffer[256];
    fgets(buffer, sizeof(buffer), pipe);
    fgets(buffer, sizeof(buffer), pipe);
    sscanf(buffer, "%s", hash_result);

    _pclose(pipe);
    #else
    char cmd[MAX_COMMAND];
    FILE* pipe;
    snprintf(cmd, sizeof(cmd), "sha256sum %s", path);

    pipe = popen(cmd, "r");
    if (!pipe) return 0;

    if (fgets(hash_result, 65, pipe) == NULL) {
        pclose(pipe);
        return 0;
    }

    pclose(pipe);
    #endif
    return 1;
}

BuildTarget* create_target(const char* name, TargetType type) {
    if (build_system.target_count >= MAX_TARGETS) {
        log_error("Maximum number of targets reached");
    }

    BuildTarget* target = &build_system.targets[build_system.target_count++];
    strncpy(target->name, name, sizeof(target->name) - 1);
    target->type = type;
    return target;
}

void add_source(BuildTarget* target, const char* source) {
    if (target->source_count >= MAX_SOURCES) {
        log_error("Maximum number of sources reached");
    }
    strncpy(target->sources[target->source_count++], source, MAX_PATH - 1);
}

void add_include_path(BuildTarget* target, const char* path) {
    if (target->include_count >= MAX_DEPENDENCIES) {
        log_error("Maximum number of include paths reached");
    }
    strncpy(target->include_paths[target->include_count++], path, MAX_PATH - 1);
}

void add_library_path(BuildTarget* target, const char* path) {
    if (target->lib_path_count >= MAX_DEPENDENCIES) {
        log_error("Maximum number of library paths reached");
    }
    strncpy(target->library_paths[target->lib_path_count++], path, MAX_PATH - 1);
}

void add_link_library(BuildTarget* target, const char* library) {
    if (target->link_lib_count >= MAX_DEPENDENCIES) {
        log_error("Maximum number of link libraries reached");
    }
    strncpy(target->link_libraries[target->link_lib_count++], library, MAX_NAME - 1);
}

char* build_target_flags(BuildTarget* target) {
    static char flags[MAX_COMMAND] = {0};
    flags[0] = '\0';

    for (int i = 0; i < target->custom_cflags_count; i++) {
        strcat(flags, target->custom_cflags[i]);
        strcat(flags, " ");
    }

    return flags;
}

char* build_include_flags(BuildTarget* target) {
    static char flags[MAX_COMMAND] = {0};
    flags[0] = '\0';

    for (int i = 0; i < target->include_count; i++) {
        strcat(flags, "-I");
        strcat(flags, target->include_paths[i]);
        strcat(flags, " ");
    }

    return flags;
}

char* build_library_flags(BuildTarget* target) {
    static char flags[MAX_COMMAND] = {0};
    flags[0] = '\0';

    for (int i = 0; i < target->lib_path_count; i++) {
        strcat(flags, "-L");
        strcat(flags, target->library_paths[i]);
        strcat(flags, " ");
    }

    for (int i = 0; i < target->link_lib_count; i++) {
        strcat(flags, "-l");
        strcat(flags, target->link_libraries[i]);
        strcat(flags, " ");
    }

    return flags;
}

void compile_target(BuildTarget* target) {
    char cmd[MAX_COMMAND];
    char sources[MAX_COMMAND] = {0};
    char output_path[MAX_PATH];
    char target_flags[MAX_COMMAND] = {0};

    strcat(target_flags, build_target_flags(target));

    create_directory(build_system.output_dir);

    for (int i = 0; i < target->source_count; i++) {
        strcat(sources, target->sources[i]);
        strcat(sources, " ");
    }

    snprintf(output_path, sizeof(output_path), "%s%s%s", build_system.output_dir, PATH_SEPARATOR, target->name);

    switch (target->type) {
        case TARGET_EXECUTABLE:
            #ifdef _WIN32
            snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s %s -o %s.exe",
                build_system.compiler,
                build_system.cflags,
                target_flags,
                sources,
                build_include_flags(target),
                build_library_flags(target),
                build_system.ldflags,
                output_path);
            #else
            snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s %s -o %s",
                build_system.compiler,
                build_system.cflags,
                target_flags,
                sources,
                build_include_flags(target),
                build_library_flags(target),
                build_system.ldflags,
                output_path);
            #endif
            break;
        case TARGET_STATIC_LIB:
            #ifdef _WIN32
            snprintf(cmd, sizeof(cmd), "lib /out:%s%s%s.lib %s",
                build_system.output_dir,
                PATH_SEPARATOR,
                target->name,
                sources);
            #else
            snprintf(cmd, sizeof(cmd), "ar rcs %s%s%s.a %s",
                build_system.output_dir,
                PATH_SEPARATOR,
                target->name,
                sources);
            #endif
            break;
        case TARGET_SHARED_LIB:
            #ifdef _WIN32
            snprintf(cmd, sizeof(cmd), "%s -shared %s %s %s %s %s -o %s%s%s.dll",
                build_system.compiler,
                sources,
                build_system.cflags,
                target_flags,
                build_include_flags(target),
                build_library_flags(target),
                build_system.output_dir,
                PATH_SEPARATOR,
                target->name);
            #else
            snprintf(cmd, sizeof(cmd), "%s -shared %s %s %s %s %s -o %s%s%s.so",
                build_system.compiler,
                sources,
                build_system.cflags,
                target_flags,
                build_include_flags(target),
                build_library_flags(target),
                build_system.output_dir,
                PATH_SEPARATOR,
                target->name);
            #endif
            break;
        default:
            log_error("Unsupported target type");
    }

    system(cmd);
}

THREAD_RETURN_TYPE compile_worker(void* arg) {
    CompileJob* job = (CompileJob*)arg;
    compile_target(job->target);

    MUTEX_LOCK(job_mutex);
    active_jobs--;
    MUTEX_UNLOCK(job_mutex);

    free(job);
    return THREAD_RETURN;
}

int compile_target_parallel(BuildTarget* target) {
    MUTEX_LOCK(job_mutex);
    while (active_jobs >= max_parallel_jobs) {
        MUTEX_UNLOCK(job_mutex);
        SLEEP_MS(10);  // 10ms
        MUTEX_LOCK(job_mutex);
    }

    CompileJob* job = malloc(sizeof(CompileJob));
    if (!job) {
        MUTEX_UNLOCK(job_mutex);
        return 0;
    }

    job->target = target;
    job->result = 0;

    THREAD_HANDLE thread;
    if (THREAD_CREATE(thread, compile_worker, job)) {
        active_jobs++;
        THREAD_DETACH(thread);
    } else {
        free(job);
        MUTEX_UNLOCK(job_mutex);
        return 0;
    }

    MUTEX_UNLOCK(job_mutex);
    return 1;
}

void wait_for_compilation() {
    while (1) {
        MUTEX_LOCK(job_mutex);
        int jobs = active_jobs;
        MUTEX_UNLOCK(job_mutex);

        if (jobs == 0) break;
        SLEEP_MS(100);  // 100ms
    }
}

int find_library(const char* name, char* result, size_t result_size, BuildTarget *target) {
    memset(result, 0, result_size);
    
#ifdef _WIN32
    const char* ext[] = { ".lib", ".dll", NULL };
    const char* paths[] = {
        "C:\\Windows\\System32",
        "C:\\Windows\\SysWOW64",
        NULL
    };
    
    char env_path[MAX_PATH * 10] = {0};
    if (GetEnvironmentVariable("PATH", env_path, sizeof(env_path)) > 0) {
        char search_path[MAX_PATH];
        
        for (int i = 0; ext[i] != NULL; i++) {
            char lib_name[MAX_PATH];
            snprintf(lib_name, sizeof(lib_name), "%s%s", name, ext[i]);
            
            for (int j = 0; paths[j] != NULL; j++) {
                snprintf(search_path, sizeof(search_path), "%s\\%s", paths[j], lib_name);
                if (file_exists(search_path)) {
                    strncpy(result, search_path, result_size - 1);
                    return 1;
                }
            }
            
            char* path_tok = strtok(env_path, ";");
            while (path_tok != NULL) {
                snprintf(search_path, sizeof(search_path), "%s\\%s", path_tok, lib_name);
                if (file_exists(search_path)) {
                    strncpy(result, search_path, result_size - 1);
                    return 1;
                }
                path_tok = strtok(NULL, ";");
            }
        }
    }
    if (target) {
        for (int i = 0; i < target->lib_path_count; i++) {
            for (int j = 0; ext[j] != NULL; j++) {
                char lib_name[MAX_PATH];
                snprintf(lib_name, sizeof(lib_name), "%s%s", name, ext[j]);
                char search_path[MAX_PATH];
                snprintf(search_path, sizeof(search_path), "%s\\%s", target->library_paths[i], lib_name);
                if (file_exists(search_path)) {
                    strncpy(result, search_path, result_size - 1);
                    return 1;
                }
            }
        }
    }
#else
    const char* ext[] = { ".so", ".a", NULL };
    const char* paths[] = {
        "/usr/lib",
        "/usr/local/lib",
        "/lib",
        "/lib64",
        "/usr/lib64",
        NULL
    };
    
    char cmd[MAX_COMMAND];
    FILE* pipe;
    
    snprintf(cmd, sizeof(cmd), "ldconfig -p | grep -E 'lib%s\\.so(\\.[0-9]+)*$'", name);
    pipe = popen(cmd, "r");
    if (pipe) {
        char buffer[MAX_PATH];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            char* arrow = strstr(buffer, " => ");
            if (arrow) {
                char* path_start = arrow + 4;
                while (*path_start == ' ') path_start++;
                path_start[strcspn(path_start, "\r\n")] = 0;
                strncpy(result, path_start, result_size - 1);
                pclose(pipe);
                return 1;
            }
        }
        pclose(pipe);
    }

    for (int i = 0; ext[i] != NULL; i++) {
        char lib_name[MAX_PATH];
        snprintf(lib_name, sizeof(lib_name), "lib%s%s", name, ext[i]);
        
        for (int j = 0; paths[j] != NULL; j++) {
            char search_path[MAX_PATH];
            snprintf(search_path, sizeof(search_path), "%s/%s", paths[j], lib_name);
            if (file_exists(search_path)) {
                strncpy(result, search_path, result_size - 1);
                return 1;
            }
        }
    }
    if (target) {
        for (int i = 0; i < target->lib_path_count; i++) {
            for (int j = 0; ext[j] != NULL; j++) {
                char lib_name[MAX_PATH];
                snprintf(lib_name, sizeof(lib_name), "lib%s%s", name, ext[j]);
                char search_path[MAX_PATH];
                snprintf(search_path, sizeof(search_path), "%s/%s", target->library_paths[i], lib_name);
                if (file_exists(search_path)) {
                    strncpy(result, search_path, result_size - 1);
                    return 1;
                }
            }
        }
    }
#endif
    
    return 0;
}

int find_executable(const char* name, char* result, size_t result_size) {
    memset(result, 0, result_size);
    
#ifdef _WIN32
    const char* ext[] = { ".exe", ".bat", ".cmd", NULL };
    char search_name[MAX_PATH];
    DWORD search_result;
    
    for (int i = 0; ext[i] != NULL; i++) {
        snprintf(search_name, sizeof(search_name), "%s%s", name, ext[i]);
        search_result = SearchPath(NULL, search_name, NULL, result_size, result, NULL);
        if (search_result > 0 && search_result < result_size) {
            return 1;
        }
    }
#else
    char cmd[MAX_COMMAND];
    FILE* pipe;
    
    snprintf(cmd, sizeof(cmd), "which %s 2>/dev/null", name);
    pipe = popen(cmd, "r");
    if (pipe) {
        if (fgets(result, result_size, pipe)) {
            result[strcspn(result, "\r\n")] = 0;
            pclose(pipe);
            return 1;
        }
        pclose(pipe);
    }
#endif
    
    return 0;
}

int find_include_path(const char* header, char* result, size_t result_size) {
    memset(result, 0, result_size);
    
#ifdef _WIN32
    const char* paths[] = {
        "C:\\Program Files\\Microsoft Visual Studio\\*\\*\\VC\\include",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\*\\*\\VC\\include",
        "C:\\Program Files (x86)\\Windows Kits\\*\\Include\\*",
        NULL
    };
    
    WIN32_FIND_DATA find_data;
    HANDLE find_handle;
    
    for (int i = 0; paths[i] != NULL; i++) {
        find_handle = FindFirstFile(paths[i], &find_data);
        if (find_handle != INVALID_HANDLE_VALUE) {
            do {
                if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    char search_path[MAX_PATH];
                    snprintf(search_path, sizeof(search_path), "%s\\%s", paths[i], find_data.cFileName);
                    
                    char file_path[MAX_PATH];
                    snprintf(file_path, sizeof(file_path), "%s\\%s", search_path, header);
                    
                    if (file_exists(file_path)) {
                        strncpy(result, search_path, result_size - 1);
                        FindClose(find_handle);
                        return 1;
                    }
                }
            } while (FindNextFile(find_handle, &find_data));
            FindClose(find_handle);
        }
    }
#else
    const char* paths[] = {
        "/usr/include",
        "/usr/local/include",
        "/opt/include",
        NULL
    };
    
    for (int i = 0; paths[i] != NULL; i++) {
        char search_path[MAX_PATH];
        snprintf(search_path, sizeof(search_path), "%s/%s", paths[i], header);
        if (file_exists(search_path)) {
            strncpy(result, paths[i], result_size - 1);
            return 1;
        }
    }
    
    FILE* pipe;
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "pkg-config --cflags-only-I 2>/dev/null | tr ' ' '\\n' | grep -v '^$' | sed 's/^-I//'");
    
    pipe = popen(cmd, "r");
    if (pipe) {
        char line[MAX_PATH];
        while (fgets(line, sizeof(line), pipe)) {
            line[strcspn(line, "\r\n")] = 0;
            if (strlen(line) > 0) {
                char search_path[MAX_PATH];
                snprintf(search_path, sizeof(search_path), "%s/%s", line, header);
                if (file_exists(search_path)) {
                    strncpy(result, line, result_size - 1);
                    pclose(pipe);
                    return 1;
                }
            }
        }
        pclose(pipe);
    }
#endif
    
    return 0;
}

int get_compiler_version(char* result, size_t result_size) {
    memset(result, 0, result_size);
    
    FILE* pipe;
    char cmd[MAX_COMMAND];
    
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "cl 2>&1");
#else
    snprintf(cmd, sizeof(cmd), "%s --version 2>&1", build_system.compiler);
#endif
    
    pipe = popen(cmd, "r");
    if (!pipe) {
        return 0;
    }
    
    char buffer[MAX_COMMAND];
    if (fgets(buffer, sizeof(buffer), pipe)) {
        strncpy(result, buffer, result_size - 1);
        result[strcspn(result, "\r\n")] = 0;
        pclose(pipe);
        return 1;
    }
    
    pclose(pipe);
    return 0;
}

int find_pkg_config(const char* package, char* cflags, size_t cflags_size, char* libs, size_t libs_size) {
    memset(cflags, 0, cflags_size);
    memset(libs, 0, libs_size);
    
#ifdef _WIN32
    return 0;
#else
    FILE* pipe;
    char cmd[MAX_COMMAND];
    
    if (!check_utility("pkg-config")) {
        return 0;
    }
    
    snprintf(cmd, sizeof(cmd), "pkg-config --cflags %s 2>/dev/null", package);
    pipe = popen(cmd, "r");
    if (pipe) {
        if (fgets(cflags, cflags_size, pipe)) {
            cflags[strcspn(cflags, "\r\n")] = 0;
        }
        pclose(pipe);
    }
    
    snprintf(cmd, sizeof(cmd), "pkg-config --libs %s 2>/dev/null", package);
    pipe = popen(cmd, "r");
    if (pipe) {
        if (fgets(libs, libs_size, pipe)) {
            libs[strcspn(libs, "\r\n")] = 0;
            return 1;
        }
        pclose(pipe);
    }
    
    return strlen(cflags) > 0 || strlen(libs) > 0;
#endif
}

char* get_os_name() {
    static char os_name[128] = {0};
    
#ifdef _WIN32
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    
    if (osvi.dwMajorVersion == 10) {
        strcpy(os_name, "Windows 10");
    } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3) {
        strcpy(os_name, "Windows 8.1");
    } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2) {
        strcpy(os_name, "Windows 8");
    } else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1) {
        strcpy(os_name, "Windows 7");
    } else {
        strcpy(os_name, "Windows");
    }
#else
    FILE* pipe = popen("lsb_release -ds 2>/dev/null || cat /etc/*release 2>/dev/null | head -n1 || uname -om", "r");
    if (pipe) {
        if (fgets(os_name, sizeof(os_name), pipe)) {
            os_name[strcspn(os_name, "\r\n")] = 0;
        }
        pclose(pipe);
    }
#endif
    
    return os_name;
}

int get_cpu_info(char* model_name, size_t name_size, int* cores, char* arch, size_t arch_size) {
    memset(model_name, 0, name_size);
    memset(arch, 0, arch_size);
    *cores = 0;
    
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    *cores = sysInfo.dwNumberOfProcessors;
    
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type;
        DWORD size = name_size;
        RegQueryValueEx(hKey, "ProcessorNameString", NULL, &type, (LPBYTE)model_name, &size);
        RegCloseKey(hKey);
    }
    
    SYSTEM_INFO systemInfo;
    GetNativeSystemInfo(&systemInfo);
    
    switch (systemInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:
            strncpy(arch, "x86_64", arch_size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_ARM:
            strncpy(arch, "arm", arch_size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            strncpy(arch, "arm64", arch_size - 1);
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            strncpy(arch, "x86", arch_size - 1);
            break;
        default:
            strncpy(arch, "unknown", arch_size - 1);
    }
#else
    FILE* pipe = popen("cat /proc/cpuinfo | grep 'model name' | head -1", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            char* value = strchr(buffer, ':');
            if (value) {
                value++;
                while (*value == ' ') value++;
                strncpy(model_name, value, name_size - 1);
                model_name[strcspn(model_name, "\r\n")] = 0;
            }
        }
        pclose(pipe);
    }
    
    pipe = popen("nproc 2>/dev/null || grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1", "r");
    if (pipe) {
        char buffer[16];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            *cores = atoi(buffer);
        }
        pclose(pipe);
    }
    
    pipe = popen("uname -m", "r");
    if (pipe) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            buffer[strcspn(buffer, "\r\n")] = 0;
            strncpy(arch, buffer, arch_size - 1);
        }
        pclose(pipe);
    }
#endif
    
    return strlen(model_name) > 0 && *cores > 0 && strlen(arch) > 0;
}

uint64_t get_available_memory() {
    uint64_t available_memory = 0;
    
#ifdef _WIN32
    if (*p == '\r' && *(p+1) == '\n') line_count++;
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        available_memory = status.ullAvailPhys;
    }
#else
    FILE* pipe = popen("free -b | grep 'Mem:' | awk '{print $7}'", "r");
    if (pipe) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            available_memory = strtoull(buffer, NULL, 10);
        }
        pclose(pipe);
    }
#endif
    
    return available_memory;
}

int is_dependency_newer(const char* target, const char* dependency) {
    struct stat target_stat, dependency_stat;
    
    if (stat(target, &target_stat) != 0) {
        return 1;
    }
    
    if (stat(dependency, &dependency_stat) != 0) {
        return 0;
    }
    
#ifdef _WIN32
    return dependency_stat.st_mtime > target_stat.st_mtime;
#else
    return dependency_stat.st_mtime > target_stat.st_mtime;
#endif
}

int find_all_files(const char* dir_path, const char* ext, char** files, int max_files) {
    int count = 0;
    
#ifdef _WIN32
    WIN32_FIND_DATA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];
    
    sprintf(search_path, "%s\\*", dir_path);
    find_handle = FindFirstFile(search_path, &find_data);
    
    if (find_handle != INVALID_HANDLE_VALUE) {
        do {
            if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && 
                strcmp(find_data.cFileName, ".") != 0 && 
                strcmp(find_data.cFileName, "..") != 0) {
                
                char next_dir[MAX_PATH];
                sprintf(next_dir, "%s\\%s", dir_path, find_data.cFileName);
                count += find_all_files(next_dir, ext, files + count, max_files - count);
            }
            else if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                char* file_ext = strrchr(find_data.cFileName, '.');
                if (file_ext && stricmp(file_ext, ext) == 0) {
                    if (count < max_files) {
                        files[count] = malloc(MAX_PATH);
                        sprintf(files[count], "%s\\%s", dir_path, find_data.cFileName);
                        count++;
                    }
                }
            }
        } while (FindNextFile(find_handle, &find_data) && count < max_files);
        
        FindClose(find_handle);
    }
#else
    DIR* dir;
    struct dirent* entry;
    
    if ((dir = opendir(dir_path)) != NULL) {
        while ((entry = readdir(dir)) != NULL && count < max_files) {
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
            
            struct stat path_stat;
            stat(path, &path_stat);
            
            if (S_ISDIR(path_stat.st_mode) && 
                strcmp(entry->d_name, ".") != 0 && 
                strcmp(entry->d_name, "..") != 0) {
                
                count += find_all_files(path, ext, files + count, max_files - count);
            }
            else if (!S_ISDIR(path_stat.st_mode)) {
                char* file_ext = strrchr(entry->d_name, '.');
                if (file_ext && strcasecmp(file_ext, ext) == 0) {
                    if (count < max_files) {
                        files[count] = malloc(MAX_PATH);
                        strcpy(files[count], path);
                        count++;
                    }
                }
            }
        }
        closedir(dir);
    }
#endif
    
    return count;
}

int find_file_in_path(const char* filename, char* result, size_t result_size) {
    memset(result, 0, result_size);
    
#ifdef _WIN32
    char* env_path = getenv("PATH");
    if (!env_path) {
        return 0;
    }
    
    char* path_copy = strdup(env_path);
    char* path_token = strtok(path_copy, ";");
    
    while (path_token) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s\\%s", path_token, filename);
        
        if (file_exists(full_path)) {
            strncpy(result, full_path, result_size - 1);
            free(path_copy);
            return 1;
        }
        
        path_token = strtok(NULL, ";");
    }
    
    free(path_copy);
#else
    char* env_path = getenv("PATH");
    if (!env_path) {
        return 0;
    }
    
    char* path_copy = strdup(env_path);
    char* path_token = strtok(path_copy, ":");
    
    while (path_token) {
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path_token, filename);
        
        if (file_exists(full_path)) {
            strncpy(result, full_path, result_size - 1);
            free(path_copy);
            return 1;
        }
        
        path_token = strtok(NULL, ":");
    }
    
    free(path_copy);
#endif
    
    return 0;
}

int find_include(const char* name, char* result, size_t result_size) {
    memset(result, 0, result_size);
    
#ifdef _WIN32
    const char* ext[] = { ".h", ".hpp", ".hxx", NULL };
    char* include_paths = getenv("INCLUDE");
    if (include_paths) {
        char* path_copy = strdup(include_paths);
        char* path_token = strtok(path_copy, ";");
        while (path_token) {
            for (int i = 0; ext[i] != NULL; i++) {
                char include_name[MAX_PATH];
                snprintf(include_name, sizeof(include_name), "%s%s", name, ext[i]);
                char search_path[MAX_PATH];
                snprintf(search_path, sizeof(search_path), "%s\\%s", path_token, include_name);
                if (file_exists(search_path)) {
                    strncpy(result, search_path, result_size - 1);
                    free(path_copy);
                    return 1;
                }
            }
            path_token = strtok(NULL, ";");
        }
        free(path_copy);
    }
#else
    const char* ext[] = { "", ".h", ".hpp", ".hxx", NULL };
    FILE* pipe;
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "gcc -E -Wp,-v -xc - </dev/null 2>&1");
    pipe = popen(cmd, "r");
    if (pipe) {
        char line[MAX_PATH];
        int in_include = 0;
        while (fgets(line, sizeof(line), pipe)) {
            line[strcspn(line, "\r\n")] = 0;
            if (strstr(line, "#include <...> search starts here:")) {
                in_include = 1;
                continue;
            }
            if (strstr(line, "End of search list.")) {
                in_include = 0;
                continue;
            }
            if (in_include) {
                char* path = line;
                while (*path == ' ') path++;
                size_t len = strlen(path);
                while (len > 0 && (path[len-1] == ' ' || path[len-1] == '\t')) len--;
                path[len] = '\0';

                for (int i = 0; ext[i] != NULL; i++) {
                    char include_name[MAX_PATH];
                    snprintf(include_name, sizeof(include_name), "%s%s", name, ext[i]);
                    char search_path[MAX_PATH];
                    snprintf(search_path, sizeof(search_path), "%s/%s", path, include_name);
                    if (file_exists(search_path)) {
                        strncpy(result, search_path, result_size - 1);
                        pclose(pipe);
                        return 1;
                    }
                }
            }
        }
        pclose(pipe);
    }
#endif

    return 0;
}


int check_function(const char* library, const char* function_name) {
    char cmd[MAX_COMMAND];
    FILE* pipe;
    int found = 0;

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "dumpbin /EXPORTS \"%s\" | findstr /C:\"%s\" > nul", library, function_name);
    if (system(cmd) == 0) {
        found = 1;
    }
#else
    snprintf(cmd, sizeof(cmd), "nm -D \"%s\" 2>/dev/null | grep ' %s$'", library, function_name);
    pipe = popen(cmd, "r");
    if (pipe) {
        if (fgets(cmd, sizeof(cmd), pipe)) {
            found = 1;
        }
        pclose(pipe);
    }
    if (!found) {
        snprintf(cmd, sizeof(cmd), "nm \"%s\" 2>/dev/null | grep ' %s$'", library, function_name);
        pipe = popen(cmd, "r");
        if (pipe) {
            if (fgets(cmd, sizeof(cmd), pipe)) {
                found = 1;
            }
            pclose(pipe);
        }
    }
#endif

    return found;
}


int dmode = 1;

static int lua_check_library(lua_State* L) {
    const char* library = luaL_checkstring(L, 1);
    lua_pushboolean(L, check_library(library));
    return 1;
}

static int lua_check_utility(lua_State* L) {
    const char* utility = luaL_checkstring(L, 1);
    lua_pushboolean(L, check_utility(utility));
    return 1;
}

static int lua_file_exists(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushboolean(L, file_exists(path));
    return 1;
}

static int lua_directory_exists(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushboolean(L, directory_exists(path));
    return 1;
}

static int lua_system_command(lua_State* L) {
    const char* command = luaL_checkstring(L, 1);
    if (dmode) printf("Executing '%s'\n", command);
    lua_pushinteger(L, system_command(command));
    return 1;
}

static int lua_add_target_flag(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    const char* flag = luaL_checkstring(L, 2);
    add_target_flag(target, flag);
    return 0;
}

static int lua_create_target(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    int type = luaL_checkinteger(L, 2);
    BuildTarget* target = create_target(name, type);
    lua_pushlightuserdata(L, target);
    return 1;
}

static int lua_add_source(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    const char* source = luaL_checkstring(L, 2);
    add_source(target, source);
    return 0;
}

static int lua_add_include_path(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    const char* path = luaL_checkstring(L, 2);
    add_include_path(target, path);
    return 0;
}

static int lua_add_library_path(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    const char* path = luaL_checkstring(L, 2);
    add_library_path(target, path);
    return 0;
}

static int lua_add_link_library(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    const char* library = luaL_checkstring(L, 2);
    add_link_library(target, library);
    return 0;
}

static int lua_compile_target(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    if (dmode) printf("Compiling target %s\n", target->name);
    compile_target(target);
    return 0;
}

static int lua_get_file_size(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushinteger(L, get_file_size(path));
    return 1;
}

static int lua_read_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    char* content = read_file_contents(path);
    if (content) {
        lua_pushstring(L, content);
        free(content);
        return 1;
    }
    return 0;
}

static int lua_write_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* content = luaL_checkstring(L, 2);
    lua_pushboolean(L, write_file_contents(path, content));
    return 1;
}

static int lua_list_directory(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushstring(L, list_directory(path));
    return 1;
}

static int lua_get_absolute_path(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushstring(L, get_absolute_path(path));
    return 1;
}

static int lua_hash_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    char hash_result[65] = {0};
    if (hash_file(path, hash_result)) {
        lua_pushstring(L, hash_result);
        return 1;
    }
    return 0;
}

static int lua_get_platform(lua_State *L) {
    lua_pushstring(L, PLATFORM);
    return 1;
}

static void push_function(lua_State *L, lua_CFunction f, const char *name) {
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, name);
}

static int lua_redirect_output(lua_State *L) {
    gout = redirectOutput(luaL_checkstring(L, 1));
    return 0;
}

static int lua_restore_output(lua_State *L) {
    restoreOutput(gout);
    return 0;
}

static int lua_set_output_directory(lua_State *L) {
    strcpy(build_system.output_dir, luaL_checkstring(L, 1));
    return 0;
}

static int lua_set_max_jobs(lua_State* L) {
    int jobs = luaL_checkinteger(L, 1);
    if (jobs > 0 && jobs <= MAX_THREADS) {
        max_parallel_jobs = jobs;
    }
    return 0;
}

static int lua_compile_target_parallel(lua_State* L) {
    BuildTarget* target = lua_touserdata(L, 1);
    if (dmode) printf("Compiling target %s\n", target->name);
    lua_pushboolean(L, compile_target_parallel(target));
    return 1;
}

static int lua_wait_for_compilation(lua_State* L) {
    wait_for_compilation();
    return 0;
}

static int lua_find_library(lua_State* L) {
    BuildTarget* target = NULL;
    const char* name;
    int nargs = lua_gettop(L);
    
    if (nargs == 2) {
        target = lua_touserdata(L, 1);
        name = luaL_checkstring(L, 2);
    } else if (nargs == 1) {
        name = luaL_checkstring(L, 1);
    } else {
        return luaL_error(L, "expected 1 or 2 arguments");
    }
    
    char result[MAX_PATH] = {0};
    if (find_library(name, result, sizeof(result), target)) {
        lua_pushstring(L, result);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_find_executable(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    char result[MAX_PATH] = {0};
    
    if (find_executable(name, result, sizeof(result))) {
        lua_pushstring(L, result);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_find_include_path(lua_State* L) {
    const char* header = luaL_checkstring(L, 1);
    char result[MAX_PATH] = {0};
    
    if (find_include_path(header, result, sizeof(result))) {
        lua_pushstring(L, result);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_get_compiler_version(lua_State* L) {
    char result[MAX_COMMAND] = {0};
    
    if (get_compiler_version(result, sizeof(result))) {
        lua_pushstring(L, result);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_find_pkg_config(lua_State* L) {
    const char* package = luaL_checkstring(L, 1);
    char cflags[MAX_COMMAND] = {0};
    char libs[MAX_COMMAND] = {0};
    
    if (find_pkg_config(package, cflags, sizeof(cflags), libs, sizeof(libs))) {
        lua_newtable(L);
        
        lua_pushstring(L, cflags);
        lua_setfield(L, -2, "cflags");
        
        lua_pushstring(L, libs);
        lua_setfield(L, -2, "libs");
        
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_get_os_name(lua_State* L) {
    lua_pushstring(L, get_os_name());
    return 1;
}

static int lua_get_cpu_info(lua_State* L) {
    char model_name[256] = {0};
    int cores = 0;
    char arch[256] = {0}; 
    if (get_cpu_info(model_name, sizeof(model_name), &cores, arch, sizeof(arch))) {
        lua_newtable(L);
        
        lua_pushstring(L, model_name);
        lua_setfield(L, -2, "model");
        
        lua_pushinteger(L, cores);
        lua_setfield(L, -2, "cores");
        
        lua_pushstring(L, arch);
        lua_setfield(L, -2, "arch");
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_get_available_memory(lua_State* L) {
    uint64_t memory = get_available_memory();
    
#if LUA_VERSION_NUM >= 503
    lua_pushinteger(L, (lua_Integer)memory);
#else
    lua_pushnumber(L, (lua_Number)memory);
#endif
    
    return 1;
}

static int lua_is_dependency_newer(lua_State* L) {
    const char* target = luaL_checkstring(L, 1);
    const char* dependency = luaL_checkstring(L, 2);
    
    lua_pushboolean(L, is_dependency_newer(target, dependency));
    return 1;
}

static int lua_find_all_files(lua_State* L) {
    const char* dir_path = luaL_checkstring(L, 1);
    const char* ext = luaL_checkstring(L, 2);
    
    char* files[1024];
    int count = find_all_files(dir_path, ext, files, 1024);
    
    lua_createtable(L, count, 0);
    
    for (int i = 0; i < count; i++) {
        lua_pushstring(L, files[i]);
        lua_rawseti(L, -2, i + 1);
        free(files[i]);
    }
    
    return 1;
}

static int lua_find_file_in_path(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    char result[MAX_PATH] = {0};
    
    if (find_file_in_path(filename, result, sizeof(result))) {
        lua_pushstring(L, result);
        return 1;
    }
    
    lua_pushnil(L);
    return 1;
}

static int lua_remove_directory_recursive(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int result = 0;
    
#ifdef _WIN32
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "rmdir /S /Q \"%s\"", path);
    result = system(cmd) == 0;
#else
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", path);
    result = system(cmd) == 0;
#endif
    
    lua_pushboolean(L, result);
    return 1;
}

static int lua_copy_file(lua_State* L) {
    const char* src = luaL_checkstring(L, 1);
    const char* dst = luaL_checkstring(L, 2);
    int result = 0;
    
#ifdef _WIN32
    result = CopyFile(src, dst, FALSE);
#else
    char cmd[MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", src, dst);
    result = system(cmd) == 0;
#endif
    
    lua_pushboolean(L, result);
    return 1;
}

static int lua_get_current_directory(lua_State* L) {
    char cwd[MAX_PATH];
    
#ifdef _WIN32
    if (_getcwd(cwd, sizeof(cwd)) != NULL) {
        lua_pushstring(L, cwd);
        return 1;
    }
#else
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        lua_pushstring(L, cwd);
        return 1;
    }
#endif
    
    lua_pushnil(L);
    return 1;
}

static int lua_set_current_directory(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    int result = 0;
    
#ifdef _WIN32
    result = _chdir(path) == 0;
#else
    result = chdir(path) == 0;
#endif
    
    lua_pushboolean(L, result);
    return 1;
}

static int lua_get_temp_directory(lua_State* L) {
    char temp_dir[MAX_PATH];
    
#ifdef _WIN32
    if (GetTempPath(MAX_PATH, temp_dir)) {
        lua_pushstring(L, temp_dir);
        return 1;
    }
#else
    const char* tmp = getenv("TMPDIR");
    if (tmp) {
        lua_pushstring(L, tmp);
        return 1;
    }
    lua_pushstring(L, "/tmp");
    return 1;
#endif
    
    lua_pushnil(L);
    return 1;
}

static int lua_generate_temp_filename(lua_State* L) {
    char temp_file[MAX_PATH];
    
#ifdef _WIN32
    char temp_path[MAX_PATH];
    char temp_name[MAX_PATH];
    
    if (GetTempPath(MAX_PATH, temp_path) && GetTempFileName(temp_path, "xpj", 0, temp_name)) {
        lua_pushstring(L, temp_name);
        return 1;
    }
#else
    snprintf(temp_file, sizeof(temp_file), "/tmp/xpj_XXXXXX");
    int fd = mkstemp(temp_file);
    if (fd != -1) {
        close(fd);
        lua_pushstring(L, temp_file);
        return 1;
    }
#endif
    
    lua_pushnil(L);
    return 1;
}

static int lua_find_include(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    char result[MAX_PATH] = {0};
    int found = find_include(filename, result, MAX_PATH);

    lua_createtable(L, 0, 2);
    
    lua_pushboolean(L, found);
    lua_setfield(L, -2, "found");
    
    if (found) {
        lua_pushstring(L, result);
        lua_setfield(L, -2, "path");
    }

    return 1;
}

static int lua_check_function(lua_State* L) {
    const char* library = luaL_checkstring(L, 1);
    const char* function = luaL_checkstring(L, 2);
    lua_pushboolean(L, check_function(library, function));
    return 1;
}

static int lua_remove_file(lua_State *L) {
    const char *file = luaL_checkstring(L, 1);
    #ifdef WIN32 
    _unlink(file);
    #else 
    unlink(file);
    #endif
}

void setup_lua_functions(lua_State* L) {
    lua_newtable(L);

    push_function(L, lua_check_library, "check_library");
    push_function(L, lua_check_utility, "check_utility");
    push_function(L, lua_file_exists, "file_exists");
    push_function(L, lua_directory_exists, "directory_exists");
    push_function(L, lua_system_command, "system");
    push_function(L, lua_create_target, "create_target");
    push_function(L, lua_add_source, "add_source");
    push_function(L, lua_add_include_path, "add_include_path");
    push_function(L, lua_add_library_path, "add_library_path");
    push_function(L, lua_add_link_library, "add_link_library");
    push_function(L, lua_add_target_flag, "add_target_flag");
    push_function(L, lua_compile_target, "compile_target");
    push_function(L, lua_get_file_size, "get_file_size");
    push_function(L, lua_read_file, "read_file");
    push_function(L, lua_write_file, "write_file");
    push_function(L, lua_list_directory, "list_directory");
    push_function(L, lua_get_absolute_path, "get_absolute_path");
    push_function(L, lua_hash_file, "hash_file");
    push_function(L, lua_get_platform, "get_platform");
    push_function(L, lua_redirect_output, "redirect_output");
    push_function(L, lua_restore_output, "restore_output");
    push_function(L, lua_set_output_directory, "set_output_directory");
    push_function(L, lua_compile_target_parallel, "compile_target_parallel");
    push_function(L, lua_wait_for_compilation, "wait_for_compilation");
    push_function(L, lua_set_max_jobs, "set_max_jobs");
    push_function(L, lua_find_library, "find_library");
    push_function(L, lua_find_executable, "find_executable");
    push_function(L, lua_find_include_path, "find_include_path");
    push_function(L, lua_get_compiler_version, "get_compiler_version");
    push_function(L, lua_find_pkg_config, "find_pkg_config");
    push_function(L, lua_get_os_name, "get_os_name");
    push_function(L, lua_get_cpu_info, "get_cpu_info");
    push_function(L, lua_get_available_memory, "get_available_memory");
    push_function(L, lua_is_dependency_newer, "is_dependency_newer");
    push_function(L, lua_find_all_files, "find_all_files");
    push_function(L, lua_find_file_in_path, "find_file_in_path");
    push_function(L, lua_remove_directory_recursive, "remove_directory_recursive");
    push_function(L, lua_copy_file, "copy_file");
    push_function(L, lua_get_current_directory, "get_current_directory");
    push_function(L, lua_set_current_directory, "set_current_directory");
    push_function(L, lua_get_temp_directory, "get_temp_directory");
    push_function(L, lua_generate_temp_filename, "generate_temp_filename");
    push_function(L, lua_find_include, "find_include");
    push_function(L, lua_check_function, "check_function");
    push_function(L, lua_remove_file, "remove_file");
    lua_setglobal(L, "x");

    lua_pushinteger(L, TARGET_EXECUTABLE);
    lua_setglobal(L, "TARGET_EXECUTABLE");
    lua_pushinteger(L, TARGET_STATIC_LIB);
    lua_setglobal(L, "TARGET_STATIC_LIB");
    lua_pushinteger(L, TARGET_SHARED_LIB);
    lua_setglobal(L, "TARGET_SHARED_LIB");
}
#endif
