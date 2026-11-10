#pragma once
// Phase171: exact native entry-point branch models from original 2006 SiN
// server.dll. Not original MSVC32 ABI nor a fabricated registry hash map.
#include <cstdint>
#include <vector>

namespace sin_compat::original_server_factory_dispatch {
inline constexpr std::uint16_t kNotFound = 0xffffu;
inline constexpr std::uint32_t kRecordSize = 16u;
inline constexpr std::uint32_t kFactoryOffsetWithinRecord = 12u;
inline constexpr std::uint32_t kPrimaryLookupWrapperVA = 0x22285550u;
inline constexpr std::uint32_t kDestroyLookupWrapperVA = 0x22285580u;
inline constexpr std::uint32_t kRegistryLookupVA = 0x2228B010u;
inline constexpr std::uint32_t kInternedKeyLookupVA = 0x2228F7A0u;
inline constexpr std::uint32_t kNativeMapFindVA = 0x2228A760u;
inline constexpr std::uint32_t kPrimaryMissingDiagnosticVA = 0x22455F50u;
inline constexpr std::uint32_t kDestroyMissingDiagnosticVA = 0x22455F80u;

// A 16-byte registry record is addressable via index*16. The original
// pointer is read from record+12, not from the string key itself.
struct RegistryRecord {
    std::uint32_t opaque_0=0,opaque_4=0,opaque_8=0;
    std::uintptr_t factory=0;
};

// Original 0x2228B010 (94 native bytes): obtain interned key word from
// +0x20 owner table (actually ECX=registry+0x20), find a 16-bit slot in
// the index at registry+0x04, and return factory pointer at +0x0C.
// All native hash/interner and associative-index operations remain callbacks.
// Unlike native unguarded record access, the reference validates the index
// before reading the supplied host vector, to avoid undefined behavior.
template<class Host>
std::uintptr_t original_registry_lookup(const char* classname,
        const std::vector<RegistryRecord>& records,Host& host) {
    const std::uint16_t symbol=host.intern_classname(classname);
    const std::uint16_t slot=host.find_registry_index(symbol,0u);
    if(slot==kNotFound)return 0;
    if(static_cast<std::size_t>(slot)>=records.size()) {
        host.report_invalid_registry_slot(slot);
        return 0;
    }
    return records[slot].factory;
}

// Original 0x2228AEA0 (18 bytes): zero-extend 16-bit parameter, left
// shift 4, return pointer to record+0x0C without any bounds checking.
inline constexpr std::uint32_t original_record_factory_byte_offset(
        std::uint16_t index) noexcept {
    return static_cast<std::uint32_t>(index)*kRecordSize+
           kFactoryOffsetWithinRecord;
}

// Original wrapper 0x22285550, 46 bytes. On successful lookup, it invokes
// the per-class factory virtual offset +0; misses report the native warning.
// Original vtable+4 registry create wrapper and per-class virtual+0.
// The original static convention also passes the classname into that virtual.
template<class Host>
std::uintptr_t original_primary_factory_dispatch(
        const char* classname,const std::vector<RegistryRecord>& records,
        Host& host) {
    const std::uintptr_t factory=original_registry_lookup(classname,records,host);
    if(factory==0){host.report_missing_primary(kPrimaryMissingDiagnosticVA,classname);
                   return 0;}
    return host.invoke_primary_factory(factory,classname);
}

// Original registry destroy wrapper 0x22285580, 50 bytes. A successful
// registry lookup invokes per-class factory virtual +4 with the second
// caller argument; on miss, prints native "destroy unknown entity" warning.
template<class Host>
void original_destroy_factory_dispatch(
        const char* classname,std::uintptr_t original_argument,
        const std::vector<RegistryRecord>& records,Host& host) {
    const std::uintptr_t factory=original_registry_lookup(classname,records,host);
    if(factory==0){host.report_missing_destroy(kDestroyMissingDiagnosticVA,
                                                   classname);return;}
    host.invoke_destroy_factory(factory,original_argument);
}
} // namespace sin_compat::original_server_factory_dispatch