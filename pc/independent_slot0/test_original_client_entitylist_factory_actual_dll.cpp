// Phase1689: LoadLibrary the ACTUAL independently built production
// client.dll and exercise real exported CreateInterface, authentic
// 21-query VClient011.Init, opt-in VClientPrediction001 and
// original-layout VClientEntityList003 secondary +0x10014.
// All real nine MSVC x86 virtual slots are invoked with actual
// ESP cleanup; no original game DLL, entity, map or asset is used.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "server_console_object_abi.h"
#include "native_original_client_entitylist_empty_factory.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1689 native MSVC x86 only
#endif

namespace api=sin_independent::native_original_client_entitylist_factory;
namespace ca=sin_independent::console_abi;
namespace {
using Factory=void* (__cdecl *)(const char*,int*);
constexpr const char* kExpectedFactoryOrder[]={
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
constexpr std::size_t kExpectedQueries=
    sizeof(kExpectedFactoryOrder)/sizeof(kExpectedFactoryOrder[0]);
int fail(int n,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1689 actual client entity-list factory #%d: %s\n",n,why);
    return n;
}
struct FakeCvar {
    virtual void __thiscall Register(void*) noexcept {}
    virtual const char* __thiscall CommandLine(
       const char*) noexcept {return nullptr;}
    virtual void* __thiscall FindMutable(
       const char*) noexcept {return nullptr;}
    virtual const void* __thiscall FindConst(
       const char* name) const noexcept {
       return name && std::strcmp(name,"cl_predict")==0?
           static_cast<const void*>(predict):nullptr;
    }
    virtual void* __thiscall GetCommands() noexcept {return nullptr;}
    ca::Variable* predict=nullptr;
};
FakeCvar g_cvar{};
ca::Variable g_predict{};
std::uint32_t g_ignoredFactory=0u;
unsigned g_factoryCalls=0u;
unsigned g_factoryErrors=0u;
unsigned g_physicsQueries=0u;
void* __cdecl appFactory(const char* name,int* status) {
    if(g_factoryCalls>=kExpectedQueries || !name || status ||
       std::strcmp(name,kExpectedFactoryOrder[g_factoryCalls])!=0)
        ++g_factoryErrors;
    ++g_factoryCalls;
    if(name && std::strcmp(name,"VEngineCvar002")==0)
        return &g_cvar;
    return &g_ignoredFactory;
}
void* __cdecl physicsFactory(const char*,int*) {
    ++g_physicsQueries;
    return nullptr;
}
bool env(const char* key,const char* value) {
    return SetEnvironmentVariableA(key,value)!=0;
}
bool session(bool entity) {
    return env("SIN_INDEPENDENT_TRACE_SESSION",
               "0123456789abcdef0123456789abcdef") &&
           env("SIN_INDEPENDENT_TRACE_FILE","1") &&
           env("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0") &&
           env("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","1") &&
           env("SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE","0") &&
           env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE",
               entity?"1":"0");
}
bool owned(HMODULE module,const void* pointer) {
    HMODULE out=nullptr;
    return pointer && GetModuleHandleExA(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCSTR>(pointer),&out) && out==module;
}
bool invoke_init(void* object,void* fn,int expected) {
    std::uintptr_t before=0u,after=0u;
    int actual=-1;
    std::uint32_t inventedGlobals=0x5E11DAB1u;
    void* globals=&inventedGlobals;
    Factory app=&appFactory,physics=&physicsFactory;
    __asm {
        mov before,esp
        push globals
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
bool invoke_int(void* owner,void* fn,int expected) {
    std::uintptr_t before=0u,after=0u;
    int result=0;
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
bool invoke_one(void* owner,void* fn,std::uint32_t arg,
                std::uintptr_t expected) {
    std::uintptr_t before=0u,after=0u;
    std::uintptr_t result=0x12345678u;
    __asm {
        mov before,esp
        push arg
        mov ecx,owner
        mov eax,fn
        call eax
        mov result,eax
        mov after,esp
    }
    return before==after && result==expected;
}
bool invoke_one_void(void* owner,void* fn,std::uint32_t arg) {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push arg
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool init_production(const char* path,HMODULE& module,
                     Factory& create,void*& client,
                     void**& clientVtable) {
    module=LoadLibraryA(path);
    if(!module)return false;
    const auto exported=GetProcAddress(module,"CreateInterface");
    if(!owned(module,reinterpret_cast<void*>(exported)))
        return false;
    create=reinterpret_cast<Factory>(exported);
    int status=-1;
    client=create("VClient011",&status);
    if(!client || status!=0)return false;
    clientVtable=*reinterpret_cast<void***>(client);
    return clientVtable && owned(module,clientVtable[0]) &&
           owned(module,clientVtable[1]);
}
bool run_guard_child(const char* path) {
    if(!session(true))return false;
    g_predict.CreateVariable(
      "cl_predict","1",0x200,nullptr,false,0,false,0,nullptr);
    if(!g_predict.string)return false;
    g_cvar.predict=&g_predict;
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** clientVtable=nullptr;
    if(!init_production(path,module,create,client,clientVtable))
        return false;
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!invoke_init(client,clientVtable[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries)
        return false;
    void* secondary=create(api::kInterfaceName,nullptr);
    if(!secondary)return false;
    const auto parent=reinterpret_cast<std::uintptr_t>(secondary)-
                      api::kOriginalSecondaryOffset;
    auto** primarySlots=*reinterpret_cast<void***>(parent);
    if(!primarySlots || !owned(module,primarySlots[0]))
        return false;
    // An attempted ORIGINAL primary registration must refuse
    // through the production fatal callback, not invent live
    // C_BaseEntity state or silently return.
    void* someSyntheticEntity=&g_predict;
    std::uint32_t serialHandle=2u;
    __asm {
        push serialHandle
        push someSyntheticEntity
        mov ecx,parent
        mov eax,primarySlots
        call dword ptr[eax]
    }
    return false; // fail if prohibited parent registration returns
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
    DWORD code=0u;
    const bool success=waited==WAIT_OBJECT_0 &&
        GetExitCodeProcess(process.hProcess,&code) &&
        code==0x53494E45u;
    if(waited==WAIT_TIMEOUT)TerminateProcess(process.hProcess,1u);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return success;
}
std::uint32_t read32(const void* ptr,unsigned offset) {
    std::uint32_t out=0u;
    std::memcpy(&out,static_cast<const std::uint8_t*>(ptr)+offset,
                sizeof(out));
    return out;
}
} // namespace

int main(int argc,char** argv) {
    if(argc!=2 && argc!=3)
        return fail(1,"expected real independent client DLL path");
    if(argc==3 && std::strcmp(argv[2],"--guard")==0)
        return run_guard_child(argv[1])?
            fail(2,"primary registration unexpectedly returned"):
            fail(3,"guard-mode could not create actual DLL instance");
    // All inherited test flags are off before first negotiation.
    if(!env("SIN_INDEPENDENT_TRACE_SESSION",nullptr) ||
       !env("SIN_INDEPENDENT_TRACE_FILE","0") ||
       !env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE","0") ||
       !env("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","0") ||
       !env("SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE","0") ||
       !env("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0"))
        return fail(4,"cannot reset inherited session flags");
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** clientVtable=nullptr;
    if(!init_production(argv[1],module,create,client,clientVtable))
        return fail(5,"load actual independent client.dll / VClient011");
    int status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(6,"default factory must refuse incomplete entity list");
    if(!env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE","1"))
        return fail(7,"cannot enable entitylist without prediction session");
    status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(8,"gating requires prediction & verified prior Init");
    if(!session(false))return fail(9,"configure owner prediction");
    status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(10,"ungated entity factory must remain refused");
    g_predict.CreateVariable(
      "cl_predict","1",0x200,nullptr,false,0,false,0,nullptr);
    if(!g_predict.string ||
       g_predict.parent!=&g_predict ||
       g_predict.integer!=1)
        return fail(11,"original-guided host-owned ConVar view");
    g_cvar.predict=&g_predict;
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!invoke_init(client,clientVtable[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries ||
       g_physicsQueries)
        return fail(12,"original 21 client factory requests and prediction Init");
    if(!create("VClientPrediction001",nullptr))
        return fail(13,"prediction must remain available when entity probe off");
    status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(14,"entitylist remains refused without separate opt-in");
    if(!session(true))return fail(15,"configure entity secondary factory session");
    status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(16,"cannot publish entity list without reentering owner Init");
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!invoke_init(client,clientVtable[0],1) ||
       g_factoryErrors || g_factoryCalls!=kExpectedQueries ||
       g_physicsQueries)
        return fail(17,"authentic 21 factory requests with entitylist opt-in");
    status=-1;
    void* secondary=create(api::kInterfaceName,&status);
    if(!secondary || status!=0 ||
       create(api::kInterfaceName,nullptr)!=secondary)
        return fail(18,"stable secondary CreateInterface identity");
    auto** slots=*reinterpret_cast<void***>(secondary);
    if(!slots || !owned(module,slots))
        return fail(19,"source secondary vptr must be in actual client.dll");
    for(unsigned i=0u;i<api::kSecondaryVirtualCount;++i)
        if(!slots[i] || !owned(module,slots[i]))
            return fail(20,"nine original secondary PE32 vtable slots owned");
    const auto sub=reinterpret_cast<std::uintptr_t>(secondary);
    const auto base=sub-api::kOriginalSecondaryOffset;
    auto** parentSlots=*reinterpret_cast<void***>(base);
    if(!parentSlots || !owned(module,parentSlots))
        return fail(21,"source +0x10014 secondary vs primary vfptr");
    for(unsigned i=0u;i<api::kParentVirtualCount;++i)
        if(!parentSlots[i] || !owned(module,parentSlots[i]))
            return fail(22,"source three native parent vtable slots owned");
    // Source offsets on the EXACT real replacement object.
    if(read32(secondary,0x04u)!=0u ||
       read32(secondary,0x08u)!=0u ||
       read32(secondary,0x0Cu)!=0u ||
       read32(secondary,0x10u)!=0xFFFFFFFFu ||
       read32(secondary,0x14u)!=0u ||
       read32(secondary,0x14u+8u*4095u)!=0u ||
       read32(reinterpret_cast<void*>(base),4u)!=0u)
        return fail(23,"original entitylist secondary offsets / empty state");
    // Retail base constructor 0x24127E80 initializes the
    // raw+8 and raw+12 intrusive links to each own raw record.
    // The host serial remains safely deterministic (0), not
    // a fabricated original vstdlib RandomInt sample.
    const std::uintptr_t firstRaw=base+api::kOriginalRawOffset;
    const std::uintptr_t lastRaw=firstRaw+
        (api::kRawCount-1u)*api::kOriginalRawStride;
    if(read32(reinterpret_cast<void*>(base),
              api::kOriginalRawOffset+8u)!=firstRaw ||
       read32(reinterpret_cast<void*>(base),
              api::kOriginalRawOffset+12u)!=firstRaw ||
       read32(reinterpret_cast<void*>(base),
              api::kOriginalRawOffset+
              (api::kRawCount-1u)*api::kOriginalRawStride+8u)!=lastRaw ||
       read32(reinterpret_cast<void*>(base),
              api::kOriginalRawOffset+
              (api::kRawCount-1u)*api::kOriginalRawStride+12u)!=lastRaw)
        return fail(36,"original 4096 raw records must begin self-linked");

    // Verify ALL 9 real MSVC vcall slot return and cleanup ABIs.
    if(!invoke_one(secondary,slots[0],0u,0u) ||
       !invoke_one(secondary,slots[0],4095u,0u) ||
       !invoke_one(secondary,slots[0],4096u,0u) ||
       !invoke_one(secondary,slots[0],0xFFFFFFFFu,0u))
        return fail(24,"slot0 GET networkable RET4 safe absent");
    if(!invoke_one(secondary,slots[1],0u,0u) ||
       !invoke_one(secondary,slots[1],api::kInvalidEhandle,0u))
        return fail(25,"slot1 GET networkable from handle RET4");
    if(!invoke_one(secondary,slots[2],0u,0u) ||
       !invoke_one(secondary,slots[2],api::kInvalidEhandle,0u) ||
       !invoke_one(secondary,slots[2],0x10000000u,0u))
        return fail(26,"slot2 GET unknown handle resolution RET4");
    if(!invoke_one(secondary,slots[3],0u,0u) ||
       !invoke_one(secondary,slots[3],4095u,0u) ||
       !invoke_one(secondary,slots[3],0xFFFFFFFFu,0u) ||
       !invoke_one(secondary,slots[3],4096u,0u))
        return fail(27,"slot3 GET entity by signed index RET4");
    if(!invoke_one(secondary,slots[4],0u,0u) ||
       !invoke_one(secondary,slots[4],api::kInvalidEhandle,0u))
        return fail(28,"slot4 GET entity from handle RET4");
    if(!invoke_one(secondary,slots[5],0u,0u) ||
       !invoke_one(secondary,slots[5],1u,0u) ||
       !invoke_int(secondary,slots[6],-1))
        return fail(29,"count and highest original native ABI RET4/RET0");
    if(!invoke_one_void(secondary,slots[7],2048u) ||
       !invoke_int(secondary,slots[8],2048) ||
       read32(secondary,api::kOriginalSecondaryMaxEntities)!=2048u ||
       !invoke_one_void(secondary,slots[7],4096u) ||
       !invoke_int(secondary,slots[8],4096) ||
       !invoke_one_void(secondary,slots[7],0u) ||
       !invoke_int(secondary,slots[8],0))
        return fail(30,"slot7 setter RET4 and slot8 getter RET0");
    // The test confirms the future requested *next* interface is
    // still denied. We did not conjure a centerprint/leaf object.
    if(create("VCENTERPRINT002",nullptr) ||
       create("ClientLeafSystem001",nullptr) ||
       create("VClientEntityList004",nullptr))
        return fail(31,"cannot invent next unimplemented factories");
    if(!check_guard_in_child(argv[1]))
        return fail(32,"source parent register RET8 must terminate not fabricate entities");
    if(!invoke_void(client,clientVtable[1]) ||
       create(api::kInterfaceName,nullptr) ||
       create("VClientPrediction001",nullptr))
        return fail(33,"client Shutdown revokes both source factory gates");
    if(!env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE","0") ||
       create(api::kInterfaceName,nullptr))
        return fail(34,"entity factory unavailable after opting out");
    // The original engine may hold the secondary pointer after
    // the LoadLibrary caller releases its own reference. Pinning
    // the actual module protects its native vptr code.
    const auto retained=module;
    if(!FreeLibrary(module) ||
       GetModuleHandleA("client.dll")!=retained)
        return fail(35,"owner VClientEntityList003 vptr module pin lifetime");
    std::puts("PASS Phase1689 actual production PE32 client.dll: "
              "21 original VClient011 factory dependencies, "
              "original VClientPrediction001 available, "
              "source secondary VClientEntityList003 +0x10014 from "
              "raw original 4096x16 owner +4, all 3 parent and "
              "9 secondary native MSVC vtable members DLL-owned, "
              "exact original secondary live/max/flagged/highwater/"
              "4096x8 mirror byte offsets, all nine actual "
              "native slot ESP return ABIs, source-faithful "
              "empty entity state, restricted mutable max entities, "
              "default/session gates, shutdown revocation, "
              "fatal parent registration child test, module pin. "
              "No original game runtime or gameplay proven.");
    return 0;
}
