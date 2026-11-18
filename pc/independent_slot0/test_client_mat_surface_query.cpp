// Native MSVC x86 original post-VGUI IAppSystem.QueryInterface boundary.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include "client_mat_surface_query.h"

static int calls=0;
static int returned=1;
struct Primary {
    virtual int ThisIsNotTheInterface() noexcept {return -1;}
};
struct AppSystem {
    virtual bool Connect(void*) noexcept {return false;}
    virtual void Disconnect() noexcept {}
    virtual void* QueryInterface(const char* version) noexcept {
        if(!version || std::strcmp(version,"MatSystemSurface003")!=0)std::abort();
        ++calls;
        return returned?static_cast<void*>(&returned):nullptr;
    }
    virtual int Init() noexcept {return 0;}
    virtual void Shutdown() noexcept {}
};
struct Surface: Primary,AppSystem {};

int main() {
    Surface surface;
    using sin_independent::mat_surface::Query;
    using sin_independent::mat_surface::Result;
    if(reinterpret_cast<std::uintptr_t>(static_cast<AppSystem*>(&surface))-
       reinterpret_cast<std::uintptr_t>(&surface)!=sizeof(void*))return 1;
    void* output=reinterpret_cast<void*>(0x2);
    if(Query(nullptr,&output)!=Result::InvalidSurface || output)return 2;
    if(Query(&surface,&output)!=Result::Found ||
       output!=&returned || calls!=1)return 3;
    returned=0;output=reinterpret_cast<void*>(0x2);
    if(Query(&surface,&output)!=Result::MissingInterface || output || calls!=2)return 4;
    struct {void* primary;void* secondary;} invalid{nullptr,nullptr};
    if(Query(&invalid,&output)!=Result::InvalidVtable || calls!=2)return 5;
    std::puts("PASS 2006 IAppSystem secondary +4, QueryInterface slot2 and null refusal");
    return 0;
}
