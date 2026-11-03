#pragma once
// SiN Episodes (2006) original x86 call-order reference; not SourceVR ABI.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::phase92 {
struct CacheRecordIO {
    std::string_view original_rtti;
    std::uint32_t original_writer, original_reader;
    bool float_prefix;
    std::uint8_t count_byte_offset, entries_byte_offset;
};
inline constexpr std::array<CacheRecordIO,2> cache_contracts{{
    {"CSceneCache",0x221a7360u,0x221ab0a0u,true,0x14,0x08},
    {"CModelSoundCache",0x221437e0u,0x221453e0u,false,0x10,0x04}
}};
inline constexpr std::size_t temporary_read_limit=0x200;
inline constexpr std::uint32_t unresolved_token=0xffffffffu;

// Original writer: scene floats first; both emit u16 count; then
// resolve each owned entry through global vtable+0x18 and pass the
// result to writer helper 0x2228DCE0. Host callbacks, not raw I/O.
template<class Float,class Count,class Token,class Write>
void describe_original_write(const CacheRecordIO& cache,std::uint32_t float_bits,
                             std::uint16_t size,Float&& put_float,Count&& put_count,
                             Token&& lookup_token,Write&& write_token) {
    if(cache.float_prefix)put_float(float_bits);
    put_count(size);
    for(std::uint32_t i=0;i<size;++i)write_token(lookup_token(i));
}

// Original reader: scene first reads x87 float into object+4; both
// read u16 count, request a 0x200-byte temporary per item, resolve
// through global vtable+0x0c, and skip 0xFFFFFFFF results.
// This deliberately does NOT decode on-disk cache record bytes.
template<class Float,class Count,class Buffer,class Resolve,class Append>
void describe_original_read(const CacheRecordIO& cache,Float&& get_float,
                            Count&& get_count,Buffer&& read_buffer,
                            Resolve&& resolve_token,Append&& append) {
    if(cache.float_prefix)get_float();
    const std::uint16_t count=get_count();
    for(std::uint32_t i=0;i<count;++i) {
        const auto temporary=read_buffer(temporary_read_limit);
        const std::uint32_t symbol=resolve_token(temporary);
        if(symbol!=unresolved_token)append(symbol);
    }
}

enum class PanelReleaseShape {Single,DoubleLoadingDisc,SingleNulling};
struct HudPanelLifecycle {
    std::string_view original_class;
    std::uint32_t original_shutdown_method;
    PanelReleaseShape shape;
};
inline constexpr std::array<HudPanelLifecycle,8> hud_panel_lifecycle{{
    {"CDebugOverlayPanel",0x241eca60u,PanelReleaseShape::Single},
    {"CNetGraphPanelInterface",0x241ef9f0u,PanelReleaseShape::Single},
    {"CFPS",0x241ed1a0u,PanelReleaseShape::Single},
    {"CMessageChars",0x241eea30u,PanelReleaseShape::Single},
    {"CLoadingDisc",0x241ee480u,PanelReleaseShape::DoubleLoadingDisc},
    {"CCenterPrint",0x241ec510u,PanelReleaseShape::SingleNulling},
    {"CTextMessage",0x241f2320u,PanelReleaseShape::Single},
    {"CStatsPanelInterface",0x24207670u,PanelReleaseShape::SingleNulling}
}};
struct PanelPointers {void* offset4=nullptr; void* offset8=nullptr;};

// Original conditional 2-step release: +0x98(0), RELOAD member,
// then +0x74(1) only if still nonnull. LoadingDisc repeats for +8,
// CenterPrint/StatsPanel set member +4 to null after both calls.
template<class First,class Second>
void describe_original_hud_shutdown(const HudPanelLifecycle& panel,PanelPointers& state,
                                    First&& first,Second&& second) {
    auto release_member=[&](void*& member) {
        if(!member)return;
        first(member,0,0x98);
        if(member)second(member,1,0x74);
    };
    release_member(state.offset4);
    if(panel.shape==PanelReleaseShape::DoubleLoadingDisc)
        release_member(state.offset8);
    if(panel.shape==PanelReleaseShape::SingleNulling)state.offset4=nullptr;
}
} // namespace sin_compat::phase92
