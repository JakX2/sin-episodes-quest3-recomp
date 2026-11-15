#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
#include "sin_compat/original_client_pvs_storage_growth.hpp"

namespace n=sin_compat::original_client_pvs_storage_growth;

struct RecordingBackend final:n::IMemoryBackend {
    struct Event {n::MemoryAction action; std::size_t bytes;};
    std::vector<Event> events{};
    bool accept{true};
    bool request(n::MemoryAction action,std::size_t bytes) override {
        events.push_back({action,bytes});
        return accept;
    }
};

static void test_eight_byte_zero_seed_and_realloc() {
    n::AuxEight state;
    RecordingBackend backend;
    assert(state.grow(1,backend)==n::MemoryAction::allocate);
    assert(state.capacity==4 && state.storage.size()==4u);
    assert(backend.events.size()==1u);
    assert(backend.events[0].action==n::MemoryAction::allocate);
    assert(backend.events[0].bytes==32u);
    state.storage[0u][1u]=0xA5u;
    // Native JGE skips capacity growth, NOT the memory helper.
    assert(state.grow(0,backend)==n::MemoryAction::reallocate);
    assert(state.capacity==4 && backend.events.size()==2u);
    assert(backend.events[1].bytes==32u);
    assert(state.grow(1,backend)==n::MemoryAction::reallocate);
    assert(state.capacity==8 && state.storage.size()==8u);
    assert(backend.events.size()==3u);
    assert(backend.events[2].action==n::MemoryAction::reallocate);
    assert(backend.events[2].bytes==64u);
    assert(state.storage[0u][1u]==0xA5u);
}
static void test_sixteen_byte_zero_seed_and_realloc() {
    n::PvsSixteen state;
    RecordingBackend backend;
    assert(state.grow(1,backend)==n::MemoryAction::allocate);
    assert(state.capacity==2 && backend.events.back().bytes==32u);
    state.storage[0][11]=0xB6u;
    assert(state.grow(1,backend)==n::MemoryAction::reallocate);
    assert(state.capacity==4 && backend.events.back().bytes==64u);
    assert(state.storage[0][11]==0xB6u);
}
static void test_increment_policy_and_large_request() {
    n::AuxEight aux;
    aux.grow_policy=3;
    RecordingBackend b;
    assert(aux.grow(7,b)==n::MemoryAction::allocate);
    assert(aux.capacity==7 && b.events.back().bytes==56u);
    assert(aux.grow(2,b)==n::MemoryAction::reallocate);
    assert(aux.capacity==10 && b.events.back().bytes==80u);
    n::PvsSixteen pvs;
    pvs.grow_policy=5;
    RecordingBackend bb;
    assert(pvs.grow(12,bb)==n::MemoryAction::allocate);
    assert(pvs.capacity==12 && bb.events.back().bytes==192u);
}
static void test_native_zero_additional_still_calls_memory() {
    n::PvsSixteen state;
    RecordingBackend backend;
    // Original can request an allocation of 0 bytes if the caller asks for 0
    // while capacity is 0. The host treats a successful callback as a buffer.
    assert(state.grow(0,backend)==n::MemoryAction::allocate);
    assert(state.capacity==0 && state.storage.empty());
    assert(backend.events.size()==1u && backend.events[0].bytes==0u);
    assert(state.grow(0,backend)==n::MemoryAction::reallocate);
    assert(backend.events.size()==2u && backend.events[1].bytes==0u);
    assert(state.grow(1,backend)==n::MemoryAction::reallocate);
    assert(state.capacity==2 && backend.events[2].bytes==32u);
}
static void test_fixed_capacity_and_existing_buffer_gate() {
    n::AuxEight state;
    RecordingBackend backend;
    state.grow_policy=-1;
    assert(state.grow(100,backend)==n::MemoryAction::unchanged);
    assert(state.capacity==0 && backend.events.empty());
    state.grow_policy=0;
    state.capacity=2;
    state.storage.resize(2u);
    state.has_storage=false;
    assert(state.grow(1,backend)==n::MemoryAction::allocate);
    assert(state.capacity==4 && backend.events.back().bytes==32u);
    state.has_storage=true;
    assert(state.grow(1,backend)==n::MemoryAction::reallocate);
    assert(state.capacity==8 && backend.events.back().bytes==64u);
}
static void test_reject_backend_invalid_and_signed_overflow() {
    RecordingBackend backend;
    n::AuxEight state;
    backend.accept=false;
    bool refused=false;
    try {(void)state.grow(1,backend);}
    catch(const std::runtime_error&) {refused=true;}
    assert(refused && state.capacity==0 && !state.has_storage);
    assert(state.storage.empty() && backend.events.size()==1u);
    backend.accept=true;
    bool invalid=false;
    try {(void)state.grow(-1,backend);}
    catch(const std::out_of_range&) {invalid=true;}
    assert(invalid && backend.events.size()==1u);
    state.grow_policy=-2;
    invalid=false;
    try {(void)state.grow(1,backend);}
    catch(const std::out_of_range&) {invalid=true;}
    assert(invalid && backend.events.size()==1u);
    state.grow_policy=0;
    state.capacity=std::numeric_limits<std::int32_t>::max();
    bool overflow=false;
    try {(void)state.grow(1,backend);}
    catch(const std::overflow_error&) {overflow=true;}
    assert(overflow && backend.events.size()==1u);
    state.capacity=1<<30;
    overflow=false;
    try {(void)state.grow(1,backend);}
    catch(const std::overflow_error&) {overflow=true;}
    assert(overflow && backend.events.size()==1u);
}
int main() {
    static_assert(n::kAuxGrowthVA==0x24106010u);
    static_assert(n::kPvsGrowthVA==0x24106090u);
    static_assert(n::kOriginalNativeBytes==226u);
    static_assert(n::kManagerStoragePointerDword==0u);
    static_assert(n::kManagerCapacityDword==4u);
    static_assert(n::kManagerGrowPolicyDword==8u);
    static_assert(n::kOpaqueOriginalExistingStorageHelper==0x2416F960u);
    static_assert(n::kOpaqueOriginalNullStorageHelper==0x2416F8F0u);
    static_assert(sizeof(n::AuxEight::Record)==8u);
    static_assert(sizeof(n::PvsSixteen::Record)==16u);
    test_eight_byte_zero_seed_and_realloc();
    test_sixteen_byte_zero_seed_and_realloc();
    test_increment_policy_and_large_request();
    test_native_zero_additional_still_calls_memory();
    test_fixed_capacity_and_existing_buffer_gate();
    test_reject_backend_invalid_and_signed_overflow();
    return 0;
}
