#pragma once
// Phase 478: Source 2004 x86 bf_read candidate snapshot ABI.
//
// Authenticated SHA256 original SiN Episodes 2006 client.dll:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// Original ReadUBitLong core 0x240A4760 loads the base pointer at +0,
// bit end at +8 and cursor at +12, and marks overflow at +16.
// Original CUserMessages dispatcher 0x241D0000 copies six dwords from
// each incoming bf_read to a stack-local reader before every callback.
// Remaining byte-count/assert/debug-name fields follow Source SDK 2004
// public tier1/bitbuf.h; they are candidate ABI until further retail audit.
//
// NEVER pass arbitrary engine memory here. This receives a *preexisting
// independently owned candidate object* and an explicitly trusted exact
// storage span. It checks pointer identity without dereferencing the
// candidate pointer. The real client.dll VClient011 slot33 remains CLOSED.
#include "client_stats_owned_dispatcher.h"
#include <cstddef>
#include <cstdint>

namespace sin_independent::bf_read_candidate {
struct Layout final {
    const std::uint8_t* data=nullptr;  // +0x00
    std::int32_t data_bytes=0;         // +0x04 (SDK candidate)
    std::int32_t data_bits=0;          // +0x08
    std::int32_t current_bit=0;        // +0x0c
    std::uint8_t overflow=0;          // +0x10
    std::uint8_t assert_on_overflow=0;// +0x11 (SDK candidate)
    std::uint8_t padding[2]{};        // +0x12
    const char* debug_name=nullptr;   // +0x14 (SDK candidate, never read)
};
#if defined(_WIN32) && (defined(_M_IX86) || defined(__i386__))
static_assert(sizeof(void*)==4,"PE32 candidate only");
static_assert(sizeof(Layout)==24,"retail dispatcher copied exactly 6 DWORDs");
static_assert(offsetof(Layout,data)==0 &&
              offsetof(Layout,data_bytes)==4 &&
              offsetof(Layout,data_bits)==8 &&
              offsetof(Layout,current_bit)==12 &&
              offsetof(Layout,overflow)==16 &&
              offsetof(Layout,assert_on_overflow)==17 &&
              offsetof(Layout,debug_name)==20,
              "SDK 2004 x86 bf_read candidate field offsets");
#endif
enum class Status {
    Ready, InvalidTrustedSpan, UntrustedPointer, ReaderOverflowed,
    NegativeOrOversize, InvalidBitExtent, InvalidCursor,
    UnalignedStorage
};
inline const char* StatusName(Status status) noexcept {
    switch(status){
        case Status::Ready:return "owned-reader-snapshot-ready";
        case Status::InvalidTrustedSpan:return "invalid-trusted-span";
        case Status::UntrustedPointer:return "untrusted-candidate-pointer";
        case Status::ReaderOverflowed:return "source-reader-overflowed";
        case Status::NegativeOrOversize:return "source-reader-size-refused";
        case Status::InvalidBitExtent:return "source-reader-bit-extent-refused";
        case Status::InvalidCursor:return "source-reader-position-refused";
        case Status::UnalignedStorage:return "source-reader-storage-unaligned";
    }
    return "unknown";
}
struct TrustedBytes final {
    const std::uint8_t* data=nullptr;
    std::size_t accessible_bytes=0;
};

// The original bit-buffer core reads 32-bit words from a dword-aligned
// data pointer. We conservatively require the same alignment even
// though our owned Cursor itself reads individual bytes.
inline Status Inspect(const Layout& candidate,TrustedBytes trusted,
                      stats_bitbuf::Cursor* output=nullptr) noexcept {
    if(output)*output={};
    if(trusted.accessible_bytes>stats_bitbuf::kMaxMessageBytes ||
       (trusted.accessible_bytes!=0 && !trusted.data))
        return Status::InvalidTrustedSpan;
    if(candidate.data!=trusted.data)return Status::UntrustedPointer;
    if(candidate.data &&
       (reinterpret_cast<std::uintptr_t>(candidate.data)&3u))
        return Status::UnalignedStorage;
    if(candidate.overflow!=0)return Status::ReaderOverflowed;
    if(candidate.data_bytes<0 || candidate.data_bits<0 ||
       candidate.current_bit<0 ||
       static_cast<std::size_t>(candidate.data_bytes)>
           stats_bitbuf::kMaxMessageBytes ||
       static_cast<std::size_t>(candidate.data_bytes)>
           trusted.accessible_bytes)
        return Status::NegativeOrOversize;
    if(static_cast<std::size_t>(candidate.data_bits)>
       static_cast<std::size_t>(candidate.data_bytes)*8u)
        return Status::InvalidBitExtent;
    if(candidate.current_bit>candidate.data_bits)
        return Status::InvalidCursor;
    if(!candidate.data && candidate.data_bytes!=0)
        return Status::InvalidTrustedSpan;
    stats_bitbuf::Cursor staged;
    if(!staged.Reset(trusted.data,
                     static_cast<std::size_t>(candidate.data_bytes),
                     static_cast<std::size_t>(candidate.data_bits),
                     static_cast<std::size_t>(candidate.current_bit)))
        return Status::InvalidBitExtent;
    if(output)*output=staged;
    return Status::Ready;
}
struct DispatchResult final {
    Status snapshot=Status::InvalidTrustedSpan;
    stats_owned_dispatch::Report delivery{};
    bool delivered() const noexcept {
        return snapshot==Status::Ready && delivery.delivered();
    }
};
// A test-only composition boundary. No casts from the real engine's
// void* to Layout, no original callbacks or engine DLL calls.
inline DispatchResult DispatchOwnedCandidate(
    stats_owned_dispatch::Dispatcher& owned,int message_id,
    const Layout& candidate,TrustedBytes trusted) noexcept {
    DispatchResult result{};
    stats_bitbuf::Cursor cursor{};
    result.snapshot=Inspect(candidate,trusted,&cursor);
    if(result.snapshot==Status::Ready)
        result.delivery=owned.Dispatch(message_id,cursor);
    return result;
}
} // namespace sin_independent::bf_read_candidate
