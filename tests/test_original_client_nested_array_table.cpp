#include "sin_compat/original_client_nested_array_table.hpp"
#include "sin_compat/original_client_recvtable_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace arrays=sin_compat::original_client_nested_array_table;
namespace graph=sin_compat::original_client_recvtable_preflight;

static void check_original_native_source_values() {
    static_assert(arrays::kOriginalElementDescriptorBytes==60);
    static_assert(arrays::kOriginalNestedRecvTableBytes==20);
    static_assert(arrays::kSourceArrayShapeCount==11);
    static_assert(arrays::kOriginalTotalGeneratedElements==579);
    static_assert(arrays::kOriginalOuterNativeType==5);
    static_assert(arrays::kSourceElementOffsetField==44);
    static_assert(arrays::kOriginalElementNamePointerTableVA==0x242fb110u);
    static_assert(arrays::kOriginalElementNamePointerCount==256u);
    static_assert(graph::kNativeNestedTableType==5);
}

static void verify_exact_shaped_elements_and_boundaries() {
    std::size_t sum=0u;
    std::unordered_set<std::string> names;
    graph::Graph schemas;
    std::vector<std::size_t> roots;
    for(const auto& source:arrays::kOriginalShapes) {
        assert(arrays::FindShape(source.clientClass,source.networkProperty)==&source);
        const auto key=std::string(source.clientClass)+":"+
                       std::string(source.networkProperty);
        assert(names.emplace(key).second);
        const auto ready=arrays::Build(source);
        assert(ready.valid && !ready.CanExposeToRetailEngine());
        assert(ready.outerNativeType==5);
        assert(ready.elements.size()==source.elements);
        for(std::size_t i=0;i<ready.elements.size();++i) {
            assert(ready.elements[i].elementIndex==i);
            assert(ready.elements[i].nativeType==source.elementNativeType);
            assert(ready.elements[i].originalX86MemberOffset==
                   source.firstX86Offset+i*source.byteStride);
            const auto& originalName=ready.elements[i].originalElementName;
            assert(originalName[0]==static_cast<char>('0'+(i/100u)));
            assert(originalName[1]==static_cast<char>('0'+((i/10u)%10u)));
            assert(originalName[2]==static_cast<char>('0'+(i%10u)));
            assert(originalName[3]=='\0');
            assert(std::string(originalName.data()).size()==3u);
        }
        assert(std::string(ready.elements.front().originalElementName.data())=="000");
        assert(ready.elements.front().originalX86MemberOffset==
               source.firstX86Offset);
        if(source.elements==256u)
            assert(std::string(ready.elements.back().originalElementName.data())=="255");
        assert(ready.elements.back().originalX86MemberOffset==
               source.firstX86Offset+
               (source.elements-1u)*source.byteStride);
        sum+=source.elements;
        const auto wrapper=schemas.AddTable("SOURCE_WRAPPER_"+key);
        const auto nested=schemas.AddTable("GENERATED_CHILD_"+key);
        roots.push_back(wrapper);
        assert(schemas.AddProperty(wrapper, std::string(source.networkProperty),5));
        assert(schemas.LinkNested(wrapper,0,nested));
        for(const auto& element:ready.elements)
            assert(schemas.AddProperty(nested,
                std::string(element.originalElementName.data()),
                static_cast<int>(element.nativeType)));
        // Mutations of authenticated COUNT/STRIDE/BASE/element type must
        // fail, not create plausible but nonoriginal x86 offsets.
        {
            auto broken=source; ++broken.elements;
            assert(!arrays::Build(broken).valid);
            broken=source; broken.byteStride=2;
            assert(!arrays::Build(broken).valid);
            broken=source; ++broken.firstX86Offset;
            assert(!arrays::Build(broken).valid);
            broken=source;broken.elementNativeType^=1u;
            assert(!arrays::Build(broken).valid);
            broken=source;broken.elements=0;
            assert(!arrays::Build(broken).valid);
        }
    }
    assert(names.size()==11 && sum==arrays::kOriginalTotalGeneratedElements);
    const auto report=schemas.Validate(roots);
    assert(report.good() && report.visitedTables==22 &&
           report.seenProperties==sum+11 &&
           report.nestedEdges==11 && report.revisitEdges==0);
    assert(!schemas.CanExposeToRetailEngine());
    // Native source case-specific witnesses: 33 4-byte pings and
    // a 256-byte surface bit-vector, with exact original x86 offsets.
    const auto* ping=arrays::FindShape("CPlayerResource","m_iPing");
    const auto* surface=arrays::FindShape("CBreakableSurface","m_RawPanelBitVec");
    const auto* pose=arrays::FindShape("CBaseAnimating","m_flPoseParameter");
    assert(ping && surface && pose);
    assert(ping->elements==33 && ping->byteStride==4 &&
           arrays::Build(*ping).elements.back().originalX86MemberOffset==2340);
    assert(surface->elements==256 && surface->byteStride==1 &&
           arrays::Build(*surface).elements.back().originalX86MemberOffset==1491);
    assert(pose->elements==24 && pose->byteStride==4 &&
           arrays::Build(*pose).elements.back().originalX86MemberOffset==1444);
    assert(!arrays::FindShape("CNPC_GruntBase","m_notOriginal"));
}

int main() {
    check_original_native_source_values();
    verify_exact_shaped_elements_and_boundaries();
    std::puts("PASS Phase1697 original 11 dynamic nested array source shapes: 579 generated elements, type5 outer, 0/1 element templates, exact x86 counts/strides/offsets and mutation refusal; no retail RecvTable pointers");
    return 0;
}
