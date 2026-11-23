#pragma once
// SiN Episodes: Emergence 2006 original server.dll, complete 232-byte
// Elite primary vtable +0x44C event handler at 0x223556A0.
// Whole native method SHA256:
// 66a3abbe73c87aaa6ea5f7c9c97547b99a4c2fe6e626c6bb1f77c1a8a8c60262
// Independent C++17 host-side reconstruction, NOT a callable engine ABI.
// Original x86 uses an event pointer at [ESP+4] at entry; its first DWORD
// is the event ID. This model receives that ID from a verified host.
#include <cstdint>
namespace sin_compat::original_elite_secondary_event_phase632 {
inline constexpr std::uint32_t kOriginalVA=0x223556A0u;
inline constexpr std::uint32_t kVtableVA=0x2248DF04u;
inline constexpr std::uint32_t kVtableOffset=0x44Cu;
inline constexpr std::uint32_t kInheritedVA=0x2235CCE0u;
inline constexpr std::uint32_t kGateVA=0x22315320u;
inline constexpr std::uint32_t kEvent43=0x43u;
inline constexpr std::uint32_t kEvent47=0x47u;
inline constexpr std::uint32_t kInvalidHandle=0xffffffffu;
enum class Stage:std::uint8_t {
  None,
  ReadElitePredicateFD9, ReadEliteHandleFDC, OpenSerialTable,
  ResolveSerial, Gate43,
  Predicate47, FirstObjectLookup47, SecondObjectLookup47,
  Virtual404OnSecond47, ThirdObjectLookup47, Component4EC47,
  NumberedCallback3, NumberedCallback4, NumberedCallback5,
  ActionZero, InheritedDispatch
};
enum class Code:std::uint8_t {
  Skipped, Inherited, ActionZero, HostRejected,
  OriginalNullTargetHazard, Reentrant
};
struct Result {
  Code code=Code::HostRejected;
  Stage stage=Stage::None;
  bool gate_called=false;
  bool inherited_called=false;
  bool action_zero_called=false;
  unsigned object_lookups=0u;
  unsigned numbered_callbacks=0u;
};
class IHost {
public:
  virtual ~IHost()=default;
  // Original 0x43 only reads Elite byte this+0xFD9, never writes it here.
  virtual bool read_elite_predicate_fd9(std::uint8_t& value)=0;
  // Re-read Elite this+0xFDC in the native 0x43 path.
  virtual bool read_elite_serial_handle_fdc(std::uint32_t& value)=0;
  // The exact original serial table base is 0x224F5620; no invented
  // CBaseHandle ABI or entity class is exposed by these callbacks.
  virtual bool open_original_serial_table()=0;
  // 1=matching live serial (output may still be null), 0=stale/missing,
  // -1=host error. The original 0x43 gate would dereference a null ECX
  // on missing/stale handle; the reference refuses instead.
  virtual int resolve_original_serial(
      std::uint32_t handle,std::uint64_t& target)=0;
  // 2006 gate 0x22315320, already independently modeled in Phase629/630.
  virtual bool original_gate43(std::uint64_t target,bool& allowed)=0;
  // Exact original call at 0x223556BA for all other events and for 0x43
  // when FD9 is nonzero. No guess about the event's C++ class/layout.
  virtual bool inherited_event_2235CCE0(std::uint32_t event_id)=0;
  // Native 0x47 chain. Original names/semantics not proven, keep AV
  // addresses and virtual slot numbers instead of inventing engine roles.
  virtual bool predicate_220326D0(bool& allowed)=0;
  // The original does not retain the first pointer and performs up to
  // three distinct 0x222AA640 calls. An absent second or third pointer
  // would be dereferenced by native code; model fails closed.
  virtual bool fetch_222AA640(std::uint64_t& object)=0;
  virtual bool object_virtual_404(
      std::uint64_t second_object,std::uint32_t& out_value)=0;
  virtual bool component_4EC_call_2203F8E0(
      std::uint64_t third_object,std::uint32_t value)=0;
  virtual bool call_220398A0(unsigned command)=0;
  virtual bool action_zero_2203D2D0()=0;
};
class Reference final {
public:
  explicit Reference(IHost& host)noexcept:host_(host){}
  Result dispatch(std::uint32_t original_event_id)noexcept;
  Reference(const Reference&)=delete;
  Reference& operator=(const Reference&)=delete;
private:
  IHost& host_;
  bool busy_=false;
};
} // namespace sin_compat::original_elite_secondary_event_phase632
