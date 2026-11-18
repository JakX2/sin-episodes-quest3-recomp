// Real native DLL loader regression for the opt-in negotiation probe.
// Tests only independently built code. Guard invocations run in children
// of this test executable; no retail executable or original DLL is loaded.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cmath>
#include "server_gameents_abi.h"

using Factory = void* (__cdecl *)(const char*,int*);
constexpr char kSession[]="32500000000000000000000000000001";
constexpr DWORD kExpectedStop=0x53494E45u;
constexpr const char* kNames[]={"ServerGameDLL003","ServerGameEnts001","ServerGameClients003"};
constexpr int kCounts[]={31,7,15};
static int fail(const char* reason) {
    std::fprintf(stderr,"FAIL server factory probe: %s\n",reason);return 1;
}
static bool configure(bool on,const char* session=kSession,bool traces=true) {
    return SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_FACTORY_PROBE",on?"1":"0") &&
           SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION",session) &&
           SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE",traces?"1":"0");
}
static bool dllOwns(HMODULE dll,const void* pointer) {
    HMODULE owner=nullptr;
    return pointer && GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        static_cast<LPCSTR>(pointer),&owner) && owner==dll;
}
static std::string logPath(DWORD pid) {
    char temp[MAX_PATH]={},name[MAX_PATH]={};
    const auto n=GetTempPathA(MAX_PATH,temp);
    if(!n || n>=MAX_PATH)return {};
    const int len=std::snprintf(name,sizeof(name),"%ssin-independent-%s-%lu.log",
        temp,kSession,static_cast<unsigned long>(pid));
    return len>0 && static_cast<size_t>(len)<sizeof(name)?name:"";
}
static bool traceHasStop(DWORD pid,const char* method) {
    auto path=logPath(pid);
    if(path.empty())return false;
    FILE* f=nullptr;
    if(fopen_s(&f,path.c_str(),"rb")||!f)return false;
    char line[512]={};
    std::string expected="role=server phase=unsupported-callback version=";
    expected+=method;
    expected+="\n";
    bool found=false;
    while(std::fgets(line,sizeof(line),f))
        if(std::strstr(line,expected.c_str())) found=true;
    std::fclose(f); DeleteFileA(path.c_str());
    return found;
}
static int invokeGuard(Factory create,int role,int slot) {
    auto* object=create(kNames[role],nullptr);
    if(!object)return fail("guard child factory");
    auto** table=*reinterpret_cast<void***>(object);
    using NoArgs=void (__thiscall *)(void*);
    if(role!=1) {
        reinterpret_cast<NoArgs>(table[slot])(object);
    } else {
        // Exercise the actual typed entity ABI. Non-null opaque tokens
        // must never be dereferenced by an unimplemented callback.
        auto* ents=static_cast<sin_independent::ServerGameEntsAbi*>(object);
        int token=0;
        auto* edict=reinterpret_cast<edict_t*>(&token);
        switch(slot) {
        case 0:
            reinterpret_cast<void* (__thiscall *)(void*,unsigned)>(table[0])(object,1u);
            break;
        case 2: ents->MarkEntitiesAsTouching(edict,edict);break;
        case 3: ents->FreeContainingEntity(edict);break;
        case 4: (void)ents->BaseEntityToEdict(reinterpret_cast<CBaseEntity*>(&token));break;
        case 5: (void)ents->EdictToBaseEntity(edict);break;
        case 6: ents->CheckTransmit(nullptr,nullptr,0);break;
        default:return fail("invalid entity guard slot");
        }
    }
    return fail("unsupported callback returned");
}
static bool runChild(const char* dll,int role,int slot,const char* method) {
    char exe[MAX_PATH]={};
    auto n=GetModuleFileNameA(nullptr,exe,MAX_PATH);
    if(!n || n>=MAX_PATH)return false;
    std::string cmd="\"";cmd+=exe;cmd+="\" \"";cmd+=dll;cmd+="\" --guard ";
    cmd+=std::to_string(role);cmd+=" ";cmd+=std::to_string(slot);
    STARTUPINFOA si{};si.cb=sizeof(si);
    PROCESS_INFORMATION pi{};
    if(!CreateProcessA(exe,&cmd[0],nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi))
        return false;
    CloseHandle(pi.hThread);
    const DWORD wait=WaitForSingleObject(pi.hProcess,10000);
    DWORD code=0;
    if(wait!=WAIT_OBJECT_0) {
        // Only terminate the child process created by this synthetic test.
        TerminateProcess(pi.hProcess,1u);
        WaitForSingleObject(pi.hProcess,5000);
    } else GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hProcess);
    if(code!=kExpectedStop || !traceHasStop(pi.dwProcessId,method)) {
        std::fprintf(stderr,"guard failed role=%d slot=%d exit=%08lx\n",
            role,slot,static_cast<unsigned long>(code));
        return false;
    }
    return true;
}
int main(int argc,char** argv) {
    if(argc!=3 && argc!=5)return fail("expected server DLL and synthetic command-line provider paths");
    if(!configure(argc==5))return fail("environment");
    HMODULE dll=LoadLibraryA(argv[1]);
    if(!dll)return fail("LoadLibrary");
    auto create=reinterpret_cast<Factory>(GetProcAddress(dll,"CreateInterface"));
    if(!create)return fail("CreateInterface");
    if(argc==5) {
        const int role=std::atoi(argv[3]),slot=std::atoi(argv[4]);
        if(std::strcmp(argv[2],"--guard") || role<0 || role>2 ||
           slot<0 || slot>=kCounts[role])return fail("child arguments");
        return invokeGuard(create,role,slot);
    }

    int code=-1;
    if(create(kNames[1],&code)||code!=1||create(kNames[2],nullptr))
        return fail("opt-in defaults off");
    if(!configure(true,"INVALID") || create(kNames[1],nullptr) ||
       !configure(true,kSession,false) || create(kNames[1],nullptr) ||
       !configure(true))return fail("trace/session gate");
    void* objects[3]={};
    for(int r=0;r<3;++r) {
        code=-1;
        objects[r]=create(kNames[r],&code);
        if(!objects[r] || code!=0 || create(kNames[r],nullptr)!=objects[r] ||
           !dllOwns(dll,objects[r]))return fail("stable factory singleton");
        auto** table=*reinterpret_cast<void***>(objects[r]);
        if(!dllOwns(dll,table))return fail("vtable ownership");
        for(int s=0;s<kCounts[r];++s)
            if(!dllOwns(dll,table[s]))return fail("missing guarded original table entry");
    }
    auto* ents=static_cast<sin_independent::ServerGameEntsAbi*>(objects[1]);
    ents->SetDebugEdictBase(nullptr);
    if(ents->EdictToBaseEntity(nullptr)||ents->BaseEntityToEdict(nullptr))
        return fail("original null conversions");
    using Limits=void (__thiscall *)(void*,int&,int&,int&);
    auto** clientsTable=*reinterpret_cast<void***>(objects[2]);
    int low=-1,high=-1,def=-1;
    reinterpret_cast<Limits>(clientsTable[0])(objects[2],low,high,def);
    if(low!=1||high!=32||def!=1)return fail("original player limits");

    // The server's actual slot 9 uses a loaded vstdlib command-line
    // singleton. This synthetic DLL lives outside the gameplay artifact.
    HMODULE fixture=LoadLibraryA(argv[2]);
    if(!fixture)return fail("test-only command-line provider");
    using SetFixture=void (__cdecl *)(int,int);
    using VerifyFixture=bool (__cdecl *)();
    auto setFixture=reinterpret_cast<SetFixture>(GetProcAddress(fixture,"SetTickFixture"));
    auto verifyFixture=reinterpret_cast<VerifyFixture>(GetProcAddress(fixture,"VerifyTickFixture"));
    if(!setFixture || !verifyFixture ||
       GetModuleHandleA("vstdlib.dll")!=fixture)
        return fail("synthetic provider identity");
    using TickInterval=float (__thiscall *)(const void*);
    auto** serverTable=*reinterpret_cast<void***>(objects[0]);
    auto tick=reinterpret_cast<TickInterval>(serverTable[9]);
    struct TickCase { int present; int rate; float expected; };
    const TickCase cases[] = {
        {0,60,0.015f},{1,-1,0.015f},{1,0,0.015f},
        {1,10,0.015f},{1,11,1.0f/11.0f},{1,60,1.0f/60.0f},
        {1,100,0.01f},{1,1000,0.001f},{1,2147483647,1.0f/2147483647.0f}
    };
    for(const auto& c:cases) {
        setFixture(c.present,c.rate);
        const float actual=tick(objects[0]);
        if(!verifyFixture() || !std::isfinite(actual) ||
           std::fabs(actual-c.expected)>1e-7f*std::fabs(c.expected))
            return fail("original tick interval or command-line callback sequence");
    }
    FreeLibrary(fixture);

    // Exact original pre-game-rules branch. Invoke via the actual exported
    // server table, not a duplicate literal-only helper.
    using Description=const char* (__thiscall *)(void*);
    auto description=reinterpret_cast<Description>(serverTable[11]);
    const char* firstDescription=description(objects[0]);
    if(!firstDescription || std::strcmp(firstDescription,"SiN Episodes") ||
       !dllOwns(dll,firstDescription) ||
       description(objects[0])!=firstDescription)
        return fail("original slot 11 stable pre-init game description");

    for(int i=2;i<31;++i) {
        if(i==11)continue;
        const auto name=i==9 ? std::string("ServerGameDLL003.GetTickInterval.missing-commandline")
            : std::string(kNames[0])+".slot"+std::to_string(i);
        if(!runChild(argv[1],0,i,name.c_str()))return fail("main table guard");
    }
    for(int i=1;i<15;++i) {
        auto name=std::string(kNames[2])+".slot"+std::to_string(i);
        if(!runChild(argv[1],2,i,name.c_str()))return fail("clients table guard");
    }
    const char* methods[]={"destructor",nullptr,"MarkEntitiesAsTouching",
        "FreeContainingEntity","BaseEntityToEdict","EdictToBaseEntity","CheckTransmit"};
    for(int i=0;i<7;++i)if(methods[i]) {
        auto name=std::string(kNames[1])+"."+methods[i];
        if(!runChild(argv[1],1,i,name.c_str()))return fail("entity table guard");
    }
    FreeLibrary(dll);
    DeleteFileA(logPath(GetCurrentProcessId()).c_str());
    std::puts("PASS opt-in server factory probe: original table spans 31/7/15, player limits, null paths, 48 terminal child cases, nine tick cases and original pre-init game description");
    return 0;
}
