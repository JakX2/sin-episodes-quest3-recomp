#pragma once
// Phase1699: source-based portable arithmetic for two ORIGINAL 2006 PE32
// specialized animation overlay receive proxies at 0x24127150/0x24127180.
//
// Both read an overlay property at native RecvProp +0x14 (helper ptr),
// its array index at RecvProp +0x30, the helper stride at +0x10 and
// the helper's parent-container field offset at +0x14. They compute
// proxyOffset = originalStructDword[helperContainerOffset] + index*stride.
//
// 0x24127150 dispatches through original helper callback pointer +4.
// 0x24127180 dispatches through original helper callback pointer +0.
// The real callbacks and their four/three native stack arguments remain
// UNIMPLEMENTED; no native pointer or retail engine call is fabricated.
// Parent bytes in this host reference are owned, bounds-checked test
// fixtures. Exact original functions are whole-SHA verified separately.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace sin_compat::original_client_overlay_proxy_offset {

inline constexpr std::uint32_t kOriginalProxyAtHelperPlus4VA=0x24127150u;
inline constexpr std::uint32_t kOriginalProxyAtHelperPlus0VA=0x24127180u;
inline constexpr std::uint32_t kOriginalAuxMetadataPointerPropOffset=20u;
inline constexpr std::uint32_t kOriginalAuxMetadataStrideOffset=16u;
inline constexpr std::uint32_t kOriginalAuxMetadataParentFieldOffset=20u;
inline constexpr std::uint32_t kOriginalRecvPropIndexOffset=48u;
inline constexpr std::uint32_t kOriginalOverlayCount=15u;
inline constexpr std::uint32_t kOriginalOverlayByteStride=32u;

enum class CallbackSlot : std::uint32_t { HelperPlus0=0u, HelperPlus4=4u };

struct Input final {
    std::uint32_t originalIndex=0u;         // original RecvProp +0x30
    std::uint32_t originalStride=0u;        // original helper +0x10
    std::uint32_t sourceContainerFieldOffset=0u; // helper +0x14
    CallbackSlot callbackSlot=CallbackSlot::HelperPlus0;
};

struct Result final {
    bool valid=false;
    std::uint32_t computedOriginalProxyOffset=0u;
    CallbackSlot callbackSlot=CallbackSlot::HelperPlus0;
    bool CanInvokeRetailCallback() const noexcept {return false;}
};

// Exact source arithmetic is 32-bit IMUL+ADD; we refuse arithmetic
// overflow rather than silently wrap a misconfigured destination and
// risk out-of-bounds process-memory writes. Native code does NOT do
// this validation. An actual runtime may require additional policies.
inline Result Calculate(const Input& input,
                        const std::vector<std::uint8_t>& sourceStruct) noexcept {
    Result out{};
    if(input.originalIndex>=kOriginalOverlayCount ||
       input.originalStride!=kOriginalOverlayByteStride ||
       (input.callbackSlot!=CallbackSlot::HelperPlus0 &&
        input.callbackSlot!=CallbackSlot::HelperPlus4))
        return out;
    const auto at=static_cast<std::size_t>(input.sourceContainerFieldOffset);
    if(at>sourceStruct.size() || sourceStruct.size()-at<4u)
        return out;
    std::uint32_t base=0u;
    for(unsigned i=0;i<4u;++i)
        base|=std::uint32_t(sourceStruct[at+i])<<(i*8u);
    const auto computed=std::uint64_t(base)+
        std::uint64_t(input.originalIndex)*input.originalStride;
    if(computed>std::numeric_limits<std::uint32_t>::max())return out;
    out.valid=true;
    out.computedOriginalProxyOffset=static_cast<std::uint32_t>(computed);
    out.callbackSlot=input.callbackSlot;
    return out;
}
inline constexpr bool CanExposeToRetailEngine() noexcept {return false;}

} // namespace sin_compat::original_client_overlay_proxy_offset
