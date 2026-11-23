#include "sin_compat/original_module_init_phase658.hpp"

namespace sin_compat::original_module_init_phase658 {
namespace {
constexpr QuerySpec kServerQueries[] = {
  {"VEngineServer021",0x221e7649u,0x22442a48u,0x22572198u,Factory::Engine},
  {"VoiceServer001",0x221e7662u,0x22442a38u,0x2257219cu,Factory::Engine},
  {"VEngineCvar002",0x221e767bu,0x22442a28u,0x225721a0u,Factory::Engine},
  {"VEngineServerStringTable001",0x221e7694u,0x22442a0cu,0x225721a8u,Factory::Engine},
  {"StaticPropMgrServer002",0x221e76adu,0x224429f4u,0x225721acu,Factory::Engine},
  {"VEngineRandom001",0x221e76c6u,0x224429e0u,0x225721b0u,Factory::Engine},
  {"IEngineSoundServer003",0x221e76dfu,0x224429c8u,0x225721b4u,Factory::Engine},
  {"SpatialPartition001",0x221e76f8u,0x224429b4u,0x225721b8u,Factory::Engine},
  {"VModelInfoServer001",0x221e7711u,0x224429a0u,0x225721bcu,Factory::Engine},
  {"EngineTraceServer003",0x221e772au,0x22442988u,0x225721c0u,Factory::Engine},
  {"VFileSystem012",0x221e7747u,0x22442978u,0x225721a4u,Factory::FileSystem},
  {"GAMEEVENTSMANAGER002",0x221e7760u,0x22442960u,0x225721c4u,Factory::Engine},
  {"VEngineCache001",0x221e7779u,0x22442950u,0x225721c8u,Factory::Engine},
  {"VSoundEmitter001",0x221e7792u,0x2244293cu,0x225721d0u,Factory::Engine},
  {"ServerUploadGameStats001",0x221e77abu,0x22442920u,0x22572194u,Factory::Engine},
  {"ISERVERPLUGINHELPERS001",0x221e77c4u,0x22442908u,0x225721d4u,Factory::Engine},
};
constexpr QuerySpec kClientQueries[] = {
  {"VEngineClient012",0x240fe109u,0x24296aecu,0x2433f684u,Factory::Engine},
  {"VEngineModel012",0x240fe127u,0x24296adcu,0x2433f688u,Factory::Engine},
  {"VEngineEffects001",0x240fe140u,0x24296ac8u,0x2433f68cu,Factory::Engine},
  {"VEngineCvar002",0x240fe159u,0x24296ab8u,0x2433f690u,Factory::Engine},
  {"EngineTraceClient003",0x240fe172u,0x24296aa0u,0x2433f6d8u,Factory::Engine},
  {"VEngineRenderView011",0x240fe18bu,0x24296a88u,0x2433f694u,Factory::Engine},
  {"VDebugOverlay003",0x240fe1a4u,0x24296a74u,0x2433f698u,Factory::Engine},
  {"VMaterialSystem076",0x240fe1bdu,0x24296a60u,0x2433f69cu,Factory::Engine},
  {"VEngineCache001",0x240fe1d6u,0x24296a50u,0x2433f6a8u,Factory::Engine},
  {"VModelInfoClient003",0x240fe1efu,0x24296a3cu,0x2433f6acu,Factory::Engine},
  {"VEngineVGui001",0x240fe208u,0x24296a2cu,0x2433f6b0u,Factory::Engine},
  {"VEngineClientStringTable001",0x240fe221u,0x24296a10u,0x2433f6b4u,Factory::Engine},
  {"SpatialPartition001",0x240fe23au,0x242969fcu,0x2433f6b8u,Factory::Engine},
  {"VEngineShadowMgr002",0x240fe253u,0x242969e8u,0x2433f6c0u,Factory::Engine},
  {"StaticPropMgrClient004",0x240fe26cu,0x242969d0u,0x2433f6c4u,Factory::Engine},
  {"IEngineSoundClient003",0x240fe285u,0x242969b8u,0x2433f6c8u,Factory::Engine},
  {"VFileSystem012",0x240fe29eu,0x242d328cu,0x2433f6bcu,Factory::Engine},
  {"VEngineRandom001",0x240fe2b7u,0x242969a4u,0x2433f6ccu,Factory::Engine},
  {"VENGINE_GAMEUIFUNCS_VERSION004",0x240fe2d0u,0x24296984u,0x2433f6dcu,Factory::Engine},
  {"GAMEEVENTSMANAGER002",0x240fe2e9u,0x2429696cu,0x2433f6e0u,Factory::Engine},
  {"VSoundEmitter001",0x240fe302u,0x24296958u,0x2433f6e4u,Factory::Engine},
};
static_assert(sizeof(kServerQueries)/sizeof(QuerySpec)==16u,"original server query count");
static_assert(sizeof(kClientQueries)/sizeof(QuerySpec)==21u,"original client query count");
struct BusyGuard {
  bool& busy;
  explicit BusyGuard(bool& ref) noexcept:busy(ref){busy=true;}
  ~BusyGuard()noexcept{busy=false;}
};
}
Manifest original_manifest(Role role) noexcept {
  return role==Role::Server
    ? Manifest{kServerQueries,sizeof(kServerQueries)/sizeof(QuerySpec)}
    : Manifest{kClientQueries,sizeof(kClientQueries)/sizeof(QuerySpec)};
}
Result AcquirePrefix::run(Role role,std::uint32_t engine_factory,
                          std::uint32_t filesystem_factory,
                          std::uint32_t globals_pointer) noexcept {
  Result r{};r.role=role;
  if(busy_){r.code=Code::Reentrant;return r;}
  if(engine_factory==0u || globals_pointer==0u ||
     (role==Role::Server&&filesystem_factory==0u)){
    r.code=Code::InvalidArgument;return r;
  }
  BusyGuard guard(busy_);
  try {
    if(role==Role::Client){
      // Original first-client query stores the third argument before
      // invoking the engine interface factory.
      r.stage=Stage::ClientStoreGlobals;
      if(!host_.write_u32(kClientGlobalsVA,globals_pointer))return r;
      ++r.writes_committed;
    }
    const auto manifest=original_manifest(role);
    std::uint32_t emitter=0u,material=0u;
    for(std::size_t i=0u;i<manifest.count;++i){
      const auto& q=manifest.queries[i];
      const auto factory=(q.factory==Factory::FileSystem)?
                           filesystem_factory:engine_factory;
      std::uint32_t ptr=0u;
      r.stage=Stage::FactoryQuery;
      r.failed_interface=q.name;
      r.failed_site_va=q.original_site_va;
      r.failed_storage_va=q.target_global_va;
      if(!host_.factory_call(q.factory,factory,q.name,ptr))return r;
      r.stage=Stage::StoreQueryResult;
      if(!host_.write_u32(q.target_global_va,ptr))return r;
      ++r.writes_committed;
      ++r.required_queries_completed;
      if(ptr==0u){
        r.code=Code::MissingRequiredInterface;
        return r;
      }
      // Both original routines invoke VSoundEmitter001 vtable slot zero
      // immediately after the required engine interface query prefix.
      if(q.target_global_va==(role==Role::Server?0x225721d0u:0x2433f6e4u))
        emitter=ptr;
      if(role==Role::Client&&q.target_global_va==0x2433f69cu)
        material=ptr;
    }
    bool success=false;
    r.stage=Stage::SoundEmitterSlotZero;
    r.emitter_object=emitter;
    r.failed_interface="VSoundEmitter001::vtable[0]";
    r.failed_site_va=role==Role::Server?0x221e77e6u:0x240fe320u;
    r.failed_storage_va=0u;
    if(!host_.sound_emitter_slot_zero(emitter,engine_factory,success))return r;
    if(!success){
      r.code=Code::MissingSoundEmitter;
      return r;
    }
    r.emitter_connected=true;
    if(role==Role::Server){
      // Original stores the fourth argument at 0x22572190 only after
      // successful sound-emitter slot-0 initialization.
      r.stage=Stage::ServerStoreGlobals;
      if(!host_.write_u32(kServerGlobalsVA,globals_pointer))return r;
      ++r.writes_committed;
    } else {
      // The original client's Stub factory query is optional. No branch
      // follows its EAX storage; null is not a failure in the native code.
      r.stage=Stage::OptionalClientMaterialStub;
      r.failed_interface="VMaterialSystemStub001";
      r.failed_site_va=0x240fe347u;
      r.failed_storage_va=0x2433f6a0u;
      std::uint32_t stub=0u;
      if(!host_.factory_call(Factory::Engine,engine_factory,
                              "VMaterialSystemStub001",stub))return r;
      r.stage=Stage::StoreOptionalStub;
      if(!host_.write_u32(0x2433f6a0u,stub))return r;
      ++r.writes_committed;
      r.material_stub_object=stub;

      // The hardware-config request is vtable slot +0x20 on the
      // previously acquired material system, never main engineFactory.
      r.stage=Stage::MaterialSystemSlotEight;
      r.failed_interface="MaterialSystemHardwareConfig012";
      r.failed_site_va=0x240fe367u;
      r.failed_storage_va=0x2433f6a4u;
      std::uint32_t hardware=0u;
      if(!host_.material_hardware_slot_eight(material,
                "MaterialSystemHardwareConfig012",hardware))return r;
      r.stage=Stage::StoreHardwareConfig;
      if(!host_.write_u32(0x2433f6a4u,hardware))return r;
      ++r.writes_committed;
      r.hardware_config_object=hardware;
      if(hardware==0u){
        r.code=Code::MissingHardwareConfig;
        return r;
      }
    }
    // Never claim that native Init returned true: original remaining engine
    // registration and gameplay initialization are still host-unmodeled.
    r.failed_interface=nullptr;r.failed_site_va=0u;r.failed_storage_va=0u;
    r.code=Code::PrefixAcquired;
  }catch(...){r.code=Code::HostRejected;}
  return r;
}
} // namespace sin_compat::original_module_init_phase658
