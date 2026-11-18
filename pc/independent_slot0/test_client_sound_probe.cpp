// Native child probe of the distributed client DLL, using owned fake
// engine interfaces only. No commercial image is loaded in CI.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
using Factory=void* (__cdecl *)(const char*,int*);
constexpr char kSession[]="33900000000000000000000000000001";
constexpr const char* names[]={
    "VEngineClient012","VEngineModel012","VEngineEffects001","VEngineCvar002",
    "EngineTraceClient003","VEngineRenderView011","VDebugOverlay003",
    "VMaterialSystem076","VEngineCache001","VModelInfoClient003",
    "VEngineVGui001","VEngineClientStringTable001","SpatialPartition001",
    "VEngineShadowMgr002","StaticPropMgrClient004","IEngineSoundClient003",
    "VFileSystem012","VEngineRandom001","VENGINE_GAMEUIFUNCS_VERSION004",
    "GAMEEVENTSMANAGER002","VSoundEmitter001"};
static unsigned progress=0,calls=0,hardwareCalls=0;static int mode=0,token=0;
static const char* fixturePath=nullptr;
using VerifyText=bool (__cdecl *)();
static VerifyText verifyText=nullptr;
static void* __cdecl App(const char*,int*);
class Sound {
public:
    virtual bool __thiscall Connect(Factory factory) noexcept {
        if(factory!=&App || progress!=21 || ++calls!=1 || (mode!=1 && mode!=2 && (mode<5 || mode>9)))
            ExitProcess(91);
        return mode!=2 && mode!=9;
    }
};
static Sound sound;
class Materials {
public:
    virtual void __thiscall Slot0() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot1() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot2() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot3() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot4() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot5() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot6() noexcept {ExitProcess(110);}
    virtual void __thiscall Slot7() noexcept {ExitProcess(110);}
    virtual void* __thiscall Hardware(const char* version,int* status) noexcept {
        if(progress!=22 || calls!=1 || ++hardwareCalls!=1 ||
           !version || std::strcmp(version,"MaterialSystemHardwareConfig012") ||
           status || !verifyText || !verifyText())ExitProcess(111);
        return mode==7?nullptr:&token;
    }
};
static Materials materials;
static void* __cdecl App(const char* name,int* status) {
    if(!name || status)ExitProcess(92);
    if(progress==21 && mode>=5 && mode<=7) {
        if(std::strcmp(name,"VMaterialSystemStub001") || calls!=1 ||
           !verifyText || !verifyText())ExitProcess(112);
        ++progress;
        return mode==5?nullptr:&token;
    }
    if(progress>=21 || std::strcmp(name,names[progress]))ExitProcess(92);
    ++progress;
    if(mode==3 && progress==21)return nullptr;
    if(progress==8)return &materials;
    return progress==21?static_cast<void*>(&sound):&token;
}
static void* __cdecl Physics(const char*,int*){ExitProcess(93);}
static bool Owns(HMODULE module,const void* ptr) {
    HMODULE owner=nullptr;
    return ptr && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,static_cast<LPCSTR>(ptr),&owner)
        && owner==module;
}
static int Child(const char* dll,int selected) {
    mode=selected;
    if(mode>=5 && mode<=7) {
        auto module=LoadLibraryA(fixturePath);
        if(!module)return 113;
        auto set=reinterpret_cast<void (__cdecl *)(int)>(GetProcAddress(module,"SetTextFixture"));
        verifyText=reinterpret_cast<VerifyText>(GetProcAddress(module,"VerifyTextFixture"));
        if(!set || !verifyText)return 114;
        set(mode==6?1:0);
    }
    if(!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE","1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION",mode==4?"invalid":kSession) ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SOUND_PROBE",mode==0?"0":"1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_MATERIAL_PROBE",mode>=5 && mode<=9?"1":"0"))
        return 94;
    HMODULE module=LoadLibraryA(dll);
    if(!module)return 95;
    auto factory=reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface"));
    if(!factory)return 96;
    auto* object=factory("VClient011",nullptr);
    if(!object)return 97;
    auto** table=*reinterpret_cast<void***>(object);
    if(!Owns(module,object)||!Owns(module,table))return 98;
    for(unsigned i=0;i<48;++i)if(!Owns(module,table[i]))return 99;
    if(mode>=102 && mode<=147) {
        reinterpret_cast<void (__thiscall *)(void*)>(table[mode-100])(object);
        return 100; // index-only terminal guard must never return
    }
    auto init=reinterpret_cast<int (__thiscall *)(void*,Factory,Factory,void*)>(table[0]);
    if(init(object,&App,&Physics,&token) || progress!=21 || calls ||
       mode==1 || mode==2)return 101;
    FreeLibrary(module);return 0;
}
static bool Run(const char* dll,int selected) {
    char exe[MAX_PATH]{},tmp[MAX_PATH]{},path[MAX_PATH]{};
    const DWORD exelen=GetModuleFileNameA(nullptr,exe,MAX_PATH);
    const DWORD tmplen=GetTempPathA(MAX_PATH,tmp);
    if(!exelen||exelen>=MAX_PATH||!tmplen||tmplen>=MAX_PATH)return false;
    std::string command="\"";command+=exe;command+="\" \"";command+=dll;
    command+="\" ";command+=std::to_string(selected);
    command+=" \"";command+=fixturePath;command+="\"";
    STARTUPINFOA si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    if(!CreateProcessA(exe,&command[0],nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi))
        return false;
    CloseHandle(pi.hThread);DWORD result=0;
    if(WaitForSingleObject(pi.hProcess,10000)!=WAIT_OBJECT_0) {
        TerminateProcess(pi.hProcess,102);WaitForSingleObject(pi.hProcess,5000);
        CloseHandle(pi.hProcess);return false;
    }
    GetExitCodeProcess(pi.hProcess,&result);CloseHandle(pi.hProcess);
    const DWORD expected=selected>=102?0x53494E45u:
        selected==5 || selected==6?0x53494E4Fu:
        selected==7 || selected==8?0x53494E50u:
        selected==1?0x53494E4Bu:selected==2 || selected==9?0x53494E4Cu:0u;
    if(result!=expected)return false;
    if(selected==4)return true; // invalid session deliberately makes no file
    const int len=std::snprintf(path,sizeof(path),"%ssin-independent-%s-%lu.log",
        tmp,kSession,static_cast<unsigned long>(pi.dwProcessId));
    if(len<=0||static_cast<size_t>(len)>=sizeof(path))return false;
    FILE* file=nullptr;if(fopen_s(&file,path,"rb")||!file)return false;
    std::string text;char buffer[512];
    while(auto n=std::fread(buffer,1,sizeof(buffer),file))text.append(buffer,n);
    std::fclose(file);DeleteFileA(path);
    const auto has=[&](const char* s){return text.find(s)!=std::string::npos;};
    if(selected>=102) {
        const auto marker="role=client phase=unsupported-callback version=VClient011.slot"+std::to_string(selected-100)+"\n";
        return text.find(marker)!=std::string::npos && !has("phase=init-entered");
    }
    if(!has("role=client phase=init-entered"))return false;
    if(selected>=5 && selected<=8) {
        const auto connected=text.find("phase=sound-connect-succeeded");
        const auto result=text.find(selected<=6?"phase=material-probe-result version=acquired":
            selected==7?"phase=material-probe-result version=missing-hardware":
            "phase=material-probe-result version=invalid-commandline");
        const auto stop=text.find("phase=material-probe-terminal-stop");
        if(connected==std::string::npos || result==std::string::npos || stop==std::string::npos ||
           !(connected<result && result<stop) || has("phase=sound-probe-terminal-stop"))return false;
        if(selected==8)return !has("phase=textmode-checked") && !has("phase=hardware-config-entered");
        const auto modeCheck=text.find(selected==6?"phase=textmode-checked version=present":"phase=textmode-checked version=absent");
        const auto optional=text.find(selected==5?"phase=optional-missing version=VMaterialSystemStub001":
            "phase=optional-found version=VMaterialSystemStub001");
        const auto entered=text.find("phase=hardware-config-entered version=MaterialSystemHardwareConfig012");
        const auto returned=text.find(selected==7?"phase=hardware-config-missing version=MaterialSystemHardwareConfig012":
            "phase=hardware-config-found version=MaterialSystemHardwareConfig012");
        return modeCheck!=std::string::npos && optional!=std::string::npos &&
            entered!=std::string::npos && returned!=std::string::npos &&
            connected<modeCheck && modeCheck<optional && optional<entered && entered<returned && returned<result;
    }
    if(selected==1 || selected==2 || selected==9) {
        const auto found=text.find("phase=found-interface version=VSoundEmitter001");
        const auto entered=text.find("phase=sound-connect-entered version=VSoundEmitter001");
        const auto returned=text.find(selected==1?"phase=sound-connect-succeeded":"phase=sound-connect-rejected");
        const auto stop=text.find("phase=sound-probe-terminal-stop");
        return found!=std::string::npos && entered!=std::string::npos &&
               returned!=std::string::npos && stop!=std::string::npos &&
               found<entered && entered<returned && returned<stop &&
               !has("phase=sdk2004-preflight-only") && !has("phase=hardware-config-entered");
    }
    return !has("phase=sound-connect-entered") &&
        (selected==3?has("phase=missing-interface version=VSoundEmitter001"):
                     has("phase=sdk2004-preflight-only"));
}
int main(int argc,char** argv) {
    if(argc==4){fixturePath=argv[3];return Child(argv[1],std::atoi(argv[2]));}
    if(argc!=3)return 1;
    fixturePath=argv[2];
    for(int i=0;i<10;++i)if(!Run(argv[1],i)){
        std::fprintf(stderr,"FAIL client sound case %d\n",i);return 2;
    }
    for(int i=102;i<=147;++i)if(!Run(argv[1],i)){
        std::fprintf(stderr,"FAIL client terminal guard %d\n",i-100);return 3;
    }
    std::puts("PASS client native 48-slot table, 46 terminal guards and five original-prefix sound cases");
    return 0;
}
