#pragma once
// Phase172: original 2006 SiN server.dll registry provider registration
// at 0x2228B140, 0x2228B190, lookup slot 0x2228AEC0 and hash 0x2228B1E0.
// Host-neutral observable behavior only; no original MSVC x86 ABI.
#include <cstdint>

namespace sin_compat::original_server_registry_registration {

inline constexpr std::uint16_t kMissingIndex=0xffffu;
inline constexpr std::uint32_t kRecordStride=16u;
inline constexpr std::uint32_t kRecordProviderOffset=12u;
inline constexpr std::uint32_t kNativeHashSeed=0x1505u;
inline constexpr std::uint32_t kNativeHashMultiplier=33u;
inline constexpr std::uint32_t kOriginalRegistryVtableVA=0x22456324u;

// Original 0x2228B1E0 (40 bytes) reads each byte using MOVSX after a
// byte load and computes 33*accumulator+signed-byte with 32-bit wrap.
// This differs from unsigned-byte DJB2 on code points >=0x80.
inline std::uint32_t original_classname_hash(const char* name) noexcept {
    std::uint32_t result=kNativeHashSeed;
    const auto* cursor=reinterpret_cast<const unsigned char*>(name);
    for(;;) {
        const unsigned char byte=*cursor++;
        if (byte==0u) break;
        const std::int32_t signed_byte=byte<0x80u
            ? static_cast<std::int32_t>(byte)
            : static_cast<std::int32_t>(byte)-0x100;
        result=result*kNativeHashMultiplier+static_cast<std::uint32_t>(signed_byte);
    }
    return result;
}

struct RegistryKey { std::uintptr_t registry_instance=0; };

// Original 0x2228AEC0 (60 bytes): 16-bit symbol intern via 0x2228F7A0
// then lookup by +0x04 table via 0x2228A760; returns AX including 0xffff.
template<class Host>
std::uint16_t original_lookup_existing_slot(RegistryKey key,
                                             const char* classname,Host& host) {
    const std::uint16_t symbol=host.intern_symbol(key,classname);
    return host.find_existing_slot(key,symbol,0u);
}

// Original registry vtable +0x00, 0x2228B190 (78 bytes): call native
// intern 0x2228F7A0 and find-or-insert 0x2228B0A0. Write the caller's
// second 32-bit value directly to registry storage[slot*16+12].
// Crucially, this is a REGISTRATION setter, NOT a registry destructor.
// The original never checks out-of-bounds indices; the host must provide
// valid indices, or explicitly reject the invalid native precondition.
template<class Host>
std::uint16_t original_register_direct_provider(
    RegistryKey key,const char* classname,std::uint32_t provider_word,Host& host) {
    const std::uint16_t symbol=host.intern_symbol(key,classname);
    const std::uint16_t slot=host.find_or_insert_slot(key,symbol);
    host.write_provider_word(key,slot,kRecordStride,kRecordProviderOffset,
                             provider_word);
    return slot;
}

// Original 0x2228B140 (79 bytes), called with an embedded registry+0x04
// subobject as ECX: same intern/find-or-insert, but loads the SECOND
// argument as a pointer to a 32-bit word and stores that word to the
// same 16-byte record+0x0C. The original assumes the pointer is valid.
template<class Host>
std::uint16_t original_register_indirect_provider(
    RegistryKey embedded_key,const char* classname,
    const std::uint32_t* provider_word_ptr,Host& host) {
    const std::uint16_t symbol=host.intern_symbol(embedded_key,classname);
    const std::uint16_t slot=host.find_or_insert_slot(embedded_key,symbol);
    // Read only AFTER the registration slot is obtained, as native code does.
    const std::uint32_t provider_word=*provider_word_ptr;
    host.write_provider_word(embedded_key,slot,kRecordStride,
                             kRecordProviderOffset,provider_word);
    return slot;
}
} // namespace sin_compat::original_server_registry_registration