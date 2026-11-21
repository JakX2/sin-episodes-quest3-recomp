#pragma once
// SiN Episodes 2006: source-authenticated gameplay trigger behavior bridge.
// Independently authored portable C++17, NOT an engine-owned CBaseTrigger.
// Original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
// CTriggerDynamicDifficulty virtual slot89 at 0x221CCB70; CTriggerSecret
// virtual slot89 at 0x221CCC10; both player-gate via actor virtual +0x120
// and call original queued UTIL_Remove at 0x22286BF0.
//
// Real engine integration MUST create engine-owned CBaseTrigger subclasses,
// load original map keyvalues, install complete send/receive tables,
// trigger collision/touch, event manager and save/restore. Registering
// these portable callbacks alone is NOT a native Windows/Android game DLL.
#include "sin_compat/original_gameplay_entity_factory.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sin_compat::map_entities {

enum class TriggerKind : std::uint8_t { DynamicDifficulty, Secret };
enum class TriggerCode : std::uint8_t {
    Handled, NonPlayer, AlreadyConsumed, UnknownEntity, InvalidActor,
    InvalidConfig, UnknownKey, HostNotLive, HostRefused, EventUnavailable,
    ManagerRefused, RemovalPending, WrongClass, Reentrant
};
struct TriggerVec3 { float x=0, y=0, z=0; };

// Original CTriggerDynamicDifficulty DATADESC at 0x2250CCFC has three
// authentic map KeyValues: EstimatedTime FLOAT +0x470,
// EstimatedDifficulty INTEGER +0x474 and EndOfGame INTEGER +0x478.
// The event maps these to 'value' (f32), 'decision' (i32), then the
// manager flag on a nonzero LOW BYTE of EndOfGame. The actor origin is
// CBaseEntity +0x2BC. Host must still instantiate a real native entity.
struct TriggerConfig {
    std::string targetname{};
    TriggerVec3 origin{};
    float estimated_time=0;             // map key EstimatedTime, +0x470
    std::int32_t estimated_difficulty=0; // map key EstimatedDifficulty, +0x474
    std::int32_t end_of_game=0;          // map key EndOfGame, +0x478; low byte tested
    bool base_special_flag_0x800=false;    // original base field +0xB8
};

enum class TriggerEventName : std::uint8_t { DynamicDifficulty, Game };
struct TriggerEvent {
    TriggerEventName event=TriggerEventName::Game;
    int descriptor=0;
    int owner=-1;
    int advisor=-1;
    std::string_view name{};
    std::int32_t decision=0;
    float value=0;
    TriggerVec3 origin{};
    // The dynamic event contains descriptor, owner, advisor, name,
    // decision, value, x/y/z. Secret touch emits game descriptor=12,
    // secret Spawn emits game descriptor=11 if native flags+0x120 & 0x08.
    // Both secret event variants carry owner=-1 and name.
};

class IOriginalTriggerHost {
public:
    virtual ~IOriginalTriggerHost()=default;
    virtual bool is_live(EntityRef entity) const=0;
    // The original CTriggerSecret Spawn invokes CBaseTrigger::Spawn at
    // 0x221CFCF0 before sending its optional descriptor=11 event.
    // The portable adapter cannot manufacture this engine lifecycle.
    virtual bool initialize_native_base_trigger(EntityRef entity)=0;
    // Original touched entity's virtual +0x120 predicate.
    virtual bool is_player(EntityRef entity) const=0;
    // Original 0x22288A40: owner index for the dynamic event, or -1.
    virtual int current_difficulty_owner_index() const=0;
    // Source base helper 0x2213FA60. Host MUST reproduce side effects
    // if original special spawn flag is active. No silent no-op substitute.
    virtual bool perform_special_trigger_base_action(EntityRef trigger)=0;
    // The caller must synchronously publish the named typed game event.
    // A false return models original CreateEvent failure; original flow
    // still performs manager signaling and queues entity removal.
    virtual bool publish(const TriggerEvent& event)=0;
    // Original 0x222B93E0 writes manager byte at +0x11 to one.
    virtual bool mark_difficulty_manager()=0;
    // Must enqueue a real entity deletion, NOT free an engine allocation
    // synchronously while a Source callback is on the stack.
    virtual bool queue_remove(EntityRef entity)=0;
};

// Inputs observed by the native host from the original source/global
// state. Original CTriggerSecret::Spawn 0x221D0790 checks engine-global
// virtual +0x88 first, then calls base Spawn, and tests this+0x120
// spawnflags BIT 3 before publishing game event descriptor=11.
struct TriggerSpawnInput {
    bool engine_requests_removal=false;
    std::uint32_t native_flags_0x120=0;
};
struct TriggerSpawnReport {
    TriggerCode code=TriggerCode::UnknownEntity;
    bool base_initialized=false;
    bool event_published=false;
    bool removal_queued=false;
    bool handled() const noexcept {return code==TriggerCode::Handled;}
};

struct TriggerTouchReport {
    TriggerCode code=TriggerCode::UnknownEntity;
    bool event_published=false;
    bool manager_signaled=false;
    bool removal_queued=false;
    bool handled() const noexcept { return code==TriggerCode::Handled; }
};

class OriginalTriggerEntityFactory final {
public:
    explicit OriginalTriggerEntityFactory(IOriginalTriggerHost& host,
                                          std::size_t maximum=4096) noexcept;
    ~OriginalTriggerEntityFactory() noexcept;
    OriginalTriggerEntityFactory(const OriginalTriggerEntityFactory&)=delete;
    OriginalTriggerEntityFactory& operator=(const OriginalTriggerEntityFactory&)=delete;

    static bool supports(std::string_view classname) noexcept;
    RegistrationCode install(IGameEntityRegistrar& registrar);
    RegistrationCode uninstall() noexcept;
    bool installed() const noexcept { return installed_dynamic_ && installed_secret_; }
    std::size_t instance_count() const noexcept { return records_.size(); }

    // The actual engine creates and owns the entity. We only attach
    // original game behavior to the host's live, generation-safe identity.
    SpawnResult spawn(const SpawnRequest& request) noexcept;
    TriggerCode configure(EntityRef trigger,const TriggerConfig& config) noexcept;
    // Strict map-side adapter for the three original DATADESC external
    // KeyValues plus generic targetname. Reject unknown/invalid keys rather
    // than guessing other SiN properties. This strict parser is NEW code.
    TriggerCode apply_map_keyvalue(EntityRef trigger,std::string_view key,
                                   std::string_view text_value) noexcept;
    // Distinct native engine Spawn callback, NOT the host entity factory
    // allocation above. Source-backed CTriggerSecret path ONLY. The
    // CTriggerDynamicDifficulty Spawn path has additional behaviors.
    TriggerSpawnReport run_secret_spawn(
        EntityRef trigger,TriggerSpawnInput input) noexcept;
    TriggerTouchReport touch(EntityRef trigger,EntityRef actor) noexcept;
    TriggerCode retry_pending_remove(EntityRef trigger) noexcept;
    TriggerCode release(EntityRef trigger) noexcept;
private:
    struct Record {
        EntityRef ref{};
        TriggerKind kind=TriggerKind::DynamicDifficulty;
        TriggerConfig config{};
        bool consumed=false;
        bool pending_remove=false;
        bool native_spawn_completed=false;
    };
    static SpawnResult callback(void* userdata,
                                const SpawnRequest& request) noexcept;
    static bool valid(EntityRef e) noexcept;
    Record* find(EntityRef e) noexcept;
    class Guard {
    public:
        explicit Guard(bool& flag) noexcept: flag_(flag),owns_(!flag){
            if(owns_)flag_=true;
        }
        ~Guard() noexcept {if(owns_)flag_=false;}
        explicit operator bool() const noexcept{return owns_;}
    private:
        bool& flag_;
        bool owns_;
    };
    IOriginalTriggerHost& host_;
    std::size_t maximum_;
    std::unordered_map<std::uint32_t,Record> records_;
    IGameEntityRegistrar* registrar_=nullptr;
    bool installed_dynamic_=false;
    bool installed_secret_=false;
    bool busy_=false;
};

} // namespace sin_compat::map_entities
