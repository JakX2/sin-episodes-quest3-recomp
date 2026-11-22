/* Phase615: true Win32 x86 proxy -> Grunt bridge load test.
 * Runs against the test-only synthetic original server DLL, not the game.
 * Modes: disabled, valid, missing, wrong-version. Every case must preserve
 * the exact original CreateInterface pointer/status even on probe failure.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef void *(__cdecl *SinFactoryFn)(const char *,int *);
typedef unsigned int (__cdecl *SinGruntVersionFn)(void);

static const char *const sin_grunt_names[]={
    "sin_grunt_behavior_abi_version_v1",
    "sin_grunt_behavior_create_v1",
    "sin_grunt_behavior_destroy_v1",
    "sin_grunt_behavior_step_v1",
    "sin_grunt_behavior_snapshot_v1"
};
static int fail(const char *message,int code) {
    fprintf(stderr,"Phase615 proxy bridge: %s; Win32 error=%lu\n",
            message,(unsigned long)GetLastError());
    return code;
}
static int sibling_path(wchar_t* output,size_t size,
                        const wchar_t *folder,const wchar_t *name) {
    int n=swprintf(output,size,L"%ls\\%ls",folder,name);
    return n>0 && (size_t)n<size;
}
static int temp_path(wchar_t *output,size_t size) {
    wchar_t root[MAX_PATH];
    DWORD n=GetTempPathW((DWORD)(sizeof(root)/sizeof(root[0])),root);
    int written;
    if(n==0 || n>=sizeof(root)/sizeof(root[0]))return 0;
    written=swprintf(output,size,L"%lssin-grunt-phase615-%lu",
                      root,(unsigned long)GetCurrentProcessId());
    return written>0 && (size_t)written<size;
}
static int trace_contains(const char *phrase) {
    wchar_t root[MAX_PATH];
    wchar_t path[MAX_PATH];
    DWORD n=GetTempPathW(MAX_PATH,root),received=0;
    HANDLE f;
    char buffer[8192];
    if(n==0 || n>=MAX_PATH)return 0;
    if(swprintf(path,MAX_PATH,L"%lssin-pc-proxy-%lu.log",
                root,(unsigned long)GetCurrentProcessId())<=0)return 0;
    f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                  NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return 0;
    if(!ReadFile(f,buffer,(DWORD)sizeof(buffer)-1U,&received,NULL)) {
        CloseHandle(f);return 0;
    }
    CloseHandle(f);
    buffer[received]='\0';
    return strstr(buffer,phrase)!=NULL;
}
static int clear_trace(void) {
    wchar_t root[MAX_PATH],path[MAX_PATH];
    DWORD n=GetTempPathW(MAX_PATH,root);
    if(n==0 || n>=MAX_PATH)return 0;
    if(swprintf(path,MAX_PATH,L"%lssin-pc-proxy-%lu.log",
                root,(unsigned long)GetCurrentProcessId())<=0)return 0;
    if(DeleteFileW(path))return 1;
    return GetLastError()==ERROR_FILE_NOT_FOUND;
}

int main(int argc,char **argv) {
    const int disabled=argc==2 && strcmp(argv[1],"disabled")==0;
    const int valid=argc==2 && strcmp(argv[1],"valid")==0;
    const int missing=argc==2 && strcmp(argv[1],"missing")==0;
    const int wrong=argc==2 && strcmp(argv[1],"wrong-version")==0;
    wchar_t directory[MAX_PATH],proxy_path[MAX_PATH];
    wchar_t copied_original[MAX_PATH],copied_grunt[MAX_PATH];
    HMODULE proxy,original,grunt;
    SinFactoryFn forwarded,direct;
    void *actual,*expected;
    int return_code=99,original_code=99;
    size_t i;

    if(!disabled && !valid && !missing && !wrong)
        return fail("expected disabled|valid|missing|wrong-version",1);
    if(!SetEnvironmentVariableA("SIN_PC_GRUNT_BRIDGE_PROBE",
                                disabled?"0":"1") ||
       !SetEnvironmentVariableA("SIN_PC_PROXY_TRACE_FILE","1"))
        return fail("cannot set opt-in environment",2);
    if(!clear_trace())return fail("cannot remove stale PID trace",3);

    if(missing || wrong) {
        /* Dedicated temporary directory ensures there can be no accidental
         * sibling bridge. We copy the fake original as the only factory DLL.
         */
        if(!temp_path(directory,MAX_PATH))return fail("temp path",4);
        if(!CreateDirectoryW(directory,NULL) &&
           GetLastError()!=ERROR_ALREADY_EXISTS)
            return fail("temp create",5);
        if(!sibling_path(proxy_path,MAX_PATH,directory,L"server.dll")||
           !sibling_path(copied_original,MAX_PATH,directory,
                         L"sin_original_server.dll")||
           !sibling_path(copied_grunt,MAX_PATH,directory,
                         L"sin_grunt_behavior_c_abi.dll"))
            return fail("temp sibling pathname",6);
        if(!CopyFileW(L".\\server.dll",proxy_path,FALSE) ||
           !CopyFileW(L".\\sin_original_server.dll",copied_original,FALSE))
            return fail("copy synthetic original pair",7);
        if(wrong &&
           !CopyFileW(L".\\sin_grunt_bad_version_phase615.dll",
                      copied_grunt,FALSE))
            return fail("copy intentionally incompatible bridge",8);
    } else {
        /* CTest working directory is the Win32 executable directory. */
        wcscpy_s(proxy_path,MAX_PATH,L".\\server.dll");
    }

    proxy=LoadLibraryW(proxy_path);
    if(proxy==NULL)return fail("proxy LoadLibrary",9);
    forwarded=(SinFactoryFn)GetProcAddress(proxy,"CreateInterface");
    if(forwarded==NULL)return fail("proxy CreateInterface export",10);
    actual=forwarded("ServerGameDLL003",&return_code);
    if(actual==NULL || return_code!=0)
        return fail("original engine factory forwarding changed",11);

    original=GetModuleHandleW(L"sin_original_server.dll");
    if(original==NULL)return fail("original synthetic server missing",12);
    direct=(SinFactoryFn)GetProcAddress(original,"CreateInterface");
    if(direct==NULL)return fail("original synthetic factory missing",13);
    expected=direct("ServerGameDLL003",&original_code);
    if(actual!=expected || return_code!=original_code)
        return fail("original factory pointer or return code changed",14);
    return_code=99;
    if(forwarded("ServerGameDLL010",&return_code)!=NULL ||
       return_code!=1)
        return fail("invalid original interface unexpectedly resolved",15);

    grunt=GetModuleHandleW(L"sin_grunt_behavior_c_abi.dll");
    if(valid) {
        FARPROC entry;
        SinGruntVersionFn version;
        if(grunt==NULL)return fail("opt-in library was not loaded",16);
        for(i=0;i<sizeof(sin_grunt_names)/sizeof(sin_grunt_names[0]);++i)
            if(GetProcAddress(grunt,sin_grunt_names[i])==NULL)
                return fail("one of five Grunt ABI exports missing",17);
        entry=GetProcAddress(grunt,sin_grunt_names[0]);
        version=(SinGruntVersionFn)entry;
        if(version()!=1U || !trace_contains("phase=grunt-bridge-ready"))
            return fail("bridge ABI/version or ready trace incorrect",18);
    } else if(disabled) {
        if(grunt!=NULL || trace_contains("phase=grunt-bridge-"))
            return fail("disabled bridge probe did work",19);
    } else if(missing) {
        if(grunt!=NULL || !trace_contains("phase=grunt-bridge-load-failed"))
            return fail("missing bridge did not fail open",20);
    } else {
        if(grunt!=NULL || !trace_contains("phase=grunt-bridge-version-rejected"))
            return fail("wrong ABI bridge was not rejected",21);
    }

    (void)FreeLibrary(proxy);
    if(missing || wrong) {
        (void)DeleteFileW(proxy_path);
        (void)DeleteFileW(copied_original);
        if(wrong)(void)DeleteFileW(copied_grunt);
        (void)RemoveDirectoryW(directory);
    }
    printf("PASS: original Win32 factory unchanged; optional Grunt bridge: %s\n",
           argv[1]);
    return 0;
}
