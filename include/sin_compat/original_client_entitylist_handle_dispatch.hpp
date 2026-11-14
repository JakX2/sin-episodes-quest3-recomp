#pragma once
// Phase230: clean-room C++17 host reference for seven complete-SHA-reviewed
// original 2006 SiN client.dll entity-list methods. All entity pointers
// are 32-bit ID TOKENS; no original Source engine object is dereferenced.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_entitylist_handle_dispatch {
using Bytes=std::vector<std::uint8_t>;

inline constexpr std::uint32_t kOriginalVClientEntityList003Vtable=0x24297390u;
inline constexpr std::uint32_t kOriginalEmbeddedInterfaceOffset=0x10014u;
inline constexpr std::uint32_t kOriginalSlotMask=0xFFFu;
inline constexpr std::uint32_t kOriginalSerialShift=12u;
inline constexpr std::uint32_t kOriginalSlotStride=16u;
inline constexpr std::uint32_t kOriginalSlotIndexBias=0x1001u;
inline constexpr std::uint32_t kOriginalParentEntryOffset=4u;
inline constexpr std::uint32_t kOriginalMaxSlot=0x7FFu;
inline constexpr std::uint32_t kOriginalHighestOccupiedOffset=0x10024u;
inline constexpr std::size_t kVerifiedOriginalNativeMethods=7u;
inline constexpr std::size_t kVerifiedOriginalNativeBytes=262u;

struct Method {
    std::uint32_t native_va{};
    std::int32_t vtable_slot{}; // -1 means private/parent helper, not interface slot.
    std::uint32_t code_bytes{};
};
inline constexpr Method kMethods[]={
    {0x24105AF0u,3,36u}, // index -> table entity -> entity vtable+0x18
    {0x24105B20u,-1,48u}, // original max-live-slot recomputation
    {0x24105B50u,-1,34u}, // parent table index -> entity vtable+0x1C
    {0x24105B80u,-1,34u}, // parent table index -> entity vtable+0x0C
    {0x24105BB0u,1,33u}, // handle resolver -> entity vtable+0x10
    {0x24105BE0u,4,33u}, // handle resolver -> entity vtable+0x18
    {0x241074D0u,2,44u}, // EHandle slot and serial resolver
};

inline std::uint32_t load_u32(const Bytes& bytes,std::size_t offset) {
    if(offset>bytes.size() || bytes.size()-offset<4)
        throw std::out_of_range("original client entity list host bounds");
    return std::uint32_t(bytes[offset]) |
           (std::uint32_t(bytes[offset+1u])<<8) |
           (std::uint32_t(bytes[offset+2u])<<16) |
           (std::uint32_t(bytes[offset+3u])<<24);
}
inline void write_u32(Bytes& bytes,std::size_t offset,std::uint32_t value) {
    if(offset>bytes.size() || bytes.size()-offset<4)
        throw std::out_of_range("original client entity list host write bounds");
    for(unsigned shift=0;shift<32u;shift+=8u)
        bytes[offset+shift/8u]=static_cast<std::uint8_t>((value>>shift)&0xffu);
}
// Signed relative pointer displacement from the *embedded* interface;
// original i386 hardware wraps unchecked. The host refuses invalid
// pointer arithmetic and makes the representable path explicit.
inline std::size_t offset_by(std::size_t base,std::int64_t delta) {
    if(delta<0) {
        const std::uint64_t magnitude=static_cast<std::uint64_t>(-delta);
        if(magnitude>base)
            throw std::out_of_range("original embedded this relative offset precedes host buffer");
        return base-static_cast<std::size_t>(magnitude);
    }
    const auto positive=static_cast<std::uint64_t>(delta);
    if(positive>std::numeric_limits<std::size_t>::max() ||
       positive>std::numeric_limits<std::size_t>::max()-base)
        throw std::overflow_error("original entity list index overflows host pointer size");
    return base+static_cast<std::size_t>(positive);
}
// Original interface slot 2 at 0x241074D0:
//   handle == 0xFFFFFFFF -> null;
//   table address = interface this + ((handle & 0xFFF)-0x1001)*16;
//   valid when table-entry serial DWORD[+4] == handle>>12;
//   return table-entry entity pointer DWORD[+0].
inline std::uint32_t interface_slot2_resolve_handle(
        const Bytes& parent,std::size_t interface_this_offset,
        std::uint32_t handle) {
    if(handle==0xFFFFFFFFu)return 0u;
    const std::int64_t slot=static_cast<std::int64_t>(handle & kOriginalSlotMask);
    const auto at=offset_by(interface_this_offset,
        (slot-static_cast<std::int64_t>(kOriginalSlotIndexBias))*
        static_cast<std::int64_t>(kOriginalSlotStride));
    const auto serial=load_u32(parent,offset_by(at,4));
    if(serial!=(handle>>kOriginalSerialShift))return 0u;
    return load_u32(parent,at);
}
struct IEntityVirtualHost {
    virtual ~IEntityVirtualHost()=default;
    // Original entity vtable function at a BYTE offset; implementer
    // owns token resolution and actual virtual method side effects.
    virtual std::uint32_t invoke(std::uint32_t entity_pointer_token,
                                 std::uint32_t vtable_byte_offset)=0;
};
inline std::uint32_t invoke_if_valid(std::uint32_t ptr,
        std::uint32_t virtual_byte_offset,IEntityVirtualHost& host) {
    if(ptr==0u)return 0u;
    return host.invoke(ptr,virtual_byte_offset);
}
// Original interface slot 1: resolve raw EHandle via *native slot2*
// and invoke the resolved entity's vtable +0x10. Null returns zero.
inline std::uint32_t interface_slot1_handle_dispatch_10(
        const Bytes& parent,std::size_t interface_this,
        std::uint32_t handle,IEntityVirtualHost& host) {
    return invoke_if_valid(interface_slot2_resolve_handle(parent,interface_this,handle),
                           0x10u,host);
}
// Original interface slot 4: same resolver; target entity vtable +0x18.
inline std::uint32_t interface_slot4_handle_dispatch_18(
        const Bytes& parent,std::size_t interface_this,
        std::uint32_t handle,IEntityVirtualHost& host) {
    return invoke_if_valid(interface_slot2_resolve_handle(parent,interface_this,handle),
                           0x18u,host);
}
// Original interface slot 3 uses a SIGNED 32-bit index, NOT an
// EHandle serial; entity pointer comes directly from the indexed slot.
inline std::uint32_t interface_slot3_index_dispatch_18(
        const Bytes& parent,std::size_t interface_this,
        std::int32_t index,IEntityVirtualHost& host) {
    if(index<0)return 0u;
    const auto at=offset_by(interface_this,
        (static_cast<std::int64_t>(index)-
         static_cast<std::int64_t>(kOriginalSlotIndexBias))*
        static_cast<std::int64_t>(kOriginalSlotStride));
    return invoke_if_valid(load_u32(parent,at),0x18u,host);
}
// Unlike the embedded interface functions, these are PARENT-object
// helpers: ECX points at containing object, and the object pointer
// lives at parent+4+index*16 (no serial validation in these helpers).
inline std::uint32_t parent_index_dispatch(const Bytes& parent,
        std::size_t parent_this,std::int32_t index,
        std::uint32_t virtual_byte_offset,IEntityVirtualHost& host) {
    if(index<0)return 0u;
    const auto displacement=static_cast<std::int64_t>(kOriginalParentEntryOffset)+
        static_cast<std::int64_t>(index)*kOriginalSlotStride;
    const auto at=offset_by(parent_this,displacement);
    return invoke_if_valid(load_u32(parent,at),virtual_byte_offset,host);
}
inline std::uint32_t parent_index_dispatch_1c(const Bytes& parent,
        std::size_t parent_this,std::int32_t index,IEntityVirtualHost& host) {
    return parent_index_dispatch(parent,parent_this,index,0x1Cu,host);
}
inline std::uint32_t parent_index_dispatch_0c(const Bytes& parent,
        std::size_t parent_this,std::int32_t index,IEntityVirtualHost& host) {
    return parent_index_dispatch(parent,parent_this,index,0x0Cu,host);
}
// Original parent helper 0x24105B20: stores 0xFFFFFFFF at parent+0x10024;
// scans index 2047 down to 0 and stores highest slot whose pointer at
// parent+4+index*16 is nonzero. Host preflights the whole bounded table.
inline std::int32_t parent_recompute_highest_live_slot(
        Bytes& parent,std::size_t parent_this) {
    const auto highest_field=offset_by(parent_this,kOriginalHighestOccupiedOffset);
    const auto last_slot=offset_by(parent_this,
        static_cast<std::int64_t>(kOriginalParentEntryOffset)+
        static_cast<std::int64_t>(kOriginalMaxSlot)*kOriginalSlotStride);
    if(highest_field>parent.size() || parent.size()-highest_field<4 ||
       last_slot>parent.size() || parent.size()-last_slot<4)
        throw std::out_of_range("host-only incomplete original entity list table");
    write_u32(parent,highest_field,0xFFFFFFFFu);
    for(std::int32_t i=static_cast<std::int32_t>(kOriginalMaxSlot);
        i>=0;--i) {
        const auto at=offset_by(parent_this,
            static_cast<std::int64_t>(kOriginalParentEntryOffset)+
            static_cast<std::int64_t>(i)*kOriginalSlotStride);
        if(load_u32(parent,at)!=0u) {
            write_u32(parent,highest_field,static_cast<std::uint32_t>(i));
            return i;
        }
    }
    return -1;
}
} // namespace sin_compat::original_client_entitylist_handle_dispatch
