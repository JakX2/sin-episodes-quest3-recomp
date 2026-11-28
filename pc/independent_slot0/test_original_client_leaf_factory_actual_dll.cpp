// Phase1695: LoadLibrary the ACTUAL independently compiled shipping
// client.dll, not a mock replacement, and verify guarded original 2006
// ClientLeafSystem001 CreateInterface after all 21 real-order factory
// queries, sound Connect, prediction, entity list and cold centerprint.
// All original leaf virtual methods intentionally STOP the process.
// The synthetic appFactory is not the retail engine or original DLL.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "native_original_client_leaf_cold_factory.h"
#include "server_console_object_abi.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1695 actual client factory native CTest requires MSVC x86
#endif

namespace api=sin_independent::native_client_leaf_cold_factory;
namespace ca=sin_independent::console_abi;
namespace {
using Factory=void* (__cdecl *)(const char*,int*);
constexpr const char* kNames[]={
    "VEngineClient012","VEngineModel012","VEngineEffects001",
    "VEngineCvar002","EngineTraceClient003","VEngineRenderView011",
    "VDebugOverlay003","VMaterialSystem076","VEngineCache001",
    "VModelInfoClient003","VEngineVGui001",
    "VEngineClientStringTable001","SpatialPartition001",
    "VEngineShadowMgr002","StaticPropMgrClient004",
    "IEngineSoundClient003","VFileSystem012","VEngineRandom001",
    "VENGINE_GAMEUIFUNCS_VERSION004","GAMEEVENTSMANAGER002",
    "VSoundEmitter001"
};
constexpr std::size_t kNamesCount=sizeof(kNames)/sizeof(kNames[0]);
int fail(int code,const char* message) {
    std::fprintf(stderr,"FAIL Phase1695 actual client leaf boundary %d: %s\n",code,message);
    return code;
}
void* __cdecl appFactory(const char*,int*);
struct FakeSoundEmitter {
    virtual bool __thiscall Connect(Factory f) noexcept {
        ++calls;
        return f==&appFactory;
    }
    unsigned calls=0u;
};
struct FakeCvar {
    virtual void __thiscall Register(void*) noexcept {}
    virtual const char* __thiscall CommandLine(const char*) noexcept {return nullptr;}
    virtual void* __thiscall FindMutable(const char*) noexcept {return nullptr;}
    virtual const void* __thiscall FindConst(const char* name) const noexcept {
        return name && std::strcmp(name,"cl_predict")==0?
            static_cast<const void*>(predict):nullptr;
    }
    virtual void* __thiscall GetCommands() noexcept {return nullptr;}
    ca::Variable* predict=nullptr;
};
FakeSoundEmitter g_sound{};
FakeCvar g_cvar{};
ca::Variable g_predict{};
std::uint32_t g_opaque=0u;
unsigned g_queries=0u,g_errors=0u;
void* __cdecl appFactory(const char* name,int* status) {
    if(g_queries>=kNamesCount || !name || status ||
       std::strcmp(name,kNames[g_queries])!=0)++g_errors;
    ++g_queries;
    if(name && std::strcmp(name,"VEngineCvar002")==0)return &g_cvar;
    if(name && std::strcmp(name,"VSoundEmitter001")==0)return &g_sound;
    return &g_opaque;
}
void* __cdecl physicsFactory(const char*,int*) {return nullptr;}
bool env(const char* key,const char* value) noexcept {
    return SetEnvironmentVariableA(key,value)!=0;
}
bool session(bool leaf) {
    return env("SIN_INDEPENDENT_TRACE_SESSION",
               "0123456789abcdef0123456789abcdef") &&
      env("SIN_INDEPENDENT_TRACE_FILE","1") &&
      env("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0") &&
      env("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","1") &&
      env("SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE","0") &&
      env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE","1") &&
      env("SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE","1") &&
      env("SIN_INDEPENDENT_CLIENT_SOUND_CONNECT_CONTINUE_PROBE","1") &&
      env("SIN_INDEPENDENT_CLIENT_LEAF_FACTORY_PROBE",leaf?"1":"0");
}
bool owned(HMODULE module,const void* pointer) {
    HMODULE result=nullptr;
    return pointer && GetModuleHandleExA(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCSTR>(pointer),&result) && result==module;
}
bool prepare() {
    g_predict.CreateVariable("cl_predict","1",0x200,
        nullptr,false,0,false,0,nullptr);
    g_cvar.predict=&g_predict;
    return g_predict.string && g_predict.parent==&g_predict &&
           g_predict.integer==1;
}
bool native_init(void* object,void* fn,int expected) {
    std::uintptr_t before=0u,after=0u;
    std::uint32_t globals=0x5E11DAB1u;
    void* g=&globals;
    Factory f=&appFactory,p=&physicsFactory;
    int actual=-999;
    __asm {
        mov before,esp
        push g
        push p
        push f
        mov ecx,object
        mov eax,fn
        call eax
        mov actual,eax
        mov after,esp
    }
    return before==after && actual==expected;
}
bool native_shutdown(void* object,void* fn) {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        mov ecx,object
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool startup(const char* path,HMODULE& mod,Factory& factory,
             void*& client,void**& cvt) {
    mod=LoadLibraryA(path);
    if(!mod)return false;
    auto* entry=GetProcAddress(mod,"CreateInterface");
    if(!owned(mod,reinterpret_cast<void*>(entry)))return false;
    factory=reinterpret_cast<Factory>(entry);
    int status=1;
    client=factory("VClient011",&status);
    if(!client || status!=0)return false;
    cvt=*reinterpret_cast<void***>(client);
    return cvt && owned(mod,cvt[0]) && owned(mod,cvt[1]);
}
bool run_init(void* client,void** cvt) {
    g_queries=g_errors=0u;
    g_sound.calls=0u;
    return native_init(client,cvt[0],1) &&
           g_queries==kNamesCount && g_errors==0u &&
           g_sound.calls==1u;
}
bool leaf_guard_child(const char* path) {
    if(!prepare() || !session(true))return false;
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** cvt=nullptr;
    if(!startup(path,module,create,client,cvt) || !run_init(client,cvt))
        return false;
    void* leaf=create(api::kInterfaceName,nullptr);
    if(!leaf)return false;
    auto** slots=*reinterpret_cast<void***>(leaf);
    if(!slots || !owned(module,slots[0]))return false;
    // The true engine calls source virtual slots 0/1/3 during
    // renderable lifecycle. We deliberately CANNOT execute them:
    // this native guard must terminate with recognizable 0x53494E45.
    using Entry=void (__thiscall *)(void*,void*,int);
    reinterpret_cast<Entry>(slots[0])(leaf,nullptr,1);
    return false;
}
bool expect_terminal_child(const char* dll) {
    char executable[MAX_PATH]{};
    if(!GetModuleFileNameA(nullptr,executable,MAX_PATH))return false;
    std::string command="\"";
    command+=executable;
    command+="\" \"";
    command+=dll;
    command+="\" --leaf-guard";
    STARTUPINFOA startupInfo{};
    startupInfo.cb=sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    if(!CreateProcessA(nullptr,command.data(),nullptr,nullptr,FALSE,
                       CREATE_NO_WINDOW,nullptr,nullptr,
                       &startupInfo,&processInfo))return false;
    const DWORD waited=WaitForSingleObject(processInfo.hProcess,30000u);
    DWORD code=0;
    const bool success=waited==WAIT_OBJECT_0 &&
        GetExitCodeProcess(processInfo.hProcess,&code) &&
        code==0x53494E45u;
    if(waited==WAIT_TIMEOUT)
        TerminateProcess(processInfo.hProcess,1u);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return success;
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2 && argc!=3)
        return fail(1,"expected actual independent client.dll path");
    if(argc==3) {
        if(std::strcmp(argv[2],"--leaf-guard")!=0)
            return fail(2,"unrecognized child mode");
        return leaf_guard_child(argv[1])?
            fail(3,"unsupported native method returned"):
            fail(4,"leaf terminal child did not reach method");
    }
    if(!prepare() || !session(false))
        return fail(5,"synthetic owned ConVar or isolated environment");
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** cvt=nullptr;
    if(!startup(argv[1],module,create,client,cvt))
        return fail(6,"LoadLibrary actual PE32 client.dll");
    int status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(7,"leaf must be absent in default shipping factory");
    if(!run_init(client,cvt))
        return fail(8,"21 real-order factory queries/Connect/prediction");
    if(create(api::kInterfaceName,nullptr))
        return fail(9,"leaf must remain absent even after normal Init");
    if(!session(true))
        return fail(10,"explicit independent cold leaf flag");
    if(create(api::kInterfaceName,nullptr))
        return fail(11,"env switch alone cannot bypass fresh Init");
    if(!run_init(client,cvt))
        return fail(12,"fresh opt-in actual PE32 client Init");
    status=-1;
    void* leaf=create(api::kInterfaceName,&status);
    if(!leaf || status!=0 || create(api::kInterfaceName,nullptr)!=leaf)
        return fail(13,"stable original-name primary singleton negotiation");
    const auto base=reinterpret_cast<std::uintptr_t>(leaf);
    auto*** ptrs=reinterpret_cast<void***>(leaf);
    auto** primary=ptrs[0];
    auto** autosystem=ptrs[1];
    auto** auxiliary=ptrs[2];
    if(!primary || !autosystem || !auxiliary ||
       reinterpret_cast<std::uintptr_t>(&ptrs[1])-base!=
             api::kOriginalAutoSystemOffset ||
       reinterpret_cast<std::uintptr_t>(&ptrs[2])-base!=
             api::kOriginalAuxiliaryOffset)
        return fail(14,"source +0,+4,+8 object subobject vptr offsets");
    if(!owned(module,primary) || !owned(module,autosystem) ||
       !owned(module,auxiliary))
        return fail(15,"vtable arrays must remain in shipping DLL");
    for(unsigned i=0;i<api::kOriginalPrimarySlots;++i)
        if(!primary[i] || !owned(module,primary[i]))
            return fail(16,"all 22 primary guards remain executable");
    for(unsigned i=0;i<api::kOriginalAutoSystemSlots;++i)
        if(!autosystem[i] || !owned(module,autosystem[i]))
            return fail(17,"12 secondary AutoGameSystem guards remain executable");
    if(!auxiliary[0] || !owned(module,auxiliary[0]))
        return fail(18,"single third-subobject guard remains executable");
    if(create("ClientLeafSystem002",nullptr))
        return fail(19,"non-original leaf interface version must refuse");
    if(!expect_terminal_child(argv[1]))
        return fail(20,"actual native leaf primary slot0 must TERMINATE, not return");
    if(!env("SIN_INDEPENDENT_CLIENT_LEAF_FACTORY_PROBE","0") ||
       create(api::kInterfaceName,nullptr))
        return fail(21,"explicit switch revocation must hide leaf interface");
    if(!env("SIN_INDEPENDENT_CLIENT_LEAF_FACTORY_PROBE","1") ||
       !native_shutdown(client,cvt[1]) ||
       create(api::kInterfaceName,nullptr) ||
       create("VCENTERPRINT002",nullptr))
        return fail(22,"Shutdown must revoke leaf and prior factory owners");
    const auto retained=module;
    if(!FreeLibrary(module) || GetModuleHandleA("client.dll")!=retained)
        return fail(23,"owner native vtables should be pinned for lifetime");
    std::puts("PASS Phase1695 ACTUAL independent PE32 client.dll "
              "21 original factories + sound Connect + prediction/entity/"
              "centerprint; opt-in ClientLeafSystem001 primary 22 guards, "
              "secondary+4 12 guards, auxiliary+8 1 guard, source singleton "
              "gating, fatal virtual method child exit, and restoration-ready "
              "shutdown. NOT leaf renderer, game boot or original game execution.");
    return 0;
}
