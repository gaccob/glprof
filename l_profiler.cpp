#include <lua.hpp>

#include "profiler.h"

static int l_profiler_start(lua_State* L) {
    LuaProfiler::Get()->Start(L);
    return 0;
}

static int l_profiler_stop(lua_State* L) {
    LuaProfiler::Get()->Stop(L);
    return 0;
}

static int l_profiler_dump(lua_State* L) {
    const char* file = luaL_checkstring(L, -1);
    LuaProfiler::Get()->Dump(file);
    return 0;
}

extern "C"
int luaopen_l_profiler(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        {"start",   l_profiler_start },
        {"stop",    l_profiler_stop },
        {"dump",    l_profiler_dump },
        {NULL,      NULL },
    };
    luaL_newlib(L, l);
    return 1;
}

