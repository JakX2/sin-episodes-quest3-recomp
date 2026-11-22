/* Phase620: native PE32 engine factory proxy <-> native-host C DLL,
 * against only synthetic originals. Every mode preserves the identical
 * ServerGameDLL003 CreateInterface pointer/status from the original.
 * The proxy MUST NOT create an NPC or call native-host attach/step.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef void *(__cdecl *FactoryFn)(const char *,int *);
typedef unsigned int (__cdecl *VersionFn)(void);
static const char *const required[]={
    "sin_grunt_native_abi_version_v1",
    "sin_grunt_native_create_v1",
    "sin_grunt_native_destroy_v1",
    "sin_grunt_native_attach_v1",
    "sin_grunt_native_step_v1",
    "sin_grunt_native_snapshot_v1",
    "sin_grunt_native_detach_v1"
};
static int fail(const char* msg,int code) {
    fprintf(stderr,"Phase620: %s, error=%lu\n",msg,(unsigned long)GetLastError());
    return code;
}
static int path_join(wchar_t *out,size_t cap,const wchar_t *dir,
                     const wchar_t *leaf) {
    const int n=swprintf(out,cap,L"%ls\\%ls",dir,leaf);
    return n>0 && (size_t)n<cap;
}
static int copy_to(const wchar_t *dir,const wchar_t *original,
                   const wchar_t *dest) {
    wchar_t target[MAX_PATH],source[MAX_PATH];
    if(!path_join(target,MAX_PATH,dir,dest))return 0;
    if(swprintf(source,MAX_PATH,L".\\%ls",original)<=0)return 0;
    return CopyFileW(source,target,FALSE)!=0;
}
static int trace_contains(const char *text) {
    wchar_t root[MAX_PATH],filename[MAX_PATH];
    DWORD n=GetTempPathW(MAX_PATH,root),read=0;
    HANDLE fd;
    char buffer[8192];
    if(n==0 || n>=MAX_PATH)return 0;
    if(swprintf(filename,MAX_PATH,L"%lssin-pc-proxy-%lu.log",root,
                (unsigned long)GetCurrentProcessId())<=0)return 0;
    fd=CreateFileW(filename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                   NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(fd==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(fd,buffer,(DWORD)sizeof(buffer)-1U,&read,NULL)) {
        CloseHandle(fd);return 0;
    }
    CloseHandle(fd);buffer[read]='\0';
    return strstr(buffer,text)!=NULL;
}
static int clear_trace(void) {
    wchar_t root[MAX_PATH],filename[MAX_PATH];
    DWORD n=GetTempPathW(MAX_PATH,root);
    if(n==0 || n>=MAX_PATH)return 0;
    if(swprintf(filename,MAX_PATH,L"%lssin-pc-proxy-%lu.log",root,
                (unsigned long)GetCurrentProcessId())<=0)return 0;
    if(DeleteFileW(filename))return 1;
    return GetLastError()==ERROR_FILE_NOT_FOUND;
}
int main(int argc,char **argv) {
    int disabled,valid,missing,wrong,bad_exports;
    wchar_t root[MAX_PATH],directory[MAX_PATH],proxy_path[MAX_PATH];
    DWORD n;
    HMODULE proxy,original,host,behavior;
    FactoryFn forward,direct;
    void *a,*b;
    int forward_code=66,direct_code=66,not_found=66;
    size_t i;
    disabled=argc==2 && strcmp(argv[1],"disabled")==0;
    valid=argc==2 && strcmp(argv[1],"valid")==0;
    missing=argc==2 && strcmp(argv[1],"missing")==0;
    wrong=argc==2 && strcmp(argv[1],"wrong-version")==0;
    bad_exports=argc==2 && strcmp(argv[1],"missing-export")==0;
    if(!disabled && !valid && !missing && !wrong && !bad_exports)
        return fail("expected disabled|valid|missing|wrong-version|missing-export",1);
    if(!SetEnvironmentVariableA("SIN_PC_GRUNT_BRIDGE_PROBE","0") ||
       !SetEnvironmentVariableA("SIN_PC_GRUNT_NATIVE_HOST_PROBE",
                                disabled?"0":"1") ||
       !SetEnvironmentVariableA("SIN_PC_PROXY_TRACE_FILE","1") ||
       !clear_trace())return fail("failed to isolate probe env/trace",2);
    n=GetTempPathW(MAX_PATH,root);
    if(n==0 || n>=MAX_PATH)return fail("GetTempPathW",3);
    if(swprintf(directory,MAX_PATH,L"%lssin-native-phase620-%lu",
                root,(unsigned long)GetCurrentProcessId())<=0)
        return fail("temp directory",4);
    if(!CreateDirectoryW(directory,NULL))
        return fail("isolated temp directory creation",5);
    if(!path_join(proxy_path,MAX_PATH,directory,L"server.dll") ||
       !copy_to(directory,L"server.dll",L"server.dll") ||
       !copy_to(directory,L"sin_original_server.dll",
                L"sin_original_server.dll"))
        return fail("stage synthetic proxy/original",6);
    if(disabled || valid) {
        if(!copy_to(directory,L"sin_grunt_native_host_c_abi.dll",
                    L"sin_grunt_native_host_c_abi.dll") ||
           !copy_to(directory,L"sin_grunt_behavior_c_abi.dll",
                    L"sin_grunt_behavior_c_abi.dll"))
            return fail("stage real independent original-free host DLLs",7);
    } else if(wrong) {
        if(!copy_to(directory,L"sin_grunt_native_bad_version_phase620.dll",
                    L"sin_grunt_native_host_c_abi.dll"))
            return fail("stage wrong native host ABI fixture",8);
    } else if(bad_exports) {
        if(!copy_to(directory,L"sin_grunt_native_missing_export_phase620.dll",
                    L"sin_grunt_native_host_c_abi.dll"))
            return fail("stage incomplete-export native host fixture",9);
    }
    proxy=LoadLibraryW(proxy_path);
    if(!proxy)return fail("LoadLibrary proxy",10);
    forward=(FactoryFn)GetProcAddress(proxy,"CreateInterface");
    if(!forward)return fail("proxy lacks original export",11);
    a=forward("ServerGameDLL003",&forward_code);
    original=GetModuleHandleW(L"sin_original_server.dll");
    if(!original)return fail("synthetic original not loaded",12);
    direct=(FactoryFn)GetProcAddress(original,"CreateInterface");
    if(!direct)return fail("synthetic original factory missing",13);
    b=direct("ServerGameDLL003",&direct_code);
    if(!a || a!=b || forward_code!=direct_code || direct_code!=0)
        return fail("factory pointer/status identity changed",14);
    if(forward("ServerGameDLL010",&not_found)!=NULL || not_found!=1)
        return fail("unknown factory interface no longer rejected",15);

    host=GetModuleHandleW(L"sin_grunt_native_host_c_abi.dll");
    behavior=GetModuleHandleW(L"sin_grunt_behavior_c_abi.dll");
    if(valid) {
        FARPROC f;
        VersionFn version;
        if(!host || !behavior)return fail("native host/behavior DLL missing",16);
        for(i=0;i<sizeof(required)/sizeof(required[0]);++i) {
            if(!GetProcAddress(host,required[i]))
                return fail("native host missing one of seven symbols",17);
        }
        f=GetProcAddress(host,required[0]);
        version=(VersionFn)f;
        if(version()!=1u ||
           !trace_contains("phase=grunt-native-host-ready") ||
           trace_contains("phase=grunt-bridge-ready"))
            return fail("host acceptance logged incorrectly",18);
    } else if(disabled) {
        if(host || behavior ||
           trace_contains("phase=grunt-native-host-"))
            return fail("disabled native host probe loaded library",19);
    } else if(missing) {
        if(host || !trace_contains("phase=grunt-native-host-load-failed"))
            return fail("missing host not rejected safely",20);
    } else if(wrong) {
        if(host || !trace_contains("phase=grunt-native-host-version-rejected"))
            return fail("wrong host ABI not rejected safely",21);
    } else {
        if(host || !trace_contains("phase=grunt-native-host-export-missing"))
            return fail("incomplete host DLL exports not rejected safely",22);
    }
    (void)FreeLibrary(proxy);
    printf("PASS: Phase620 original factory identity retained, native host probe=%s\n",
           argv[1]);
    return 0;
}
