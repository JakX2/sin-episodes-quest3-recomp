// Phase 1657: execute ALL nine compiler-generated x86 virtual entries
// on an original-free, owner-pinned, bounded synthetic entity list. This
// is a behavioral diagnostic, NOT the retail engine or original DLL.
#include "native_client_entitylist_bounded_reads.h"
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1657 tests require native MSVC Win32 x86
#endif

namespace api=sin_independent::client_entitylist_bounded;
namespace {
struct FakeUnknown {
    std::uint32_t token=0u;
    std::uint32_t networkable=0u;
    std::uint32_t entity=0u;
};
struct CallState {
    void* expected_unknown=nullptr;
    unsigned network_calls=0u;
    unsigned entity_calls=0u;
    unsigned invalid_unknown_calls=0u;
};
void* fake_network(void* context,void* unknown) noexcept {
    auto* cs=static_cast<CallState*>(context);
    if(unknown!=cs->expected_unknown) {
        ++cs->invalid_unknown_calls;
        return nullptr;
    }
    ++cs->network_calls;
    return &static_cast<FakeUnknown*>(unknown)->networkable;
}
void* fake_entity(void* context,void* unknown) noexcept {
    auto* cs=static_cast<CallState*>(context);
    if(unknown!=cs->expected_unknown) {
        ++cs->invalid_unknown_calls;
        return nullptr;
    }
    ++cs->entity_calls;
    return &static_cast<FakeUnknown*>(unknown)->entity;
}
int failure(int n,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1657 client bounded nine-method native test #%d: %s\n",
        n,why);
    return n;
}
bool one_arg_dispatch(void* receiver,void* method,std::uint32_t argument,
                      void*& result) noexcept {
    std::uintptr_t before=0u,after=0u;
    void* returned=nullptr;
    __asm {
        mov before, esp
        push argument
        mov ecx, receiver
        mov eax, method
        call eax
        mov returned, eax
        mov after, esp
    }
    result=returned;
    return before==after;
}
bool no_arg_dispatch(void* receiver,void* method,
                     int& result) noexcept {
    std::uintptr_t before=0u,after=0u;
    int returned=0;
    __asm {
        mov before, esp
        mov ecx, receiver
        mov eax, method
        call eax
        mov returned, eax
        mov after, esp
    }
    result=returned;
    return before==after;
}
bool one_arg_void_dispatch(void* receiver,void* method,
                           int argument) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm {
        mov before, esp
        push argument
        mov ecx, receiver
        mov eax, method
        call eax
        mov after, esp
    }
    return before==after;
}
} // namespace

int main() {
    auto source=std::make_unique<api::State>();
    auto unknown=std::make_unique<FakeUnknown>();
    CallState calls{};
    calls.expected_unknown=unknown.get();
    source->callbacks={&calls,&fake_network,&fake_entity};
    source->networked_count=2;
    source->secondary_flagged_count=1;
    source->highest_live_index=2047;
    source->max_entities=2048;
    source->mirrors[5].networkable=&unknown->networkable;
    source->mirrors[2047].networkable=&unknown->networkable;
    constexpr std::uint32_t kSerial=0xABCDEu;
    constexpr unsigned kIndex=5u;
    constexpr auto kHandle=(kSerial<<12u)|kIndex;
    static_assert((kHandle & api::kEhandleIndexMask)==kIndex,
                  "handle must index original 4096 slots");
    source->raw[kIndex].unknown=unknown.get();
    source->raw[kIndex].serial=kSerial;
    source->raw[2047].unknown=unknown.get();
    source->raw[2047].serial=0x13579u;
    source->raw[4095].unknown=unknown.get();
    source->raw[4095].serial=0x24680u;
    api::BoundedReader reader(*source);
    api::NineMethodInterface* iface=&reader;
    auto** table=*reinterpret_cast<void***>(iface);
    if(!table)return failure(1,"native nine-slot interface vptr is missing");
    for(unsigned i=0u;i<9u;++i) {
        if(!table[i])return failure(2,"missing original-ordered slot");
        for(unsigned j=0u;j<i;++j)
            if(table[i]==table[j])
                return failure(3,"compiler folded distinct slot targets");
    }
    if(iface->GetClientNetworkable(5)!=&unknown->networkable ||
       iface->GetClientNetworkable(2047)!=&unknown->networkable ||
       iface->GetClientNetworkable(-1)!=nullptr ||
       iface->GetClientNetworkable(2048)!=nullptr)
        return failure(4,"slot0 valid/missing/mirror boundary");
    if(iface->GetClientUnknownFromHandle(kHandle)!=unknown.get() ||
       iface->GetClientUnknownFromHandle(kHandle^(1u<<12u))!=nullptr ||
       iface->GetClientUnknownFromHandle(kHandle^1u)!=nullptr ||
       iface->GetClientUnknownFromHandle(api::kInvalidHandle)!=nullptr)
        return failure(5,"slot2 handle serial/index/invalid sentinel");
    if(iface->GetClientNetworkableFromHandle(kHandle)!=&unknown->networkable ||
       iface->GetClientEntityFromHandle(kHandle)!=&unknown->entity ||
       iface->GetClientNetworkableFromHandle(kHandle^(1u<<12u))!=nullptr ||
       iface->GetClientEntityFromHandle(api::kInvalidHandle)!=nullptr)
        return failure(6,"slots1/4 conversion and stale-handle refusal");
    if(iface->GetClientEntity(5)!=&unknown->entity ||
       iface->GetClientEntity(4095)!=&unknown->entity ||
       iface->GetClientEntity(-1)!=nullptr ||
       iface->GetClientEntity(4096)!=nullptr ||
       iface->GetClientEntity(11)!=nullptr)
        return failure(7,"slot3 raw unknown->entity and safety bounds");
    if(iface->NumberOfEntities(false)!=2 ||
       iface->NumberOfEntities(true)!=3 ||
       iface->GetHighestEntityIndex()!=2047 ||
       iface->GetMaxEntities()!=2048)
        return failure(8,"slots5/6/8 original count/highwater/max values");
    iface->SetMaxEntities(4096);
    if(iface->GetMaxEntities()!=4096)
        return failure(9,"slot7 setter did not write host max value");
    iface->SetMaxEntities(-1);
    iface->SetMaxEntities(4097);
    if(iface->GetMaxEntities()!=4096)
        return failure(10,"slot7 must safely refuse unsafe host maximum");
    // Run each *actual x86 virtual entry*, verify 'this' in ECX, and
    // verify exactly the callee-popped 4-byte argument or no arguments.
    void* returned=nullptr;
    for(unsigned i=0u;i<9u;++i) {
        bool balanced=false;
        if(i==6u || i==8u) {
            int value=0;
            balanced=no_arg_dispatch(iface,table[i],value);
            const int expected=(i==6u)?2047:4096;
            if(value!=expected)
                return failure(11,"native no-arg slot returned wrong value");
        } else if(i==7u) {
            balanced=one_arg_void_dispatch(iface,table[i],1024);
        } else {
            const auto arg=(i==5u)?1u:
                (i==0u || i==3u)?5u:kHandle;
            balanced=one_arg_dispatch(iface,table[i],arg,returned);
            void* expected=(i==0u || i==1u)?&unknown->networkable:
                           (i==2u)?static_cast<void*>(unknown.get()):
                           (i==3u || i==4u)?&unknown->entity:
                           reinterpret_cast<void*>(3u);
            if(returned!=expected)
                return failure(12,"native one-DWORD vtable result or slot order");
        }
        if(!balanced)
            return failure(13,"native ECX/ESP/callee-cleanup ABI drift");
    }
    if(iface->GetMaxEntities()!=1024 ||
       calls.invalid_unknown_calls!=0u ||
       calls.network_calls==0u || calls.entity_calls==0u)
        return failure(14,"native virtual dispatch/owned callback integrity");
    // The original method stores unknown+serial in 16-byte raw records.
    // Reusing the same index with a new serial invalidates old handles;
    // no stale pointers may be issued after the owner updates the slot.
    source->raw[kIndex].serial=0xABCDDu;
    if(iface->GetClientUnknownFromHandle(kHandle)!=nullptr ||
       iface->GetClientEntityFromHandle(kHandle)!=nullptr ||
       iface->GetClientNetworkableFromHandle(kHandle)!=nullptr)
        return failure(15,"stale handle after serial reuse");
    // Deliberately clear one host adapter to prove no unknown vcall is
    // performed and no fabricated entity pointer is returned.
    source->callbacks.client_entity=nullptr;
    if(iface->GetClientEntity(5)!=nullptr ||
       iface->GetClientEntity(4095)!=nullptr)
        return failure(16,"missing host unknown-to-entity adapter accepted");
    source->networked_count=-1;
    if(iface->NumberOfEntities(true)!=0)
        return failure(17,"corrupt host network count accepted");
    source->networked_count=2;
    source->secondary_flagged_count=4097;
    if(iface->NumberOfEntities(true)!=0)
        return failure(18,"corrupt host secondary count accepted");
    source->highest_live_index=4096;
    if(iface->GetHighestEntityIndex()!=-1)
        return failure(19,"corrupt highest index not refused");
    // Nine virtuals have been executed solely against owned C++ inputs.
    std::puts("PASS Phase1657 original-free MSVC x86: all nine real vtable "
              "positions, network mirror, handle serials, client unknown "
              "callbacks, bounded indexes, optional entity counts, highest "
              "index, max setter/getter, stale-lifetime refusal, ECX/ESP "
              "thiscall cleanup and missing-provider safety");
    return 0;
}
