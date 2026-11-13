#pragma once
// Phase211: independently written portable C++17 references for THREE
// SHA-authenticated original 2006 SiN client.dll snapshot-related routines.
// Native pointers / external engine serializer objects are NOT reconstructed.
// All words are synthetic host tokens; never call original tier0.dll by this ABI.
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_set>

namespace sin_compat::original_client_prediction_snapshot_dispatch {
inline constexpr std::uint32_t kOriginalArgPackConstructorVA=0x241978a0u;
inline constexpr std::uint32_t kOriginalDispatcherVA=0x24199ba0u;
inline constexpr std::uint32_t kOriginalRecordWalkVA=0x24199b50u;
inline constexpr std::uint32_t kExternalPreprocessVA=0x24197f50u;
inline constexpr std::uint32_t kExternalPrepareVA=0x24199100u;
inline constexpr std::uint32_t kExternalRecordWriterVA=0x241996f0u;
inline constexpr std::uint32_t kProcessGlobalCounterVA=0x242fdb50u;
inline constexpr std::uint32_t kArgPackByteSize=0x34u;
inline constexpr std::uint32_t kResultWordOffset=0x28u;

// Exact 52-byte native memory span; no unproved field/source-class names.
// Unwritten bytes must remain unchanged: the original function does NOT
// clear the entire object. This includes holes at +0x17 and +0x26..0x27.
struct ArgumentBlock32 {
    std::array<std::uint8_t,kArgPackByteSize> bytes{};
    std::uint32_t get_word(std::uint32_t off) const {
        if(off+4u>bytes.size())throw std::out_of_range("argpack DWORD read");
        return std::uint32_t(bytes[off]) |
            (std::uint32_t(bytes[off+1u])<<8u) |
            (std::uint32_t(bytes[off+2u])<<16u) |
            (std::uint32_t(bytes[off+3u])<<24u);
    }
    void put_word(std::uint32_t off,std::uint32_t value) {
        if(off+4u>bytes.size())throw std::out_of_range("argpack DWORD write");
        for(std::uint32_t i=0;i!=4u;++i)
            bytes[off+i]=std::uint8_t(value>>(8u*i));
    }
};
static_assert(sizeof(ArgumentBlock32)==kArgPackByteSize);

// Original 104-byte __thiscall-compatible entrypoint at 0x241978A0.
// 10 original stack DWORD arguments; ret 0x28. Native booleans and small
// flags use only the LOW BYTE of arguments 3, 5..9.
// arg[0..9] below correspond exactly to the original source-order stack.
inline ArgumentBlock32& construct_argument_block(
    ArgumentBlock32& out,const std::array<std::uint32_t,10>& arg) {
    out.put_word(0x00u,arg[0]);
    out.put_word(0x04u,arg[1]);
    out.put_word(0x08u,arg[3]);
    out.put_word(0x0cu,std::uint8_t(arg[2])!=0u ? 1u : 0u);
    out.put_word(0x10u,std::uint8_t(arg[4])!=0u ? 1u : 0u);
    out.bytes[0x14u]=std::uint8_t(arg[5]);
    out.bytes[0x15u]=std::uint8_t(arg[6]);
    out.bytes[0x16u]=std::uint8_t(arg[8]);
    out.put_word(0x18u,0u);
    out.put_word(0x1cu,0u);
    out.put_word(0x20u,0u);
    out.bytes[0x24u]=0u;
    out.bytes[0x25u]=0u;
    out.put_word(0x28u,0u);
    out.bytes[0x2cu]=std::uint8_t(arg[7]);
    out.put_word(0x30u,arg[9]);
    return out;
}

// A single 2006 serializer record is only partially understood.
// Original +0x00, +0x04 and +0x0C are read by 0x24199B50;
// original +0x10 is tested on the first/head record by 0x24199BA0.
struct RecordView32 {
    std::uint32_t word_00{};
    std::uint32_t word_04{};
    std::uint32_t next_token_0c{};
    std::uint8_t byte_10{};
};

// Original 65-byte 0x24199B50 immediately dispatches the FIRST/head record,
// then advances through +0x0C and forwards four arguments per record to
// 0x241996F0: (process counter, record token, word_00, word_04).
// Importantly, the next pointer is read AFTER the opaque callback returns.
// Synthetic host-only cycle/bounds checks prevent the native infinite loop.
template<class Host>
void walk_original_records(ArgumentBlock32& state,Host& host,
                           std::uint32_t process_count,
                           std::uint32_t head_token) {
    if(head_token==0u)
        throw std::invalid_argument("host-only null original record head");
    std::unordered_set<std::uint32_t> visited;
    for(std::uint32_t current=head_token;current!=0u;) {
        if(visited.size()>=4096u || !visited.insert(current).second)
            throw std::runtime_error("host-only original record cycle/limit");
        const RecordView32 snapshot=host.load_record(current);
        host.write_record_241996f0(state,process_count,current,
                                   snapshot.word_00,snapshot.word_04);
        current=host.load_record(current).next_token_0c;
    }
}

// Original 71-byte 0x24199BA0 increments 2006 global +0x242FDB50
// BEFORE reading descriptor byte +0x10 or invoking any callback.
// If byte +0x10 is zero: call opaque helper 0x24197F50(descriptor).
// Then call 0x24199100(this,arg1,arg2,descriptor), and call the original
// walker using the GLOBAL COUNTER read *after* that preparation callback.
// The native return is the current DWORD at state+0x28, not a computed
// success/failure code. No guessed snapshot serializer implementation.
template<class Host>
std::uint32_t dispatch_original_snapshot(ArgumentBlock32& state,Host& host,
                                         std::uint32_t& process_global_count,
                                         std::uint32_t first_arg,
                                         std::uint32_t second_arg,
                                         std::uint32_t descriptor_token) {
    process_global_count+=1u; // native ADD DWORD wraps modulo 2^32
    const auto descriptor=host.load_record(descriptor_token);
    if(descriptor.byte_10==0u)
        host.preprocess_24197f50(descriptor_token);
    host.prepare_24199100(state,first_arg,second_arg,descriptor_token);
    walk_original_records(state,host,process_global_count,descriptor_token);
    return state.get_word(kResultWordOffset);
}

} // namespace sin_compat::original_client_prediction_snapshot_dispatch
