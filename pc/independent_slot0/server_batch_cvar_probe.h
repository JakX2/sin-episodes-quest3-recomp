#pragma once
// Original-guided bounded registration of a selected, callback-free
// subset. Every declaration is authenticated on owner-local originals.
// Not the complete game registry; selection order is call-site order.
#include "server_single_cvar_probe.h"
#include <array>
namespace sin_independent::console_abi {
struct BatchSpec {const char* name;const char* value;unsigned callRva;unsigned objectRva;};
inline constexpr BatchSpec kBatchSpecs[]={
    {"g_debug_dynamic_difficulty","0",0x3c2d81,0x592108},
    {"dd_log_events","0",0x3c2fb1,0x5923e8},
    {"g_debug_damage","0",0x3c3311,0x592ec0},
    {"sk_dynamic_resupply_modifier","1.0",0x3c4111,0x596510},
    {"sk_healthmega","100",0x3c4231,0x5966b8},
    {"sk_flashgrenade_radius","0",0x3c4c81,0x598058},
    {"sk_dmg_homer_grenade","20",0x3c4d41,0x598128},
    {"sk_homer_grenade_radius","100",0x3c4d71,0x598170},
    {"sk_dmg_pathfollower_grenade","0",0x3c4dd1,0x5981d8},
    {"sk_pathfollower_grenade_radius","0",0x3c4e01,0x598220},
    {"grunt_spawn_health","1",0x3c6c61,0x59c598},
    {"ai_use_readiness","1",0x3c7111,0x59cd78},
    {"ai_readiness_decay","120",0x3c7141,0x59cdc0},
    {"ai_new_aiming","1",0x3c7171,0x59ce08},
    {"g_debug_bigmutant","0",0x3c7291,0x59d010},
    {"major_ambushdist","16000",0x3c7481,0x59d318},
    {"g_debug_major_phys_hit","0",0x3c7521,0x59d368},
    {"minor_spit_arc","1",0x3c7671,0x59d550},
    {"g_debug_turret_ceiling","0",0x3c7881,0x59d8e0},
    {"g_debug_turret","0",0x3c7911,0x59d9a8},
    {"ai_newgroundturret","0",0x3c79e1,0x59dac0},
    {"ai_ally_manager_debug","0",0x3c7a81,0x59db90},
    {"player_sprint_enabled","0",0x3c8321,0x59eb28},
    {"accuracy_change_time","10.0",0x3c8491,0x59ec18},
};
inline constexpr std::size_t kBatchCount=sizeof(kBatchSpecs)/sizeof(kBatchSpecs[0]);
static_assert(kBatchCount==24);
struct BatchOutcome {
    SingleCvarResult result;
    std::size_t verified;
};
using BatchTrace=void (*)(const char*,const char*) noexcept;
template<std::size_t Count>
class BatchVariablesFor {
    static_assert(Count>0 && Count<=1024,"bounded catalog batch");
public:
    BatchVariablesFor()=default;
    BatchVariablesFor(const BatchVariablesFor&)=delete;
    BatchVariablesFor& operator=(const BatchVariablesFor&)=delete;
    BatchOutcome Register(const BatchSpec (&specs)[Count],void* engine,BatchTrace trace) noexcept {
        if(!engine || !trace || attempted_)return {SingleCvarResult::InvalidInterface,0};
        // The catalog string buffers must have static lifetime for the
        // engine's retained object names/defaults. Reject duplicates before
        // querying any engine interface. One terminal attempt per owner.
        attempted_=true;
        for(std::size_t i=0;i<Count;++i) {
            if(!specs[i].name || !*specs[i].name || !specs[i].value)
                return {SingleCvarResult::BadObject,0};
            for(std::size_t j=0;j<i;++j)
                if(SameName(specs[i].name,specs[j].name))
                    return {SingleCvarResult::BadObject,0};
        }
        auto** slots=*reinterpret_cast<void***>(engine);
        if(!slots || !slots[0] || !slots[1] || !slots[3] || !slots[4])
            return {SingleCvarResult::InvalidInterface,0};
        auto reg=reinterpret_cast<RegisterSlot>(slots[0]);
        auto getOverride=reinterpret_cast<OverrideSlot>(slots[1]);
        auto find=reinterpret_cast<FindSlot>(slots[3]);
        auto commands=reinterpret_cast<CommandsSlot>(slots[4]);
        // An existing name or override anywhere in this subset prevents
        // ALL registration. No writes to existing engine-owned variables.
        for(const auto& spec:specs) {
            if(find(engine,spec.name))return {SingleCvarResult::ExistingName,0};
            if(getOverride(engine,spec.name))return {SingleCvarResult::OverridePresent,0};
        }
        auto* node=commands(engine);
        for(unsigned i=0;node;++i) {
            if(i==16384)return {SingleCvarResult::InvalidList,0};
            auto** vt=*reinterpret_cast<void***>(node);
            if(!vt || !vt[4])return {SingleCvarResult::InvalidList,0};
            const char* name=reinterpret_cast<NameSlot>(vt[4])(node);
            if(!name)return {SingleCvarResult::InvalidList,0};
            for(const auto& spec:specs)
                if(SameName(name,spec.name))return {SingleCvarResult::ExistingName,0};
            void* next=nullptr;
            std::memcpy(&next,static_cast<const unsigned char*>(node)+4,sizeof(next));
            node=next;
        }
        // Allocate every owned variable before first external registration.
        for(std::size_t i=0;i<Count;++i) {
            const auto& spec=specs[i];auto& v=variables_[i];
            v.CreateVariable(spec.name,spec.value,0,nullptr,false,0,false,0,nullptr);
            if(!v.string || v.parent!=&v || v.registered || v.next ||
               std::strcmp(v.String(),spec.value))
                return {SingleCvarResult::BadObject,0};
        }
        char total[16]={};
        std::snprintf(total,sizeof(total),"%u",static_cast<unsigned>(Count));
        trace("cvar-batch-preflight-complete",total);
        std::array<const char*,Count> strings{};
        std::array<float,Count> reals{};
        std::array<int,Count> integers{};
        std::size_t verified=0;
        for(std::size_t i=0;i<Count;++i) {
            const auto& spec=specs[i];auto& v=variables_[i];
            if(find(engine,spec.name))return {SingleCvarResult::ExistingName,verified};
            const auto* storage=v.string;
            const float originalReal=v.real;const int originalInt=v.integer;
            strings[i]=storage;reals[i]=originalReal;integers[i]=originalInt;
            v.AddFlags(4);v.next=nullptr;
            trace("cvar-batch-register-entered",spec.name);
            reg(engine,&v);
            trace("cvar-batch-register-returned",spec.name);
            if(v.parent!=&v || v.string!=storage)return {SingleCvarResult::Relinked,verified};
            if(getOverride(engine,spec.name))return {SingleCvarResult::OverridePresent,verified};
            v.registered=true;
            if(find(engine,spec.name)!=&v)return {SingleCvarResult::LookupMismatch,verified};
            if(v.parent!=&v || v.string!=storage || !v.GetName() ||
               std::strcmp(v.GetName(),spec.name) || !v.IsBitSet(4) ||
               v.real!=originalReal || v.integer!=originalInt ||
               std::strcmp(v.String(),spec.value))
                return {SingleCvarResult::BadObject,verified};
            ++verified;
            trace("cvar-batch-registration-verified",spec.name);
        }
        // Re-query the whole registry after the last insertion. Earlier
        // objects must remain visible and self-owned after later relinks.
        for(std::size_t i=0;i<Count;++i) {
            auto& v=variables_[i];const auto& spec=specs[i];
            if(find(engine,spec.name)!=&v || v.parent!=&v || !v.registered ||
               v.string!=strings[i] || !v.GetName() || std::strcmp(v.GetName(),spec.name) ||
               !v.IsBitSet(4) || v.real!=reals[i] || v.integer!=integers[i] ||
               std::strcmp(v.String(),spec.value))
                return {SingleCvarResult::LookupMismatch,verified};
        }
        return {SingleCvarResult::Verified,verified};
    }
private:
    std::array<Variable,Count> variables_{};
    bool attempted_=false;
};
// Preserve the existing retail-tested 24-declaration entry point and
// its diagnostic labels.
class BatchVariables final:public BatchVariablesFor<kBatchCount> {
public:
    BatchOutcome Register(void* engine,BatchTrace trace) noexcept {
        return BatchVariablesFor<kBatchCount>::Register(kBatchSpecs,engine,trace);
    }
};
} // namespace sin_independent::console_abi
