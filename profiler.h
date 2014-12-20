#pragma once

#include <assert.h>
#include <stdint.h>

#include <string>
#include <map>
#include <vector>

#include <lua.hpp>

struct LuaProfilerNode;
struct LuaCallStack;

class LuaProfiler {
public:
    LuaProfiler();
    ~LuaProfiler();

    void Start(lua_State* L);
    void Stop(lua_State* L);

    // hook function for lua
    static void Hook(lua_State* L, lua_Debug* ar);

    // singleton
    static LuaProfiler* Get() { return &_self; }

    bool IsStarted() const { return _started; }

    // dump profiler
    void Dump(const std::string& file);

private:
    void set_current(lua_Debug* ar);

    std::string node_key_get(lua_Debug* ar) const;
    LuaProfilerNode* node_get(const std::string& key);
    LuaProfilerNode* node_create(const std::string& key, lua_Debug* ar);

private:
    bool _started;
    lua_Hook _origin_hook;
    int _origin_mask;

    int _current_is_lua;
    int _current_is_main;
    int _current_is_native;

    std::map<std::string, LuaProfilerNode*> _nodes;
    LuaCallStack* _stack;

    // singleton
    static LuaProfiler _self;
};

