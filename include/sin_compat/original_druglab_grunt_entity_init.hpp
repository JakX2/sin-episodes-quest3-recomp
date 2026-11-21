#pragma once
// Phase533: source-verified CDruglabGrunt creation AFTER the native factory
// installs its real vptrs. Original server.dll:
//
// Virtual +0x70 -> 0x22031B10 (SHA256 8b4c539f3d96b95df7c70b4918c8af0d442635b4ac078a8c6e3b51b123c1a6ad)
// calls base +0x70 method 0x2213B140 (SHA256 46d43167fb58428dafe3d886f42c4f38004c214adfb5268bd87cd50672febe7c)
// then native virtual +0x3F8 -> 0x22359210 (SHA256 848486f30a6c0d1bf820eecbb544c451f91a362070ca3693f500165b74fbfbc6).
// The engine-edict binding helper is 0x2227F070 (53 original bytes).
//
// This does NOT implement a CBaseNPC/edict in memory. It requires a real
// Source-compatible engine host to own classname symbols, edicts,
// networkable vtables, handles and NPC schedule interfaces.
// Never invoke original x86 virtual function VA on ARM64 or a new PE32 DLL.
#include <cstdint>
#include <string_view>

namespace sin_compat::original_druglab_grunt::entity_init {

enum class Stage:std::uint8_t {
    None, ClassnameSymbol, SpecialBit9, AttachNetworkable,
    ClearEdictContext, BacklinkEdict, BaseInitBF30, BaseInitB280,
    GruntPostInit3F8
};
enum class Code:std::uint8_t { Applied, HostRefused, Reentrant };
struct Input {
    // Original native factory passes a nullable classname input to
    // +0x70. A nonnull empty string is different from a null arg.
    bool has_classname_argument=true;
    std::string_view classname{};
    // Original entity field +0xB8 bit9/bit10 controls edict attach.
    std::uint32_t native_flags_B8=0;
    // Original private global +0x22558764 is either an already allocated
    // edict or null. If null, native engine virtual +0x54(-1) allocates one.
    bool factory_edict_context_present=false;
};
struct Result {
    Code code=Code::HostRefused;
    Stage failed_stage=Stage::None;
    bool special_bit9=false;
    bool skipped_edict_attachment=false;
    bool attachment_invoked=false;
    bool edict_backlink_written=false;
    bool ok() const noexcept{return code==Code::Applied;}
};

class IHost {
public:
    virtual ~IHost()=default;
    // Original 0x2213B151 string-symbol interner + owner field +0x0C.
    virtual bool intern_native_classname(std::string_view classname)=0;
    // Original 0x22164460 special path when field+0xB8 bit9 set.
    virtual bool special_bit9_native_registration()=0;
    // Original 0x2227F070 acts on networkable subobject owner+0x160,
    // using existing global edict when supplied, otherwise engine
    // CreateEdict(-1) virtual +0x54. Native host MUST enforce ownership.
    virtual bool attach_native_networkable(bool context_present)=0;
    // Original 0x2213B1AB clears global +0x22558764 after networkable
    // attach; this independent port additionally clears it on errors.
    virtual bool clear_original_factory_edict_context()=0;
    // Read original owner+0x16C after attach (or on skip-bit10 path).
    virtual bool has_live_native_edict() const=0;
    // Original engine vslot +0x48 plus helper 0x22164440 and
    // edict field +0x08 = owner+0x160 networkable subobject.
    virtual bool link_edict_networkable_backpointer()=0;
    virtual bool base_initialize_2214BF30_zero()=0;
    virtual bool base_initialize_2214B280()=0;
    // Original +0x70 ends by dispatching vslot +0x3F8. The original
    // return value is ignored here; independent host refusal is a
    // deliberate fail-closed strategy before declaring initialization.
    virtual bool invoke_real_grunt_postinit_slot3F8()=0;
};
class EntityInitializer final {
public:
    explicit EntityInitializer(IHost& host) noexcept:host_(host){}
    EntityInitializer(const EntityInitializer&)=delete;
    EntityInitializer& operator=(const EntityInitializer&)=delete;
    Result initialize(const Input& input) noexcept;
private:
    IHost& host_;
    bool busy_=false;
};
} // namespace sin_compat::original_druglab_grunt::entity_init
