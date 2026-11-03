#include "sin_compat/original_subsystem_startup.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
#include <vector>
using namespace sin_compat::startup;
namespace {
int x=1,y=2;
std::vector<std::string> calls;
void* goodx(){calls.emplace_back("x-init");return &x;}
void* goody(){calls.emplace_back("y-init");return &y;}
void* failed(){calls.emplace_back("failed-init");return nullptr;}
void end(void* p){calls.emplace_back(p==&x?"end-x":p==&y?"end-y":"end-null");}
bool servergood(void* p){calls.emplace_back(p==&x?"server-x":p==&y?"server-y":"server-other");return true;}
bool serverbad(void* p){calls.emplace_back(p==&y?"server-bad-y":"server-bad");return false;}
}
int main(){
    void* a=nullptr;void* b=nullptr;
    calls.clear();
    assert(start_client_nodes({{&a,goodx,end},{&b,goody,end}}));
    assert(a==&x && b==&y);
    assert((calls==std::vector<std::string>{"x-init","y-init"}));
    calls.clear();a=nullptr;b=nullptr;
    assert(!start_client_nodes({{&a,goodx,end},{&b,failed,end}}));
    assert(!a&&!b);
    assert((calls==std::vector<std::string>{"x-init","failed-init","end-x","end-null"}));
    calls.clear();
    assert(start_client_nodes({{nullptr,failed,end},{&b,goody,end}}));
    assert(b==&y && calls.size()==2);
    calls.clear();
    ServerSystemStartup server;
    server.pending={{&x,servergood},{&x,servergood},{&y,serverbad}};
    assert(!start_server_systems(server));
    assert(server.initialized && server.pending.empty());
    assert(server.registered.size()==2 && server.duplicate_count==1);
    assert((calls==std::vector<std::string>{"server-x","server-bad-y"}));
    server.registered[1].initialize=servergood;
    calls.clear();
    assert(start_server_systems(server));
    assert((calls==std::vector<std::string>{"server-x","server-y"}));
    ServerSystemStartup empty;
    assert(start_server_systems(empty) && empty.initialized);
    VGuiStartupState vgui;
    calls.clear();
    auto load=[](std::string_view name){calls.emplace_back(std::string(name));return true;};
    auto factory=[](std::string_view name)->void*{
        calls.emplace_back(std::string(name));return name==client_vgui_input?&x:&y;};
    auto registration=[](){calls.emplace_back("register");};
    auto first=[](){calls.emplace_back("aux-1");};
    auto second=[](){calls.emplace_back("aux-2");return true;};
    assert(start_client_vgui(vgui,load,factory,registration,first,second));
    assert(vgui.input==&x && vgui.system==&y && vgui.one_time_registered);
    assert((calls==std::vector<std::string>{"CLIENT","VGUI_InputInternal001",
        "VGUI_System010","register","aux-1","aux-2"}));
    calls.clear();
    assert(start_client_vgui(vgui,load,factory,registration,first,second));
    assert(calls.size()==5);
    calls.clear();
    auto missing=[](std::string_view)->void*{calls.emplace_back("missing");return nullptr;};
    assert(!start_client_vgui(vgui,load,missing,registration,first,second));
    assert(!vgui.input && calls.size()==2);
    assert(original_particle_bucket_allocation==48);
    assert(original_particle_bucket_constructor_arg==96);
    assert(original_particle_bucket_label=="CParticleMgr::m_pParticleBucket");
}
