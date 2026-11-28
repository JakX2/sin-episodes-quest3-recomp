// Phase1727: literal host-only port of the two CWorld constructor 85B
// static-global initialization effects, never an executable engine global.
#include "sin_compat/original_client_world_lazy_static_phase1727.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace w = sin_compat::original_client_world_lazy_static_phase1727;
struct RegistrarContext final {
    w::HostOwnedStatic* observed = nullptr;
    w::Family family = w::Family::First;
    std::uint32_t destructorVA = 0u;
    unsigned calls = 0;
    bool sawReentrantAlreadyInitialized = false;
};
void recordRegister(std::uint32_t destructorVA, void* context) noexcept {
    auto& seen=*static_cast<RegistrarContext*>(context);
    ++seen.calls;
    seen.destructorVA=destructorVA;
    assert(seen.observed);
    assert((seen.observed->nativeModeledGuard & 1u) != 0u);
    for (const auto word : seen.observed->words) assert(word == 0u);
    // The source sets the guard before it invokes the registrar.
    const auto again=w::InitOnce(seen.family,*seen.observed,nullptr);
    seen.sawReentrantAlreadyInitialized =
        again.code == w::ResultCode::AlreadyInitialized;
}
int main() {
    static_assert(!w::CanPublishRetailWorld());
    static_assert(!w::CanExecuteOriginalCleanup());
    w::HostOwnedStatic first{}, second{};
    first.words.fill(0xAAAAAAAAu);
    second.words.fill(0xBBBBBBBBu);
    first.nativeModeledGuard=0x22u;
    second.nativeModeledGuard=0x08u;
    RegistrarContext f{&first,w::Family::First}, s{&second,w::Family::Second};
    const auto refused=w::InitOnce(w::Family::First,first,nullptr);
    assert(refused.code == w::ResultCode::RefusedNoHostRegistrar);
    assert(refused.hostStorage == nullptr);
    assert(first.words[0]==0xAAAAAAAAu && first.nativeModeledGuard==0x22u);
    const auto initFirst=w::InitOnce(w::Family::First,first,recordRegister,&f);
    assert(initFirst.code == w::ResultCode::InitializedAndRegistered);
    assert(initFirst.hostStorage == &first);
    assert(initFirst.sourceIdentity == &w::Identity(w::Family::First));
    assert(initFirst.sourceIdentity->sourceInitializerVA == 0x24061540u);
    assert(initFirst.sourceIdentity->sourceGlobalFirstWordVA == 0x24324698u);
    assert(first.nativeModeledGuard==0x23u);
    assert(f.calls==1u && f.sawReentrantAlreadyInitialized);
    assert(f.destructorVA==0x24271DA0u);
    const auto repeated=w::InitOnce(w::Family::First,first,recordRegister,&f);
    assert(repeated.code==w::ResultCode::AlreadyInitialized);
    assert(f.calls==1u);
    first.words[2]=0x12345678u;
    const auto noReset=w::InitOnce(w::Family::First,first,recordRegister,&f);
    assert(noReset.code==w::ResultCode::AlreadyInitialized);
    assert(first.words[2]==0x12345678u);
    assert(second.words[0]==0xBBBBBBBBu && second.nativeModeledGuard==0x08u);
    const auto initSecond=w::InitOnce(w::Family::Second,second,recordRegister,&s);
    assert(initSecond.code==w::ResultCode::InitializedAndRegistered);
    assert(initSecond.sourceIdentity->sourceInitializerVA==0x240CB2F0u);
    assert(initSecond.sourceIdentity->sourceGlobalFirstWordVA==0x24338274u);
    assert(second.nativeModeledGuard==0x09u);
    assert(s.calls==1u && s.sawReentrantAlreadyInitialized);
    assert(s.destructorVA==0x24272B10u);
    for (const auto value:second.words) assert(value==0u);
    assert(first.words[2]==0x12345678u);
    assert(w::Identity(w::Family::First).sourceCleanupNestedCallVA==0x240608F0u);
    assert(w::Identity(w::Family::Second).sourceCleanupNestedCallVA==0x240C7050u);
    assert(!w::CanPublishRetailWorld() && !w::CanExecuteOriginalCleanup());
    std::puts("PASS Phase1727 dual 85B 2006 CWorld lazy globals: guards, "
              "9xDWORD zero, registration/reentry, isolation, fail-closed; "
              "no original destructor or retail engine execution");
}
