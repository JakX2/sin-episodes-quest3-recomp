// Phase657: owner-side Win32 x86 retail CreateInterface validation.
// No private DLL files or binary bytes are bundled with this probe.
// Compile with MSVC /W4 /WX /EHsc /std:c++17, link bcrypt.lib.
// Usage: sin_phase657_factory_probe.exe C:\trusted\bin\client.dll C:\trusted\SE1\bin\server.dll
// ONLY trusted owner retail images with the pinned SHA256 are loaded.
// LoadLibraryEx invokes DllMain. No object vtable methods are ever called.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../pc/independent_slot0/original_interfacereg_readonly_snapshot.h"
#include "../../pc/independent_slot0/original_client_entitylist_readonly_snapshot.h"
#include "../../pc/independent_slot0/original_client_entitylist_state_snapshot.h"
#include "../../pc/independent_slot0/original_client_prediction23_readonly_snapshot.h"
#include "../../pc/independent_slot0/original_client_prediction_convar_readonly_snapshot.h"
#include "../../pc/independent_slot0/original_client_local_player_readonly_snapshot.h"
#pragma comment(lib, "bcrypt.lib")

#if !defined(_M_IX86) && !defined(__i386__)
#error Retail factory validation requires native 32-bit Win32 x86.
#endif

namespace {
struct NameExpected { const char* name; std::uint32_t object_rva; };
struct ModuleExpected {
 const char* side;
 const char* sha256;
 std::uint32_t factory_rva;
 const NameExpected* expected;
 std::size_t count;
};
constexpr NameExpected kClient[]={
 {"VClient011",0x2fa4b8u},
 {"VClientEntityList003",0x34fddcu},
 {"VClientPrediction001",0x37fa00u},
 {"GameClientExports001",0x2fa4b0u}
};
constexpr NameExpected kServer[]={
 {"ServerGameDLL003",0x510650u},
 {"ServerGameEnts001",0x510654u},
 {"ServerGameClients003",0x510658u}
};
constexpr ModuleExpected kModules[]={
 {"client","6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d",
  0x164180u,kClient,3u},
 {"server","568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650",
  0x1fb5f0u,kServer,3u}
};
using CreateInterfaceFn=void* (__cdecl *)(const char*,int*);

bool retail_sha256(const char* path,const char* expected) noexcept {
 std::FILE* f=nullptr;
 if(fopen_s(&f,path,"rb")!=0 || !f)return false;
 BCRYPT_ALG_HANDLE alg=nullptr;
 BCRYPT_HASH_HANDLE hash=nullptr;
 ULONG written=0,object_size=0,digest_size=0;
 bool ok=false;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0 &&
    BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&object_size),
                      sizeof(object_size),&written,0)>=0 &&
    BCryptGetProperty(alg,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&digest_size),
                      sizeof(digest_size),&written,0)>=0 &&
    digest_size==32u && object_size>0u && object_size<1048576u) {
  std::vector<UCHAR> object(object_size,0u);
  if(BCryptCreateHash(alg,&hash,object.data(),object_size,nullptr,0,0)>=0) {
   std::array<UCHAR,32768> block{};
   ok=true;
   for(;;) {
    const auto n=std::fread(block.data(),1u,block.size(),f);
    if(n && BCryptHashData(hash,block.data(),static_cast<ULONG>(n),0)<0) {
     ok=false;break;
    }
    if(n<block.size()) {
     if(std::ferror(f))ok=false;
     break;
    }
   }
   if(ok) {
    std::array<UCHAR,32> digest{};
    if(BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)<0) {
     ok=false;
    } else {
     char hex[65]{};
     constexpr char kDigits[]="0123456789abcdef";
     for(std::size_t i=0;i<digest.size();++i) {
      hex[2u*i]=kDigits[digest[i]>>4u];
      hex[2u*i+1u]=kDigits[digest[i]&15u];
     }
     ok=std::strcmp(hex,expected)==0;
    }
   }
  }
 }
 if(hash)BCryptDestroyHash(hash);
 if(alg)BCryptCloseAlgorithmProvider(alg,0);
 std::fclose(f);
 return ok;
}

// Phase659 read-only owner runtime witness for the original client app-system
// registration list at RVA 0x369374. No unknown native methods are invoked.
// Reads only DWORDs; does not print process ASLR addresses or interface pointers.
bool read_current_process_dword(std::uintptr_t address,std::uint32_t& value) noexcept {
 SIZE_T got=0;
 return ReadProcessMemory(GetCurrentProcess(),
    reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&got)!=0 &&
    got==sizeof(value);
}
bool inspect_client_registration_list(HMODULE original_client) noexcept {
 constexpr std::uint32_t kHeadRva=0x369374u;
 const auto base=reinterpret_cast<std::uintptr_t>(original_client);
 std::uint32_t cursor=0u;
 if(!read_current_process_dword(base+kHeadRva,cursor)) {
  std::printf("{\"module\":\"client\",\"event\":\"phase659_registry_snapshot\","
              "\"status\":\"head_unreadable\"}\n");
  return false;
 }
 const bool populated=cursor!=0u;
 std::array<std::uint32_t,64> visited{};
 std::size_t count=0u,required=0u,optional=0u,callbacks_nonzero=0u;
 const char* status="read_ok";
 while(cursor!=0u) {
  if(count==visited.size()){status="list_truncated";break;}
  bool repeated=false;
  for(std::size_t i=0u;i<count;++i)
   if(visited[i]==cursor){repeated=true;break;}
  if(repeated){status="cycle_detected";break;}
  if(cursor>0xfffffff3u){status="invalid_entry_address";break;}
  visited[count++]=cursor;
  std::uint32_t target=0u,resolve=0u,release=0u,next=0u;
  if(!read_current_process_dword(cursor,target) ||
     !read_current_process_dword(cursor+4u,resolve) ||
     !read_current_process_dword(cursor+8u,release) ||
     !read_current_process_dword(cursor+12u,next)) {
   status="entry_unreadable";break;
  }
  if(target)++required;else ++optional;
  if(resolve && release)++callbacks_nonzero;
  cursor=next;
 }
 const bool ok=std::strcmp(status,"read_ok")==0;
 std::printf("{\"module\":\"client\",\"event\":\"phase659_registry_snapshot\","
             "\"status\":\"%s\",\"head_populated\":%s,\"entries\":%u,"
             "\"required_entries\":%u,\"optional_entries\":%u,"
             "\"paired_callbacks_nonnull\":%u}\n",
             status,populated?"true":"false",
             static_cast<unsigned int>(count),
             static_cast<unsigned int>(required),
             static_cast<unsigned int>(optional),
             static_cast<unsigned int>(callbacks_nonzero));
 return ok;
}

// Phase660: read-only post-DllMain observation of original server's
// AutoGameSystem pending registrations and live array. This is explicitly
// NOT an in-engine GameDLLInit call and no system virtual method is invoked.
bool read_current_process_byte(std::uintptr_t address,std::uint8_t& value) noexcept {
 SIZE_T got=0;
 return ReadProcessMemory(GetCurrentProcess(),
     reinterpret_cast<LPCVOID>(address),&value,sizeof(value),&got)!=0 &&
     got==sizeof(value);
}
// Phase1653: read-only observation of the original 12-byte InterfaceReg
// linked list after DllMain. This is SEPARATE from Phase659's app-system
// registry and from game entity class factories. Do not invoke creator
// functions or any virtual methods. The SHA of the original DLL is already
// required by inspect() BEFORE LoadLibraryEx calls this observer.
bool read_original_interfacereg_dword(
    void*,std::uintptr_t address,std::uint32_t* value) noexcept {
 if(!value)return false;
 return read_current_process_dword(address,*value);
}
bool read_original_interfacereg_byte(
    void*,std::uintptr_t address,std::uint8_t* value) noexcept {
 if(!value)return false;
 return read_current_process_byte(address,*value);
}
// Phase1654: owner-only post-DllMain read of the authentic original
// GameClientExports001 singleton vptr and all four virtual function RVAs.
// The exact original client.dll SHA is verified by inspect() before
// LoadLibraryEx; this code never invokes a vtable function or pointer.
// Phase1655: SHA-authenticated original server module's 15-slot
// ServerGameClients003 singleton. This is post-original-DllMain readback;
// never invoke virtual callbacks, especially client-connect callbacks.
// The originating server.dll full-file SHA is verified BEFORE LoadLibraryEx.
bool inspect_server_game_clients_fifteen_slots(HMODULE server) noexcept {
 const auto base=reinterpret_cast<std::uintptr_t>(server);
 constexpr std::uintptr_t singleton_rva=0x510658u;
 constexpr std::uintptr_t vtable_rva=0x443054u;
 constexpr std::uint32_t method_rvas[15]={
     0x1e63f0u,0x1e8330u,0x1ea330u,0x1ea3a0u,0x1e8340u,
     0x1e9650u,0x1e8510u,0x1e8370u,0x1e9c00u,0x1ea600u,
     0x1e84d0u,0x1e85b0u,0x1e8530u,0x1e8520u,0x1e85e0u
 };
 std::uint32_t pointer=0u;
 bool ok=read_current_process_dword(base+singleton_rva,pointer)&&
         static_cast<std::uintptr_t>(pointer)==base+vtable_rva;
 unsigned matched=0u;
 for(unsigned i=0u;ok&&i<15u;++i) {
    std::uint32_t target=0u;
    if(!read_current_process_dword(base+vtable_rva+i*4u,target)) {
       ok=false;
       break;
    }
    if(static_cast<std::uintptr_t>(target)!=base+method_rvas[i]) {
       ok=false;
       break;
    }
    ++matched;
 }
 std::printf(
     "{\"module\":\"server\",\"event\":"
     "\"original_servergameclients_vtable\","
     "\"status\":\"%s\",\"matched_slots\":%u,"
     "\"expected_slots\":15,\"actual_method_invocations\":0}\n",
     ok?"pinned_original_vptr_read":"vptr_or_slot_mismatch",
     matched);
 return ok;
}

// Phase1656: after exact SHA gate and owner-authorized original
// LoadLibraryEx, inspect the distinct parent and embedded client entity-list
// pointers. No factory virtual callbacks or original engine methods run here.
// Phase1657: original SHA-authenticated DllMain-only client entity list
// readback of count/max/secondary count/highest index. Never call client
// entitylist vtable methods or log real module addresses/raw object bytes.
// Phase1658: after SHA-authenticated owner-original LoadLibraryEx,
// snapshot the 23 prediction function addresses in the loaded image.
// The prediction initializer dereferences an engine global at +0x2C;
// NEVER call it in this owner factory probe. The game engine has not
// initialized its dependency service in this disposable process.
// Phase1658: read ONLY the original SHA-pinned cl_predict parent DWORD and
// self-referenced numeric value after owner-confirmed original DllMain.
// Never chase an external ConVar m_pParent or run prediction vtable methods.
bool inspect_client_prediction_convar(HMODULE client) noexcept {
  namespace cv=sin_independent::original_client_prediction_convar_snapshot;
  const auto base=reinterpret_cast<std::uintptr_t>(client);
  const auto result=cv::Observe(
      base,{nullptr,&read_original_interfacereg_dword});
  // No raw pointers, original code or private object contents printed.
  std::printf(
      "{\"module\":\"client\",\"event\":"
      "\"original_cl_predict_convar_startup\","
      "\"status\":\"%s\",\"fields_read\":%u,"
      "\"parent_self\":%s,\"numeric_value\":%d,"
      "\"retail_method_invocations\":0}\n",
      cv::StatusName(result.status),result.successful_reads,
      result.parent_is_expected_self?"true":"false",
      static_cast<int>(result.numeric_value));
  // An external ConVar parent may occur in an actual engine registration
  // context; this owner-only stand-alone probe cannot classify it safely.
  return result.status==cv::Status::VerifiedSelfParent;
}

bool inspect_client_prediction23(HMODULE client) noexcept {
  namespace pred=sin_independent::original_client_prediction23_readonly_snapshot;
  const auto base=reinterpret_cast<std::uintptr_t>(client);
  const auto result=pred::Observe(
      base,{nullptr,&read_original_interfacereg_dword});
  std::printf(
      "{\"module\":\"client\",\"event\":"
      "\"original_client_prediction23_vtable\","
      "\"status\":\"%s\",\"matched_slots\":%u,"
      "\"expected_slots\":23,\"actual_method_invocations\":0,"
      "\"retail_engine_prediction_init_executed\":false}\n",
      pred::StatusName(result.status),result.matched_slots);
  return result.status==pred::Status::Verified;
}

// Phase1660: after owner-original full SHA gate and optional DllMain,
// inspect the original local C_BasePlayer pointer with exactly bounded
// DWORD reads. Never invoke C_BasePlayer or prediction virtual methods.
// The normal pre-engine DLL-load state has NO player and is not an error.
// A live subclass with a distinct vptr is not treated as exact base
// C_BasePlayer, and its fields are never read from this probe.
bool inspect_client_local_player_readonly(HMODULE client) noexcept {
  namespace lp=sin_independent::original_client_local_player_readonly_snapshot;
  const auto base=reinterpret_cast<std::uintptr_t>(client);
  const auto result=lp::Observe(
      base,{nullptr,&read_original_interfacereg_dword});
  // No raw pointers, view coordinates, float bits or memory addresses
  // are printed. This is metadata about snapshot verification only.
  const unsigned nonzero_view_words=
      static_cast<unsigned>(result.vector_bits[0]!=0u)+
      static_cast<unsigned>(result.vector_bits[1]!=0u)+
      static_cast<unsigned>(result.vector_bits[2]!=0u);
  std::printf(
      "{\"module\":\"client\",\"event\":"
      "\"original_local_cbaseplayer_prediction_view_snapshot\","
      "\"status\":\"%s\",\"dwords_read\":%u,"
      "\"primary_vptr_match\":%s,\"scalar_nonzero\":%s,"
      "\"nonzero_view_words\":%u,\"retail_method_invocations\":0,"
      "\"retail_virtual_invocations\":0}\n",
      lp::StatusName(result.status),result.successful_reads,
      result.exact_primary_vptr_match?"true":"false",
      result.scalar!=0u?"true":"false",nonzero_view_words);
  // An owner probe is deliberately loaded without a running engine,
  // so there is normally no current local player. A present player
  // is supported only when its EXACT 2006 base-class primary vptr
  // matched on both reads, and the owner global was unchanged.
  return result.status==lp::Status::NoLocalPlayer ||
         result.status==lp::Status::VerifiedExactBasePlayer;
}

bool inspect_client_entitylist_startup_state(HMODULE client) noexcept {
  namespace state=sin_independent::original_client_entitylist_state_snapshot;
  const auto base=reinterpret_cast<std::uintptr_t>(client);
  const auto result=state::Observe(
      base,{nullptr,&read_original_interfacereg_dword});
  std::printf(
      "{\"module\":\"client\",\"event\":"
      "\"original_client_entitylist_startup_state\","
      "\"status\":\"%s\",\"fields_read\":%u,"
      "\"networked_count\":%u,\"max_entities\":%u,"
      "\"secondary_flagged_count\":%u,\"highest_live_index\":%d,"
      "\"actual_method_invocations\":0}\n",
      state::StatusName(result.status),result.successful_reads,
      static_cast<unsigned>(result.networked_count),
      static_cast<unsigned>(result.max_entities),
      static_cast<unsigned>(result.secondary_flagged_count),
      static_cast<int>(result.highest_live_index));
  return result.status==state::Status::Valid;
}

bool inspect_client_entitylist_nine_slots(HMODULE client) noexcept {
  namespace abi=sin_independent::original_client_entitylist_snapshot;
  const auto base=reinterpret_cast<std::uintptr_t>(client);
  const auto result=abi::Observe(
      base,{nullptr,&read_original_interfacereg_dword});
  // Redacted metadata only. Never print original pointers or native code.
  std::printf(
      "{\"module\":\"client\",\"event\":"
      "\"original_client_entitylist_secondary_vtable\","
      "\"status\":\"%s\",\"secondary_offset_bytes\":65556,"
      "\"secondary_slots_matched\":%u,\"secondary_slots_expected\":9,"
      "\"parent_slots_matched\":%u,\"parent_slots_expected\":3,"
      "\"actual_method_invocations\":0}\n",
      abi::StatusName(result.status),result.secondary_slots_matched,
      result.parent_slots_matched);
  return result.status==abi::Status::Verified;
}

bool inspect_game_client_exports_four_slots(HMODULE client) noexcept {
 const auto base=reinterpret_cast<std::uintptr_t>(client);
 constexpr std::uintptr_t singleton_rva=0x2fa4b0u;
 constexpr std::uintptr_t vtable_rva=0x296b18u;
 constexpr std::uint32_t expected_method_rvas[4]={
     0xfe9b0u,0xfe950u,0xfe970u,0xfe990u
 };
 std::uint32_t vptr=0u;
 bool ok=read_current_process_dword(
     base+singleton_rva,vptr)&&
     static_cast<std::uintptr_t>(vptr)==base+vtable_rva;
 unsigned matched=0u;
 for(unsigned i=0u;ok&&i<4u;++i) {
    std::uint32_t target=0u;
    if(!read_current_process_dword(base+vtable_rva+i*4u,target)) {
       ok=false;
       break;
    }
    if(static_cast<std::uintptr_t>(target)!=base+expected_method_rvas[i]) {
       ok=false;
       break;
    }
    ++matched;
 }
 std::printf(
     "{\"module\":\"client\",\"event\":"
     "\"original_gameclientexports_vtable\","
     "\"status\":\"%s\",\"matched_slots\":%u,"
     "\"expected_slots\":4,\"actual_method_invocations\":0}\n",
     ok?"pinned_original_vptr_read":"vptr_or_slot_mismatch",
     matched);
 return ok;
}

bool inspect_original_interface_registry(
    HMODULE original,const ModuleExpected& module) noexcept {
 namespace abi=sin_independent::original_interfacereg_snapshot;
 const auto base=reinterpret_cast<std::uintptr_t>(original);
 const auto layout=std::strcmp(module.side,"client")==0 ?
       abi::OriginalClientLayout(base) : abi::OriginalServerLayout(base);
 const auto result=abi::Observe(
     layout,{nullptr,&read_original_interfacereg_dword,
                      &read_original_interfacereg_byte});
 // No raw pointers, original executable bytes, or third-party interface
 // names are printed. Counts and status labels are the only observations.
 std::printf(
     "{\"module\":\"%s\",\"event\":\"original_interfacereg_snapshot\","
     "\"status\":\"%s\",\"entries\":%u,\"expected\":%u,"
     "\"priority_found\":%u,\"priority_expected\":%u,"
     "\"continuous_owner_pin_proven\":false}\n",
     module.side,abi::StatusName(result.status),
     static_cast<unsigned int>(result.entries),
     static_cast<unsigned int>(layout.expected_count),
     static_cast<unsigned int>(result.priority_found),
     static_cast<unsigned int>(layout.priority_count));
 return result.accepted();
}

bool inspect_server_autogame_systems(HMODULE original_server) noexcept {
 const auto base=reinterpret_cast<std::uintptr_t>(original_server);
 constexpr std::uintptr_t head_rva=0x572cc0u,flag_rva=0x572cb8u;
 constexpr std::uintptr_t count_rva=0x510a64u,cap_rva=0x510a5cu;
 std::uint32_t cursor=0u,count=0u,cap=0u;
 std::uint8_t initialized=0u;
 if(!read_current_process_dword(base+head_rva,cursor) ||
    !read_current_process_byte(base+flag_rva,initialized) ||
    !read_current_process_dword(base+count_rva,count) ||
    !read_current_process_dword(base+cap_rva,cap)) {
  std::printf("{\"module\":\"server\",\"event\":\"phase660_autogame_snapshot\","
              "\"status\":\"globals_unreadable\"}\n");
  return false;
 }
 const bool head_populated=cursor!=0u;
 std::array<std::uint32_t,64> visited{};
 std::size_t pending_count=0u;
 const char* status="read_ok";
 while(cursor) {
  if(pending_count==visited.size()){status="list_truncated";break;}
  bool repeated=false;
  for(std::size_t i=0u;i<pending_count;++i)
   if(visited[i]==cursor){repeated=true;break;}
  if(repeated){status="cycle_detected";break;}
  if(cursor>0xfffffffbu){status="invalid_entry_address";break;}
  visited[pending_count++]=cursor;
  std::uint32_t next=0u;
  if(!read_current_process_dword(cursor+4u,next)) {
   status="entry_unreadable";break;
  }
  cursor=next;
 }
 std::printf("{\"module\":\"server\",\"event\":\"phase660_autogame_snapshot\","
             "\"status\":\"%s\",\"pending_head_populated\":%s,"
             "\"pending_entries\":%u,\"live_count\":%u,\"live_capacity\":%u,"
             "\"initialized_flag\":%u}\n",
             status,head_populated?"true":"false",
             static_cast<unsigned int>(pending_count),
             static_cast<unsigned int>(count),
             static_cast<unsigned int>(cap),
             static_cast<unsigned int>(initialized));
 return std::strcmp(status,"read_ok")==0;
}

// Phase664: owner-only read-only snapshot of the authentic server.dll
// gameplay variable data image. The two retail DLL SHA256 fingerprints are
// verified in inspect() before LoadLibraryEx, so these RVAs are only applied
// to the matching original 2006 binary. No writes and no unknown vcall.
struct MetricWitness {
 const char* name;
 std::uint32_t initial_float_word;
};
constexpr MetricWitness kOriginalMetrics[]={
 {"DAMAGE_TAKEN_SCALE",0x3f89999au},
 {"DAMAGE_DONE_SCALE",0x3f800000u},
 {"AMMO_QUANTITY_SCALE",0x3f800000u},
 {"NUM_FORCED_HEALTH_KITS",0x00000000u},
 {"PERCENT_HEALTH_KITS_DROP",0x3ecccccdu},
 {"PERCENT_NPC_2ND_FIRE",0x3e800000u},
 {"PERCENT_NPC_MAKE_DIFFERENT",0x3e19999au},
 {"PERCENT_NPC_CHANGE_SPAWN_NUM",0x3e19999au},
 {"PERCENT_NPC_WEARS_HELMET",0x3eb33334u},
 {"PERCENT_NPC_HELMET_SHOT_OFF",0x3f19999au},
 {"PERCENT_NPC_TAKES_COVER",0x3e4ccccdu},
 {"RANDOM_NPC_WAIT_BEFORE_SHOOT",0x00000000u},
 {"PERCENT_NPC_ACCURATE",0x3f400000u},
 {"PERCENT_NPC_MOBILE",0x3f000000u}
};
bool inspect_original_server_metric_slots(HMODULE server) noexcept {
 constexpr std::uint32_t kCurrentRva=0x523478u;
 constexpr std::uint32_t kPointerRva=0x45f04cu;
 constexpr std::uint32_t kImageBytes=0x628000u;
 constexpr std::uint32_t kConvarParentRva=0x5923a8u;
 const auto base=reinterpret_cast<std::uintptr_t>(server);
 const auto image_end=base+kImageBytes;
 std::size_t names_ok=0u,initial_values=0u,read_slots=0u;
 bool complete=true;
 for(std::size_t i=0u;i<sizeof(kOriginalMetrics)/sizeof(kOriginalMetrics[0]);++i){
  std::uint32_t pointer=0u,current=0u;
  const auto& expected=kOriginalMetrics[i];
  bool valid=read_current_process_dword(base+kPointerRva+4u*i,pointer) &&
             read_current_process_dword(base+kCurrentRva+28u*i,current);
  if(valid)++read_slots;
  bool name_matches=valid &&
       static_cast<std::uintptr_t>(pointer)>=base &&
       static_cast<std::uintptr_t>(pointer)<image_end;
  if(name_matches) {
   const auto count=std::strlen(expected.name);
   if(count+1u>image_end-static_cast<std::uintptr_t>(pointer))
    name_matches=false;
   for(std::size_t j=0u;name_matches&&j<=count;++j) {
    std::uint8_t actual=0u;
    if(!read_current_process_byte(static_cast<std::uintptr_t>(pointer)+j,actual)||
       actual!=static_cast<std::uint8_t>(expected.name[j]))
     name_matches=false;
   }
  }
  if(name_matches)++names_ok;
  if(valid&&current==expected.initial_float_word)++initial_values;
  else if(!valid)complete=false;
  if(!name_matches)complete=false;
  std::printf("{\"module\":\"server\",\"event\":\"phase664_metric_current\","
              "\"index\":%u,\"metric\":\"%s\",\"status\":\"%s\","
              "\"observed_float_bits\":\"0x%08x\","
              "\"static_initial_bits\":\"0x%08x\"}\n",
              static_cast<unsigned int>(i),expected.name,
              name_matches?"name_verified":"name_or_slot_mismatch",
              static_cast<unsigned int>(current),
              static_cast<unsigned int>(expected.initial_float_word));
 }
 std::uint32_t parent=0u,adaptive_value=0u;
 const bool parent_read=read_current_process_dword(
      base+kConvarParentRva,parent);
 const bool value_read=parent_read&&parent!=0u &&
      read_current_process_dword(parent+0x2cu,adaptive_value);
 std::printf("{\"module\":\"server\",\"event\":\"phase664_metric_snapshot\","
             "\"status\":\"%s\",\"entries_read\":%u,\"name_matches\":%u,"
             "\"initial_current_matches\":%u,\"dd_enabled_readable\":%s,"
             "\"dd_enabled_value\":%u,"
             "\"engine_GameDLLInit_called\":false}\n",
             complete?"read_ok":"incomplete",
             static_cast<unsigned int>(read_slots),
             static_cast<unsigned int>(names_ok),
             static_cast<unsigned int>(initial_values),
             value_read?"true":"false",
             static_cast<unsigned int>(adaptive_value));
 return complete;
}

bool inspect(const char* path,const ModuleExpected& spec) noexcept {
 // Resolve paths before LOAD_WITH_ALTERED_SEARCH_PATH.
 std::array<char,32768> absolute{};
 const DWORD size=GetFullPathNameA(path,static_cast<DWORD>(absolute.size()),
                                  absolute.data(),nullptr);
 if(!size || size>=absolute.size()) {
  std::fprintf(stderr,"%s path resolution failed\n",spec.side);
  return false;
 }
 if(!retail_sha256(absolute.data(),spec.sha256)) {
  std::fprintf(stderr,"%s retail SHA256 mismatch/unreadable; refusing LoadLibrary\n",spec.side);
  return false;
 }
 HMODULE module=LoadLibraryExA(absolute.data(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
 if(!module) {
  std::fprintf(stderr,"%s trusted retail DLL LoadLibraryEx error=%lu\n",
               spec.side,static_cast<unsigned long>(GetLastError()));
  return false;
 }
 bool ok=true;
 auto* symbol=GetProcAddress(module,"CreateInterface");
 if(!symbol) {
  std::fprintf(stderr,"%s missing CreateInterface export\n",spec.side);
  ok=false;
 } else {
  const auto base=reinterpret_cast<std::uintptr_t>(module);
  const auto proc=reinterpret_cast<std::uintptr_t>(symbol);
  if(proc-base!=spec.factory_rva) {
   std::fprintf(stderr,"%s CreateInterface export offset mismatch\n",spec.side);
   ok=false;
  } else {
   auto* factory=reinterpret_cast<CreateInterfaceFn>(symbol);
   for(std::size_t i=0;i<spec.count;++i) {
    const auto& entry=spec.expected[i];
    int status=-777;
    void* ptr=factory(entry.name,&status);
    const auto result=reinterpret_cast<std::uintptr_t>(ptr);
    const bool match=(ptr!=nullptr && status==0 &&
                      result-base==entry.object_rva);
    std::printf("{\"module\":\"%s\",\"interface\":\"%s\","
                "\"return_code\":%d,\"singleton_rva\":\"0x%08x\","
                "\"status\":\"%s\"}\n",
                spec.side,entry.name,status,
                ptr?static_cast<unsigned int>(result-base):0u,
                match?"pinned_factory_return":"mismatch");
    if(!match)ok=false;
   }
   int missing_rc=-777;
   const void* absent=factory("__SiN_Phase657_Invalid_Interface__",&missing_rc);
   const bool miss_ok=(absent==nullptr && missing_rc==1);
   std::printf("{\"module\":\"%s\",\"interface\":\"absent_interface_probe\","
               "\"return_code\":%d,\"status\":\"%s\"}\n",
               spec.side,missing_rc,miss_ok?"not_found_as_expected":"mismatch");
   if(!miss_ok)ok=false;
  }
 }
 if(!inspect_original_interface_registry(module,spec))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 &&
    !inspect_game_client_exports_four_slots(module))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 &&
    !inspect_client_prediction23(module))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 &&
    !inspect_client_prediction_convar(module))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 &&
    !inspect_client_local_player_readonly(module))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 &&
    !inspect_client_entitylist_nine_slots(module))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 &&
    !inspect_client_entitylist_startup_state(module))
  ok=false;
 if(std::strcmp(spec.side,"server")==0 &&
    !inspect_server_game_clients_fifteen_slots(module))
  ok=false;
 if(std::strcmp(spec.side,"client")==0 && !inspect_client_registration_list(module))
  ok=false;
 if(std::strcmp(spec.side,"server")==0 && !inspect_server_autogame_systems(module))
  ok=false;
 if(std::strcmp(spec.side,"server")==0 && !inspect_original_server_metric_slots(module))
  ok=false;
 FreeLibrary(module);
 return ok;
}
} // namespace

int main(int argc,char** argv) {
 if(argc!=3) {
  std::fprintf(stderr,"usage: %s <original-client.dll> <original-server.dll>\n",
               argc>0?argv[0]:"sin_phase657_factory_probe.exe");
  return 2;
 }
 bool all=true;
 // Continue to check both trusted modules if the first has a load failure.
 for(int i=0;i<2;++i)
  if(!inspect(argv[i+1],kModules[i]))all=false;
 return all?0:1;
}
