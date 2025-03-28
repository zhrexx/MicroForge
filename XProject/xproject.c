#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define PLATFORM "win"
#else
#include <dirent.h>
#include <unistd.h>
#define PLATFORM "linux"
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
    TargetType type;
    int source_count;
    int include_count;
    int lib_path_count;
    int link_lib_count;
} BuildTarget;

typedef struct {
    BuildTarget targets[MAX_TARGETS];
    int target_count;
    char compiler[64];
    char cflags[256];
    char ldflags[256];
    char output_dir[MAX_PATH];
} BuildSystem;

BuildSystem build_system = {0};

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

    create_directory(build_system.output_dir);

    for (int i = 0; i < target->source_count; i++) {
        strcat(sources, target->sources[i]);
        strcat(sources, " ");
    }

    snprintf(output_path, sizeof(output_path), "%s%s%s", build_system.output_dir, PATH_SEPARATOR, target->name);

    switch (target->type) {
        case TARGET_EXECUTABLE:
            #ifdef _WIN32
            snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s -o %s.exe", 
                build_system.compiler, 
                build_system.cflags, 
                sources, 
                build_include_flags(target),
                build_library_flags(target),
                build_system.ldflags,
                output_path);
            #else
            snprintf(cmd, sizeof(cmd), "%s %s %s %s %s %s -o %s", 
                build_system.compiler, 
                build_system.cflags, 
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
            snprintf(cmd, sizeof(cmd), "%s -shared %s %s %s %s -o %s%s%s.dll", 
                build_system.compiler,
                sources,
                build_system.cflags,
                build_include_flags(target),
                build_library_flags(target),
                build_system.output_dir,
                PATH_SEPARATOR,
                target->name);
            #else
            snprintf(cmd, sizeof(cmd), "%s -shared %s %s %s %s -o %s%s%s.so", 
                build_system.compiler,
                sources,
                build_system.cflags,
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
    lua_pushinteger(L, system_command(command));
    return 1;
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

void setup_lua_functions(lua_State* L) {
    // functions
    lua_newtable(L);

    lua_pushcfunction(L, lua_check_library);
    lua_setfield(L, -2, "check_library");

    lua_pushcfunction(L, lua_check_utility);
    lua_setfield(L, -2, "check_utility");

    lua_pushcfunction(L, lua_file_exists);
    lua_setfield(L, -2, "file_exists");

    lua_pushcfunction(L, lua_directory_exists);
    lua_setfield(L, -2, "directory_exists");

    lua_pushcfunction(L, lua_system_command);
    lua_setfield(L, -2, "system_command");

    lua_pushcfunction(L, lua_create_target);
    lua_setfield(L, -2, "create_target");

    lua_pushcfunction(L, lua_add_source);
    lua_setfield(L, -2, "add_source");

    lua_pushcfunction(L, lua_add_include_path);
    lua_setfield(L, -2, "add_include_path");

    lua_pushcfunction(L, lua_add_library_path);
    lua_setfield(L, -2, "add_library_path");

    lua_pushcfunction(L, lua_add_link_library);
    lua_setfield(L, -2, "add_link_library");

    lua_pushcfunction(L, lua_compile_target);
    lua_setfield(L, -2, "compile_target");

    lua_pushcfunction(L, lua_get_file_size);
    lua_setfield(L, -2, "get_file_size");

    lua_pushcfunction(L, lua_read_file);
    lua_setfield(L, -2, "read_file");

    lua_pushcfunction(L, lua_write_file);
    lua_setfield(L, -2, "write_file");

    lua_pushcfunction(L, lua_list_directory);
    lua_setfield(L, -2, "list_directory");

    lua_pushcfunction(L, lua_get_absolute_path);
    lua_setfield(L, -2, "get_absolute_path");

    lua_pushcfunction(L, lua_hash_file);
    lua_setfield(L, -2, "hash_file");

    lua_pushcfunction(L, lua_get_platform);
    lua_setfield(L, -2, "get_platform");

    lua_setglobal(L, "x");

    // variables
    lua_pushinteger(L, TARGET_EXECUTABLE);
    lua_setglobal(L, "TARGET_EXECUTABLE");
    lua_pushinteger(L, TARGET_STATIC_LIB);
    lua_setglobal(L, "TARGET_STATIC_LIB");
    lua_pushinteger(L, TARGET_SHARED_LIB);
    lua_setglobal(L, "TARGET_SHARED_LIB");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        log_error("Usage: %s <build_script.lua>", argv[0]);
    }

    #ifdef _WIN32
    strcpy(build_system.compiler, "cl");
    strcpy(build_system.cflags, "/W4 /O2");
    strcpy(build_system.ldflags, "");
    #else
    strcpy(build_system.compiler, "gcc");
    strcpy(build_system.cflags, "-Wall -Wextra -O2");
    strcpy(build_system.ldflags, "");
    #endif

    strcpy(build_system.output_dir, "build");

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    setup_lua_functions(L);

    if (luaL_dofile(L, argv[1]) != LUA_OK) {
        log_error("Error executing Lua script: %s", lua_tostring(L, -1));
    }

    lua_close(L);
    return 0;
}
