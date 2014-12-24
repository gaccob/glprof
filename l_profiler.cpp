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

static int l_profiler_cleanup(lua_State* L) {
    LuaProfiler::Get()->CleanUp();
    return 0;
}

static int l_profiler_dump_file(lua_State* L) {
    const char* file = luaL_checkstring(L, -1);
    LuaProfiler::GetConst()->Dump2File(file);
    return 0;
}

static int l_profiler_dump_string(lua_State* L) {
    std::string dump = LuaProfiler::GetConst()->DumpString();
    lua_pushstring(L, dump.c_str());
    return 1;
}

extern "C"
int luaopen_l_profiler(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg l[] = {
        {"start",       l_profiler_start },
        {"stop",        l_profiler_stop },
        {"cleanup",     l_profiler_cleanup },
        {"dump_file",   l_profiler_dump_file },
        {"dump_string", l_profiler_dump_string },
        {NULL,      NULL },
    };
    luaL_newlib(L, l);
    return 1;
}

