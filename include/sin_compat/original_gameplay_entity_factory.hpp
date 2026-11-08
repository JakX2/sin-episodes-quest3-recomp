#pragma once
// Independently authored gameplay-entity factory seam for native Quest/Source
// hosts. Original SiN Episodes 2006 server.dll registered these exact names:
// weapon_assault_rifle at x86 VA 0x223C4F60 (factory 0x2252D4FC), and
// grenade_ar at x86 VA 0x223C4FC0 (factory 0x2252D500).
//
// This factory actually constructs portable WeaponSession and CGrenadeAR
// behaviors, instead of installing arbitrary classname aliases. A native
// host MUST still implement compatible SourceVR game module/entity factories,
// server startup and actual map data/lifetime; this is NOT IServerGameDLL.
#include "sin_compat/original_ar_grenade_runtime.hpp"
#include "sin_compat/original_ar_grenade_touch.hpp"
#include "sin_compat/weapon_session.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace sin_compat::map_entities {

enum class Kind : std::uint8_t {
    AssaultRifle, ARGrenade
};

enum class SpawnCode : std::uint8_t {
    Created,
    UnknownOrUnimplementedClass,
    InvalidEntity,
    HostEntityNotLive,
    DuplicateIndex,
    InvalidProjectileKinematics,
    HostRejected,
    AlreadyBound,
    WrongClass,
    NotFound,
    Reentrant,
    CapacityExceeded,
    AllocationFailed
};

struct SpawnResult {
    SpawnCode code = SpawnCode::UnknownOrUnimplementedClass;
    Kind kind = Kind::AssaultRifle;
    bool ok() const noexcept { return code == SpawnCode::Created; }
};

struct SpawnRequest {
    std::string_view classname;
    EntityRef entity{};  // HOST-owned generation-safe entity identity
    ar_grenade::Vec3 origin{};
    ar_grenade::Vec3 velocity{};
    float game_time_seconds = 0.0f;
};

// These callbacks operate on an engine-assigned, LIVE entity identity. The
// native engine host must publish a correct engine factory that creates a
// native entity *first*, then calls this portable behavior constructor.
// These callbacks alone cannot make SourceVR recognize a classname.
using SpawnCallback = SpawnResult (*)(void*, const SpawnRequest&) noexcept;

class IGameEntityRegistrar {
public:
    virtual ~IGameEntityRegistrar() = default;
    virtual bool install(std::string_view classname, SpawnCallback callback,
                         void* context) = 0;
    virtual bool uninstall(std::string_view classname,
                           SpawnCallback callback, void* context) = 0;
};

class IGameplayEntityHost : public IWeaponSessionHost {
public:
    ~IGameplayEntityHost() override = default;
    // Existing original setter 0x222B2B40 mutates +0x795 and has additional
    // network side effects. The future native host must reimplement them.
    // Returning false prevents gameplay dispatch, never assumes success.
    virtual bool write_original_grenade_contact_flag(EntityRef grenade,
                                                      bool value) = 0;
};

enum class RegistrationCode : std::uint8_t {
    Installed,
    AlreadyInstalled,
    HostRejected,
    PartialRollbackFailed,
    NotInstalled,
    Uninstalled,
    Reentrant
};

struct TouchResult {
    SpawnCode code = SpawnCode::NotFound;
    ar_grenade_touch::Evaluation evaluation{};
    ar_grenade::StepResult projectile_step = ar_grenade::StepResult::TriggerRejected;
    bool dispatched() const noexcept {
        return code == SpawnCode::Created &&
               projectile_step == ar_grenade::StepResult::Applied;
    }
};

class GameplayEntityFactory final {
public:
    explicit GameplayEntityFactory(IGameplayEntityHost& host,
                                   std::size_t max_instances = 8192) noexcept;
    ~GameplayEntityFactory() noexcept;
    GameplayEntityFactory(const GameplayEntityFactory&) = delete;
    GameplayEntityFactory& operator=(const GameplayEntityFactory&) = delete;

    // The host installs these callbacks in its *own* correctly implemented
    // entity registry. If either registration fails, no dummy class is
    // substituted. Revoke before destroying the host; host must outlive us.
    RegistrationCode install(IGameEntityRegistrar& registrar);
    RegistrationCode uninstall() noexcept;
    bool installed() const noexcept { return rifle_installed_ && grenade_installed_; }

    // Only two BEHAVIOR-BACKED original names are currently creatable. Others
    // from the five map logs, including trigger_dynamic_difficulty and the
    // grunt NPC variants, fail closed, even though original DLLs register them.
    static bool supports(std::string_view classname) noexcept;

    // Creating a weapon in the world is valid without any current player.
    // This produces a real portable gameplay instance, not a dummy engine
    // prop. Bind its WeaponSession only after the engine assigns an owner.
    SpawnResult spawn(const SpawnRequest& request) noexcept;
    SpawnCode bind_weapon(EntityRef weapon, EntityRef owner,
                          bool vr_mode = true) noexcept;
    SpawnCode unbind_weapon(EntityRef weapon) noexcept;
    SpawnCode remove(EntityRef entity) noexcept;

    WeaponSession* weapon(EntityRef entity) noexcept;
    const WeaponSession* weapon(EntityRef entity) const noexcept;
    ar_grenade::Runtime* grenade(EntityRef entity) noexcept;
    const ar_grenade::Runtime* grenade(EntityRef entity) const noexcept;
    bool original_grenade_touch_flag(EntityRef entity) const noexcept;
    std::size_t instance_count() const noexcept { return instances_.size(); }

    // Contacts only dispatch when the original raw +0x11C, +0x120,
    // +0xFC-reference and +0x795 gates pass. The host supplies related-
    // object observations but cannot override our recorded original
    // grenade flag. Setter/network side effects must succeed FIRST.
    TouchResult grenade_touch(EntityRef grenade_entity,
        const ar_grenade_touch::OriginalTouchObservation& contact,
        ar_grenade::Decal selected_decal,
        ar_grenade::IHost& blast_host) noexcept;

private:
    struct Instance {
        EntityRef ref{};
        Kind kind = Kind::AssaultRifle;
        std::unique_ptr<WeaponSession> weapon_session{};
        ar_grenade::Runtime grenade_runtime{};
        bool original_grenade_field_795 = false;
    };
    class Guard {
    public:
        explicit Guard(bool& busy) noexcept : flag_(busy), entered_(!busy) {
            if (entered_) flag_ = true;
        }
        ~Guard() noexcept { if (entered_) flag_ = false; }
        explicit operator bool() const noexcept { return entered_; }
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
    private:
        bool& flag_;
        bool entered_;
    };
    static SpawnResult callback(void*, const SpawnRequest&) noexcept;
    static bool valid(EntityRef) noexcept;
    Instance* find(EntityRef) noexcept;
    const Instance* find(EntityRef) const noexcept;

    IGameplayEntityHost& host_;
    std::unordered_map<std::uint32_t, std::unique_ptr<Instance>> instances_;
    std::size_t max_instances_;
    IGameEntityRegistrar* registrar_ = nullptr;
    bool rifle_installed_ = false;
    bool grenade_installed_ = false;
    bool busy_ = false;
};

} // namespace sin_compat::map_entities
