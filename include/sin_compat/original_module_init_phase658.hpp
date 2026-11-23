#pragma once
// Bounded source reconstruction of original SiN Episodes 2006 engine-interface
// acquisition prefix. The *rest* of GameDLLInit / IBaseClientDLL::Init is not
// implemented. Entire retail x86 method bodies audited separately in Phase658.
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_module_init_phase658 {
inline constexpr std::uint32_t kServerInitVA = 0x221e7640u;
inline constexpr std::uint32_t kServerInitBytes = 875u;
inline constexpr std::uint32_t kClientInitVA = 0x240fe0e0u;
inline constexpr std::uint32_t kClientInitBytes = 1220u;
inline constexpr std::uint32_t kServerGlobalsVA = 0x22572190u;
inline constexpr std::uint32_t kClientGlobalsVA = 0x242fa458u;

enum class Role : std::uint8_t { Server, Client };
enum class Factory : std::uint8_t { Engine, FileSystem };
struct QuerySpec {
  const char* name;
  std::uint32_t original_site_va;
  std::uint32_t original_name_va;
  std::uint32_t target_global_va;
  Factory factory;
};
struct Manifest {
  const QuerySpec* queries;
  std::size_t count;
};
Manifest original_manifest(Role role) noexcept;

enum class Code : std::uint8_t {
  PrefixAcquired, MissingRequiredInterface, MissingSoundEmitter,
  MissingHardwareConfig, HostRejected, InvalidArgument, Reentrant
};
enum class Stage : std::uint8_t {
  None, ClientStoreGlobals, FactoryQuery, StoreQueryResult,
  SoundEmitterSlotZero, OptionalClientMaterialStub,
  StoreOptionalStub, MaterialSystemSlotEight, StoreHardwareConfig,
  ServerStoreGlobals
};
struct Result {
  Code code = Code::HostRejected;
  Stage stage = Stage::None;
  Role role = Role::Server;
  const char* failed_interface = nullptr;
  std::uint32_t failed_site_va = 0;
  std::uint32_t failed_storage_va = 0;
  std::uint32_t required_queries_completed = 0;
  std::uint32_t writes_committed = 0;
  std::uint32_t emitter_object = 0;
  std::uint32_t material_stub_object = 0;
  std::uint32_t hardware_config_object = 0;
  bool emitter_connected = false;
  bool original_init_tail_not_executed = true;
};
class IHost {
public:
  virtual ~IHost() = default;
  // Factory addresses are guest ABI function pointers. A null result denotes
  // the original engine's missing-version response, not a C++ host error.
  virtual bool factory_call(Factory provenance, std::uint32_t factory_va,
      const char* exact_version, std::uint32_t& interface_object_va) = 0;
  virtual bool write_u32(std::uint32_t guest_va, std::uint32_t value) = 0;
  // Native call through vtable slot 0 of VSoundEmitter001, passing engine factory.
  virtual bool sound_emitter_slot_zero(std::uint32_t emitter_va,
      std::uint32_t engine_factory_va, bool& al_success) = 0;
  // Native call through VMaterialSystem076 vtable byte offset +0x20 (slot 8).
  // The exact engine method prototype remains a typed external dependency.
  virtual bool material_hardware_slot_eight(std::uint32_t material_system_va,
      const char* exact_name, std::uint32_t& result_object_va) = 0;
};
class AcquirePrefix final {
public:
  explicit AcquirePrefix(IHost& host) noexcept : host_(host) {}
  // Original first stack arg = engineFactory. Server uses the third stack
  // arg as filesystemFactory and fourth as global-vars pointer. Client uses
  // third as global-vars pointer and does not use filesystemFactory.
  Result run(Role role, std::uint32_t engine_factory,
             std::uint32_t filesystem_factory,
             std::uint32_t globals_pointer) noexcept;
private:
  IHost& host_;
  bool busy_ = false;
};
} // namespace sin_compat::original_module_init_phase658
