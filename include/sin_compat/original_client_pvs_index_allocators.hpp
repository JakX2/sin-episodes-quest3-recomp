#pragma once
// Phase234, original 2006 SiN client.dll allocator dispatch:
//   0x24106570: 8-byte auxiliary indices; 0x24106610: 16-byte PVS indices.
// Clean-room, engine-neutral host reference, NOT the Source game allocator.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_pvs_index_allocators {
inline constexpr std::uint32_t kAuxAllocatorVA=0x24106570u;
inline constexpr std::uint32_t kPvsAllocatorVA=0x24106610u;
inline constexpr std::size_t kNativeReviewedBytes=308u;
inline constexpr std::uint16_t kAbsent=0xFFFFu;
inline constexpr std::size_t kNativeFreeHeadWord=0x10u;
inline constexpr std::size_t kNativeNextUnusedWord=0x14u;
inline constexpr std::size_t kNativeCapacityDword=0x04u;
inline constexpr std::size_t kNativeBasePointerDword=0x00u;
inline constexpr std::size_t kNativeCachedBasePointerDword=0x18u;

// The two original methods have identical control flow, but different record
// strides and prev/next WORD offsets. Payload bytes are deliberately opaque.
template<std::size_t Stride,std::size_t Prev,std::size_t Next>
struct Allocator {
    static_assert((Stride==8u&&Prev==4u&&Next==6u)||
                  (Stride==16u&&Prev==12u&&Next==14u),
                  "only the two independently verified native layouts");
    using Record=std::array<std::uint8_t,Stride>;
    std::vector<Record> records;
    std::uint16_t free_head{kAbsent};
    std::uint16_t next_unused{};
    std::size_t cached_base_refreshes{};

    explicit Allocator(std::size_t capacity):records(capacity) {
        if(capacity>kAbsent)
            throw std::out_of_range("PVS allocator capacity exceeds native 16-bit indices");
    }
    std::uint16_t read_word(std::uint16_t index,std::size_t offset) const {
        if(index>=records.size())throw std::out_of_range("PVS allocator index");
        const auto& r=records[index];
        return static_cast<std::uint16_t>(r[offset]|
               (static_cast<unsigned>(r[offset+1])<<8));
    }
    void write_word(std::uint16_t index,std::size_t offset,std::uint16_t value) {
        if(index>=records.size())throw std::out_of_range("PVS allocator index");
        records[index][offset]=static_cast<std::uint8_t>(value);
        records[index][offset+1]=static_cast<std::uint8_t>(value>>8);
    }
    // Synthetic way to supply a free-chain head; native free producers live
    // in separate helpers. This does not claim to recover their semantics.
    void seed_free(std::uint16_t index,std::uint16_t next_free) {
        write_word(index,Next,next_free);
        free_head=index;
    }
    struct Growth {
        virtual ~Growth()=default;
        // Original direct CALL 0x24106010 / 0x24106090 with PUSH 1.
        // No claim about the full body or allocator's memory ownership.
        virtual void request_one(std::vector<Record>& records)=0;
    };
    std::uint16_t allocate(bool absent_links,Growth* grow=nullptr) {
        std::uint16_t index;
        if(free_head==kAbsent) {
            if(next_unused==records.size()) {
                if(!grow)throw std::out_of_range("native growth callback required");
                grow->request_one(records);
            }
            // The original invokes tier0!Error on index 0xFFFF; the host
            // rejects it without writing through a sentinel/out-of-bounds slot.
            if(next_unused==kAbsent||next_unused>=records.size()||
               records.size()>kAbsent)
                throw std::out_of_range("PVS allocator cannot reserve native index");
            index=next_unused++;
        } else {
            index=free_head;
            const auto successor=read_word(index,Next);
            free_head=successor;
        }
        write_word(index,Prev,absent_links?kAbsent:index);
        write_word(index,Next,absent_links?kAbsent:index);
        ++cached_base_refreshes; // original [this+0x18]=[this+0x00]
        return index;
    }
};
using AuxEight=Allocator<8u,4u,6u>;
using PvsSixteen=Allocator<16u,12u,14u>;
} // namespace sin_compat::original_client_pvs_index_allocators
