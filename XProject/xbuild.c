#include "xproject.h"

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    strcpy(build_system.compiler, "cl");
    strcpy(build_system.cflags, "");
    strcpy(build_system.ldflags, "");
    #else
    strcpy(build_system.compiler, "gcc");
    strcpy(build_system.cflags, "");
    strcpy(build_system.ldflags, "");
    #endif

    strcpy(build_system.output_dir, "build");

    init_parallel_system();

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    setup_lua_functions(L);
    
    lua_newtable(L);
    lua_pushstring(L, "count");
    lua_pushinteger(L, argc-1);
    lua_rawset(L, -3); 
    for (int i = 1; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "args");

    if (luaL_dofile(L, "XBuild.lua") != LUA_OK) {
        log_error("Error executing Lua script: %s", lua_tostring(L, -1));
    }

    lua_close(L);
    cleanup_parallel_system();
    return 0;
}
