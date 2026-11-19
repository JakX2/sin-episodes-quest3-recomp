// Native child probe of the distributed client DLL, using owned fake
// engine interfaces only. No commercial image is loaded in CI.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "client_cvar_probe.h"
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
static unsigned progress=0,calls=0,hardwareCalls=0,screenCalls=0,soundSystemInitCalls=0;static int mode=0,token=0;
static const char* fixturePath=nullptr;
static const char* vguiFixturePath=nullptr;
static const char* missingFactoryFixturePath=nullptr;
using VerifyText=bool (__cdecl *)();
static VerifyText verifyText=nullptr;
static void* __cdecl App(const char*,int*);
static unsigned matSurfaceQueries=0;
class Sound {
public:
    virtual bool __thiscall Connect(Factory factory) noexcept {
        if(factory!=&App || progress!=21 || ++calls!=1 || (mode!=1 && mode!=2 && (mode<5 || mode>50)))
            ExitProcess(91);
        return mode!=2 && mode!=9 && mode!=13 && mode!=17;
    }
    virtual bool __thiscall Init() noexcept {
        if(mode<48 || mode>49 || ++soundSystemInitCalls!=1 ||
           progress!=22 || calls!=1 || matSurfaceQueries!=1)ExitProcess(155);
        return mode==48;
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
        return mode==7 || mode==12 || mode==16?nullptr:&token;
    }
};
static Materials materials;
class ScreenEngine {
public:
    virtual void __thiscall Guard0() noexcept {ExitProcess(120);}
    virtual void __thiscall Guard1() noexcept {ExitProcess(120);}
    virtual void __thiscall Guard2() noexcept {ExitProcess(120);}
    virtual void __thiscall Guard3() noexcept {ExitProcess(120);}
    virtual void __thiscall Guard4() noexcept {ExitProcess(120);}
    virtual void __thiscall GetScreenSize(int& w,int& h) noexcept {
        if(progress!=22 || calls!=1 || hardwareCalls!=1 || ++screenCalls!=1 ||
           &w==&h || w || h || (mode!=10 && mode!=11 && mode!=14 && mode!=15 && (mode<18 || mode>50)))ExitProcess(121);
        w=mode==11?0:1920;h=mode==11?0:1080;
    }
};
static ScreenEngine screenEngine;
static unsigned engineVGuiCalls=0;
class EngineVGuiFixture {
public:
    virtual std::uintptr_t __thiscall Guard0() noexcept {ExitProcess(145);return 0;}
    virtual std::uintptr_t __thiscall Slot1(int value) noexcept {
        if(value!=2 || ++engineVGuiCalls!=1 || mode<38 || mode>50)ExitProcess(146);
        return mode==39?0:0x1234u;
    }
};
static EngineVGuiFixture engineVGuiFixture;
using namespace sin_independent::console_abi;
static void __cdecl Noop(){}
class ClientCvarFixture final:public EngineCvar {
public:
    Variable existing;
    Command conflicting;
    CommandBase* head=nullptr;
    Variable* objects[kClientSpecCount]{};
    unsigned calls=0,lists=0;
    mutable unsigned finds=0;
    void Prepare(){
        existing.CreateVariable(kClientSpecs[2].name,"99",0,nullptr,false,0,false,0,nullptr);
        conflicting.CreateCommand(kClientSpecs[2].name,&Noop,nullptr,0,nullptr);
    }
    static unsigned Index(const char* n) {
        for(unsigned i=0;i<kClientSpecCount;++i)if(!std::strcmp(n,kClientSpecs[i].name))return i;
        ExitProcess(123);
    }
    void __thiscall RegisterConCommandBase(CommandBase* object) noexcept override {
        if(mode!=18 && mode!=20 && mode!=21 && mode!=24 && (mode<27 || mode>31) && mode!=33 && (mode<34 || mode>50))ExitProcess(124);
        if(screenCalls!=1 || calls>=kClientSpecCount || lists!=1 ||
           !object || object->IsCommand() || object->registered ||
           object->next || object->flags!=8)ExitProcess(125);
        auto* v=static_cast<Variable*>(object);
        const auto& spec=kClientSpecs[calls];
        if(!v->GetName() || std::strcmp(v->GetName(),spec.name) ||
           std::strcmp(v->String(),spec.value) || v->callback ||
           v->hasMin || v->hasMax || v->parent!=v)ExitProcess(126);
        objects[calls++]=v;v->next=head;head=v;
        if(mode==20 && calls==2)v->parent=&existing;
        if(mode==21 && calls==kClientSpecCount)objects[0]->integer++;
    }
    const char* __thiscall GetCommandLineValue(const char*) noexcept override {ExitProcess(127);}
    Variable* __thiscall FindMutableVar(const char*) noexcept override {ExitProcess(128);}
    const Variable* __thiscall FindConstVar(const char* name) const noexcept override {
        if(screenCalls!=1 || mode==22)ExitProcess(129);
        const auto i=Index(name);++finds;
        if((mode==19 || mode==25 || mode==32) && i==2)return &existing;
        if(objects[i] && !objects[i]->registered)ExitProcess(130);
        return objects[i];
    }
    CommandBase* __thiscall GetCommands() noexcept override {
        if(++lists!=1 || finds!=kClientSpecCount || calls || mode==19 || mode==22 || mode==25 || mode==26 || mode==32)
            ExitProcess(131);
        return mode==23?&conflicting:head;
    }
};
static ClientCvarFixture clientCvar;
static unsigned guiFactoryQueries=0,guiInitCalls=0;
static const char* guiNames[]={"VGUI_ivgui006","VGUI_Panel009",
    "VGUI_Surface028","VGUI_Scheme010","VGUI_System010",
    "VGUI_Input005","VGUI_Localize003","VFileSystem012"};
class VGuiControlsFixture {
public:
    virtual ~VGuiControlsFixture()=default;
    virtual bool __thiscall Init(Factory* list,int count) noexcept {
        if(++guiInitCalls!=1 || count!=1 || !list || list[0]!=&App)
            ExitProcess(147);
        return mode!=43;
    }
};
static VGuiControlsFixture guiControlsFixture;
struct SurfaceBaseFixture {
    virtual int __thiscall Guard() noexcept {ExitProcess(149);return 0;}
};
struct AppSystemSurfaceFixture {
    virtual bool __thiscall Connect(Factory) noexcept {ExitProcess(150);return false;}
    virtual void __thiscall Disconnect() noexcept {ExitProcess(151);}
    virtual void* __thiscall QueryInterface(const char* version) noexcept {
        if(++matSurfaceQueries!=1 || !version ||
           std::strcmp(version,"MatSystemSurface003"))ExitProcess(152);
        if(mode==50)*reinterpret_cast<void**>(&sound)=nullptr;
        return mode==46?nullptr:static_cast<void*>(&token);
    }
    virtual int __thiscall Init() noexcept {ExitProcess(153);return 0;}
    virtual void __thiscall Shutdown() noexcept {ExitProcess(154);}
};
struct SurfaceFixture final:SurfaceBaseFixture,AppSystemSurfaceFixture {};
static SurfaceFixture surfaceFixture;
static struct {void* primary;void* secondary;} malformedSurface{nullptr,nullptr};
static void* __cdecl App(const char* name,int* status) {
    if(!name || status)ExitProcess(92);
    if(progress==22 && mode>=41 && mode<=50) {
        if(guiFactoryQueries>=8 || std::strcmp(name,guiNames[guiFactoryQueries]))
            ExitProcess(148);
        const unsigned index=guiFactoryQueries++;
        if(mode==42 && index==3)return nullptr;
        if(index==0)return mode==44?static_cast<void*>(&token):
            static_cast<void*>(&guiControlsFixture);
        if(index==2 && mode>=45 && mode<=50)
            return mode==47?static_cast<void*>(&malformedSurface):static_cast<void*>(&surfaceFixture);
        return &token;
    }
    if(progress==21 && ((mode>=5 && mode<=7) || ((mode>=10 && mode<=16 && mode!=13) || (mode>=18 && mode<=50)))) {
        if(std::strcmp(name,"VMaterialSystemStub001") || calls!=1 ||
           !verifyText || !verifyText())ExitProcess(112);
        ++progress;
        return mode==5?nullptr:&token;
    }
    if(progress>=21 || std::strcmp(name,names[progress]))ExitProcess(92);
    ++progress;
    if(mode==3 && progress==21)return nullptr;
    if(progress==1)return &screenEngine;
    if(progress==4)return &clientCvar;
    if(progress==8)return &materials;
    if(progress==11 && mode>=38 && mode<=50)
        return mode==40?static_cast<void*>(&token):static_cast<void*>(&engineVGuiFixture);
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
    clientCvar.Prepare();
    if((mode>=5 && mode<=7) || ((mode>=10 && mode<=16 && mode!=13) || (mode>=18 && mode<=50))) {
        auto module=LoadLibraryA(fixturePath);
        if(!module)return 113;
        auto set=reinterpret_cast<void (__cdecl *)(int)>(GetProcAddress(module,"SetTextFixture"));
        verifyText=reinterpret_cast<VerifyText>(GetProcAddress(module,"VerifyTextFixture"));
        if(!set || !verifyText)return 114;
        set(mode==6?1:0);
        auto gaussian=reinterpret_cast<void (__cdecl *)(void*,int)>(GetProcAddress(module,"SetGaussianFixture"));
        if(!gaussian)return 122;
        gaussian(&token,mode==15 || mode==22 || mode==26?1:0);
    }
    if(mode==33 && !LoadLibraryA(missingFactoryFixturePath))return 144;
    if((mode>=27 && mode<=31 && mode!=28) || (mode>=34 && mode<=50)) {
        if(!LoadLibraryA(vguiFixturePath) ||
           !SetEnvironmentVariableA("SIN_TEST_VGUI_FIXTURE_MODE",std::to_string(mode>=38?34:mode).c_str()))return 143;
    }
    if(!SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_FILE","1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_TRACE_SESSION",mode==4?"invalid":kSession) ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SOUND_PROBE",mode==0?"0":"1") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_MATERIAL_PROBE",mode>=5 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SCREEN_PROBE",mode>=10 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_GAUSSIAN_PROBE",mode>=14 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_CVAR_PROBE",mode>=18 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_OWNED_STARTUP_PROBE",mode>=24 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_VGUI_PROBE",(mode>=27 && mode<=31) || mode==33 || (mode>=34 && mode<=50)?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_VGUI_LAYOUT_PROBE",mode>=34 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_ENGINE_VGUI_SLOT1_PROBE",mode>=38 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_VGUI_CONTROLS_BOOTSTRAP_PROBE",mode>=41 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_MAT_SURFACE_PROBE",mode>=45 && mode<=50?"1":"0") ||
       !SetEnvironmentVariableA("SIN_INDEPENDENT_CLIENT_SOUND_SYSTEM_INIT_PROBE",mode>=48 && mode<=50?"1":"0"))
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
    command+=" \"";command+=vguiFixturePath;command+="\"";
    command+=" \"";command+=missingFactoryFixturePath;command+="\"";
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
        selected==48?0x53494E63u:
        selected==49 || selected==50?0x53494E64u:
        selected==45?0x53494E61u:
        selected==46 || selected==47?0x53494E62u:
        selected==41 || selected==43?0x53494E5Fu:
        selected==42 || selected==44?0x53494E60u:
        selected==38?0x53494E5Du:
        selected==39 || selected==40?0x53494E5Eu:
        selected==34?0x53494E5Bu:
        selected>=35 && selected<=37?0x53494E5Cu:
        selected==27?0x53494E59u:
        (selected>=28 && selected<=31) || selected==33?0x53494E5Au:
        selected==24?0x53494E57u:
        selected==18?0x53494E55u:
        selected==19 || selected==20 || selected==21 || selected==23 || selected==25 || selected==32?0x53494E56u:
        selected==14?0x53494E53u:selected==15 || selected==22 || selected==26?0x53494E54u:
        selected==10 || selected==11?0x53494E51u:
        selected==5 || selected==6?0x53494E4Fu:
        selected==7 || selected==8 || selected==12 || selected==16?0x53494E50u:
        selected==1?0x53494E4Bu:selected==2 || selected==9 || selected==13 || selected==17?0x53494E4Cu:0u;
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
    if(selected>=48 && selected<=50) {
        const char* expected=selected==48?"called-true":
            selected==49?"called-false":"invalid-vtable";
        std::size_t after=0;
        const auto ordered=[&](const std::string& str){
            const auto at=text.find(str,after);
            if(at==std::string::npos)return false;
            after=at+str.size();return true;
        };
        return ordered("phase=vgui-controls-bootstrap-result version=init-true") &&
            ordered("phase=engine-vgui-slot1-result version=nonzero") &&
            ordered("phase=mat-surface-query-result version=found") &&
            ordered("phase=sound-system-init-entered version=CSoundEmitterSystem:VSoundEmitter001+1") &&
            ordered(std::string("phase=sound-system-init-result version=")+expected) &&
            ordered("phase=sound-system-init-terminal-stop version=original-slot1") &&
            !has("phase=mat-surface-query-terminal-stop");
    }
    if(selected>=45 && selected<=47) {
        const char* result=selected==45?"found":
            selected==46?"missing-interface":"invalid-vtable";
        std::size_t after=0;
        const auto ordered=[&](const std::string& str){
            const auto at=text.find(str,after);
            if(at==std::string::npos)return false;
            after=at+str.size();return true;
        };
        return ordered("phase=vgui-controls-bootstrap-result version=init-true") &&
            ordered("phase=engine-vgui-slot1-result version=nonzero") &&
            ordered("phase=engine-vgui-slot1-terminal-stop version=original-slot1") &&
            ordered("phase=mat-surface-query-entered version=VGUI_Surface028:IAppSystem+4") &&
            ordered(std::string("phase=mat-surface-query-result version=")+result) &&
            ordered("phase=mat-surface-query-terminal-stop version=original-abi") &&
            !has("phase=vgui-controls-bootstrap-terminal-stop");
    }
    if(selected>=41 && selected<=44) {
        std::size_t after=0;
        const auto ordered=[&](const std::string& str){
            const auto at=text.find(str,after);
            if(at==std::string::npos)return false;
            after=at+str.size();return true;
        };
        if(!ordered("phase=vgui-layout-probe-result version=verified") ||
           !ordered("phase=vgui-controls-bootstrap-entered version=CLIENT:factory-count-1"))
            return false;
        for(unsigned i=0;i<8;++i) {
            if(!ordered(std::string("phase=vgui-controls-request version=")+guiNames[i]))
                return false;
            if(!ordered(std::string("phase=vgui-controls-")+
                (selected==42 && i==3?"missing":"found")+" version="+guiNames[i]))
                return false;
        }
        const bool called=selected==41 || selected==43;
        if(called) {
            if(!ordered("phase=vgui-controls-init-entered version=VGUI_ivgui006:factory-count-1") ||
               !ordered(std::string("phase=vgui-controls-init-returned version=")+
                    (selected==43?"false":"true")))return false;
        } else if(has("phase=vgui-controls-init-entered")) return false;
        const char* status=selected==41?"init-true":selected==42?"missing-interface":
            selected==43?"init-false":"invalid-core-vtable";
        if(!ordered(std::string("phase=vgui-controls-bootstrap-result version=")+status))
            return false;
        if(called) {
            if(!ordered("phase=engine-vgui-slot1-entered version=VEngineVGui001:arg2") ||
               !ordered("phase=engine-vgui-slot1-result version=nonzero") ||
               !ordered("phase=engine-vgui-slot1-terminal-stop version=original-slot1"))
                return false;
        } else if(has("phase=engine-vgui-slot1-entered"))return false;
        return ordered("phase=vgui-controls-bootstrap-terminal-stop version=source2004") &&
               !has("phase=vgui-layout-terminal-stop");
    }
    if(selected>=38 && selected<=40) {
        const char* outcome=selected==38?"nonzero":selected==39?"zero":"invalid-vtable";
        std::size_t after=0;
        const auto ordered=[&](const std::string& str) {
            const auto at=text.find(str,after);
            if(at==std::string::npos)return false;
            after=at+str.size();return true;
        };
        return ordered("phase=vgui-layout-probe-result version=verified") &&
            ordered("phase=engine-vgui-slot1-entered version=VEngineVGui001:arg2") &&
            ordered(std::string("phase=engine-vgui-slot1-result version=")+outcome) &&
            ordered("phase=engine-vgui-slot1-terminal-stop version=original-slot1") &&
            !has("phase=vgui-layout-terminal-stop") && !has("phase=vgui-probe-terminal-stop");
    }
    if(selected>=34 && selected<=37) {
        const char* expected=selected==34?"verified":
            selected==36?"invalid-system":"invalid-input";
        const auto ordered=[&](std::size_t& after,const std::string& str){
            const auto at=text.find(str,after);
            if(at==std::string::npos)return false;
            after=at+str.length();return true;
        };
        std::size_t after=0;
        if(!ordered(after,"phase=vgui-probe-result version=acquired") ||
           !ordered(after,"phase=vgui-layout-entered version=input:0+56,system:0+24"))
            return false;
        if(selected==34 || selected==36)
            if(!ordered(after,"phase=vgui-layout-input-verified version=0+56"))return false;
        if(selected==34)
            if(!ordered(after,"phase=vgui-layout-system-verified version=0+24"))return false;
        if(!ordered(after,std::string("phase=vgui-layout-probe-result version=")+expected) ||
           !ordered(after,"phase=vgui-layout-terminal-stop version=read-only"))
            return false;
        return !has("phase=vgui-probe-terminal-stop") &&
            (selected==34 || !has("phase=vgui-layout-system-verified")) &&
            (selected==34 || selected==36 || !has("phase=vgui-layout-input-verified"));
    }
    if((selected>=27 && selected<=31) || selected==33) {
        const char* result=selected==27?"acquired":selected==28?"missing-module":
            selected==29?"missing-input":selected==30?"missing-system":
            selected==33?"missing-factory":"aliased-interfaces";
        const char* prefix[]={"phase=owned-startup-probe-result version=verified",
            "phase=vgui-probe-entered version=vgui2.dll"};
        std::size_t after=0;
        auto ordered=[&](const std::string& event){
            const auto at=text.find(event,after);
            if(at==std::string::npos)return false;
            after=at+event.size();return true;
        };
        for(auto event:prefix)if(!ordered(event))return false;
        if(selected!=28 && selected!=33) {
            if(!ordered("phase=vgui-provider-found version=vgui2.dll") ||
               !ordered("phase=vgui-query-entered version=VGUI_InputInternal001"))return false;
            if(selected!=29) {
                if(!ordered("phase=vgui-query-found version=VGUI_InputInternal001") ||
                   !ordered("phase=vgui-query-entered version=VGUI_System010"))return false;
                if(selected!=30 && !ordered("phase=vgui-query-found version=VGUI_System010"))return false;
            } else if(has("phase=vgui-query-entered version=VGUI_System010"))return false;
        } else if(has("phase=vgui-query-entered"))return false;
        return ordered(std::string("phase=vgui-probe-result version=")+result) &&
            ordered("phase=vgui-probe-terminal-stop version=factory-only") &&
            !has("phase=owned-startup-terminal-stop");
    }
    if(selected==24) {
        const char* ordered[]={
            "phase=client-cvar-probe-result version=verified:3/3",
            "phase=owned-startup-entered version=independent-only",
            "phase=owned-startup-objects-ready version=1",
            "phase=owned-startup-pool-ready version=2048x96",
            "phase=owned-startup-pool-verified version=allocate-reuse-release",
            "phase=owned-startup-cleanup-verified version=pool-objects",
            "phase=owned-startup-probe-result version=verified",
            "phase=owned-startup-terminal-stop version=independent-only"};
        std::size_t after=0;
        for(const auto* event:ordered){
            auto at=text.find(event,after);
            if(at==std::string::npos)return false;
            after=at+std::strlen(event);
        }
        return !has("phase=client-cvar-probe-terminal-stop");
    }
    if((selected>=18 && selected<=25 && selected!=22) || selected==32) {
        auto gaussian=text.find("phase=gaussian-probe-result version=attached");
        const char* outcome=selected==18?"verified:3/3":
            selected==19 || selected==23 || selected==25 || selected==32?"existing-name:0/3":
            selected==20?"unexpected-relink:1/3":"lookup-mismatch:3/3";
        auto result=text.find(std::string("phase=client-cvar-probe-result version=")+outcome);
        auto stop=text.find("phase=client-cvar-probe-terminal-stop version=3");
        if(gaussian==std::string::npos || result==std::string::npos ||
           stop==std::string::npos || !(gaussian<result && result<stop) ||
           has("phase=gaussian-probe-terminal-stop"))return false;
        const unsigned verified=selected==18 || selected==21?3:selected==20?1:0;
        auto last=gaussian;
        for(unsigned i=0;i<verified;++i) {
            const auto name=std::string(kClientSpecs[i].name)+"\n";
            const auto entered=text.find("phase=client-cvar-register-entered version="+name);
            const auto returned=text.find("phase=client-cvar-register-returned version="+name);
            const auto checked=text.find("phase=client-cvar-registration-verified version="+name);
            if(!(last<entered && entered<returned && returned<checked && checked<result))return false;
            last=checked;
        }
        return (verified || !has("phase=client-cvar-register-entered")) && !has("phase=owned-startup-entered") && !has("phase=vgui-probe-entered");
    }
    if(selected==14 || selected==15 || selected==22 || selected==26) {
        auto screen=text.find("phase=screen-probe-result version=returned");
        auto entry=text.find("phase=gaussian-attach-entered version=VEngineRandom001");
        auto verify=text.find("phase=gaussian-attach-verified version=VEngineRandom001");
        auto result=text.find(selected==14?"phase=gaussian-probe-result version=attached":
            "phase=gaussian-probe-result version=layout-or-attach-refused");
        auto stop=text.find("phase=gaussian-probe-terminal-stop version=VEngineRandom001");
        return screen!=std::string::npos && entry!=std::string::npos &&
            result!=std::string::npos && stop!=std::string::npos &&
            screen<entry && entry<result && result<stop &&
            (selected==14?entry<verify && verify<result:verify==std::string::npos) &&
            !has("phase=screen-probe-terminal-stop") && !has("phase=client-cvar-register-entered") && !has("phase=owned-startup-entered");
    }
    if(selected==10 || selected==11) {
        const auto acquired=text.find("phase=material-probe-result version=acquired");
        const auto query=text.find("phase=screen-query-entered version=VEngineClient012.GetScreenSize");
        const auto returned=text.find(selected==10?"phase=screen-query-returned version=1920:1080":
            "phase=screen-query-returned version=0:0");
        const auto result=text.find("phase=screen-probe-result version=returned");
        const auto stop=text.find("phase=screen-probe-terminal-stop version=VEngineClient012.GetScreenSize");
        return acquired!=std::string::npos && query!=std::string::npos &&
            returned!=std::string::npos && result!=std::string::npos && stop!=std::string::npos &&
            acquired<query && query<returned && returned<result && result<stop &&
            !has("phase=material-probe-terminal-stop") && !has("phase=gaussian-attach-entered");
    }
    if(selected>=5 && selected<=8 || selected==12 || selected==16) {
        const auto connected=text.find("phase=sound-connect-succeeded");
        const auto result=text.find(selected<=6?"phase=material-probe-result version=acquired":
            selected==7 || selected==12 || selected==16?"phase=material-probe-result version=missing-hardware":
            "phase=material-probe-result version=invalid-commandline");
        const auto stop=text.find("phase=material-probe-terminal-stop");
        if(connected==std::string::npos || result==std::string::npos || stop==std::string::npos ||
           !(connected<result && result<stop) || has("phase=sound-probe-terminal-stop"))return false;
        if(selected==8)return !has("phase=textmode-checked") && !has("phase=hardware-config-entered");
        const auto modeCheck=text.find(selected==6?"phase=textmode-checked version=present":"phase=textmode-checked version=absent");
        const auto optional=text.find(selected==5?"phase=optional-missing version=VMaterialSystemStub001":
            "phase=optional-found version=VMaterialSystemStub001");
        const auto entered=text.find("phase=hardware-config-entered version=MaterialSystemHardwareConfig012");
        const auto returned=text.find(selected==7 || selected==12 || selected==16?"phase=hardware-config-missing version=MaterialSystemHardwareConfig012":
            "phase=hardware-config-found version=MaterialSystemHardwareConfig012");
        return modeCheck!=std::string::npos && optional!=std::string::npos &&
            entered!=std::string::npos && returned!=std::string::npos &&
            connected<modeCheck && modeCheck<optional && optional<entered && entered<returned && returned<result && !has("phase=screen-query-entered");
    }
    if(selected==1 || selected==2 || selected==9 || selected==13 || selected==17) {
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
    if(argc==6){fixturePath=argv[3];vguiFixturePath=argv[4];missingFactoryFixturePath=argv[5];return Child(argv[1],std::atoi(argv[2]));}
    if(argc!=5)return 1;
    fixturePath=argv[2];vguiFixturePath=argv[3];missingFactoryFixturePath=argv[4];
    for(int i=0;i<51;++i)if(!Run(argv[1],i)){
        std::fprintf(stderr,"FAIL client sound case %d\n",i);return 2;
    }
    for(int i=102;i<=147;++i)if(!Run(argv[1],i)){
        std::fprintf(stderr,"FAIL client terminal guard %d\n",i-100);return 3;
    }
    std::puts("PASS client native 48-slot table, 46 terminal guards, sound/material/screen query cases");
    return 0;
}
