// PE32 original-networkproperty reciprocal link attestation: original-free.
// Pure synthetic memory. The trusted host pins all backing allocations.
#include "server_gameents_pinned_original_link_attacher.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>
using namespace sin_independent;
using Registry=GameEntsOwnerLifetimeRegistry<2,2>;
using Attacher=GameEntsPinnedOriginalLinkAttacher<2,2>;
using Status=GameEntsOriginalLinkStatus;
static_assert(kOriginalNetworkPropertyOuterOffset==0x168 &&
              kOriginalNetworkPropertyEdictOffset==0x16c &&
              kOriginalEdictUnknownLinkOffset==0x0c,
              "original binary authenticated 2006 PE32 link offsets");

struct Owner {
    std::array<unsigned char,0x14> edict{};
    std::array<unsigned char,0x180> entity{};
    std::array<unsigned char,0x90> touch{};
    std::uint32_t edict_pe32=0,entity_pe32=0;
    std::uint64_t epoch=0,pin_token=80;
    int acquire_calls=0,validation_calls=0,end_calls=0,
        nested_busy=0,parallel_busy=0;
    bool pinned=false,deny_acquire=false,deny_release=false,
         bad_extent=false,bad_entity_extent=false,
         revoke_second_validation=false,revoke_third_validation=false,
         trigger_nested=false,
         trigger_parallel=false;
    Attacher* nested_attacher=nullptr;
    Registry* nested_registry=nullptr;
    GameEntsOwnerIdentity nested_identity{};
};
static void WritePE32(unsigned char* p,std::uint32_t v) noexcept {
    p[0]=static_cast<unsigned char>(v);
    p[1]=static_cast<unsigned char>(v>>8);
    p[2]=static_cast<unsigned char>(v>>16);
    p[3]=static_cast<unsigned char>(v>>24);
}
static std::uint32_t PointerPE32(const void* ptr,
                                  std::uint32_t x64_fixture_token) noexcept {
    if(sizeof(void*)==4)
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(ptr));
    return x64_fixture_token;
}
static GameEntsOwnerIdentity Identity(Owner& o) noexcept {
    return {o.edict.data(),o.edict.size(),o.entity.data(),o.entity.size(),
            o.entity.data(),sizeof(void*),o.touch.data(),o.touch.size(),
            o.epoch};
}
static bool Prepare(Owner& o,Registry& r,std::uint64_t epoch) noexcept {
    o.epoch=epoch;
    o.edict_pe32=PointerPE32(o.edict.data(),0x10110000);
    o.entity_pe32=PointerPE32(o.entity.data(),0x20220000);
    WritePE32(o.edict.data()+0x0c,o.entity_pe32);
    WritePE32(o.entity.data()+0x168,o.entity_pe32);
    WritePE32(o.entity.data()+0x16c,o.edict_pe32);
    return r.ReserveEdict(0,o.edict.data(),o.edict.size(),epoch);
}
static bool Acquire(void* p,GameEntsOriginalLinkSnapshot* out) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!out||o.deny_acquire||o.pinned)return false;
    o.pinned=true;
    ++o.acquire_calls;
    *out={o.edict.data(),o.bad_extent?0x10:o.edict.size(),
          o.entity.data(),o.bad_entity_extent?0x16f:o.entity.size(),
          o.touch.data(),o.touch.size(),o.edict_pe32,o.entity_pe32,
          o.epoch,++o.pin_token};
    return true;
}
static GameEntsOriginalLinkHost Host(Owner& o) noexcept;
static bool Validate(void* p,const GameEntsOriginalLinkSnapshot* snap)
    noexcept {
    auto& o=*static_cast<Owner*>(p);
    ++o.validation_calls;
    if(o.trigger_nested&&o.validation_calls==1&&o.nested_attacher&&
       o.nested_registry) {
        const auto status=o.nested_attacher->Attach(
            *o.nested_registry,0,o.nested_identity,Host(o));
        if(status==Status::Busy)++o.nested_busy;
        if(o.trigger_parallel) {
            Status result=Status::InvalidHost;
            std::thread worker([&]{
                result=o.nested_attacher->Attach(
                    *o.nested_registry,0,o.nested_identity,Host(o));
            });
            worker.join();
            if(result==Status::Busy)++o.parallel_busy;
        }
    }
    return snap&&o.pinned&&
        (!o.revoke_second_validation||o.validation_calls!=2)&&
        (!o.revoke_third_validation||o.validation_calls!=3)&&
        snap->pin_token==o.pin_token&&snap->epoch==o.epoch&&
        snap->edict==o.edict.data()&&snap->entity==o.entity.data()&&
        snap->touch==o.touch.data()&&snap->edict_pe32==o.edict_pe32&&
        snap->entity_pe32==o.entity_pe32;
}
static bool End(void* p,std::uint64_t token) noexcept {
    auto& o=*static_cast<Owner*>(p);
    if(!o.pinned||o.pin_token!=token)return false;
    ++o.end_calls;
    o.pinned=false;
    return !o.deny_release;
}
static GameEntsOriginalLinkHost Host(Owner& o) noexcept {
    return {&o,&Acquire,&Validate,&End};
}
static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL pinned original entity link %d: %s\n",n,why);
    return n;
}
int main() {
    Owner good{};
    Registry good_registry;
    Attacher good_attacher;
    if(!Prepare(good,good_registry,500))
        return Fail(1,"initial pinned edict reservation");
    const auto good_id=Identity(good);
    if(good_attacher.Attach(good_registry,0,good_id,Host(good))!=
           Status::Activated||
       good.pinned||good.acquire_calls!=1||good.end_calls!=1||
       good.validation_calls!=3||good_attacher.Quarantined())
        return Fail(2,"reciprocal entity/edict/outer link attach");
    std::uint64_t lease=0;
    if(!good_registry.Begin(5,&lease)||
       !good_registry.ValidEdict(good.edict.data())||
       !good_registry.ValidUnknownPair(good.edict.data(),
                                       good.entity.data())||
       !good_registry.ValidUnknownPairAtEpoch(good.edict.data(),
                                              good.entity.data(),good.epoch)||
       good_registry.ValidUnknownPairAtEpoch(good.edict.data(),
                                             good.entity.data(),good.epoch+1)||
       !good_registry.ValidPairAtEpoch(good.edict.data(),
                                       good.entity.data(),good.epoch)||
       !good_registry.End(5,lease))
        return Fail(3,"attached source-backed alias not visible to lease");
    Attacher duplicate_attacher;
    if(duplicate_attacher.Attach(good_registry,0,good_id,Host(good))!=
       Status::RegistryRefused||good.end_calls!=2)
        return Fail(4,"duplicate original pointer pair activation");

    // Corrupt every one of the three separately SHA-attested PE32 links.
    for(int field=0;field!=3;++field) {
        Owner o{};
        Registry r;
        Attacher a;
        if(!Prepare(o,r,600+field))
            return Fail(5,"reciprocal fixture reserve");
        unsigned char* corrupt=field==0?o.edict.data()+0x0c:
            field==1?o.entity.data()+0x168:o.entity.data()+0x16c;
        const auto correct=field==2?o.edict_pe32:o.entity_pe32;
        WritePE32(corrupt,correct^0x01000000u);
        if(a.Attach(r,0,Identity(o),Host(o))!=Status::MismatchedLinks||
           a.Quarantined()||o.end_calls!=1||
           !r.Begin(5,&lease)||
           r.ValidUnknownPair(o.edict.data(),o.entity.data())||
           !r.End(5,lease))
            return Fail(6,"forged reciprocal field authorized");
        WritePE32(corrupt,correct);
        if(a.Attach(r,0,Identity(o),Host(o))!=Status::Activated||
           o.end_calls!=2)
            return Fail(7,"mismatched PE32 links poisoned pending identity");
    }
    Owner malformed{};
    Registry malformed_reg;
    Attacher malformed_att;
    if(!Prepare(malformed,malformed_reg,800))
        return Fail(8,"malformed fixture reserve");
    malformed.bad_extent=true;
    if(malformed_att.Attach(malformed_reg,0,Identity(malformed),
                            Host(malformed))!=Status::BadSnapshot||
       malformed.end_calls!=1||malformed_att.Quarantined())
        return Fail(9,"short edict was dereferenced");
    malformed.bad_extent=false;
    malformed.bad_entity_extent=true;
    if(malformed_att.Attach(malformed_reg,0,Identity(malformed),
                            Host(malformed))!=Status::BadSnapshot||
       malformed.end_calls!=2)
        return Fail(10,"short entity was dereferenced");
    malformed.bad_entity_extent=false;
    auto wrong=Identity(malformed);
    wrong.generation++;
    if(malformed_att.Attach(malformed_reg,0,wrong,Host(malformed))!=
           Status::BadSnapshot||malformed.end_calls!=3)
        return Fail(11,"mismatched reserved epoch used in binding");
    wrong=Identity(malformed);
    wrong.unknown=malformed.entity.data()+4;
    if(malformed_att.Attach(malformed_reg,0,wrong,Host(malformed))!=
           Status::BadSnapshot||malformed.end_calls!=4)
        return Fail(12,"shifted unknown bypassed exact CBaseEntity base");
    wrong=Identity(malformed);
    wrong.touch=malformed.entity.data();
    if(malformed_att.Attach(malformed_reg,0,wrong,Host(malformed))!=
           Status::BadSnapshot||malformed.end_calls!=5)
        return Fail(13,"unattested touch backing accepted");
    if(malformed_att.Attach(malformed_reg,0,Identity(malformed),
                            Host(malformed))!=Status::Activated)
        return Fail(14,"valid entity denied after malformed attempts");

    Owner revoked{};
    Registry revoked_reg;
    Attacher revoked_att;
    if(!Prepare(revoked,revoked_reg,900))
        return Fail(15,"revoked fixture reserve");
    revoked.revoke_second_validation=true;
    if(revoked_att.Attach(revoked_reg,0,Identity(revoked),Host(revoked))!=
           Status::PinRevoked||revoked.end_calls!=1||
       !revoked_reg.Begin(5,&lease)||
       revoked_reg.ValidUnknownPair(revoked.edict.data(),
                                    revoked.entity.data())||
       !revoked_reg.End(5,lease))
        return Fail(16,"link readback lost host authorization");
    revoked.revoke_second_validation=false;
    if(revoked_att.Attach(revoked_reg,0,Identity(revoked),Host(revoked))!=
       Status::Activated)
        return Fail(17,"clean pin revocation damaged reservation");

    // Third validation is deliberately AFTER registry activation. A host
    // revocation here leaves live owner metadata and requires quarantine.
    Owner revoked_after{};
    Registry revoked_after_reg;
    Attacher revoked_after_att;
    if(!Prepare(revoked_after,revoked_after_reg,950))
        return Fail(27,"post-attach revoke fixture reserve");
    revoked_after.revoke_third_validation=true;
    if(revoked_after_att.Attach(revoked_after_reg,0,
           Identity(revoked_after),Host(revoked_after))!=Status::PinRevoked||
       !revoked_after_att.Quarantined()||revoked_after.end_calls!=1||
       revoked_after_att.Attach(revoked_after_reg,0,
           Identity(revoked_after),Host(revoked_after))!=Status::Quarantined||
       revoked_after.acquire_calls!=1||
       !revoked_after_reg.Begin(5,&lease)||
       !revoked_after_reg.ValidUnknownPair(revoked_after.edict.data(),
                                           revoked_after.entity.data())||
       !revoked_after_reg.End(5,lease))
        return Fail(28,"post-activation physical pin loss did not quarantine");

    Owner locked{};
    Registry locked_reg;
    Attacher locked_att;
    if(!Prepare(locked,locked_reg,1000)||!locked_reg.Begin(5,&lease))
        return Fail(18,"lease-conflict fixture reserve");
    if(locked_att.Attach(locked_reg,0,Identity(locked),Host(locked))!=
           Status::RegistryRefused||!locked_reg.End(5,lease)||
       locked_att.Attach(locked_reg,0,Identity(locked),Host(locked))!=
           Status::Activated)
        return Fail(19,"leased registry allowed unsafe construction");

    Owner busy{};
    Registry busy_reg;
    Attacher busy_att;
    if(!Prepare(busy,busy_reg,1100))
        return Fail(20,"busy fixture reserve");
    busy.trigger_nested=true;
    busy.trigger_parallel=true;
    busy.nested_attacher=&busy_att;
    busy.nested_registry=&busy_reg;
    busy.nested_identity=Identity(busy);
    if(busy_att.Attach(busy_reg,0,Identity(busy),Host(busy))!=
           Status::Activated||
       busy.nested_busy!=1||busy.parallel_busy!=1||
       busy.acquire_calls!=1||busy.end_calls!=1)
        return Fail(21,"nested or concurrent host pins admitted");

    Owner no_pin{};
    Registry no_pin_reg;
    Attacher no_pin_att;
    if(!Prepare(no_pin,no_pin_reg,1200))
        return Fail(22,"pin-denied fixture reserve");
    no_pin.deny_acquire=true;
    if(no_pin_att.Attach(no_pin_reg,0,Identity(no_pin),Host(no_pin))!=
           Status::PinUnavailable||no_pin.end_calls||
       no_pin_att.Quarantined())
        return Fail(23,"failed acquire ran cleanup or poisoned adapter");
    auto incomplete=Host(no_pin);
    incomplete.validate_pin=nullptr;
    if(no_pin_att.Attach(no_pin_reg,0,Identity(no_pin),incomplete)!=
       Status::InvalidHost)return Fail(24,"incomplete trusted host accepted");

    Owner release{};
    Registry release_reg;
    Attacher release_att;
    if(!Prepare(release,release_reg,1300))
        return Fail(25,"failed-release fixture reserve");
    release.deny_release=true;
    if(release_att.Attach(release_reg,0,Identity(release),Host(release))!=
           Status::PinReleaseFailed||!release_att.Quarantined()||
       release.end_calls!=1||release.pinned||
       release_att.Attach(release_reg,0,Identity(release),Host(release))!=
           Status::Quarantined||release.acquire_calls!=1||
       !release_reg.Begin(5,&lease)||
       !release_reg.ValidUnknownPair(release.edict.data(),
                                     release.entity.data())||
       !release_reg.End(5,lease))
        return Fail(26,"failed unpin returned successful live identity");
    std::puts("PASS pinned original entity links: 3 reciprocal PE32 fields, host-held attachment, stale epochs, malformed buffers, callback reentrancy, concurrent Busy, denied lease, release quarantine");
    return 0;
}
