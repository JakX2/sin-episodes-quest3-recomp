#pragma once
// Bounded, terminal client registration of three authenticated original
// declarations. No game-side command callbacks or engine defaults changed.
#include "server_single_cvar_probe.h"
#include <array>
namespace sin_independent::console_abi {
struct ClientProbeSpec {const char* name;const char* value;unsigned callRva;unsigned objectRva;};
// Descending constructor call order models prepending the selected
// subset. It does not assert the full original static init order.
inline constexpr ClientProbeSpec kClientSpecs[]={
    {"cl_starfield_distance","256.0",0x26f031,0x37b108},
    {"cl_starfield_diameter","128.0",0x26f001,0x37b0c0},
    {"cl_show_bloodspray","1",0x268e01,0x361978},
};
inline constexpr std::size_t kClientSpecCount=sizeof(kClientSpecs)/sizeof(kClientSpecs[0]);
struct ClientProbeOutcome {SingleCvarResult result;std::size_t verified;};
using ClientProbeTrace=void (*)(const char*,const char*) noexcept;
class ClientProbeVariables {
public:
    ClientProbeVariables()=default;
    ClientProbeVariables(const ClientProbeVariables&)=delete;
    ClientProbeVariables& operator=(const ClientProbeVariables&)=delete;
    ClientProbeOutcome Register(void* engine,ClientProbeTrace trace) noexcept {
        if(!engine || !trace || attempted_)return {SingleCvarResult::InvalidInterface,0};
        attempted_=true;
        auto** slots=*reinterpret_cast<void***>(engine);
        if(!slots || !slots[0] || !slots[3] || !slots[4])return {SingleCvarResult::InvalidInterface,0};
        auto reg=reinterpret_cast<RegisterSlot>(slots[0]);
        auto find=reinterpret_cast<FindSlot>(slots[3]);
        auto commands=reinterpret_cast<CommandsSlot>(slots[4]);
        // No engine command-line query or replicated-default writes:
        // the original client accessor only adds 0x8, detaches, registers.
        for(const auto& spec:kClientSpecs)
            if(find(engine,spec.name))return {SingleCvarResult::ExistingName,0};
        auto* node=commands(engine);
        for(unsigned i=0;node;++i) {
            if(i==16384)return {SingleCvarResult::InvalidList,0};
            auto** vt=*reinterpret_cast<void***>(node);
            if(!vt || !vt[4])return {SingleCvarResult::InvalidList,0};
            const char* n=reinterpret_cast<NameSlot>(vt[4])(node);
            if(!n)return {SingleCvarResult::InvalidList,0};
            for(const auto& spec:kClientSpecs)
                if(SameName(n,spec.name))return {SingleCvarResult::ExistingName,0};
            void* next=nullptr;
            std::memcpy(&next,static_cast<const unsigned char*>(node)+4,sizeof(next));
            node=next;
        }
        std::array<const char*,kClientSpecCount> storage{};
        std::array<float,kClientSpecCount> reals{};
        std::array<int,kClientSpecCount> integers{};
        for(std::size_t i=0;i<kClientSpecCount;++i) {
            auto& v=variables_[i];const auto& s=kClientSpecs[i];
            v.CreateVariable(s.name,s.value,0,nullptr,false,0,false,0,nullptr);
            if(!v.string || v.parent!=&v || v.registered || v.next ||
               std::strcmp(v.String(),s.value))return {SingleCvarResult::BadObject,0};
            storage[i]=v.string;reals[i]=v.real;integers[i]=v.integer;
        }
        trace("client-cvar-preflight-complete","3");
        const auto valid=[&](std::size_t i) noexcept {
            auto& v=variables_[i];const auto& s=kClientSpecs[i];
            return v.parent==&v && v.string==storage[i] && v.registered &&
                v.GetName() && !std::strcmp(v.GetName(),s.name) &&
                v.flags==8 && !v.callback && !v.hasMin && !v.hasMax &&
                v.real==reals[i] && v.integer==integers[i] && !std::strcmp(v.String(),s.value);
        };
        std::size_t verified=0;
        for(std::size_t i=0;i<kClientSpecCount;++i) {
            auto& v=variables_[i];const auto& s=kClientSpecs[i];
            if(find(engine,s.name))return {SingleCvarResult::ExistingName,verified};
            v.AddFlags(8);v.next=nullptr;
            trace("client-cvar-register-entered",s.name);
            reg(engine,&v);
            trace("client-cvar-register-returned",s.name);
            if(v.parent!=&v || v.string!=storage[i])return {SingleCvarResult::Relinked,verified};
            v.registered=true;
            if(find(engine,s.name)!=&v)return {SingleCvarResult::LookupMismatch,verified};
            if(!valid(i))return {SingleCvarResult::BadObject,verified};
            ++verified;
            trace("client-cvar-registration-verified",s.name);
        }
        for(std::size_t i=0;i<kClientSpecCount;++i)
            if(find(engine,kClientSpecs[i].name)!=&variables_[i] || !valid(i))
                return {SingleCvarResult::LookupMismatch,verified};
        return {SingleCvarResult::Verified,verified};
    }
private:
    std::array<Variable,kClientSpecCount> variables_{};
    bool attempted_=false;
};
} // namespace sin_independent::console_abi
