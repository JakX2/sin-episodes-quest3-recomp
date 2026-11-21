#pragma once
// Phase534: original 2006 SiN server.dll NPC post-construction AI dependencies.
// This is a host-driven control-flow reference, not a real Source CBaseNPC.
// Engine-owned CAI_* types and pointer ownership must come from the host.
#include <array>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_druglab_grunt::ai_init {

enum class Component : std::uint8_t {
    Senses, BlendedMotor, LocalNavigator, MoveProbe,
    Navigator, Pathfinder, TacticalServices
};
struct ComponentSpec {
    Component kind;
    std::string_view rtti_name;
    std::uint32_t virtual_byte_offset;
    std::uint32_t owner_field_offset;
    std::uint32_t initial_allocation_bytes;
    std::uint32_t original_factory_va;
};
// Explicitly in source execution order, NOT sorted by vtable or owner offset.
inline constexpr std::array<ComponentSpec,7> kOriginalComponents{{
    {Component::Senses,           "CAI_Senses",           0x3FC,0x898,0x78,0x22048F90},
    {Component::BlendedMotor,     "CAI_BlendedMotor",     0x404,0x908,0xF0,0x2216E9C0},
    {Component::LocalNavigator,   "CAI_LocalNavigator",   0x408,0x8FC,0xD4,0x22031ED0},
    {Component::MoveProbe,        "CAI_MoveProbe",        0x400,0x904,0x44,0x22031EA0},
    {Component::Navigator,        "CAI_Navigator",        0x40C,0x8F8,0x9C,0x2216DDD0},
    {Component::Pathfinder,       "CAI_Pathfinder",       0x410,0x900,0x18,0x220379F0},
    {Component::TacticalServices, "CAI_TacticalServices", 0x414,0x9D8,0x10,0x22031F10},
}};
inline constexpr std::uint32_t kOriginalInitializerVA=0x22031D50;
inline constexpr std::uint32_t kSharedContextGlobalVA=0x225490C8;
inline constexpr std::uint32_t kEmbeddedOwnerRelayOffset=0x9E4;

enum class Stage : std::uint8_t {
    None, Senses, BlendedMotor, LocalNavigator, MoveProbe,
    Navigator, Pathfinder, TacticalServices,
    OwnerRelay, MotorToLocalNavigator, LocalNavigatorToNavigator,
    NavigatorContext, PathfinderContext, TacticalServicesContext
};
enum class Code : std::uint8_t { Success, HostRefused, Reentrant };
struct Result {
    Code code=Code::HostRefused;
    Stage failed_stage=Stage::None;
    std::uint8_t created_count=0;
    bool wiring_completed=false;
    bool ok() const noexcept {return code==Code::Success;}
};

class IHost {
public:
    virtual ~IHost()=default;
    // The true native virtual creates an engine-owned CAI_* instance and
    // stores a pointer at this+owner_field_offset. Return false for NULL.
    // No original 0x22... address may be called as an ARM64 function.
    virtual bool construct_and_store(ComponentSpec spec)=0;
    // Original +0x9E4 embedded-member virtual +0x04 with owner argument.
    virtual bool wire_embedded_owner_relay()=0;
    // Original +0x908 method 0x2206BF10 receives +0x8FC component+8.
    virtual bool wire_motor_to_local_navigator()=0;
    // Original +0x8FC method 0x220674D0 receives +0x8F8 component+8.
    virtual bool wire_local_navigator_to_navigator()=0;
    // Both these and TacticalServices receive the same original pointer
    // loaded from global 0x225490C8. Semantic meaning remains unproven.
    virtual bool wire_navigator_shared_context()=0;
    virtual bool wire_pathfinder_shared_context()=0;
    virtual bool wire_tactical_services_shared_context()=0;
};

class Initializer final {
public:
    explicit Initializer(IHost& host) noexcept:host_(host){}
    Initializer(const Initializer&)=delete;
    Initializer& operator=(const Initializer&)=delete;
    Result initialize() noexcept;
private:
    IHost& host_;
    bool busy_=false;
};
} // namespace sin_compat::original_druglab_grunt::ai_init