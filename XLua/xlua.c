#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

#define LUA_GCSETLIMIT 10000

#ifdef _WIN32
#include <windows.h>
#define PLATFORM_SEPARATOR '\\'
#define PLATFORM_NAME "windows"
#else
#include <unistd.h>
#include <sys/stat.h>
#define PLATFORM_SEPARATOR '/'
#define PLATFORM_NAME "unix"
#endif

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"
#define COLOR_BRIGHT_RED "\x1b[91m"
#define COLOR_RESET   "\x1b[0m"

#define MAX_LINE_LENGTH 4096
#define MAX_SCRIPT_TIMEOUT 60
#define MAX_INCLUDE_DEPTH 10
#define MAX_SANDBOX_MEMORY 100 * 1024 * 1024

typedef struct {
    int verbose;
    int interactive;
    int safe_mode;
    int debug_mode;
    int profile_mode;
    int sandbox_mode;
    int watch_mode;
    const char* script_path;
    const char** script_args;
    int script_arg_count;
    const char* output_file;
} RunnerConfig;

static volatile sig_atomic_t timeout_occurred = 0;
static lua_State* global_lua_state = NULL;

void print_banner();
void print_help();
RunnerConfig parse_arguments(int argc, char* argv[]);
void setup_lua_path(lua_State* L);
void run_lua_script(RunnerConfig config);
void run_interactive_repl(lua_State* L);
void lua_error_handler(lua_State* L, int error_code);
void set_os_global(lua_State* L);
void enable_script_timeout(int seconds);
void disable_script_timeout();
void timeout_handler(int signum);
void sandbox_lua_environment(lua_State* L);
void redirect_output(const char* filename);
void restore_output();
void* memory_panic_handler(void* ud, void* ptr, size_t osize, size_t nsize);
void watch_script_and_reload(RunnerConfig config);
int safe_lua_loader(lua_State* L, const char* filename);
void enhanced_package_loader(lua_State* L);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }
    RunnerConfig config = parse_arguments(argc, argv);

    if (config.verbose) {
        print_banner();
    }

    lua_State* L = luaL_newstate();
    if (!L) {
        fprintf(stderr, COLOR_BRIGHT_RED "Fatal: Could not create Lua state\n" COLOR_RESET);
        return EXIT_FAILURE;
    }

    global_lua_state = L;

    lua_setallocf(L, memory_panic_handler, NULL);

    luaL_openlibs(L);
    setup_lua_path(L);
    set_os_global(L);

    enhanced_package_loader(L);

    if (config.sandbox_mode) {
        sandbox_lua_environment(L);
    }

    if (config.output_file) {
        redirect_output(config.output_file);
    }

    if (config.interactive) {
        run_interactive_repl(L);
        lua_close(L);
        return EXIT_SUCCESS;
    }

    if (config.script_path) {
        if (config.watch_mode) {
            watch_script_and_reload(config);
        } else {
            enable_script_timeout(MAX_SCRIPT_TIMEOUT);
            run_lua_script(config);
            disable_script_timeout();
        }
    }

    if (config.output_file) {
        restore_output();
    }

    return EXIT_SUCCESS;
}

int safe_lua_loader(lua_State* L, const char* filename) {
    static int include_depth = 0;

    if (include_depth >= MAX_INCLUDE_DEPTH) {
        return luaL_error(L, "Maximum include depth exceeded");
    }

    include_depth++;
    int result = luaL_dofile(L, filename);
    include_depth--;

    return result;
}

void enhanced_package_loader(lua_State* L) {
    lua_settop(L, 0);
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaders");
    if (!lua_istable(L, -1)) {
        fprintf(stderr, "Error: package.loaders is not a table\n");
        lua_pop(L, 2);
        return;
    }
    int loader_count = lua_rawlen(L, -1);
    lua_pushcfunction(L, (lua_CFunction)safe_lua_loader);
    lua_rawseti(L, -2, loader_count + 1);
    lua_pop(L, 2);
}

void* memory_panic_handler(void* ud, void* ptr, size_t osize, size_t nsize) {
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        void* new_ptr = malloc(nsize);
        if (!new_ptr) {
            fprintf(stderr, COLOR_RED "Memory allocation failed: %zu bytes\n" COLOR_RESET, nsize);
            
            if (global_lua_state) {
                luaL_error(global_lua_state, "Out of memory");
            }
            
            exit(EXIT_FAILURE);
        }
        return new_ptr;
    }

    void* new_ptr = realloc(ptr, nsize);
    if (!new_ptr) {
        fprintf(stderr, COLOR_RED "Memory reallocation failed: %zu bytes\n" COLOR_RESET, nsize);
        
        if (global_lua_state) {
            luaL_error(global_lua_state, "Out of memory");
        }
        
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

void timeout_handler(int signum) {
    timeout_occurred = 1;
    if (global_lua_state) {
        luaL_error(global_lua_state, "Script execution timed out");
    }
}

void enable_script_timeout(int seconds) {
    timeout_occurred = 0;
    signal(SIGALRM, timeout_handler);
    
    struct itimerval timer;
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_REAL, &timer, NULL);
}

void disable_script_timeout() {
    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    
    setitimer(ITIMER_REAL, &timer, NULL);
    signal(SIGALRM, SIG_IGN);
}

void sandbox_lua_environment(lua_State* L) {
    lua_pushnil(L);
    lua_setglobal(L, "dofile");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");
    
    lua_gc(L, LUA_GCSETLIMIT, MAX_SANDBOX_MEMORY);
}

void watch_script_and_reload(RunnerConfig config) {
    time_t last_modified = 0;
    struct stat file_stat;

    printf(COLOR_CYAN "Watching script: %s (Press Ctrl+C to exit)\n" COLOR_RESET, config.script_path);

    while (1) {
        if (stat(config.script_path, &file_stat) == 0) {
            if (file_stat.st_mtime > last_modified) {
                printf(COLOR_GREEN "Script changed. Reloading...\n" COLOR_RESET);
                
                run_lua_script(config);
                last_modified = file_stat.st_mtime;
            }
        }

        #ifdef _WIN32
        Sleep(1000);
        #else
        sleep(1);
        #endif
    }
}

void redirect_output(const char* filename) {
    FILE* output_file = freopen(filename, "w", stdout);
    if (!output_file) {
        perror("Could not redirect output");
        exit(EXIT_FAILURE);
    }
}

void restore_output() {
    fclose(stdout);
    stdout = fdopen(STDOUT_FILENO, "w");
}

RunnerConfig parse_arguments(int argc, char* argv[]) {
    RunnerConfig config = {0};
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help();
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-v") == 0) {
            config.verbose = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            config.interactive = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            config.safe_mode = 1;
        } else if (strcmp(argv[i], "-x") == 0) {
            config.sandbox_mode = 1;
        } else if (strcmp(argv[i], "-w") == 0) {
            config.watch_mode = 1;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            config.output_file = argv[++i];
        } else if (!config.script_path) {
            config.script_path = argv[i];
            config.script_args = (const char**)&argv[i + 1];
            config.script_arg_count = argc - i - 1;
            break;
        }
    }

    return config;
}

void print_help() {
    printf(COLOR_BLUE "Usage: xlua [OPTIONS] <script> [SCRIPT ARGS]\n\n" COLOR_RESET
           "Options:\n"
           "  -h         Show this help message\n"
           "  -v         Enable verbose output\n"
           "  -i         Start interactive REPL\n"
           "  -s         Run in safe mode\n"
           "  -x         Run in sandbox mode (restrict environment)\n"
           "  -w         Watch and auto-reload script on change\n"
           "  -o FILE    Redirect output to a file\n"
           "\nExamples:\n"
           "  xlua script.lua\n"
           "  xlua -x script.lua        (sandbox mode)\n"
           "  xlua -w script.lua        (watch & auto-reload)\n"
           "  xlua -o output.txt script.lua  (redirect output)\n"
           COLOR_RESET);
}

void print_banner() {
    printf(COLOR_CYAN 
        "╔═══════════════════════════════════╗\n"
        "║         " COLOR_MAGENTA "XLUA RUNNER" COLOR_CYAN "       ║\n"
        "╚═══════════════════════════════════╝\n" 
        COLOR_GREEN "Version 1.0.0 - Lua Runner\n" 
        COLOR_RESET);
}

void setup_lua_path(lua_State* L) {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    
    const char* current_path = lua_tostring(L, -1);
    char new_path[1024];
    snprintf(new_path, sizeof(new_path), 
             "%s;.%c?.lua;.%c?%cinit.lua;/usr/local/share/lua/5.4/?.lua", 
             current_path, PLATFORM_SEPARATOR, PLATFORM_SEPARATOR, PLATFORM_SEPARATOR);
    
    lua_pop(L, 1);
    lua_pushstring(L, new_path);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

void set_os_global(lua_State* L) {
    lua_pushstring(L, PLATFORM_NAME);
    lua_setglobal(L, "OS");
}

void lua_error_handler(lua_State* L, int error_code) {
    if (error_code != LUA_OK) {
        fprintf(stderr, COLOR_RED "Lua Error: %s\n" COLOR_RESET, 
                lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void run_lua_script(RunnerConfig config) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    set_os_global(L);

    lua_createtable(L, config.script_arg_count, 0);
    for (int i = 0; i < config.script_arg_count; i++) {
        lua_pushstring(L, config.script_args[i]);
        lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "arg");

    int result = luaL_dofile(L, config.script_path);
    lua_error_handler(L, result);

    lua_close(L);
}

void run_interactive_repl(lua_State* L) {
    char buffer[1024];
    printf(COLOR_GREEN "XLua Interactive REPL (type 'exit()' to quit)\n" COLOR_RESET);
    
    while (1) {
        printf(COLOR_CYAN "xlua> " COLOR_RESET);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strcmp(buffer, "exit()") == 0) break;
        
        int result = luaL_dostring(L, buffer);
        lua_error_handler(L, result);
    }
}
