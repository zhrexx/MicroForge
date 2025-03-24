#include "deps/lua.h"
#include "deps/lauxlib.h"
#include "deps/lualib.h"
#include "xam.h"

int main(int argc, char **argv) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    lua_newtable(L);
    for (int i = 1; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i);
    }
    lua_setglobal(L, "arg");

    if (argc == 0) {
        lua_pushstring(L, "");
        lua_rawseti(L, -2, 1);
    }

    if (luaL_loadbuffer(L, (const char *)xam_luac, xam_luac_len, "bytecode") || lua_pcall(L, 0, 0, 0)) {
        printf("Error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }

    lua_close(L);
    return 0;
}

