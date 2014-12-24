#include <strings.h>
#include <iomanip>
#include <fstream>
#include <sstream>

#include "profiler.h"
#include "tsc.h"

LuaProfiler LuaProfiler::_self;

////////////////////////////////////////////////////////////////////////////

struct LuaProfilerNode {

    std::string key;
    std::string func;
    int call_count;
    uint64_t full_elapse;   // including calling stack
    uint64_t inner_elapse;

    explicit LuaProfilerNode(const std::string& key)
           : key(key)
           , call_count(0)
           , full_elapse(0)
           , inner_elapse(0) {}

    void OnEnter() {
        ++ call_count;
    }

    void OnExit(uint64_t elapse, uint64_t children_elapse) {
        full_elapse += elapse;
        inner_elapse += (elapse - children_elapse);
    }
};

////////////////////////////////////////////////////////////////////////////

struct LuaCallStack {

    // frame data
    struct Frame {
        LuaProfilerNode* node;
        bool tailcall;
        uint64_t enter_stamp;
        uint64_t exit_stamp;
        uint64_t child_elapse;

        Frame() : node(NULL)
                , tailcall(false)
                , enter_stamp(0)
                , exit_stamp(0)
                , child_elapse(0) {}
    };
    std::vector<Frame> frames;

    void Reset() { frames.clear(); }

    void Push(LuaProfilerNode* node, bool tailcall = false) {
        Frame f;
        f.node = node;
        f.tailcall = tailcall;
        rdtscll(f.enter_stamp);
        f.node->OnEnter();
        frames.push_back(f);
    }

    void Pop(LuaProfilerNode* node, bool pop_all = false) {
        if (frames.empty() || frames.back().node != node) {
            return;
        }

        // pop self
        Frame& frame = frames.back();
        rdtscll(frame.exit_stamp);
        uint64_t elapse = frame.exit_stamp - frame.enter_stamp;
        pop_all = (pop_all || frame.tailcall);
        frame.node->OnExit(elapse, frame.child_elapse);
        frames.pop_back();

        // parent's child elapse
        if (!frames.empty()) {
            Frame& parent = frames.back();
            parent.child_elapse += elapse;
        }

        // tailcall recursive pop
        if (pop_all && !frames.empty()) {
            return Pop(frames.back().node, true);
        }
    }

    void Debug(const std::string& name, int event) {
        printf("%s event=%d:\n", name.c_str(), event);
        for (size_t i = 0; i < frames.size(); ++ i) {
        #if defined(__x86_64) && !defined(__APPLE__)
            printf("\t%s: enter[%lu] quit[%lu] full[%lu] child[%lu] inner[%lu]\n",
        #else
            printf("\t%s: enter[%llu] quit[%llu] full[%llu] child[%llu] inner[%llu]\n",
        #endif
            frames[i].node->key.c_str(), frames[i].enter_stamp, frames[i].exit_stamp,
            frames[i].node->full_elapse, frames[i].child_elapse, frames[i].node->inner_elapse);
        }
        printf("\n");
    }

};

////////////////////////////////////////////////////////////////////////////

LuaProfiler::LuaProfiler()
           : _started(false)
           , _origin_hook(NULL)
           , _origin_mask(0)
           , _current_is_lua(-1)
           , _current_is_main(-1)
           , _current_is_native(-1)
           , _stack(new LuaCallStack)
{
}

LuaProfiler::~LuaProfiler()
{
    CleanUp();
    delete _stack;
    _stack = NULL;
}

void LuaProfiler::Start(lua_State* L)
{
    if (!IsStarted()) {
        _origin_hook = lua_gethook(L);
        _origin_mask = lua_gethookmask(L);
        lua_sethook(L, hook, _origin_mask | LUA_MASKCALL | LUA_MASKRET | LUA_HOOKTAILCALL, lua_gethookcount(L));
        _started = true;
    }
}

void LuaProfiler::Stop(lua_State* L)
{
    if (IsStarted()) {
        lua_sethook(L, _origin_hook, _origin_mask, lua_gethookcount(L));
        _origin_hook = NULL;
        _origin_mask = 0;
        _started = false;
    }
}

void LuaProfiler::CleanUp()
{
    for (std::map<std::string, LuaProfilerNode*>::iterator it = _nodes.begin();
        it != _nodes.end(); ++ it) {
        delete it->second;
        it->second = NULL;
    }
    _nodes.clear();
    _stack->Reset();
}

void LuaProfiler::set_current(lua_Debug* ar)
{
    _current_is_lua = strcasecmp(ar->what, "lua");
    _current_is_main = strcasecmp(ar->what, "main");
    _current_is_native = strcasecmp(ar->what, "C");
}

std::string LuaProfiler::node_key_get(lua_Debug* ar) const
{
    char key[1024];
    key[0] = 0;
    if (_current_is_lua == 0) {
        snprintf(key, sizeof(key), "%s:%d", ar->source, ar->linedefined);
    } else if (_current_is_main == 0) {
        snprintf(key, sizeof(key), "main:%d", ar->linedefined);
    } else if (_current_is_native == 0) {
        snprintf(key, sizeof(key), "native:%s", ar->name ? ar->name : "null");
    }
    return std::string(key);
}

LuaProfilerNode* LuaProfiler::node_get(const std::string& key)
{
    std::map<std::string, LuaProfilerNode*>::iterator it = _nodes.find(key);
    return it == _nodes.end() ? NULL : it->second;
}

LuaProfilerNode* LuaProfiler::node_create(const std::string& key, lua_Debug* ar)
{
    LuaProfilerNode* node = node_get(key);
    if (node) {
        fprintf(stderr, "node[%s] already existed!", key.c_str());
        return node;
    }
    node = new LuaProfilerNode(key);
    if (ar->name && ar->name[0]) {
        node->func = std::string(ar->name);
    } else if (ar->namewhat && ar->namewhat[0]) {
        node->func = std::string(ar->namewhat);
    } else {
        node->func = key;
    }
    _nodes.insert(std::make_pair(node->key, node));
    return node;
}

void LuaProfiler::hook(lua_State* L, lua_Debug* ar)
{
    LuaProfiler* lp = LuaProfiler::Get();

    // original hook first
    if (lp->_origin_hook && (1 << ar->event) & lp->_origin_mask) {
        lp->_origin_hook(L, ar);
    }

    // catch only call & return & tail-call
    if (ar->event != LUA_HOOKCALL
        && ar->event != LUA_HOOKRET
        && ar->event != LUA_HOOKTAILCALL) {
        return;
    }

    // get debug info
    ar->name = NULL;
    lua_getinfo(L, "Sln", ar);

    // current info
    lp->set_current(ar);

    // node
    std::string key = lp->node_key_get(ar);
    LuaProfilerNode* node = lp->node_get(key);
    if (!node) {
        node = lp->node_create(key, ar);
    }
    assert(node);

    // enter
    if (ar->event == LUA_HOOKCALL) {
        lp->_stack->Push(node);
    }
    // exit function
    else if (ar->event == LUA_HOOKRET) {
        lp->_stack->Pop(node);
    }
    // tail return
    else if (ar->event == LUA_HOOKTAILCALL) {
        lp->_stack->Push(node, true);
    }

    // lp->_stack->Debug(key, ar->event);
}

void LuaProfiler::Dump2File(const std::string& file) const
{
    std::fstream fs;
    fs.open(file, std::ios::out);
    assert(fs.is_open());
    fs << DumpString();
    fs.close();
}

std::string LuaProfiler::DumpString() const
{
    std::stringstream ss;
#define _align() std::setiosflags(std::ios::left) << std::setw(30)
    ss << _align() << "[position]"
       << _align() << "[function]"
       << _align() << "[call times]"
       << _align() << "[average cost]"
       << _align() << "[average inner cost]" << std::endl;
    std::map<std::string, LuaProfilerNode*>::const_iterator it;
    for (it = _nodes.begin(); it != _nodes.end(); ++ it) {
        LuaProfilerNode* node = it->second;
        if (node->call_count > 0) {
            ss << _align() << node->key
               << _align() << node->func
               << _align() << node->call_count
               << _align() << node->full_elapse / node->call_count
               << _align() <<  node->inner_elapse / node->call_count << std::endl;
        }
    }
    ss << std::endl;
    return ss.str();
}

