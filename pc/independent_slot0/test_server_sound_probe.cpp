// Test the distributed server DLL's actual opt-in branch against owned
// synthetic engine interfaces. Each case runs in its own child process.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include "server_single_cvar_probe.h"
using Factory=void* (__cdecl *)(const char*,int*);
static constexpr const char* kSession="33000000000000000000000000000001";
static constexpr const char* kSequence[]={
    "VEngineServer021","VoiceServer001","VEngineCvar002",
    "VEngineServerStringTable001","StaticPropMgrServer002","VEngineRandom001",
    "IEngineSoundServer003","SpatialPartition001","VModelInfoServer001",
    "EngineTraceServer003","VFileSystem012","GAMEEVENTSMANAGER002",
    "VEngineCache001","VSoundEmitter001","ServerUploadGameStats001",
    "ISERVERPLUGINHELPERS001"
};
static int step=0,mode=0,token=0,calls=0,eventCalls=0;
static void* __cdecl Engine(const char*,int*);
class Sound final {
public:
    virtual bool __thiscall Connect(Factory f) noexcept {
        if(f!=&Engine || step!=16 || ++calls!=1 ||
           (mode!=1 && mode!=2 && mode<5)) ExitProcess(91);
        return mode!=2 && mode!=7;
    }
};
static Sound sound;
class Events final {
public:
    virtual void __thiscall Guard0() noexcept { ExitProcess(101); }
    virtual int __thiscall Load(const char* name) noexcept {
        if(calls!=1 || step!=16 || ++eventCalls!=1 || !name ||
           std::strcmp(name,"resource/gameevents.res") || (mode!=5 && mode!=6 && mode!=8 && mode<9)) ExitProcess(102);
        return mode==6 || mode==15?0:mode==8?-1:42;
    }
};
static Events events;
using namespace sin_independent::console_abi;
static void __cdecl NoopCommand(){}
class CvarFixture final:public EngineCvar {
public:
    Variable existing;
    Command conflict;
    CommandBase* registeredObject=nullptr;
    mutable unsigned finds=0;
    unsigned lists=0,regs=0,overrides=0;
    void Prepare() {
        existing.CreateVariable(kProbeVarName,"99",0,nullptr,false,0,false,0,nullptr);
        conflict.CreateCommand(kProbeVarName,&NoopCommand,nullptr,0,nullptr);
        if(mode==16)conflict.next=&conflict;
    }
    void __thiscall RegisterConCommandBase(CommandBase* object) noexcept override {
        if(eventCalls!=1 || finds!=1 || lists!=1 || ++regs!=1 ||
           (mode!=9 && mode!=12 && mode!=13 && mode!=14) || !object ||
           object->IsCommand() || object->registered || object->next ||
           !object->IsBitSet(4) || std::strcmp(object->GetName(),kProbeVarName))
            ExitProcess(103);
        auto* v=static_cast<Variable*>(object);
        if(v->parent!=v || v->callback || v->integer ||
           std::strcmp(v->String(),"0") || v->hasMin || v->hasMax)ExitProcess(104);
        registeredObject=object;
        if(mode==14)v->parent=&existing;
    }
    const char* __thiscall GetCommandLineValue(const char* n) noexcept override {
        if(regs!=1 || ++overrides!=1 || std::strcmp(n,kProbeVarName) || mode==14)
            ExitProcess(105);
        return mode==13?"1":nullptr;
    }
    Variable* __thiscall FindMutableVar(const char*) noexcept override {ExitProcess(106);}
    const Variable* __thiscall FindConstVar(const char* n) const noexcept override {
        if(eventCalls!=1 || std::strcmp(n,kProbeVarName) || ++finds>2)ExitProcess(107);
        if(finds==1)return mode==10?&existing:nullptr;
        if(regs!=1 || overrides!=1 || !registeredObject || !registeredObject->registered)
            ExitProcess(108);
        return mode==12?nullptr:static_cast<const Variable*>(registeredObject);
    }
    CommandBase* __thiscall GetCommands() noexcept override {
        if(finds!=1 || ++lists!=1 || mode==10)ExitProcess(109);
        if(mode==16) {conflict.name="cycle";return &conflict;}
        return mode==11?&conflict:nullptr;
    }
};
static CvarFixture cvar;
static void* __cdecl Engine(const char* name,int* status) {
    if(step>=16 || step==10 || !name || status ||
       std::strcmp(name,kSequence[step])) ExitProcess(92);
    ++step;
    if(mode==3 && step==16)return nullptr;
    if(std::strcmp(name,"VSoundEmitter001")==0)return &sound;
    if(std::strcmp(name,"GAMEEVENTSMANAGER002")==0)return &events;
    if(std::strcmp(name,"VEngineCvar002")==0)return &cvar;
    return &token;
}
static void* __cdecl FileSystem(const char* name,int* status) {
    if(step!=10 || !name || status || std::strcmp(name,kSequence[step]))
        ExitProcess(93);
    ++step;return &token;
}
static void* __cdecl Physics(const char*,int*) { ExitProcess(94); }
static int Child(const char* dll,int selected) {
    mode=selected;
    cvar.Prepare();
    if(!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE","1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION",kSession) ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_FACTORY_PROBE",mode==4?"0":"1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_SOUND_PROBE",mode==0?"0":"1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_EVENT_PROBE",mode>=5?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_CVAR_PROBE",mode>=9?"1":"0"))
        return 95;
    HMODULE module=LoadLibraryA(dll);
    if(!module)return 96;
    auto factory=reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface"));
    if(!factory)return 97;
    auto* object=factory("ServerGameDLL003",nullptr);
    if(!object)return 98;
    auto** table=*reinterpret_cast<void***>(object);
    auto init=reinterpret_cast<bool (__thiscall *)(void*,Factory,Factory,Factory,void*)>(table[0]);
    const bool result=init(object,&Engine,&Physics,&FileSystem,&token);
    if(result || calls || eventCalls || step!=16 || mode==1 || mode==2 || mode>=5)return 99;
    FreeLibrary(module);
    return 0;
}
static bool Run(const char* dll,int selected) {
    char exe[MAX_PATH]={}, temp[MAX_PATH]={}, log[MAX_PATH]={};
    if(!GetModuleFileNameA(nullptr,exe,MAX_PATH) || !GetTempPathA(MAX_PATH,temp))
        return false;
    std::string cmd="\"";cmd+=exe;cmd+="\" \"";cmd+=dll;cmd+="\" ";
    cmd+=std::to_string(selected);
    STARTUPINFOA si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    if(!CreateProcessA(exe,&cmd[0],nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi))
        return false;
    CloseHandle(pi.hThread);
    DWORD result=0;
    if(WaitForSingleObject(pi.hProcess,10000)!=WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess,100);
        WaitForSingleObject(pi.hProcess,5000);
        CloseHandle(pi.hProcess);return false;
    }
    GetExitCodeProcess(pi.hProcess,&result);CloseHandle(pi.hProcess);
    std::snprintf(log,sizeof(log),"%ssin-independent-%s-%lu.log",temp,kSession,
                  static_cast<unsigned long>(pi.dwProcessId));
    FILE* file=nullptr;
    if(fopen_s(&file,log,"rb") || !file)return false;
    std::string data;char buf[512];
    while(auto n=std::fread(buf,1,sizeof(buf),file))data.append(buf,n);
    std::fclose(file);DeleteFileA(log);
    const auto has=[&](const char* text){return data.find(text)!=std::string::npos;};
    const bool cvarCase=selected>=9 && selected!=15;
    const bool event=selected==5 || selected==6 || selected==8 || selected==15;
    const DWORD expected=cvarCase?(selected==9?0x53494E49u:0x53494E4Au):
        event?0x53494E48u:selected==1?0x53494E46u:
        selected==2 || selected==7?0x53494E47u:0u;
    if(result!=expected || !has("phase=init-entered"))return false;
    if(cvarCase) {
        const auto eventReturn=data.find("phase=event-load-returned version=resource/gameevents.res:42");
        const char* outcome=selected==9?"verified":
            selected==10 || selected==11?"existing-name":
            selected==12?"lookup-mismatch":selected==13?"override-present":
            selected==14?"unexpected-relink":"invalid-list";
        const auto result=data.find(std::string("phase=cvar-probe-result version=")+outcome);
        const auto terminal=data.find("phase=cvar-probe-terminal-stop version=dd_log_events");
        if(eventReturn==std::string::npos || result==std::string::npos ||
           terminal==std::string::npos || !(eventReturn<result && result<terminal) ||
           has("phase=event-probe-terminal-stop") || has("phase=sdk2004-preflight-only"))
            return false;
        const bool registration=selected==9 || selected==12 || selected==13 || selected==14;
        const auto entered=data.find("phase=cvar-register-entered version=dd_log_events");
        const auto returned=data.find("phase=cvar-register-returned version=dd_log_events");
        const auto verified=data.find("phase=cvar-registration-verified version=dd_log_events");
        return (registration?(eventReturn<entered && entered<returned && returned<result):
                    entered==std::string::npos && returned==std::string::npos) &&
               ((selected==9)==(verified!=std::string::npos));
    }
    if(event) {
        const auto connected=data.find("phase=sound-connect-succeeded");
        const auto math=data.find("phase=math-tables-initialized version=server-scalar");
        const auto cached=data.find("phase=factory-state-cached version=server");
        const auto entered=data.find("phase=event-load-entered version=resource/gameevents.res");
        const auto returned=data.find(selected==5?"phase=event-load-returned version=resource/gameevents.res:42":
            selected==6 || selected==15?"phase=event-load-returned version=resource/gameevents.res:0":
            "phase=event-load-returned version=resource/gameevents.res:-1");
        const auto stop=data.find("phase=event-probe-terminal-stop");
        return connected!=std::string::npos && math!=std::string::npos && cached!=std::string::npos &&
            entered!=std::string::npos && returned!=std::string::npos && stop!=std::string::npos &&
            connected<math && math<cached && cached<entered && entered<returned && returned<stop &&
            !has("phase=sound-probe-terminal-stop") && !has("phase=sdk2004-preflight-only") &&
            !has("phase=cvar-register-entered");
    }
    if(selected==1 || selected==2 || selected==7) {
        const auto found=data.find("phase=found-interface version=ISERVERPLUGINHELPERS001");
        const auto entered=data.find("phase=sound-connect-entered version=VSoundEmitter001");
        const auto returned=data.find(selected==1?"phase=sound-connect-succeeded":"phase=sound-connect-rejected");
        const auto stopped=data.find("phase=sound-probe-terminal-stop");
        return found!=std::string::npos && entered!=std::string::npos &&
               returned!=std::string::npos && stopped!=std::string::npos &&
               found<entered && entered<returned && returned<stopped &&
               !has("phase=sdk2004-preflight-only") && !has("phase=event-load-entered");
    }
    return !has("phase=sound-connect-entered") && !has("phase=sound-probe-terminal-stop") &&
           (selected==3?has("phase=missing-interface version=ISERVERPLUGINHELPERS001"):
                        has("phase=sdk2004-preflight-only"));
}
int main(int argc,char** argv) {
    if(argc==3)return Child(argv[1],std::atoi(argv[2]));
    if(argc!=2)return 1;
    for(int i=0;i<17;++i)if(!Run(argv[1],i)) {
        std::fprintf(stderr,"FAIL actual server sound probe child case %d\n",i);return 2;
    }
    std::puts("PASS distributed DLL: sound/event stages and single native console registration with conflict, override, relink, lookup and cyclic-list refusals");
    return 0;
}
