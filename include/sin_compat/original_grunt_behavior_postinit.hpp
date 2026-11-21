#pragma once
// Phase535: original SiN 2006 CNPC_MobGrunt embedded behavior registration.
// This metadata/control-flow reference NEVER executes 2006 PE32 vtable pointers.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_druglab_grunt::behavior_postinit {

enum class Behavior : std::uint8_t {
    Rappel, FuncTank, Takecover, Beserk, ActBusy, Assault, Standoff, Follow
};

struct Spec {
    Behavior kind;
    std::string_view exact_rtti_name;
    std::uint32_t owner_offset;
    std::uint32_t original_constructor_va;
    std::uint32_t original_vtable_va;
};
// The actual eight insertions in 0x2235F6E0, NOT numerical owner-offset order.
inline constexpr std::array<Spec, 8> kOriginalBehaviors{{
    {Behavior::Rappel,   "CAI_RappelBehavior",    0xCFC, 0x220DEC20, 0x223F956C},
    {Behavior::FuncTank, "CAI_FuncTankBehavior",  0xCD8, 0x22393E00, 0x224ABEC4},
    {Behavior::Takecover,"CAI_TakecoverBehavior", 0xD20, 0x22399CD0, 0x224AE624},
    {Behavior::Beserk,   "CAI_BeserkBehavior",    0xDA0, 0x223925C0, 0x224AB88C},
    {Behavior::ActBusy,  "CAI_ActBusyBehavior",   0xF88, 0x22391BB0, 0x224AB494},
    {Behavior::Assault,  "CAI_AssaultBehavior",   0xDB8, 0x220D0C80, 0x223F5544},
    {Behavior::Standoff, "CGruntStandoffBehavior",0xDE8, 0x220E2850, 0x22495344},
    {Behavior::Follow,   "CAI_FollowBehavior",    0xEB8, 0x220D6690, 0x223F6B7C},
}};
inline constexpr std::uint32_t kGruntPrimaryVtable = 0x2249AAA4;
inline constexpr std::uint32_t kBehaviorInitVslot = 0x76C;
inline constexpr std::uint32_t kBehaviorRegistrationVA = 0x2235F6E0;
inline constexpr std::uint32_t kBehaviorArrayOffset = 0xC1C;
inline constexpr std::uint32_t kBehaviorArrayCountField = 0xC28;
inline constexpr std::uint32_t kBehaviorLinkOwnerSubobject = 0xC14;
inline constexpr std::uint32_t kBehaviorLinkFieldWithinBehavior = 0x0C;
inline constexpr std::uint32_t kStandoffBaseConstructorVtable = 0x223FA1B4;
inline constexpr std::uint32_t kStandoffDerivedVtable = 0x22495344;
inline constexpr std::uint32_t kSentenceKeyVslot = 0x7AC;
inline constexpr std::uint32_t kSentenceKeyMethodVA = 0x22353F80;
inline constexpr std::uint32_t kSentenceInitVA = 0x22358FA0;
inline constexpr std::uint32_t kEmbeddedSentenceStateOffset = 0xCC0;
inline constexpr std::uint32_t kSentenceObjectVtable = 0x22490BB0;
inline constexpr std::uint32_t kSentenceBindOwnerMethodVA = 0x2202E0F0;
inline constexpr std::uint32_t kSentenceOwnerPointerField = 0x04;
inline constexpr std::string_view kSentenceObjectClass = "CAI_Sentence<CNPC_GruntBase>";
inline constexpr std::string_view kOriginalSentenceKey = "NPC_Grunt.SentenceParameters";

enum class Operation : std::uint8_t {
    None, AppendToCollection, BindNPC, StoreOwnerSubobject,
    BindSentenceOwner, InitializeSentenceParameters
};
enum class Code : std::uint8_t { Success, HostRefused, Reentrant };
struct Result {
    Code code=Code::HostRefused;
    Operation failed_operation=Operation::None;
    std::uint8_t behavior_index=0; // 0..7 for behavior stages, 8 for sentence stage
    std::uint8_t registered_count=0;
    bool ok() const noexcept {return code==Code::Success;}
};

class IHost {
public:
    virtual ~IHost()=default;
    // Host must create/own actual behavior instances separately. This model
    // only reconstructs original registration order, not their constructors.
    virtual bool append_to_collection(Spec behavior,
                                     std::uint32_t list_offset,
                                     std::uint32_t count_field_offset)=0;
    // Original behavior's virtual +0x04 stores owner at behavior+0x04.
    virtual bool bind_to_npc(Spec behavior)=0;
    // Original stores owner+0xC14 at the behavior's +0x0C field.
    virtual bool set_owner_subobject_reference(Spec behavior,
                                              std::uint32_t npc_subobject_offset,
                                              std::uint32_t behavior_field_offset)=0;
    // Original 0x22358FA0 calls embedded CAI_Sentence virtual +0x04,
    // setting member+0x04 to the owner before any configuration read.
    virtual bool bind_sentence_owner(std::uint32_t embedded_offset,
                                     std::uint32_t sentence_owner_field)=0;
    // Original 0x22358FA0 runs configuration code with the vslot+0x7AC key.
    // Host, not this reference, interprets parameters/environment globals.
    virtual bool initialize_sentence_parameters(std::uint32_t embedded_offset,
                                                std::string_view exact_key)=0;
};

class Registrar final {
public:
    explicit Registrar(IHost& host) noexcept : host_(host) {}
    Registrar(const Registrar&)=delete;
    Registrar& operator=(const Registrar&)=delete;
    Result initialize() noexcept;
private:
    IHost& host_;
    bool busy_=false;
};
} // namespace sin_compat::original_druglab_grunt::behavior_postinit