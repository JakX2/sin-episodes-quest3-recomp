#include "sin_compat/original_gameplay_entity_factory.hpp"
#include "sin_compat/original_assault_rifle_profile.hpp"

#include <exception>
#include <limits>
#include <new>
#include <utility>

namespace sin_compat::map_entities {
namespace {
constexpr std::string_view kRifle = "weapon_assault_rifle";
constexpr std::string_view kGrenade = "grenade_ar";
}

GameplayEntityFactory::GameplayEntityFactory(IGameplayEntityHost& host,
                                             std::size_t maximum) noexcept
    : host_(host), max_instances_(maximum) {}

GameplayEntityFactory::~GameplayEntityFactory() noexcept {
    // Host lifetime contract: registrar and entity host MUST outlive bridge.
    // Never silently retain callbacks which point at a destroyed bridge.
    (void)uninstall();
}

bool GameplayEntityFactory::valid(EntityRef e) noexcept {
    return e.index != std::numeric_limits<std::uint32_t>::max() &&
           e.generation != std::numeric_limits<std::uint32_t>::max();
}

bool GameplayEntityFactory::supports(std::string_view name) noexcept {
    return name == kRifle || name == kGrenade;
}

SpawnResult GameplayEntityFactory::callback(void* userdata,
                                            const SpawnRequest& request) noexcept {
    if (!userdata) return {SpawnCode::HostRejected,Kind::AssaultRifle};
    try {
        return static_cast<GameplayEntityFactory*>(userdata)->spawn(request);
    } catch (...) {
        return {SpawnCode::HostRejected,Kind::AssaultRifle};
    }
}

RegistrationCode GameplayEntityFactory::install(IGameEntityRegistrar& registrar) {
    Guard guard(busy_);
    if (!guard) return RegistrationCode::Reentrant;
    if (registrar_ || rifle_installed_ || grenade_installed_)
        return RegistrationCode::AlreadyInstalled;
    try {
        if (!registrar.install(kRifle,callback,this))
            return RegistrationCode::HostRejected;
        registrar_=&registrar;
        rifle_installed_=true;
        if (registrar.install(kGrenade,callback,this)) {
            grenade_installed_=true;
            return RegistrationCode::Installed;
        }
        if (registrar.uninstall(kRifle,callback,this)) {
            rifle_installed_=false;
            registrar_=nullptr;
            return RegistrationCode::HostRejected;
        }
        // A failed unregister is retained so an explicit subsequent
        // uninstall() can try again instead of assuming callbacks vanished.
        return RegistrationCode::PartialRollbackFailed;
    } catch (...) {
        if (registrar_ && rifle_installed_) {
            try {
                if (registrar.uninstall(kRifle,callback,this)) {
                    rifle_installed_=false;
                    registrar_=nullptr;
                    return RegistrationCode::HostRejected;
                }
            } catch (...) {}
            return RegistrationCode::PartialRollbackFailed;
        }
        return RegistrationCode::HostRejected;
    }
}

RegistrationCode GameplayEntityFactory::uninstall() noexcept {
    Guard guard(busy_);
    if (!guard) return RegistrationCode::Reentrant;
    if (!registrar_) return RegistrationCode::NotInstalled;
    bool ok=true;
    // Revoke both names, even if the first one fails, so the host can
    // retry a partial teardown; don't pretend the callbacks disappeared.
    try {
        if (grenade_installed_) {
            if (registrar_->uninstall(kGrenade,callback,this))
                grenade_installed_=false;
            else ok=false;
        }
        if (rifle_installed_) {
            if (registrar_->uninstall(kRifle,callback,this))
                rifle_installed_=false;
            else ok=false;
        }
    } catch (...) { ok=false; }
    if (!rifle_installed_ && !grenade_installed_) registrar_=nullptr;
    return ok ? RegistrationCode::Uninstalled
              : RegistrationCode::PartialRollbackFailed;
}

GameplayEntityFactory::Instance* GameplayEntityFactory::find(
    EntityRef e) noexcept {
    if (!valid(e)) return nullptr;
    const auto it=instances_.find(e.index);
    if (it==instances_.end() || it->second->ref!=e) return nullptr;
    return it->second.get();
}

const GameplayEntityFactory::Instance* GameplayEntityFactory::find(
    EntityRef e) const noexcept {
    if (!valid(e)) return nullptr;
    const auto it=instances_.find(e.index);
    if (it==instances_.end() || it->second->ref!=e) return nullptr;
    return it->second.get();
}

SpawnResult GameplayEntityFactory::spawn(const SpawnRequest& request) noexcept {
    Guard guard(busy_);
    if (!guard) return {SpawnCode::Reentrant,Kind::AssaultRifle};
    const Kind kind=(request.classname==kGrenade)
        ? Kind::ARGrenade : Kind::AssaultRifle;
    if (!supports(request.classname))
        return {SpawnCode::UnknownOrUnimplementedClass,kind};
    if (!valid(request.entity))
        return {SpawnCode::InvalidEntity,kind};
    // An index reused with a new generation must be released first. No
    // Source EHANDLE bit-layout assumptions or silent old-state replacement.
    if (instances_.find(request.entity.index)!=instances_.end())
        return {SpawnCode::DuplicateIndex,kind};
    if (instances_.size()>=max_instances_)
        return {SpawnCode::CapacityExceeded,kind};
    try {
        if (!host_.is_live(request.entity))
            return {SpawnCode::HostEntityNotLive,kind};
        auto instance=std::make_unique<Instance>();
        instance->ref=request.entity;
        instance->kind=kind;
        if (kind==Kind::ARGrenade &&
            !instance->grenade_runtime.spawn(
                request.origin,request.velocity,request.game_time_seconds))
            return {SpawnCode::InvalidProjectileKinematics,kind};
        // An unowned world weapon is a legitimate map pickup. It receives
        // a WeaponSession only after the host binds a live player owner.
        instances_.emplace(request.entity.index,std::move(instance));
        return {SpawnCode::Created,kind};
    } catch (const std::bad_alloc&) {
        return {SpawnCode::AllocationFailed,kind};
    } catch (...) {
        return {SpawnCode::HostRejected,kind};
    }
}

SpawnCode GameplayEntityFactory::bind_weapon(EntityRef weapon_entity,
                                              EntityRef owner,bool vr_mode) noexcept {
    Guard guard(busy_);
    if (!guard) return SpawnCode::Reentrant;
    auto* instance=find(weapon_entity);
    if (!instance) return SpawnCode::NotFound;
    if (instance->kind!=Kind::AssaultRifle) return SpawnCode::WrongClass;
    if (instance->weapon_session) return SpawnCode::AlreadyBound;
    if (!valid(owner)) return SpawnCode::InvalidEntity;
    try {
        if (!host_.is_live(weapon_entity) || !host_.is_live(owner) ||
            host_.owner_of(weapon_entity)!=owner ||
            !host_.is_player_owner(owner))
            return SpawnCode::HostRejected;
        instance->weapon_session=std::make_unique<WeaponSession>(
            host_,owner,weapon_entity,WeaponKind::AssaultRifle,
            OriginalAssaultRifle::make_runtime_config(),vr_mode);
        return SpawnCode::Created;
    } catch (const std::bad_alloc&) {
        return SpawnCode::AllocationFailed;
    } catch (...) {
        return SpawnCode::HostRejected;
    }
}

SpawnCode GameplayEntityFactory::unbind_weapon(EntityRef weapon_entity) noexcept {
    Guard guard(busy_);
    if (!guard) return SpawnCode::Reentrant;
    auto* instance=find(weapon_entity);
    if (!instance) return SpawnCode::NotFound;
    if (instance->kind!=Kind::AssaultRifle) return SpawnCode::WrongClass;
    if (!instance->weapon_session) return SpawnCode::NotFound;
    try {
        instance->weapon_session->on_player_cleanup();
        instance->weapon_session.reset();
        return SpawnCode::Created;
    } catch (...) {
        return SpawnCode::HostRejected;
    }
}

SpawnCode GameplayEntityFactory::remove(EntityRef entity) noexcept {
    Guard guard(busy_);
    if (!guard) return SpawnCode::Reentrant;
    if (!find(entity)) return SpawnCode::NotFound;
    try {
        auto it=instances_.find(entity.index);
        if (it->second->weapon_session)
            it->second->weapon_session->on_player_cleanup();
        instances_.erase(it);
        return SpawnCode::Created;
    } catch (...) {
        return SpawnCode::HostRejected;
    }
}

WeaponSession* GameplayEntityFactory::weapon(EntityRef e) noexcept {
    auto* entry=find(e);
    return entry && entry->kind==Kind::AssaultRifle
        ? entry->weapon_session.get():nullptr;
}
const WeaponSession* GameplayEntityFactory::weapon(EntityRef e) const noexcept {
    auto* entry=find(e);
    return entry && entry->kind==Kind::AssaultRifle
        ? entry->weapon_session.get():nullptr;
}
ar_grenade::Runtime* GameplayEntityFactory::grenade(EntityRef e) noexcept {
    auto* entry=find(e);
    return entry && entry->kind==Kind::ARGrenade
        ? &entry->grenade_runtime:nullptr;
}
const ar_grenade::Runtime* GameplayEntityFactory::grenade(EntityRef e) const noexcept {
    auto* entry=find(e);
    return entry && entry->kind==Kind::ARGrenade
        ? &entry->grenade_runtime:nullptr;
}
bool GameplayEntityFactory::original_grenade_touch_flag(EntityRef e) const noexcept {
    auto* entry=find(e);
    return entry && entry->kind==Kind::ARGrenade &&
           entry->original_grenade_field_795;
}

TouchResult GameplayEntityFactory::grenade_touch(
    EntityRef grenade_entity,
    const ar_grenade_touch::OriginalTouchObservation& contact,
    ar_grenade::Decal decal, ar_grenade::IHost& blast_host) noexcept {
    Guard guard(busy_);
    if (!guard) return {SpawnCode::Reentrant,{},ar_grenade::StepResult::TriggerRejected};
    auto* entry=find(grenade_entity);
    if (!entry) return {SpawnCode::NotFound,{},ar_grenade::StepResult::TriggerRejected};
    if (entry->kind!=Kind::ARGrenade)
        return {SpawnCode::WrongClass,{},ar_grenade::StepResult::TriggerRejected};

    try {
        if (!host_.is_live(grenade_entity))
            return {SpawnCode::HostEntityNotLive,{},
                    ar_grenade::StepResult::TriggerRejected};
        auto observed=contact;
        // Host cannot trick us by setting this field in the observation.
        observed.grenade_original_field_795=entry->original_grenade_field_795;
        const auto decision=ar_grenade_touch::evaluate(observed);
        if (decision.original_flag_795_set_requested) {
            if (!host_.write_original_grenade_contact_flag(grenade_entity,true))
                return {SpawnCode::HostRejected,decision,
                        ar_grenade::StepResult::TriggerRejected};
            entry->original_grenade_field_795=true;
            observed.grenade_original_field_795=true;
        }
        const auto result=ar_grenade_touch::dispatch(
            observed,entry->grenade_runtime,decal,blast_host);
        return {SpawnCode::Created,result.evaluation,result.projectile_step};
    } catch (...) {
        return {SpawnCode::HostRejected,{},
                ar_grenade::StepResult::TriggerRejected};
    }
}

} // namespace sin_compat::map_entities
