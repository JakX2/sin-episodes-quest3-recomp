// Phase1660: original-client C_BasePlayer local pointer and prediction
// host-data snapshot, strictly bounded read-only PE32 original-DLL witness.
//
// Exact owner original client.dll SHA gate belongs to the calling probe.
// This observer is original-free in CI. It checks the current-player
// global RVA 0x338CF0 against the actual 2006 C_BasePlayer primary
// MSVC vptr RVA 0x28AAE4 BEFORE inspecting host+0x1DC/+0xF04..+0xF0C.
// Original CPrediction slots 8 and 13 are NEVER invoked. No writable
// global, actual client/server engine, or gameplay object is constructed.
//
// A subclass may have a different vptr and legitimately be refused.
// Never equate "unknown vptr" with an invalid player or a playable game.
// The repeated vptr/global checks detect simple recycling; they cannot
// lock the original engine's lifetime. Owner probe does not run in-game.
#pragma once

#include "original_client_entitylist_readonly_snapshot.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_independent {
namespace original_client_local_player_readonly_snapshot {
namespace abi=original_client_entitylist_snapshot;

constexpr std::uintptr_t kPlayerGlobalRva=0x338CF0u;
constexpr std::uintptr_t kOriginalCBasePlayerPrimaryVtableRva=0x28AAE4u;
constexpr std::uintptr_t kOriginalScalarOffset=0x1DCu;
constexpr std::uintptr_t kOriginalViewOffset=0xF04u;
constexpr std::uintptr_t kOriginalViewLastDwordOffset=kOriginalViewOffset+8u;
constexpr std::uintptr_t kLastRequiredOffset=kOriginalViewLastDwordOffset+4u;
constexpr unsigned kReadCountVerified=8u;
static_assert(kLastRequiredOffset==0xF10u,
              "last original CPrediction view DWORD extent changed");

enum class Status {
    MissingReader,
    InvalidModuleBase,
    GlobalUnreadable,
    NoLocalPlayer,
    InvalidPlayerAddress,
    VptrUnreadable,
    UnrecognizedDynamicVptr,
    ScalarUnreadable,
    VectorUnreadable,
    VptrChanged,
    GlobalChanged,
    VerifiedExactBasePlayer,
};
struct Result {
    Status status=Status::MissingReader;
    unsigned successful_reads=0u;
    std::uint32_t scalar=0u;
    std::array<std::uint32_t,3> vector_bits{};
    bool exact_primary_vptr_match=false;
    unsigned retail_virtual_invocations=0u;
    unsigned retail_method_invocations=0u;
};
inline const char* StatusName(Status s) noexcept {
    switch(s) {
    case Status::MissingReader:return "missing_reader";
    case Status::InvalidModuleBase:return "invalid_module_base";
    case Status::GlobalUnreadable:return "player_global_unreadable";
    case Status::NoLocalPlayer:return "no_local_player";
    case Status::InvalidPlayerAddress:return "invalid_player_address";
    case Status::VptrUnreadable:return "player_vptr_unreadable";
    case Status::UnrecognizedDynamicVptr:return "unrecognized_player_vptr";
    case Status::ScalarUnreadable:return "player_scalar_unreadable";
    case Status::VectorUnreadable:return "player_view_word_unreadable";
    case Status::VptrChanged:return "player_vptr_changed";
    case Status::GlobalChanged:return "local_player_pointer_changed";
    case Status::VerifiedExactBasePlayer:return "exact_c_baseplayer_readonly_snapshot";
    }
    return "unknown_status";
}
inline bool RangeWithinPE32(std::uintptr_t base,
                           std::uintptr_t last_byte) noexcept {
    return base!=0u &&
           base<=static_cast<std::uintptr_t>(0xFFFFFFFFu)-last_byte;
}
inline Result Observe(std::uintptr_t client_base,abi::Reader reader) noexcept {
    Result out{};
    if(!reader.read)return out;
    // The global RVA and vtable RVA both refer to the SAME pinned
    // original 2006 client.dll, not any Source/HL2 alternative.
    constexpr auto kMaxModuleRva=kPlayerGlobalRva+4u;
    if(!RangeWithinPE32(client_base,kMaxModuleRva)) {
        out.status=Status::InvalidModuleBase;
        return out;
    }
    std::uint32_t player=0u;
    if(!reader.read(reader.context,client_base+kPlayerGlobalRva,&player)) {
        out.status=Status::GlobalUnreadable;
        return out;
    }
    ++out.successful_reads;
    if(player==0u) {
        // Expected for a DLL that has only run static constructors
        // and DllMain, before an engine creates local player entities.
        out.status=Status::NoLocalPlayer;
        return out;
    }
    if((player&3u)!=0u || !RangeWithinPE32(player,kLastRequiredOffset)) {
        out.status=Status::InvalidPlayerAddress;
        return out;
    }
    const auto target_vptr=static_cast<std::uint32_t>(
        client_base+kOriginalCBasePlayerPrimaryVtableRva);
    std::uint32_t actual_vptr=0u;
    if(!reader.read(reader.context,player,&actual_vptr)) {
        out.status=Status::VptrUnreadable;
        return out;
    }
    ++out.successful_reads;
    if(actual_vptr!=target_vptr) {
        // A derived game class can override the primary C_BasePlayer
        // vptr, so never follow unknown vptr or claim class parity.
        out.status=Status::UnrecognizedDynamicVptr;
        return out;
    }
    out.exact_primary_vptr_match=true;
    if(!reader.read(reader.context,player+kOriginalScalarOffset,
                    &out.scalar)) {
        out.status=Status::ScalarUnreadable;
        return out;
    }
    ++out.successful_reads;
    for(unsigned i=0u;i<3u;++i) {
        if(!reader.read(reader.context,
                        player+kOriginalViewOffset+i*4u,
                        &out.vector_bits[i])) {
            out.status=Status::VectorUnreadable;
            return out;
        }
        ++out.successful_reads;
    }
    // Read the vptr and the owning global AGAIN. A changed pointer
    // must not be presented as a stable player snapshot. This does
    // not guarantee lifetime synchronization with another thread.
    std::uint32_t verify_vptr=0u,verify_global=0u;
    if(!reader.read(reader.context,player,&verify_vptr)) {
        out.status=Status::VptrUnreadable;
        return out;
    }
    ++out.successful_reads;
    if(verify_vptr!=actual_vptr) {
        out.status=Status::VptrChanged;
        return out;
    }
    if(!reader.read(reader.context,client_base+kPlayerGlobalRva,
                    &verify_global)) {
        out.status=Status::GlobalUnreadable;
        return out;
    }
    ++out.successful_reads;
    if(verify_global!=player) {
        out.status=Status::GlobalChanged;
        return out;
    }
    out.status=Status::VerifiedExactBasePlayer;
    return out;
}
} // namespace original_client_local_player_readonly_snapshot
} // namespace sin_independent
