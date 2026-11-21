#include "sin_compat/original_sin_trigger_entities.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <new>
#include <utility>

namespace sin_compat::map_entities {
namespace {
constexpr std::string_view kDynamic="trigger_dynamic_difficulty";
constexpr std::string_view kSecret="trigger_secret";

}

OriginalTriggerEntityFactory::OriginalTriggerEntityFactory(
    IOriginalTriggerHost& host,std::size_t maximum) noexcept
    :host_(host),maximum_(maximum){}

OriginalTriggerEntityFactory::~OriginalTriggerEntityFactory() noexcept {
    // Host's registrar must outlive this adapter; no Source entity memory
    // belongs to us. A failed unregistration is a host lifetime error.
    (void)uninstall();
}
bool OriginalTriggerEntityFactory::supports(std::string_view name) noexcept {
    return name==kDynamic || name==kSecret;
}
bool OriginalTriggerEntityFactory::valid(EntityRef e) noexcept {
    return e.index!=std::numeric_limits<std::uint32_t>::max() &&
           e.generation!=std::numeric_limits<std::uint32_t>::max();
}
SpawnResult OriginalTriggerEntityFactory::callback(
    void* userdata,const SpawnRequest& request) noexcept {
    if(!userdata)return {SpawnCode::HostRejected,Kind::DynamicDifficultyTrigger};
    return static_cast<OriginalTriggerEntityFactory*>(userdata)->spawn(request);
}
RegistrationCode OriginalTriggerEntityFactory::install(
    IGameEntityRegistrar& registrar) {
    Guard guard(busy_);
    if(!guard)return RegistrationCode::Reentrant;
    if(registrar_ || installed_dynamic_ || installed_secret_)
        return RegistrationCode::AlreadyInstalled;
    try {
        if(!registrar.install(kDynamic,&callback,this))
            return RegistrationCode::HostRejected;
        registrar_=&registrar;installed_dynamic_=true;
        if(registrar.install(kSecret,&callback,this)){
            installed_secret_=true;
            return RegistrationCode::Installed;
        }
    } catch (...) {}
    // Preserve revocation evidence if a host refuses to unregister the
    // first factory. Never falsely advertise both registrations.
    try {
        if(registrar.uninstall(kDynamic,&callback,this)){
            registrar_=nullptr;installed_dynamic_=false;
            return RegistrationCode::HostRejected;
        }
    } catch (...) {}
    return RegistrationCode::PartialRollbackFailed;
}
RegistrationCode OriginalTriggerEntityFactory::uninstall() noexcept {
    Guard guard(busy_);
    if(!guard)return RegistrationCode::Reentrant;
    if(!registrar_)return RegistrationCode::NotInstalled;
    bool ok=true;
    try {
        if(installed_secret_) {
            if(registrar_->uninstall(kSecret,&callback,this))
                installed_secret_=false;
            else ok=false;
        }
    } catch (...) {ok=false;}
    try {
        if(installed_dynamic_) {
            if(registrar_->uninstall(kDynamic,&callback,this))
                installed_dynamic_=false;
            else ok=false;
        }
    } catch (...) {ok=false;}
    if(!installed_dynamic_ && !installed_secret_)registrar_=nullptr;
    return ok?RegistrationCode::Uninstalled:
        RegistrationCode::PartialRollbackFailed;
}
OriginalTriggerEntityFactory::Record* OriginalTriggerEntityFactory::find(
    EntityRef ref) noexcept {
    if(!valid(ref))return nullptr;
    const auto it=records_.find(ref.index);
    return it==records_.end()||it->second.ref!=ref?nullptr:&it->second;
}
SpawnResult OriginalTriggerEntityFactory::spawn(
    const SpawnRequest& request) noexcept {
    Guard guard(busy_);
    const Kind kind=request.classname==kSecret?
        Kind::SecretTrigger:Kind::DynamicDifficultyTrigger;
    if(!guard)return {SpawnCode::Reentrant,kind};
    if(!supports(request.classname))
        return {SpawnCode::UnknownOrUnimplementedClass,kind};
    if(!valid(request.entity))
        return {SpawnCode::InvalidEntity,kind};
    if(records_.find(request.entity.index)!=records_.end())
        return {SpawnCode::DuplicateIndex,kind};
    if(records_.size()>=maximum_)
        return {SpawnCode::CapacityExceeded,kind};
    try {
        if(!host_.is_live(request.entity))
            return {SpawnCode::HostEntityNotLive,kind};
        Record record{};
        record.ref=request.entity;
        record.kind=request.classname==kSecret?
            TriggerKind::Secret:TriggerKind::DynamicDifficulty;
        if(!std::isfinite(request.origin.x) ||
           !std::isfinite(request.origin.y) ||
           !std::isfinite(request.origin.z))
            return {SpawnCode::HostRejected,kind};
        record.config.origin={request.origin.x,request.origin.y,
                              request.origin.z};
        const auto added=records_.emplace(request.entity.index,
                                          std::move(record));
        return {added.second?SpawnCode::Created:SpawnCode::DuplicateIndex,kind};
    } catch (const std::bad_alloc&) {
        return {SpawnCode::AllocationFailed,kind};
    } catch (...) {
        return {SpawnCode::HostRejected,kind};
    }
}
TriggerCode OriginalTriggerEntityFactory::configure(
    EntityRef trigger,const TriggerConfig& config) noexcept {
    Guard guard(busy_);
    if(!guard)return TriggerCode::Reentrant;
    Record* r=find(trigger);
    if(!r)return TriggerCode::UnknownEntity;
    if(r->consumed)return TriggerCode::AlreadyConsumed;
    if(!std::isfinite(config.estimated_time) ||
       !std::isfinite(config.origin.x) ||
       !std::isfinite(config.origin.y) ||
       !std::isfinite(config.origin.z))
        return TriggerCode::InvalidConfig;
    try {
        if(!host_.is_live(trigger))return TriggerCode::HostNotLive;
        // Copy first; a failed std::string allocation cannot partially
        // overwrite the previous configuration.
        TriggerConfig owned=config;
        r->config=std::move(owned);
        return TriggerCode::Handled;
    } catch (...) {return TriggerCode::HostRefused;}
}
TriggerCode OriginalTriggerEntityFactory::apply_map_keyvalue(
    EntityRef trigger,std::string_view key,
    std::string_view text_value) noexcept {
    Guard guard(busy_);
    if(!guard)return TriggerCode::Reentrant;
    Record* r=find(trigger);
    if(!r)return TriggerCode::UnknownEntity;
    if(r->consumed)return TriggerCode::AlreadyConsumed;
    if(key!="targetname" && (r->kind!=TriggerKind::DynamicDifficulty ||
       (key!="EstimatedTime" && key!="EstimatedDifficulty" &&
        key!="EndOfGame")))
        return TriggerCode::UnknownKey;
    // 256-byte scalar/targetname limit is a new input-safety policy,
    // not an assertion about Ritual's native KeyValues size limits.
    if(text_value.size()>256)return TriggerCode::InvalidConfig;
    try {
        if(!host_.is_live(trigger))return TriggerCode::HostNotLive;
        TriggerConfig next=r->config;
        if(key=="targetname") {
            if(text_value.empty())next.targetname.clear();
            else next.targetname.assign(text_value.data(),text_value.size());
        } else if(key=="EstimatedTime") {
            std::string text(text_value);
            if(text.empty())return TriggerCode::InvalidConfig;
            char* end=nullptr;
            errno=0;
            const float v=std::strtof(text.c_str(),&end);
            if(errno==ERANGE || end!=text.c_str()+text.size() ||
               !std::isfinite(v))return TriggerCode::InvalidConfig;
            next.estimated_time=v;
        } else {
            std::int32_t value=0;
            if(text_value.empty())return TriggerCode::InvalidConfig;
            const char* first=text_value.data();
            const char* last=first+text_value.size();
            const auto parsed=std::from_chars(first,last,value,10);
            if(parsed.ec!=std::errc{} || parsed.ptr!=last)
                return TriggerCode::InvalidConfig;
            if(key=="EstimatedDifficulty")
                next.estimated_difficulty=value;
            else next.end_of_game=value;
        }
        r->config=std::move(next);
        return TriggerCode::Handled;
    } catch (...) {return TriggerCode::HostRefused;}
}
TriggerSpawnReport OriginalTriggerEntityFactory::run_secret_spawn(
    EntityRef trigger,TriggerSpawnInput input) noexcept {
    Guard guard(busy_);
    if(!guard)return {TriggerCode::Reentrant};
    Record* r=find(trigger);
    if(!r)return {TriggerCode::UnknownEntity};
    if(r->kind!=TriggerKind::Secret)return {TriggerCode::WrongClass};
    if(r->native_spawn_completed || r->consumed)
        return {TriggerCode::AlreadyConsumed};
    try {
        if(!host_.is_live(trigger))return {TriggerCode::HostNotLive};
    } catch (...) {return {TriggerCode::HostRefused};}
    TriggerSpawnReport report{};
    if(input.engine_requests_removal) {
        // The original engine-global +0x88 check precedes base Spawn
        // and the optional secret discovered event. No fake CBaseTrigger.
        r->consumed=true;
        r->native_spawn_completed=true;
        r->pending_remove=true;
        try {report.removal_queued=host_.queue_remove(trigger);}
        catch (...) {report.removal_queued=false;}
        if(report.removal_queued)r->pending_remove=false;
        report.code=report.removal_queued?
            TriggerCode::Handled:TriggerCode::RemovalPending;
        return report;
    }
    try {
        if(!host_.initialize_native_base_trigger(trigger))
            return {TriggerCode::HostRefused};
    } catch (...) {return {TriggerCode::HostRefused};}
    report.base_initialized=true;
    r->native_spawn_completed=true;
    if((input.native_flags_0x120 & 0x08u)!=0u) {
        // CTriggerSecret::Spawn calls original 0x222B63B0
        // with descriptor=11 when this+0x120 contains bit3.
        TriggerEvent event{};
        event.event=TriggerEventName::Game;
        event.descriptor=11;
        event.owner=-1;
        event.name=r->config.targetname.empty()?
            kSecret:std::string_view(r->config.targetname);
        try {report.event_published=host_.publish(event);}
        catch (...) {report.event_published=false;}
        report.code=report.event_published?
            TriggerCode::Handled:TriggerCode::EventUnavailable;
    } else {
        report.code=TriggerCode::Handled;
    }
    return report;
}
TriggerTouchReport OriginalTriggerEntityFactory::touch(
    EntityRef trigger,EntityRef actor) noexcept {
    Guard guard(busy_);
    if(!guard)return {TriggerCode::Reentrant};
    Record* r=find(trigger);
    if(!r)return {TriggerCode::UnknownEntity};
    if(r->consumed)return {TriggerCode::AlreadyConsumed};
    if(!valid(actor))return {TriggerCode::InvalidActor};
    try {
        if(!host_.is_live(trigger))return {TriggerCode::HostNotLive};
        if(!host_.is_live(actor))return {TriggerCode::InvalidActor};
        // Original actor virtual +0x120 is a boolean player-only gate.
        if(!host_.is_player(actor))return {TriggerCode::NonPlayer};
        if(r->kind==TriggerKind::DynamicDifficulty &&
           r->config.base_special_flag_0x800 &&
           !host_.perform_special_trigger_base_action(trigger))
            return {TriggerCode::HostRefused};
    } catch (...) {return {TriggerCode::HostRefused};}

    // Latch before callbacks: a host event/listener cannot re-enter to
    // produce another trigger event during deferred engine removal.
    r->consumed=true;
    TriggerEvent event{};
    const auto& c=r->config;
    const std::string_view name=c.targetname.empty()
        ?(r->kind==TriggerKind::DynamicDifficulty?kDynamic:kSecret)
        :std::string_view(c.targetname);
    event.name=name;
    event.event=r->kind==TriggerKind::DynamicDifficulty?
        TriggerEventName::DynamicDifficulty:TriggerEventName::Game;
    event.descriptor=r->kind==TriggerKind::DynamicDifficulty?0:12;
    if(r->kind==TriggerKind::DynamicDifficulty) {
        try {event.owner=host_.current_difficulty_owner_index();}
        catch (...) {event.owner=-1;}
        event.advisor=-1;
        event.decision=c.estimated_difficulty;event.value=c.estimated_time;event.origin=c.origin;
    }
    TriggerTouchReport report{};
    try {report.event_published=host_.publish(event);}
    catch (...) {report.event_published=false;}

    if(r->kind==TriggerKind::DynamicDifficulty &&
       ((static_cast<std::uint32_t>(c.end_of_game)&0xffu)!=0u)) {
        try {report.manager_signaled=host_.mark_difficulty_manager();}
        catch (...) {report.manager_signaled=false;}
    }
    // The original removes the trigger even if event allocation failed.
    // Removal must be deferred by the host's engine entity-list queue.
    r->pending_remove=true;
    try {report.removal_queued=host_.queue_remove(trigger);}
    catch (...) {report.removal_queued=false;}
    if(report.removal_queued)r->pending_remove=false;
    report.code=!report.removal_queued?TriggerCode::RemovalPending:
        (r->kind==TriggerKind::DynamicDifficulty &&
         (static_cast<std::uint32_t>(c.end_of_game)&0xffu)!=0u &&
         !report.manager_signaled)?TriggerCode::ManagerRefused:
        !report.event_published?TriggerCode::EventUnavailable:
        TriggerCode::Handled;
    return report;
}
TriggerCode OriginalTriggerEntityFactory::retry_pending_remove(
    EntityRef trigger) noexcept {
    Guard guard(busy_);
    if(!guard)return TriggerCode::Reentrant;
    Record* r=find(trigger);
    if(!r)return TriggerCode::UnknownEntity;
    if(!r->pending_remove)return TriggerCode::AlreadyConsumed;
    try {
        if(!host_.queue_remove(trigger))return TriggerCode::RemovalPending;
    } catch (...) {return TriggerCode::RemovalPending;}
    r->pending_remove=false;
    return TriggerCode::Handled;
}
TriggerCode OriginalTriggerEntityFactory::release(EntityRef trigger) noexcept {
    Guard guard(busy_);
    if(!guard)return TriggerCode::Reentrant;
    if(!find(trigger))return TriggerCode::UnknownEntity;
    records_.erase(trigger.index);
    return TriggerCode::Handled;
}

} // namespace sin_compat::map_entities
