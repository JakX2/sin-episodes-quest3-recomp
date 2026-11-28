// Phase1690: actual production PE32 client.dll VCENTERPRINT002
// 8-slot source-authenticated cold-state factory and explicit refusal
// to fabricate a backing 0xF8-byte GUI service at original slot6.
//
// This test does not run retail original game code. It loads the
// ACTUAL independent shipping client.dll via Win32 LoadLibrary,
// negotiates the real CreateInterface and its real MSVC __thiscall
// vtables with invented host-owned factory services/ConVar.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "native_original_client_centerprint_cold_factory.h"
#include "server_console_object_abi.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1690 CTest must use native Win32 MSVC x86
#endif

namespace api=sin_independent::native_original_client_centerprint;
namespace ca=sin_independent::console_abi;
namespace {
using Factory=void* (__cdecl *)(const char*,int*);
constexpr const char* kFactoryNames[]={
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
constexpr std::size_t kFactoryNamesCount=
    sizeof(kFactoryNames)/sizeof(kFactoryNames[0]);
int fail(int n,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1690 original client centerprint actual DLL #%d: %s\n",n,why);
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
// The original client Init after the 21 factory acquisitions
// invokes VSoundEmitter001 virtual slot0 Connect(appFactory) and
// branches to failure when its native bool is false. This is a
// fully caller-owned synthetic engine service, not retail code.
void* __cdecl appFactory(const char*,int*);
struct FakeSoundEmitter {
    virtual bool __thiscall Connect(Factory factory) noexcept {
        ++connects;
        last=factory;
        return allowed && factory==&appFactory;
    }
    virtual bool __thiscall Init() noexcept {return false;}
    unsigned connects=0u;
    Factory last=nullptr;
    bool allowed=true;
};
FakeSoundEmitter g_sound{};
std::uint32_t g_ignoredFactory=0u;
// Three deliberately malformed, CALLER-owned Source interface
// shapes. Never execute these as native C++ methods: the
// shipping DLL must reject them before its first virtual call.
unsigned g_soundFactoryMode=0u;
void* g_nonExecutableVtable[1]{&g_ignoredFactory};
void* g_nonExecutableSoundObject=g_nonExecutableVtable;
void* g_nullVtableSoundObject=nullptr;
unsigned g_factoryCalls=0u;
unsigned g_factoryErrors=0u;
unsigned g_physicsQueries=0u;
void* __cdecl appFactory(const char* name,int* status) {
    if(g_factoryCalls>=kFactoryNamesCount || !name || status ||
       std::strcmp(name,kFactoryNames[g_factoryCalls])!=0)
        ++g_factoryErrors;
    ++g_factoryCalls;
    if(name && std::strcmp(name,"VEngineCvar002")==0)
        return &g_cvar;
    if(name && std::strcmp(name,"VSoundEmitter001")==0) {
        if(g_soundFactoryMode==1u)
            return reinterpret_cast<void*>(0x1u);
        if(g_soundFactoryMode==2u)
            return &g_nonExecutableSoundObject;
        if(g_soundFactoryMode==3u)
            return &g_nullVtableSoundObject;
        return &g_sound;
    }
    return &g_ignoredFactory;
}
void* __cdecl physicsFactory(const char*,int*) {
    ++g_physicsQueries;
    return nullptr;
}
bool env(const char* key,const char* value) {
    return SetEnvironmentVariableA(key,value)!=0;
}
bool reset_environment() {
    return env("SIN_INDEPENDENT_TRACE_SESSION",nullptr) &&
        env("SIN_INDEPENDENT_TRACE_FILE","0") &&
        env("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0") &&
        env("SIN_INDEPENDENT_CLIENT_SOUND_CONNECT_CONTINUE_PROBE","0") &&
        env("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","0") &&
        env("SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE","0") &&
        env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE","0") &&
        env("SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE","0");
}
bool set_session(bool centerprint) {
    return env("SIN_INDEPENDENT_TRACE_SESSION",
               "0123456789abcdef0123456789abcdef") &&
       env("SIN_INDEPENDENT_TRACE_FILE","1") &&
       env("SIN_INDEPENDENT_CLIENT_SOUND_PROBE","0") &&
       env("SIN_INDEPENDENT_CLIENT_PREDICTION_FACTORY_PROBE","1") &&
       env("SIN_INDEPENDENT_CLIENT_PREDICTION_CVAR_REGISTER_PROBE","0") &&
       env("SIN_INDEPENDENT_CLIENT_ENTITYLIST_FACTORY_PROBE","1") &&
       env("SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE",
           centerprint?"1":"0");
}
bool owned(HMODULE module,const void* pointer) {
    HMODULE found=nullptr;
    return pointer && GetModuleHandleExA(
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      reinterpret_cast<LPCSTR>(pointer),&found) && found==module;
}
bool init_dll(const char* path,HMODULE& module,Factory& factory,
              void*& client,void**& clientVptr) {
    module=LoadLibraryA(path);
    if(!module)return false;
    auto exported=GetProcAddress(module,"CreateInterface");
    if(!owned(module,reinterpret_cast<void*>(exported)))
        return false;
    factory=reinterpret_cast<Factory>(exported);
    int status=-1;
    client=factory("VClient011",&status);
    if(!client || status!=0)return false;
    clientVptr=*reinterpret_cast<void***>(client);
    return clientVptr && owned(module,clientVptr[0]) &&
           owned(module,clientVptr[1]);
}
bool call_client_init(void* client,void* fn,int expected) {
    std::uintptr_t before=0u,after=0u;
    int actual=-999;
    std::uint32_t fakeGlobals=0x5E11DAB1u;
    void* globals=&fakeGlobals;
    Factory app=&appFactory,phys=&physicsFactory;
    __asm {
        mov before,esp
        push globals
        push phys
        push app
        mov ecx,client
        mov eax,fn
        call eax
        mov actual,eax
        mov after,esp
    }
    return before==after && actual==expected;
}
bool call_noarg(void* owner,void* fn) {
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
bool call_one(void* owner,void* fn,std::uint32_t a) {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push a
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool call_four(void* owner,void* fn,
               std::uint32_t a,std::uint32_t b,
               std::uint32_t c,std::uint32_t d) {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push d
        push c
        push b
        push a
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
bool call_five(void* owner,void* fn,
               std::uint32_t a,std::uint32_t b,std::uint32_t c,
               std::uint32_t d,std::uint32_t e) {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before,esp
        push e
        push d
        push c
        push b
        push a
        mov ecx,owner
        mov eax,fn
        call eax
        mov after,esp
    }
    return before==after;
}
std::uint32_t backing(void* instance) {
    std::uint32_t data=0xFEEDFACEu;
    std::memcpy(&data,
        static_cast<const std::uint8_t*>(instance)+
        api::kOriginalBackingOffset,sizeof(data));
    return data;
}
bool prepare() {
    g_predict.CreateVariable(
      "cl_predict","1",0x200,nullptr,false,0,false,0,nullptr);
    return g_predict.string &&
           g_predict.parent==&g_predict &&
           g_predict.integer==1 &&
           (g_cvar.predict=&g_predict)!=nullptr;
}
bool ready(Factory create,void* client,void** cvt,
           bool center) {
    if(!set_session(center))return false;
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!call_client_init(client,cvt[0],1) ||
       g_factoryErrors || g_factoryCalls!=kFactoryNamesCount ||
       g_physicsQueries)return false;
    return create("VClientPrediction001",nullptr) &&
           create("VClientEntityList003",nullptr);
}
// Read ONLY our own process-scoped diagnostic file, never the
// retail game's logs. This distinguishes actual native slot
// invocation from an accepted factory pointer. First-use-only
// per-session traces must not amplify frequent UI callbacks.
bool check_source_engine_call_events() {
    char temp[MAX_PATH]{};
    const DWORD n=GetTempPathA(MAX_PATH,temp);
    if(!n || n>=MAX_PATH)return false;
    char filename[MAX_PATH]{};
    const int written=std::snprintf(
        filename,sizeof(filename),
        "%ssin-independent-%s-%lu.log",
        temp,"0123456789abcdef0123456789abcdef",
        static_cast<unsigned long>(GetCurrentProcessId()));
    if(written<=0 || static_cast<std::size_t>(written)>=sizeof(filename))
        return false;
    std::ifstream file(filename,std::ios::binary);
    if(!file)return false;
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    const auto count=[&](const std::string& needle)->unsigned {
        unsigned found=0u;
        std::size_t pos=0u;
        while((pos=text.find(needle,pos))!=std::string::npos) {
            ++found;
            pos+=needle.size();
        }
        return found;
    };
    return count("[SiN independent] role=client phase=client-centerprint-slot4-cold version=five-arg-no-backing")==1u &&
           count("[SiN independent] role=client phase=client-centerprint-slot5-cold version=noarg-no-backing")==1u;
}
bool fatal_child(const char* path) {
    if(!prepare() || !set_session(true))return false;
    HMODULE m=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** table=nullptr;
    if(!init_dll(path,m,create,client,table) ||
       !ready(create,client,table,true))return false;
    void* cold=create(api::kInterfaceName,nullptr);
    if(!cold)return false;
    auto** slots=*reinterpret_cast<void***>(cold);
    if(!slots || !owned(m,slots[6]))return false;
    // The retail slot6 invokes a not-yet-reconstructed GUI service
    // constructor. The production boundary MUST terminate here,
    // with the exact native fatal code. An actual return is failure.
    using Slot6=void* (__thiscall *)(void*,void*);
    reinterpret_cast<Slot6>(slots[6])(cold,&g_predict);
    return false;
}
bool test_child_fatal(const char* dll) {
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
} // namespace

int main(int argc,char** argv) {
    if(argc!=2 && argc!=3)
        return fail(1,"expected actual independent client.dll path");
    if(argc==3 && std::strcmp(argv[2],"--guard")==0)
        return fatal_child(argv[1])?
            fail(2,"service initializer unexpectedly returned"):
            fail(3,"service initializer child setup failed");
    if(!reset_environment())return fail(4,"inherited flags not reset");
    HMODULE module=nullptr;
    Factory create=nullptr;
    void* client=nullptr;
    void** table=nullptr;
    if(!init_dll(argv[1],module,create,client,table))
        return fail(5,"load real PE32 client.dll/CreateInterface/VClient011");
    int status=-1;
    if(create(api::kInterfaceName,&status)||status!=1)
        return fail(6,"default shipping DLL must refuse unbuilt centerprint");
    if(!env("SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE","1"))
        return fail(7,"cannot enable centerprint environment flag");
    if(create(api::kInterfaceName,nullptr))
        return fail(8,"no prediction/valid session means no centerprint");
    if(!prepare())return fail(9,"owned cl_predict ConVar cannot be built");
    if(!set_session(false))return fail(10,"owner prediction/entity session");
    if(!ready(create,client,table,false))
        return fail(11,"original 21 client factory requests/prediction/entity");
    if(create(api::kInterfaceName,nullptr))
        return fail(12,"centerprint remains denied without distinct gate");
    if(!set_session(true))return fail(13,"enable original centerprint cold gate");
    if(create(api::kInterfaceName,nullptr))
        return fail(14,"cannot publish cold centerprint before fresh Init");
    if(!ready(create,client,table,true))
        return fail(15,"owner cold wrapper after source dependency prefix");
    status=-1;
    void* cold=create(api::kInterfaceName,&status);
    if(!cold || status!=0 || create(api::kInterfaceName,nullptr)!=cold)
        return fail(16,"real CreateInterface returns stable cold VCENTERPRINT002");
    auto** slots=*reinterpret_cast<void***>(cold);
    if(!slots || !owned(module,slots))
        return fail(17,"8-slot original centerprint vptr in client.dll");
    for(unsigned i=0u;i<api::kOriginalSlots;++i) {
        if(!slots[i] || !owned(module,slots[i]))
            return fail(18,"all eight source slots actual native client.dll-owned");
    }
    if(backing(cold)!=0u ||
       create("VCENTERPRINT003",nullptr) ||
       create("ClientLeafSystem001",nullptr))
        return fail(19,"source cold backend null and wrong factories refused");
    // All eight source native signatures are verified by source
    // static original audits. Here 0–5,7 run with native ECX +
    // EXACT MSVC x86 RET16/RET4/RET20/RET0 stack cleanup.
    if(!call_four(cold,slots[0],1u,2u,3u,4u) ||
       !call_one(cold,slots[1],0x1234u) ||
       !call_one(cold,slots[2],0x5678u) ||
       !call_five(cold,slots[3],1u,2u,3u,4u,5u) ||
       !call_five(cold,slots[4],0xFFu,0xFFu,0xFFu,0xFFu,0u) ||
       !call_noarg(cold,slots[5]) ||
       !call_noarg(cold,slots[5]) ||
       !call_noarg(cold,slots[7]) ||
       backing(cold)!=0u)
        return fail(20,"cold slots0–5,7 exact native ABI/empty backing");
    if(!check_source_engine_call_events())
        return fail(25,"actual centerprint source slot4/5 must emit one owner trace each, not just factory acceptance");
    // Phase1691: strengthen the SAME actual shipping DLL test,
    // not a standalone interpreter fixture. First prove a real
    // VSoundEmitter001 virtual slot0 bool=false makes the
    // original 21-name VClient011.Init fail before exporting
    // the later engine-visible client interfaces.
    if(!env("SIN_INDEPENDENT_CLIENT_SOUND_CONNECT_CONTINUE_PROBE","1"))
        return fail(26,"cannot enter owner sound Connect continuation gate");
    g_sound.allowed=false;
    g_sound.connects=0u;
    g_sound.last=nullptr;
    g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
    if(!call_client_init(client,table[0],0) ||
       g_factoryErrors || g_factoryCalls!=kFactoryNamesCount ||
       g_sound.connects!=1u || g_sound.last!=&appFactory ||
       create("VClientPrediction001",nullptr) ||
       create("VClientEntityList003",nullptr) ||
       create(api::kInterfaceName,nullptr))
        return fail(27,"retail source sound Connect=false must refuse before factory publication");
    // The real engine is trusted only up to the exact ABI
    // evidence supplied. Non-null VSoundEmitter001 from its
    // CreateInterface is NOT sufficient: forged pointers,
    // a non-executable virtual0 target, or a null vfptr must
    // refuse safely with original Init=0 and no native vcall.
    g_sound.allowed=true;
    for(unsigned mode=1u;mode<=3u;++mode) {
        g_soundFactoryMode=mode;
        g_sound.connects=0u;
        g_sound.last=nullptr;
        g_factoryCalls=g_factoryErrors=g_physicsQueries=0u;
        if(!call_client_init(client,table[0],0) ||
           g_factoryErrors || g_factoryCalls!=kFactoryNamesCount ||
           g_sound.connects!=0u || g_sound.last!=nullptr ||
           create("VClientPrediction001",nullptr) ||
           create("VClientEntityList003",nullptr) ||
           create(api::kInterfaceName,nullptr))
            return fail(29,"forged sound vptr/native nonexecutable callback was not refused before Connect");
    }
    g_soundFactoryMode=0u;
    // A real native virtual Connect true now permits the
    // SAME x86 VClient011.Init to reacquire the cl_predict
    // source parent, prediction, entity list and cold centerprint.
    g_sound.allowed=true;
    g_sound.connects=0u;
    g_sound.last=nullptr;
    if(!ready(create,client,table,true) ||
       g_sound.connects!=1u || g_sound.last!=&appFactory ||
       !create(api::kInterfaceName,nullptr))
        return fail(28,"source sound Connect=true must continue to original prediction/factory boundaries");
    if(!test_child_fatal(argv[1]))
        return fail(21,"source backing service Init slot6 did not terminate");
    if(!env("SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE","0") ||
       create(api::kInterfaceName,nullptr))
        return fail(22,"factory may not survive explicit flag revocation");
    if(!env("SIN_INDEPENDENT_CLIENT_CENTERPRINT_FACTORY_PROBE","1") ||
       !call_noarg(client,table[1]) ||
       create(api::kInterfaceName,nullptr) ||
       create("VClientEntityList003",nullptr) ||
       create("VClientPrediction001",nullptr))
        return fail(23,"client Shutdown must revoke all 3 startup factories");
    const auto retained=module;
    if(!FreeLibrary(module) ||
       GetModuleHandleA("client.dll")!=retained)
        return fail(24,"owner-pinned native cold centerprint vtable unmapped");
    std::puts("PASS Phase1690 ACTUAL independent PE32 client.dll "
              "21 original engine dependencies, prediction + "
              "entity-list owner opt-in, real CreateInterface("
              "VCENTERPRINT002) 8-slot MSVC x86 vtable, original "
              "cold null backing +4, all source 0–5/7 "
              "thiscall ESP cleanup 16/4/4/20/20/0/0, "
              "fatal slot6 GUI service Init child process, "
              "revocation, module pin, wrong leaf/version refusal. "
              "Original game engine not executed, no renderer/boot.");
    return 0;
}
