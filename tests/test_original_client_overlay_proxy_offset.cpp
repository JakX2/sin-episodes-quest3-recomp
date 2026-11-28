#include "sin_compat/original_client_overlay_proxy_offset.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <vector>

namespace native=sin_compat::original_client_overlay_proxy_offset;

static void put32(std::vector<std::uint8_t>& v,std::size_t at,
                  std::uint32_t x) {
    assert(at+4<=v.size());
    for(unsigned i=0;i<4u;++i)
        v[at+i]=static_cast<std::uint8_t>(x>>(8u*i));
}

static void test_both_original_paths_and_native_bounds() {
    static_assert(native::kOriginalProxyAtHelperPlus4VA==0x24127150u);
    static_assert(native::kOriginalProxyAtHelperPlus0VA==0x24127180u);
    static_assert(native::kOriginalAuxMetadataPointerPropOffset==20u);
    static_assert(native::kOriginalRecvPropIndexOffset==48u);
    static_assert(native::kOriginalOverlayByteStride==32u);
    static_assert(native::kOriginalOverlayCount==15u);
    assert(!native::CanExposeToRetailEngine());
    std::vector<std::uint8_t> originalStruct(32u,0xCCu);
    // Host fixture stores a DWORD base offset in source container at
    // helper+0x14-specified index 8. This is NOT a native engine pointer.
    put32(originalStruct,8u,1956u);
    for(const auto callback:{
        native::CallbackSlot::HelperPlus0,
        native::CallbackSlot::HelperPlus4}) {
        for(std::uint32_t idx=0u;idx<15u;++idx) {
            const native::Input input{idx,32u,8u,callback};
            const auto result=native::Calculate(input,originalStruct);
            assert(result.valid && !result.CanInvokeRetailCallback());
            assert(result.callbackSlot==callback);
            assert(result.computedOriginalProxyOffset==1956u+idx*32u);
        }
    }
    const native::Input good{14u,32u,8u,native::CallbackSlot::HelperPlus4};
    {
        auto wrong=good;wrong.originalIndex=15u;
        assert(!native::Calculate(wrong,originalStruct).valid);
        wrong=good;wrong.originalStride=16u;
        assert(!native::Calculate(wrong,originalStruct).valid);
        wrong=good;wrong.sourceContainerFieldOffset=30u;
        assert(!native::Calculate(wrong,originalStruct).valid);
        wrong=good;wrong.callbackSlot=static_cast<native::CallbackSlot>(8u);
        assert(!native::Calculate(wrong,originalStruct).valid);
        assert(!native::Calculate(good,std::vector<std::uint8_t>{}).valid);
    }
    {
        std::vector<std::uint8_t> overflow=originalStruct;
        put32(overflow,8u,std::numeric_limits<std::uint32_t>::max());
        assert(!native::Calculate(good,overflow).valid);
        put32(overflow,8u,0u);
        const auto result=native::Calculate(good,overflow);
        assert(result.valid && result.computedOriginalProxyOffset==448u);
    }
}

int main() {
    test_both_original_paths_and_native_bounds();
    std::puts("PASS Phase1699 original two overlay proxy arithmetic paths: type5 index x 32 + parent field, distinct callback slots +0/+4, bounds and overflow fail closed; no native callback invocation");
    return 0;
}
