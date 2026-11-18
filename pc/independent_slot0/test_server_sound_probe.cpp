// Test the distributed server DLL's actual opt-in branch against owned
// synthetic engine interfaces. Each case runs in its own child process.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
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
static int step=0,mode=0,token=0,calls=0;
static void* __cdecl Engine(const char*,int*);
class Sound final {
public:
    virtual bool __thiscall Connect(Factory f) noexcept {
        if(f!=&Engine || step!=16 || ++calls!=1 ||
           (mode!=1 && mode!=2)) ExitProcess(91);
        return mode==1;
    }
};
static Sound sound;
static void* __cdecl Engine(const char* name,int* status) {
    if(step>=16 || step==10 || !name || status ||
       std::strcmp(name,kSequence[step])) ExitProcess(92);
    ++step;
    if(mode==3 && step==16)return nullptr;
    return std::strcmp(name,"VSoundEmitter001")==0 ? static_cast<void*>(&sound) : &token;
}
static void* __cdecl FileSystem(const char* name,int* status) {
    if(step!=10 || !name || status || std::strcmp(name,kSequence[step]))
        ExitProcess(93);
    ++step;return &token;
}
static void* __cdecl Physics(const char*,int*) { ExitProcess(94); }
static int Child(const char* dll,int selected) {
    mode=selected;
    if(!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE","1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION",kSession) ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_FACTORY_PROBE",mode==4?"0":"1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_SERVER_SOUND_PROBE",mode==0?"0":"1"))
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
    if(result || calls || step!=16 || mode==1 || mode==2)return 99;
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
    const DWORD expected=selected==1?0x53494E46u:selected==2?0x53494E47u:0u;
    if(result!=expected || !has("phase=init-entered"))return false;
    if(selected==1 || selected==2) {
        const auto found=data.find("phase=found-interface version=ISERVERPLUGINHELPERS001");
        const auto entered=data.find("phase=sound-connect-entered version=VSoundEmitter001");
        const auto returned=data.find(selected==1?"phase=sound-connect-succeeded":"phase=sound-connect-rejected");
        const auto stopped=data.find("phase=sound-probe-terminal-stop");
        return found!=std::string::npos && entered!=std::string::npos &&
               returned!=std::string::npos && stopped!=std::string::npos &&
               found<entered && entered<returned && returned<stopped &&
               !has("phase=sdk2004-preflight-only");
    }
    return !has("phase=sound-connect-entered") && !has("phase=sound-probe-terminal-stop") &&
           (selected==3?has("phase=missing-interface version=ISERVERPLUGINHELPERS001"):
                        has("phase=sdk2004-preflight-only"));
}
int main(int argc,char** argv) {
    if(argc==3)return Child(argv[1],std::atoi(argv[2]));
    if(argc!=2)return 1;
    for(int i=0;i<5;++i)if(!Run(argv[1],i)) {
        std::fprintf(stderr,"FAIL actual server sound probe child case %d\n",i);return 2;
    }
    std::puts("PASS distributed DLL: default off, real typed Connect success/rejection, missing dependency, factory gate");
    return 0;
}
