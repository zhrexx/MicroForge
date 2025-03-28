#ifndef XSTD_H
#define XSTD_H 

#include "deps/lua.h" 
#include "deps/lauxlib.h" 
#include "deps/lualib.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void xstd_push_function(lua_State *L, lua_CFunction f, const char *name) {
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, name);
}

void xstd_init(lua_State *L) {
    lua_newtable(L); 
    
    lua_setglobal(L, "std");
}


#endif // XSTD_H 
