// Phase1686: the ACTUAL independently built production client.dll
// exposes the original engine-requested VClientPrediction001 through
// CreateInterface only after a short-lived owner-opted-in VClient011
// Init captures a real caller-provided VEngineCvar002 cl_predict view.
//
// Unlike earlier Prediction23Fixture unit tests, every native
// invocation here is through LoadLibrary/GetProcAddress/actual real
// client.dll CreateInterface and its real MSVC x86 vtable.
// Unknown production prediction callbacks terminate the process.
//
// This test uses an invented engine ICvar implementation and
// invented ConVar storage. No original retail game DLL is loaded.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "server_console_object_abi.h"
#include "native_client_prediction_factory_boundary.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1686 needs actual Win32 MSVC x86 PE32 factory ABI
#endif
namespace pf=sin_independent::native_client_prediction_factory;
namespace ca=sin_independent::console_abi;
namespace {
using Factory=void* (__cdecl *)(const char*,int*);
using ClientInit=int (__thiscall *)(void*,Factory,Factory,void*) noexcept;
using VoidSlot=void (__thiscall *)(void*) noexcept;
using ByteSlot=std::uint8_t (__thiscall *)(void*) noexcept;
using DwordSlot=std::uint32_t (__thiscall *)(void*) noexcept;

int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1686 actual production prediction factory #%d: %s\n",
      code,why);
    return code;
}
class MockCvar final {
public:
    // The original VEngineCvar002 first five native slots.
    // NO virtual destructor before register slot0.
    virtual void __thiscall Register(void*) noexcept {}
    virtual const char* __thiscall CommandLine(
        const char*) noexcept {return nullptr;}
    virtual void* __thiscall FindMutable(
        const char*) noexcept {return nullptr;}
    virtual const void* __thiscall FindConst(
        const char* name) const noexcept {
        ++queries;
        return name && std::strcmp(name,"cl_predict")==0?
            returned:nullptr;
    }
    virtual void* __thiscall GetCommands() noexcept {return nullptr;}
    mutable unsigned queries=0u;
    const void* returned=nullptr;
};
MockCvar g_cvar{};
ca::Variable g_predict{};
ca::Variable g_root{};
std::uint32_t g_ignored_interface=0x53494E45u;
constexpr const char* kExpectedFactoryOrder[]={
    "VEngineClient012",
    "VEngineModel012",
    "VEngineEffects001",
    "VEngineCvar002",
    "EngineTraceClient003",
    "VEngineRenderView011",
    "VDebugOverlay003",
    "VMaterialSystem076",
    "VEngineCache001",
    "VModelInfoClient003",
    "VEngineVGui001",
    "VEngineClientStringTable001",
    "SpatialPartition001",
    "VEngineShadowMgr002",
    "StaticPropMgrClient004",
    "IEngineSoundClient003",
    "VFileSystem012",
    "VEngineRandom001",
    "VENGINE_GAMEUIFUNCS_VERSION004",
    "GAMEEVENTSMANAGER002",
    "VSoundEmitter001"
};
constexpr std::size_t kExpectedQueries=
   sizeof(kExpectedFactoryOrder)/sizeof(kExpectedFactoryOrder[0]);
unsigned g_factoryCalls=0u;
unsigned g_factoryErrors=0u;
unsigned g_physicsQueries=0u;
void* __cdecl engineFactory(const char* name,int* status) {
    if(g_factoryCalls>=kExpectedQueries || !name || status ||
       std::strcmp(name,kExpectedFactoryOrder[g_factoryCalls])!=0)
        ++g_factoryErrors;
    ++g_factoryCalls;
    if(name && std::strcmp(name,"VEngineCvar002")==0)
        return &g_cvar;
    return &g_ignored_interface;
}
void* __cdecl physicsFactory(const char*,int*) {
    ++g_physicsQueries;
    return nullptr;
}
bool setenv(const char* name,const char* value) {
    return SetEnvironmentVariableA(name,value)!=0;
}
bool configure_session() {
    // Requires BOTH 32-byte lowercase hex session + tracing flag.
    return setenv("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0") &&
           setenv("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","1") &&
           setenv("SIN_INDEPENDENT_TRACE_FILE","1") &&
           setenv("SIN_INDEPENDENT_TRACE_SESSION",
                  "0123456789abcdef0123456789abcdef");
}
bool owned(HMODULE module,const void* pointer) {
    HMODULE found=nullptr;
    return pointer && GetModuleHandleExA(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCSTR>(pointer),&found) && found==module;
}
bool invoke_init(void* object,void* fn,int expected) {
    static std::uint32_t invented_globals=0u;
    auto* dummy=&invented_globals;
    Factory app=&engineFactory,physics=&physicsFactory;
    std::uintptr_t before=0u,after=0u;
    int actual=-999;
    __asm {
        mov before,esp
        push dummy
        push physics
        push app
        mov ecx,object
        mov eax,fn
        call eax
        mov actual,eax
        mov after,esp
    }
    return before==after && actual==expected;
}
bool invoke_void(void* owner,void* fn) {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool invoke_byte(void* owner,void* fn,std::uint8_t expected) {
    std::uintptr_t before=0u,after=0u;
    std::uint32_t result=0xffu;
    __asm {
        mov before,esp
        mov ecx,owner
        mov eax,fn
        call eax
        movzx eax,al
        mov result,eax
        mov after,esp
    }
    return before==after && result==expected;
}
bool invoke_word(void* owner,void* fn,std::uint32_t expected) {
    std::uintptr_t before=0u,after=0u;
    std::uint32_t result=0u;
    __asm {
        mov before,esp
        mov ecx,owner
        mov eax,fn
        call eax
        mov result,eax
        mov after,esp
    }
    return before==after && result==expected;
}
bool prepare_real_client(HMODULE& module,Factory& create,
                         void*& client,void*** clientVtable,
                         const char* path) {
    module=LoadLibraryA(path);
    if(!module)return false;
    auto exported=GetProcAddress(module,"CreateInterface");
    if(!owned(module,reinterpret_cast<void*>(exported)))
        return false;
    create=reinterpret_cast<Factory>(exported);
    int status=-1;
    client=create("VClient011",&status);
    if(!client || status!=0)return false;
    *clientVtable=*reinterpret_cast<void***>(client);
    return *clientVtable && owned(module,(*clientVtable)[0]) &&
           owned(module,(*clientVtable)[1]);
}
bool run_rejected(HMODULE module,Factory create,
                  void* client,void** clientVtable) {
    int status=-1;
    if(create(pf::kInterfaceName,&status)||status!=1)
        return false;
    g_factoryCalls=0u;
    g_factoryErrors=0u;
    g_physicsQueries=0u;
    if(!invoke_init(client,clientVtable[0],0) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries ||
       g_physicsQueries)return false;
    status=-1;
    return !create(pf::kInterfaceName,&status) && status==1 &&
           module!=nullptr;
}
bool run_child_unsupported(const char* path) {
    if(!configure_session())return false;
    g_predict.CreateVariable(
      "cl_predict","1",0x200,nullptr,false,0,false,0,nullptr);
    if(!g_predict.string)return false;
    g_cvar.returned=&g_predict;
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** table=nullptr;
    if(!prepare_real_client(module,create,client,&table,path))return false;
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!invoke_init(client,table[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries)return false;
    void* prediction=create(pf::kInterfaceName,nullptr);
    if(!prediction)return false;
    auto** slots=*reinterpret_cast<void***>(prediction);
    if(!slots || !owned(module,slots[3]))return false;
    // MUST terminate the child process via production
    // stopUnsupported() with exact 0x53494E45 exit status.
    reinterpret_cast<VoidSlot>(slots[3])(prediction);
    return false; // an unsupported method must NEVER return
}
bool check_guard_in_child(const char* dll) {
    char executable[MAX_PATH]{};
    if(!GetModuleFileNameA(nullptr,executable,MAX_PATH))return false;
    std::string command="\"";
    command+=executable;
    command+="\" \"";
    command+=dll;
    command+="\" --guard";
    STARTUPINFOA startup{};
    startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    if(!CreateProcessA(nullptr,command.data(),nullptr,nullptr,FALSE,
                       CREATE_NO_WINDOW,nullptr,nullptr,
                       &startup,&process))
        return false;
    const DWORD waited=WaitForSingleObject(process.hProcess,30000u);
    DWORD code=0;
    const bool success=waited==WAIT_OBJECT_0 &&
        GetExitCodeProcess(process.hProcess,&code) &&
        code==0x53494E45u;
    if(waited==WAIT_TIMEOUT)TerminateProcess(process.hProcess,1u);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return success;
}
std::uint32_t read32(void* owner,unsigned offset) {
    std::uint32_t result=0u;
    std::memcpy(&result,static_cast<std::uint8_t*>(owner)+offset,
                sizeof(result));
    return result;
}
void write32(void* owner,unsigned offset,std::uint32_t value) {
    std::memcpy(static_cast<std::uint8_t*>(owner)+offset,
                &value,sizeof(value));
}
} // namespace
int main(int argc,char** argv) {
    if(argc<2 || argc>3)
        return fail(1,"expected path to actual independent client.dll");
    if(argc==3 && std::strcmp(argv[2],"--guard")==0)
        return run_child_unsupported(argv[1])?
            fail(2,"unknown client prediction slot unexpectedly returned"):
            fail(3,"guard-mode setup refused");
    // All default tests must preserve production refusal even when
    // an owner environment from a previous test remains set.
    if(!setenv("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","0") ||
       !setenv("SIN_INDEPENDENT_TRACE_FILE","0") ||
       !setenv("SIN_INDEPENDENT_TRACE_SESSION",nullptr) ||
       !setenv("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0"))
        return fail(4,"cannot reset inherited owner diagnostic gates");
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** table=nullptr;
    if(!prepare_real_client(module,create,client,&table,argv[1]))
        return fail(5,"load/resolve actual production VClient011");
    int status=-1;
    if(create(pf::kInterfaceName,&status) || status!=1)
        return fail(6,"default shipping factory must NOT expose partial IPrediction");
    if(!setenv("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","1"))
        return fail(7,"cannot enable explicit owner opt-in");
    status=-1;
    if(create(pf::kInterfaceName,&status) || status!=1)
        return fail(8,"missing valid trace session must refuse prediction");
    if(!configure_session())return fail(9,"owner session setup");
    status=-1;
    if(create(pf::kInterfaceName,&status) || status!=1)
        return fail(10,"cannot expose prediction before VClient011 Init");

    // Full 21-query prefix is acquired, but cl_predict isn't
    // available. The first production class must return failure,
    // with no premature prediction factory registration.
    g_cvar.returned=nullptr;
    if(!run_rejected(module,create,client,table))
        return fail(11,"original ICvar.FindConstVar absent cl_predict refusal");
    // Fake registered ConVar with source-confirmed parent+0x18
    // and numeric+0x2C, owned by this test process.
    g_predict.CreateVariable(
      "cl_predict","1",0x200,nullptr,false,0,false,0,nullptr);
    if(!g_predict.string || g_predict.parent!=&g_predict ||
       g_predict.integer!=1)
        return fail(12,"owned source exact ConVar layout");
    g_cvar.returned=&g_predict;
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!invoke_init(client,table[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries ||
       g_physicsQueries)
        return fail(13,"actual VClient011 owner-gated successful prefix");
    status=-1;
    void* prediction=create(pf::kInterfaceName,&status);
    if(!prediction || status!=0 ||
       create(pf::kInterfaceName,nullptr)!=prediction)
        return fail(14,"actual CreateInterface prediction singleton");
    if(create("VClientPrediction002",nullptr) ||
       create("VClientEntityList003",nullptr) ||
       create("GameClientExports001",nullptr))
        return fail(15,"cannot falsely advertise unimplemented client factories");
    auto** predVtable=*reinterpret_cast<void***>(prediction);
    if(!predVtable || !owned(module,predVtable))
        return fail(16,"native production prediction vtable must be in client.dll");
    for(unsigned i=0u;i<pf::kOriginalSlots;++i) {
        if(!predVtable[i] || !owned(module,predVtable[i]))
            return fail(17,"all 23 native original slots must be owned");
    }
    if(predVtable[1]==predVtable[2] ||
       predVtable[1]==predVtable[7] ||
       read32(prediction,pf::kOriginalHighestOffset)!=0xFFFFFFFFu)
        return fail(18,"source prediction native offset or virtual slots");
    if(!invoke_void(prediction,predVtable[1]) ||
       *reinterpret_cast<std::uint8_t*>(
           static_cast<std::uint8_t*>(prediction)+
           pf::kOriginalEnabledOffset)!=1u)
        return fail(19,"engine-invoked original slot1 caches cl_predict enabled");
    if(!invoke_void(prediction,predVtable[2]) ||
       !invoke_byte(prediction,predVtable[15],0u) ||
       !invoke_byte(prediction,predVtable[16],0u) ||
       !invoke_word(prediction,predVtable[17],0u))
        return fail(20,"original source slot2 and 15/16/17 native CDECL-return ABI");
    write32(prediction,pf::kOriginalHighestOffset,77u);
    write32(prediction,pf::kOriginalCounterAOffset,88u);
    write32(prediction,pf::kOriginalCounterBOffset,99u);
    if(!invoke_void(prediction,predVtable[7]) ||
       read32(prediction,pf::kOriginalHighestOffset)!=0xFFFFFFFFu ||
       read32(prediction,pf::kOriginalCounterAOffset)!=0u ||
       read32(prediction,pf::kOriginalCounterBOffset)!=0u)
        return fail(21,"source original slot7 resets three DWORDs");

    // The production observer does not hardcode a value of one.
    // Reinitializing with effective cl_predict==0 must preserve
    // the same singleton pointer but update slot1 cached byte.
    g_predict.SetInt(0);
    g_factoryCalls=g_factoryErrors=0u;
    if(!invoke_init(client,table[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries ||
       create(pf::kInterfaceName,nullptr)!=prediction ||
       !invoke_void(prediction,predVtable[1]) ||
       *reinterpret_cast<std::uint8_t*>(
           static_cast<std::uint8_t*>(prediction)+
           pf::kOriginalEnabledOffset)!=0u)
        return fail(22,"engine ConVar numeric zero must disable prediction");

    // The original reads the PARENT of the returned ConVar, not
    // necessarily that ConVar's own integer. Test an invented
    // linked root with a distinct value to prove the offset.
    g_root.CreateVariable(
      "source_root","1",0,nullptr,false,0,false,0,nullptr);
    if(!g_root.string)return fail(23,"linked root synthetic ConVar");
    g_predict.parent=&g_root;
    g_factoryCalls=g_factoryErrors=0u;
    if(!invoke_init(client,table[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries ||
       !invoke_void(prediction,predVtable[1]) ||
       *reinterpret_cast<std::uint8_t*>(
           static_cast<std::uint8_t*>(prediction)+
           pf::kOriginalEnabledOffset)!=1u)
        return fail(24,"source +0x18 parent should control slot1 value");
    g_predict.parent=&g_predict;
    // An invalid engine return is never dereferenced.
    g_cvar.returned=reinterpret_cast<void*>(0x1u);
    g_factoryCalls=g_factoryErrors=0u;
    if(!invoke_init(client,table[0],0) ||
       g_factoryCalls!=kExpectedQueries ||
       create(pf::kInterfaceName,nullptr)!=nullptr)
        return fail(25,"invalid engine ConVar pointer must revoke factory");

    // Renew, then verify VClient011 slot1 Shutdown revokes the
    // factory even if explicit session flags remain set.
    g_cvar.returned=&g_predict;
    g_factoryCalls=g_factoryErrors=0u;
    if(!invoke_init(client,table[0],1) ||
       !create(pf::kInterfaceName,nullptr) ||
       !invoke_void(client,table[1]) ||
       create(pf::kInterfaceName,nullptr))
        return fail(26,"production shutdown must revoke prediction factory");
    if(!check_guard_in_child(argv[1]))
        return fail(27,"unimplemented native prediction method did not fail-closed");

    // Opt-out must not accidentally expose retained singleton.
    if(!setenv("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","0") ||
       create(pf::kInterfaceName,nullptr))
        return fail(28,"default factory refusal after diagnostic test");
    // The test-created linked ConVars are destroyed only after
    // production DLL runtime is unloaded. The real game never
    // receives their addresses outside the mock's lifetime.
    FreeLibrary(module);
    std::puts("PASS Phase1686 actual production PE32 client.dll: "
              "21 original VClient011 factory dependencies, guarded "
              "VEngineCvar002 FindConstVar cl_predict parent+0x18/"
              "numeric+0x2C, genuine CreateInterface("
              "VClientPrediction001) with all 23 owned x86 virtual "
              "entries, real native slot1 RET0 enabling/disabling, "
              "source 2/7/15/16/17 native ABI, missing/forged "
              "engine pointer refusal, owner-session-only Init success, "
              "shutdown revocation and child-process termination "
              "for unsupported slot3. No original game boot claimed.");
    return 0;
}
