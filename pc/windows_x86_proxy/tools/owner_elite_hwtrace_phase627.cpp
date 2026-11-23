// Phase627: owner-only, read-only hardware execution trace of actual 2006
// SiN Episodes original native Elite factory / Spawn / animation-event calls.
// It launches a DISPOSABLE owner-provided game copy under Win32 DEBUG API.
// No code patches, Source vtable writes, memory injection, imported engine
// guesses, CreateInterface substitution or simulated gameplay callbacks.
// Only the SHA256-authenticated original 2006 server module may be armed.
//
// IMPORTANT: Hardware breakpoints consume DR0..DR3 of each game thread.
// Run OFFLINE, only on a disposable test copy, not a production install.
// This is observational evidence, NOT implementation of gameplay callbacks.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <map>
#include <string>
#include <vector>
#pragma comment(lib, "bcrypt.lib")

#if !defined(_M_IX86)
#error This owner-original hardware debugger must be built as 32-bit Win32
#endif

namespace {
constexpr DWORD kRetailImageBase=0x22000000u;
constexpr DWORD kOriginalServerSize=6049792u;
constexpr const char* kRetailHash=
  "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650";
struct Watch {
  const char* name;
  DWORD original_va;
  DWORD original_rva;
};
// The first four original instruction addresses are authenticated elsewhere:
// Phase530 Elite factory, Phase622 factory delegate, Phase623 Spawn,
// Phase593 360-byte animation-event override.
constexpr std::array<Watch,4> kWatch{{
  {"elite_factory_create",0x22355c70u,0x355c70u},
  {"elite_spawn",0x22355820u,0x355820u},
  {"elite_animation_event",0x22355480u,0x355480u},
  {"elite_factory_release",0x223552c0u,0x3552c0u}
}};
constexpr DWORD kDr7LocalExecWatch=0x55u;
constexpr DWORD kResumeFlag=0x10000u;
constexpr DWORD kMaxHits=30000u;

struct Options {
  std::wstring game,cwd,log;
  std::vector<std::wstring> extra;
  DWORD timeout_ms=180000u;
  bool disposable=false;
  bool selftest=false;
};
struct Counts {
  unsigned hits[4]={0u,0u,0u,0u};
  std::map<DWORD,unsigned> object_ordinals;
  unsigned next_ordinal=1u;
  unsigned total=0u;
};
static void log_json(FILE* file,DWORD pid,DWORD tid,
                     const char* phase,unsigned detail,unsigned ordinal) {
  if(!file)return;
  std::fprintf(file,
    "{\"phase\":\"%s\",\"pid\":%lu,\"tid\":%lu,\"detail\":%u,\"actor_ordinal\":%u}\n",
    phase,static_cast<unsigned long>(pid),
    static_cast<unsigned long>(tid),detail,ordinal);
  std::fflush(file);
}
static std::wstring quote_win_arg(const std::wstring& arg) {
  std::wstring out=L"\"";
  unsigned slashes=0;
  for(wchar_t c:arg) {
    if(c==L'\\'){++slashes;continue;}
    if(c==L'"'){
      out.append(slashes*2u+1u,L'\\');
      out+=L'"';slashes=0;continue;
    }
    out.append(slashes,L'\\');slashes=0;
    out+=c;
  }
  out.append(slashes*2u,L'\\');
  out+=L'"';
  return out;
}
static bool parse_positive_ms(const wchar_t* s,DWORD& v) {
  if(!s||!*s)return false;
  unsigned long long n=0;
  for(const wchar_t* p=s;*p;++p) {
    if(*p<L'0'||*p>L'9')return false;
    n=n*10u+static_cast<unsigned>(*p-L'0');
    if(n>900000u)return false;
  }
  if(n<1000u)return false;
  v=static_cast<DWORD>(n);
  return true;
}
static bool parse(int argc,wchar_t** argv,Options& o) {
  if(argc==2 && std::wcscmp(argv[1],L"--self-test")==0) {
    o.selftest=true;return true;
  }
  bool extras=false;
  for(int i=1;i<argc;++i) {
    if(extras) {o.extra.emplace_back(argv[i]);continue;}
    if(std::wcscmp(argv[i],L"--")==0){extras=true;continue;}
    if(std::wcscmp(argv[i],L"--disposable-confirmed")==0){
      o.disposable=true;continue;
    }
    if(i+1>=argc)return false;
    if(std::wcscmp(argv[i],L"--game")==0)o.game=argv[++i];
    else if(std::wcscmp(argv[i],L"--cwd")==0)o.cwd=argv[++i];
    else if(std::wcscmp(argv[i],L"--log")==0)o.log=argv[++i];
    else if(std::wcscmp(argv[i],L"--timeout-ms")==0) {
      if(!parse_positive_ms(argv[++i],o.timeout_ms))return false;
    } else return false;
  }
  return o.disposable && !o.game.empty() && !o.log.empty();
}
static std::wstring dirname(const std::wstring& path) {
  const size_t pos=path.find_last_of(L"\\/");
  return pos==std::wstring::npos?L".":path.substr(0,pos);
}
static bool is_original_name(HANDLE hfile) {
  if(hfile==nullptr || hfile==INVALID_HANDLE_VALUE)return false;
  wchar_t buf[4096]={};
  DWORD n=GetFinalPathNameByHandleW(hfile,buf,4096u,FILE_NAME_NORMALIZED);
  if(n==0 || n>=4096u)return false;
  const wchar_t* slash=std::wcsrchr(buf,L'\\');
  const wchar_t* name=slash?slash+1:buf;
  return _wcsicmp(name,L"sin_original_server.dll")==0 ||
         _wcsicmp(name,L"server.dll")==0;
}
static bool sha256_file_handle(HANDLE file) {
  if(file==nullptr || file==INVALID_HANDLE_VALUE)return false;
  LARGE_INTEGER size{};
  if(!GetFileSizeEx(file,&size) ||
     size.QuadPart!=static_cast<LONGLONG>(kOriginalServerSize))
    return false;
  LARGE_INTEGER beginning{};
  if(!SetFilePointerEx(file,beginning,nullptr,FILE_BEGIN))return false;

  BCRYPT_ALG_HANDLE alg=nullptr;
  BCRYPT_HASH_HANDLE hash=nullptr;
  bool good=false;
  // BCrypt uses this caller-owned hash-object buffer until the handle
  // is destroyed. Keep its storage alive beyond the scoped hash work.
  std::vector<UCHAR> object;
  do {
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)
      break;
    DWORD obj_size=0,digest_size=0,received=0;
    if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&obj_size),sizeof(obj_size),&received,0)<0 ||
       obj_size==0u || obj_size>4096u)break;
    if(BCryptGetProperty(alg,BCRYPT_HASH_LENGTH,
        reinterpret_cast<PUCHAR>(&digest_size),sizeof(digest_size),&received,0)<0 ||
       digest_size!=32u)break;
    object.assign(obj_size,0u);
    if(BCryptCreateHash(alg,&hash,object.data(),obj_size,nullptr,0,0)<0)
      break;
    std::array<UCHAR,65536> chunk{};
    DWORD read=0,total=0;
    bool io_ok=true;
    for(;;) {
      if(!ReadFile(file,chunk.data(),static_cast<DWORD>(chunk.size()),
                   &read,nullptr)){io_ok=false;break;}
      if(read==0)break;
      if(BCryptHashData(hash,chunk.data(),read,0)<0) {
        io_ok=false;break;
      }
      total+=read;
      if(total>kOriginalServerSize){io_ok=false;break;}
    }
    if(!io_ok || total!=kOriginalServerSize)break;
    std::array<UCHAR,32> digest{};
    if(BCryptFinishHash(hash,digest.data(),
                       static_cast<ULONG>(digest.size()),0)<0)break;
    char hex[65]={};
    for(size_t i=0;i<digest.size();++i) {
      static const char alphabet[]="0123456789abcdef";
      hex[i*2u]=alphabet[digest[i]>>4u];
      hex[i*2u+1u]=alphabet[digest[i]&0x0fu];
    }
    good=std::string(hex)==kRetailHash;
  } while(false);
  if(hash)BCryptDestroyHash(hash);
  if(alg)BCryptCloseAlgorithmProvider(alg,0);
  return good;
}
static bool file_sha256_is_retail(HANDLE event_file) {
  // Windows LOAD_DLL_DEBUG_EVENT hFile is not guaranteed to grant
  // FILE_READ_DATA. Resolve the exact handle-backed path and reopen only
  // that file read-only, with no game/process memory modification.
  if(!event_file || event_file==INVALID_HANDLE_VALUE)return false;
  wchar_t path[4096]={};
  const DWORD n=GetFinalPathNameByHandleW(
      event_file,path,4096u,FILE_NAME_NORMALIZED);
  if(n==0 || n>=4096u)return false;
  HANDLE file=CreateFileW(path,GENERIC_READ,
      FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
      nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file==INVALID_HANDLE_VALUE)return false;
  const bool matched=sha256_file_handle(file);
  CloseHandle(file);
  return matched;
}
static bool original_live_entry_prefixes_match(HANDLE process,DWORD base) {
  // These tiny non-relocated instruction prefixes come from the owner's
  // SHA-pinned retail server.dll. Check actual loaded process memory too
  // before claiming original instructions can be observed.
  struct Prefix {DWORD rva; UCHAR bytes[4]; SIZE_T len;};
  const Prefix prefixes[]={
    {0x355c70u,{0x8bu,0x44u,0x24u,0x04u},4u}, // factory
    {0x355820u,{0x83u,0xecu,0x18u,0u},3u},   // Spawn
    {0x355480u,{0x51u,0x56u,0x57u,0u},3u},   // event
    {0x3552c0u,{0x8bu,0x4cu,0x24u,0x04u},4u} // release
  };
  for(const auto& prefix:prefixes) {
    UCHAR live[4]={};
    SIZE_T received=0;
    const auto address=reinterpret_cast<LPCVOID>(
        static_cast<std::uintptr_t>(base)+prefix.rva);
    if(!ReadProcessMemory(process,address,live,prefix.len,&received) ||
       received!=prefix.len ||
       std::memcmp(live,prefix.bytes,prefix.len)!=0)
      return false;
  }
  return true;
}
// Phase628 watch-lease policy: never overwrite another debugger's DR slots.
// Windows may set a fixed/reserved DR7 control bit (for example 0x400).
// Preserve that control, and change only our four local-enable bits.
static bool update_owned_watch_registers(CONTEXT& ctx,DWORD base,bool enable) {
  constexpr DWORD kEnableMask=0xffu;
  constexpr DWORD kConditionMask=0xffff0000u;
  constexpr DWORD kOwnershipMask=kDr7LocalExecWatch;
  if(enable) {
    if((ctx.Dr7 & (kEnableMask|kConditionMask))!=0u ||
       ctx.Dr0!=0u || ctx.Dr1!=0u ||
       ctx.Dr2!=0u || ctx.Dr3!=0u)
      return false;
    ctx.Dr0=base+kWatch[0].original_rva;
    ctx.Dr1=base+kWatch[1].original_rva;
    ctx.Dr2=base+kWatch[2].original_rva;
    ctx.Dr3=base+kWatch[3].original_rva;
    ctx.Dr7=(ctx.Dr7&~kEnableMask)|kOwnershipMask;
    ctx.Dr6=0u;
    return true;
  }
  const DWORD enabled=ctx.Dr7&kEnableMask;
  if(enabled==0u)return true; // unarmed thread; no change
  // Stop rather than wiping debugging state that isn't demonstrably ours.
  if(enabled!=kOwnershipMask || (ctx.Dr7&kConditionMask)!=0u ||
     ctx.Dr0!=base+kWatch[0].original_rva ||
     ctx.Dr1!=base+kWatch[1].original_rva ||
     ctx.Dr2!=base+kWatch[2].original_rva ||
     ctx.Dr3!=base+kWatch[3].original_rva)
    return false;
  ctx.Dr7&=~kEnableMask;
  ctx.Dr0=ctx.Dr1=ctx.Dr2=ctx.Dr3=0u;
  ctx.Dr6=0u;
  return true;
}
static bool set_watch_for_thread(DWORD tid,DWORD base,bool enable) {
  HANDLE thread=OpenThread(THREAD_GET_CONTEXT|THREAD_SET_CONTEXT|
                          THREAD_SUSPEND_RESUME,FALSE,tid);
  if(!thread)return false;
  // This tool only examines game threads, never its own thread. Windows
  // suspends the debuggee for each DEBUG_EVENT; this additionally freezes
  // a thread during the timeout/detach path.
  if(SuspendThread(thread)==static_cast<DWORD>(-1)) {
    CloseHandle(thread);return false;
  }
  CONTEXT ctx{};
  ctx.ContextFlags=CONTEXT_DEBUG_REGISTERS;
  bool ok=GetThreadContext(thread,&ctx)!=FALSE;
  if(ok) {
    const CONTEXT original=ctx;
    ok=update_owned_watch_registers(ctx,base,enable);
    if(ok) {
      if(ctx.Dr0!=original.Dr0 || ctx.Dr1!=original.Dr1 ||
         ctx.Dr2!=original.Dr2 || ctx.Dr3!=original.Dr3 ||
         ctx.Dr6!=original.Dr6 || ctx.Dr7!=original.Dr7)
        ok=SetThreadContext(thread,&ctx)!=FALSE;
    }
  }
  if(ResumeThread(thread)==static_cast<DWORD>(-1))ok=false;
  CloseHandle(thread);
  return ok;
}
enum class ArmResult {Armed,RejectedClean,FailedUnclean};
static bool disarm_all_threads(DWORD pid,DWORD base) {
  HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
  if(snapshot==INVALID_HANDLE_VALUE)return false;
  THREADENTRY32 entry{};
  entry.dwSize=sizeof(entry);
  bool first=Thread32First(snapshot,&entry)!=FALSE;
  bool all_clean=first;
  while(first) {
    if(entry.th32OwnerProcessID==pid &&
       !set_watch_for_thread(entry.th32ThreadID,base,false))
      all_clean=false; // still attempt every other thread
    if(!Thread32Next(snapshot,&entry))break;
  }
  CloseHandle(snapshot);
  return all_clean;
}
static ArmResult arm_all_threads(DWORD pid,DWORD base) {
  HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);
  if(snapshot==INVALID_HANDLE_VALUE)return ArmResult::RejectedClean;
  THREADENTRY32 entry{};
  entry.dwSize=sizeof(entry);
  if(!Thread32First(snapshot,&entry)) {
    CloseHandle(snapshot);
    return ArmResult::RejectedClean;
  }
  std::vector<DWORD> attempted; // includes thread on which SetContext fails
  bool error=false;
  do {
    if(entry.th32OwnerProcessID!=pid)continue;
    attempted.push_back(entry.th32ThreadID);
    if(!set_watch_for_thread(entry.th32ThreadID,base,true)) {
      error=true;break;
    }
  } while(Thread32Next(snapshot,&entry));
  CloseHandle(snapshot);
  if(!error && !attempted.empty())return ArmResult::Armed;
  bool rollback_ok=true;
  for(DWORD tid:attempted) {
    if(!set_watch_for_thread(tid,base,false))rollback_ok=false;
  }
  return rollback_ok?ArmResult::RejectedClean:ArmResult::FailedUnclean;
}
static int test_helpers() {
  if(kWatch[0].original_va-kRetailImageBase!=kWatch[0].original_rva ||
     kWatch[1].original_va-kRetailImageBase!=kWatch[1].original_rva ||
     kWatch[2].original_va-kRetailImageBase!=kWatch[2].original_rva ||
     kWatch[3].original_va-kRetailImageBase!=kWatch[3].original_rva)
    return 1;
  if(quote_win_arg(L"C:\\Demo Game\\test.exe")!=
      L"\"C:\\Demo Game\\test.exe\"")return 2;
  if(quote_win_arg(L"x\\\"y")!=L"\"x\\\\\\\"y\"")return 3;
  DWORD ms=0;
  if(!parse_positive_ms(L"120000",ms)||ms!=120000u)return 4;
  if(parse_positive_ms(L"0",ms)||parse_positive_ms(L"-1",ms)||
     parse_positive_ms(L"9999999999",ms))return 5;
  // Allocate a wholly fabricated fake module in this test process to
  // exercise the REAL Win32 ReadProcessMemory source-prefix gate.
  void* fake=VirtualAlloc(nullptr,0x360000u,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
  if(!fake)return 6;
  const DWORD base=static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(fake));
  const std::array<std::array<UCHAR,4>,4> words{{
    {{0x8bu,0x44u,0x24u,0x04u}},
    {{0x83u,0xecu,0x18u,0x00u}},
    {{0x51u,0x56u,0x57u,0x00u}},
    {{0x8bu,0x4cu,0x24u,0x04u}}
  }};
  for(size_t i=0;i<words.size();++i) {
    UCHAR* dest=static_cast<UCHAR*>(fake)+kWatch[i].original_rva;
    std::memcpy(dest,words[i].data(),i==1u||i==2u?3u:4u);
  }
  if(!original_live_entry_prefixes_match(GetCurrentProcess(),base)) {
    VirtualFree(fake,0,MEM_RELEASE);
    return 7;
  }
  UCHAR* changed=static_cast<UCHAR*>(fake)+kWatch[1].original_rva;
  changed[0]^=1u;
  if(original_live_entry_prefixes_match(GetCurrentProcess(),base)) {
    VirtualFree(fake,0,MEM_RELEASE);
    return 8;
  }
  VirtualFree(fake,0,MEM_RELEASE);
  std::puts("PASS Phase627 original-only Elite RVA pins, Win32 source-prefix negative mutation and CLI self-tests");
  return 0;
}
static int trace_original(Options options) {
  if(options.cwd.empty())options.cwd=dirname(options.game);
  FILE* out=nullptr;
  if(_wfopen_s(&out,options.log.c_str(),L"wb")!=0 || !out) {
    std::fwprintf(stderr,L"Cannot open trace file: %ls\n",options.log.c_str());
    return 2;
  }
  std::wstring command=quote_win_arg(options.game);
  for(const auto& a:options.extra)command+=L" "+quote_win_arg(a);
  std::vector<wchar_t> mutable_command(command.begin(),command.end());
  mutable_command.push_back(L'\0');
  STARTUPINFOW startup{};
  startup.cb=sizeof(startup);
  PROCESS_INFORMATION process{};
  if(!CreateProcessW(options.game.c_str(),mutable_command.data(),
      nullptr,nullptr,FALSE,DEBUG_ONLY_THIS_PROCESS,
      nullptr,options.cwd.c_str(),&startup,&process)) {
    std::fprintf(stderr,"CreateProcessW failed, Win32 error=%lu\n",
                 static_cast<unsigned long>(GetLastError()));
    std::fclose(out);return 3;
  }
  const DWORD pid=process.dwProcessId;
  // Explicitly choose detach-on-debugger-exit behavior so a debugger
  // failure does not automatically terminate the user's test game.
  // Win32 requires a debug connection before this API can succeed.
  if(!DebugSetProcessKillOnExit(FALSE)) {
    const DWORD err=GetLastError();
    std::fprintf(stderr,
       "Could not set safe debug-exit behavior (Win32=%lu).\n",
       static_cast<unsigned long>(err));
    (void)DebugActiveProcessStop(pid);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    std::fclose(out);
    return 5;
  }
  std::printf("Original SiN Elite hardware callback trace: PID %lu\n",
              static_cast<unsigned long>(pid));
  log_json(out,pid,0,"trace_process_started",0,0);

  DWORD base=0;
  bool verified=false,armed=false,missed_thread=false;
  bool loader_break_handled=false;
  Counts counts;
  const ULONGLONG deadline=GetTickCount64()+options.timeout_ms;
  bool exited=false;
  while(GetTickCount64()<deadline && counts.total<kMaxHits && !exited) {
    DEBUG_EVENT event{};
    if(!WaitForDebugEvent(&event,350))continue;
    DWORD response=DBG_CONTINUE;
    if(event.dwDebugEventCode==LOAD_DLL_DEBUG_EVENT) {
      HANDLE file=event.u.LoadDll.hFile;
      if(!verified && is_original_name(file) &&
         file_sha256_is_retail(file)) {
        base=static_cast<DWORD>(
              reinterpret_cast<std::uintptr_t>(event.u.LoadDll.lpBaseOfDll));
        if(!original_live_entry_prefixes_match(process.hProcess,base)) {
          log_json(out,pid,event.dwThreadId,
                   "original_live_code_prefix_mismatch",0,0);
        } else {
          verified=true;
          log_json(out,pid,event.dwThreadId,"retail_server_sha256_verified",
                   base,0);
          armed=apply_all_threads(pid,base,true);
          log_json(out,pid,event.dwThreadId,
                   armed?"elite_hardware_watch_armed":"elite_watch_arm_failed",
                   armed?4u:0u,0);
        }
      }
      if(file && file!=INVALID_HANDLE_VALUE)CloseHandle(file);
    } else if(event.dwDebugEventCode==CREATE_THREAD_DEBUG_EVENT) {
      if(armed && !set_watch_for_thread(event.dwThreadId,base,true)) {
        missed_thread=true;
        log_json(out,pid,event.dwThreadId,"elite_thread_watch_failed",0,0);
      }
      // Windows itself closes this DEBUG_EVENT hThread when the
      // matching EXIT_THREAD_DEBUG_EVENT is continued. Do not close early.
    } else if(event.dwDebugEventCode==CREATE_PROCESS_DEBUG_EVENT) {
      if(event.u.CreateProcessInfo.hFile)
        CloseHandle(event.u.CreateProcessInfo.hFile);
      // The DEBUG_EVENT hProcess/hThread are owned by the debug subsystem
      // until the matching exit event is continued. The independent
      // PROCESS_INFORMATION handles are closed by this tracer on exit.
    } else if(event.dwDebugEventCode==EXCEPTION_DEBUG_EVENT) {
      const auto& info=event.u.Exception.ExceptionRecord;
      if(info.ExceptionCode==EXCEPTION_BREAKPOINT) {
        // Only the process loader's initial breakpoint is ours.
        // Other application breakpoints remain with the debuggee.
        if(!loader_break_handled) {
          loader_break_handled=true;
          response=DBG_CONTINUE;
        } else response=DBG_EXCEPTION_NOT_HANDLED;
      } else if(info.ExceptionCode==EXCEPTION_SINGLE_STEP && armed) {
        HANDLE thread=OpenThread(THREAD_GET_CONTEXT|THREAD_SET_CONTEXT,
                                FALSE,event.dwThreadId);
        if(!thread) {
          response=DBG_EXCEPTION_NOT_HANDLED;
          missed_thread=true;
        } else {
          CONTEXT c{};c.ContextFlags=CONTEXT_CONTROL|CONTEXT_INTEGER|
                                     CONTEXT_DEBUG_REGISTERS;
          if(!GetThreadContext(thread,&c)) {
            response=DBG_EXCEPTION_NOT_HANDLED;
            missed_thread=true;
          } else {
            DWORD hit=static_cast<DWORD>(c.Dr6)&0x0fu;
            int match=-1;
            for(unsigned i=0;i<kWatch.size();++i) {
              const DWORD expected=base+kWatch[i].original_rva;
              if((hit&(1u<<i)) && c.Eip==expected) {
                match=static_cast<int>(i);
                break;
              }
            }
            if(match>=0) {
              unsigned ordinal=0;
              if(match==1 || match==2) {
                const DWORD object=c.Ecx;
                if(object!=0) {
                  auto it=counts.object_ordinals.find(object);
                  if(it==counts.object_ordinals.end())
                    it=counts.object_ordinals.emplace(
                         object,counts.next_ordinal++).first;
                  ordinal=it->second;
                }
              }
              ++counts.hits[match];
              ++counts.total;
              log_json(out,pid,event.dwThreadId,kWatch[match].name,
                       counts.hits[match],ordinal);
              // RF prevents the *same* execute breakpoint retriggering
              // before the current original instruction executes once.
              c.EFlags|=kResumeFlag;
              c.Dr6=0;
              if(!SetThreadContext(thread,&c)) {
                missed_thread=true;
                response=DBG_EXCEPTION_NOT_HANDLED;
              } else response=DBG_CONTINUE;
            } else response=DBG_EXCEPTION_NOT_HANDLED;
          }
          CloseHandle(thread);
        }
      } else {
        response=DBG_EXCEPTION_NOT_HANDLED;
      }
    } else if(event.dwDebugEventCode==EXIT_PROCESS_DEBUG_EVENT) {
      log_json(out,pid,event.dwThreadId,"process_exited",
               event.u.ExitProcess.dwExitCode,0);
      exited=true;
    }
    if(!ContinueDebugEvent(event.dwProcessId,event.dwThreadId,response)) {
      log_json(out,pid,event.dwThreadId,"continue_debug_failed",
               GetLastError(),0);
      missed_thread=true;break;
    }
  }
  if(!exited) {
    if(armed) {
      // A detached game must not retain hardware breakpoint registers.
      if(!apply_all_threads(pid,base,false)) {
        log_json(out,pid,0,"watch_disarm_failed_detach_unsafe",0,0);
        std::fprintf(stderr,
          "Hardware watch disarm failed. Do not reuse this test session.\n");
      }
    }
    if(!DebugActiveProcessStop(pid)) {
      log_json(out,pid,0,"debugger_detach_failed",GetLastError(),0);
    } else {
      log_json(out,pid,0,"debugger_detached_game_continues",counts.total,0);
    }
  }
  if(!verified)log_json(out,pid,0,"retail_server_not_authenticated",0,0);
  if(missed_thread)log_json(out,pid,0,"watch_completeness_unproven",0,0);
  for(unsigned i=0;i<kWatch.size();++i) {
    log_json(out,pid,0,kWatch[i].name,counts.hits[i],0);
  }
  log_json(out,pid,0,"trace_finished",counts.total,0);
  std::fclose(out);
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  std::printf("Authenticated retail server=%d; armed=%d; misses=%d; "
              "native hit count=%u (create=%u spawn=%u event=%u release=%u)\n",
              verified?1:0,armed?1:0,missed_thread?1:0,counts.total,
              counts.hits[0],counts.hits[1],counts.hits[2],counts.hits[3]);
  // No original callbacks observed is not a probe success.
  return verified&&armed&&!missed_thread ? 0:4;
}
} // namespace
int wmain(int argc,wchar_t** argv) {
  Options options;
  if(!parse(argc,argv,options)) {
    std::fwprintf(stderr,
      L"Usage: sin_pc_owner_elite_hwtrace_phase627 "
      L"--disposable-confirmed --game <disposable-exe> "
      L"--log <fresh-jsonl> [--cwd <game-dir>] "
      L"[--timeout-ms 120000] [-- <game arguments>]\n"
      L"CI: --self-test (no executable launched).\n");
    return 1;
  }
  return options.selftest ? test_helpers() : trace_original(options);
}
