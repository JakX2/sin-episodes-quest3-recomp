#include "sin_compat/original_client_aux_dynamic_receivers.hpp"
#include "sin_compat/original_client_recvtable_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

namespace native=sin_compat::original_client_aux_dynamic_receivers;
namespace graph=sin_compat::original_client_recvtable_preflight;

static void assert_native_offsets_and_all_original_aux_source_names() {
    static_assert(native::kGenericNestedArrayCount==3u);
    static_assert(native::kGenericGeneratedElementCount==112u);
    static_assert(native::kGenericNestedOriginalFactoryVA==0x24126E90u);
    static_assert(native::kOverlayOriginalFactoryVA==0x241271B0u);
    static_assert(native::kArray4OriginalFactoryVA==0x24126BD0u);
    assert(!native::CanExposeToRetailEngine());
    std::unordered_set<std::string> seen;
    std::size_t sum=0u;
    graph::Graph schemas;
    std::vector<std::size_t> roots;
    for(const auto& source:native::kSourceShapes) {
        const auto key=std::string(source.auxTable)+":"+std::string(source.networkProperty);
        assert(seen.emplace(key).second);
        assert(native::FindShape(source.auxTable,source.networkProperty)==&source);
        const auto built=native::Build(source);
        assert(built.valid && !built.CanExposeToRetailEngine());
        assert(built.elements.size()==source.elements);
        for(std::size_t i=0u;i<built.elements.size();++i) {
            const auto& item=built.elements[i];
            assert(item.index==i);
            assert(item.originalChildRelativeMemberOffset==i*source.byteStride);
            assert(item.derivedParentObjectX86Offset==
                   source.originalOuterParentX86Offset+i*source.byteStride);
            assert(item.originalNativeElementType==0u);
            const auto name=std::string(item.originalName.data());
            assert(name.size()==3u);
            assert(name==std::string({
                static_cast<char>('0'+(i/100u)),
                static_cast<char>('0'+((i/10u)%10u)),
                static_cast<char>('0'+(i%10u))
            }));
            assert(item.originalName[3]=='\0');
        }
        assert(built.elements.front().originalChildRelativeMemberOffset==0u);
        assert(std::string(built.elements.front().originalName.data())=="000");
        sum+=built.elements.size();
        const auto parent=schemas.AddTable("OUTER:"+key);
        const auto child=schemas.AddTable("CHILD:"+key);
        roots.push_back(parent);
        assert(schemas.AddProperty(parent,std::string(source.networkProperty),5));
        assert(schemas.LinkNested(parent,0u,child));
        for(const auto& item:built.elements)
            assert(schemas.AddProperty(child,
                std::string(item.originalName.data()),0));
        {
            auto broken=source; ++broken.elements;
            assert(!native::Build(broken).valid);
            broken=source;broken.byteStride=2;
            assert(!native::Build(broken).valid);
            broken=source;++broken.originalOuterParentX86Offset;
            assert(!native::Build(broken).valid);
            broken=source;broken.originalElementType=1u;
            assert(!native::Build(broken).valid);
            broken=source;broken.elements=0u;
            assert(!native::Build(broken).valid);
        }
    }
    assert(seen.size()==3u && sum==112u);
    const auto report=schemas.Validate(roots);
    assert(report.good() && report.visitedTables==6u &&
           report.seenProperties==115u && report.nestedEdges==3u);
    assert(!schemas.CanExposeToRetailEngine());
    assert(!native::FindShape("DT_FabricatedUnknown","bad"));
    // Parent offset must remain separate from child-native byte +44.
    auto* weapons=native::FindShape("DT_BCCLocalPlayerExclusive","m_hMyWeapons");
    auto* bits=native::FindShape("DT_Local","m_chAreaBits");
    auto* ammo=native::FindShape("DT_LocalPlayerExclusive","m_iAmmo");
    assert(weapons && bits && ammo);
    assert(native::Build(*weapons).elements.back().originalChildRelativeMemberOffset==188u);
    assert(native::Build(*weapons).elements.back().derivedParentObjectX86Offset==3388u);
    assert(native::Build(*bits).elements.back().originalChildRelativeMemberOffset==31u);
    assert(native::Build(*bits).elements.back().derivedParentObjectX86Offset==35u);
    assert(native::Build(*ammo).elements.back().originalChildRelativeMemberOffset==124u);
    assert(native::Build(*ammo).elements.back().derivedParentObjectX86Offset==3196u);
}

static void preserve_special_original_facts_without_fake_callbacks() {
    static_assert(native::kSourceType4ViewModel.nativeType==4u);
    static_assert(native::kSourceType4ViewModel.count==2u);
    static_assert(native::kSourceType4ViewModel.byteWidth==4u);
    assert(native::kSourceType4ViewModel.auxTable=="DT_LocalPlayerExclusive");
    assert(native::kSourceType4ViewModel.networkProperty=="m_hViewModel");
    // There is no child table to link for native type4; do not treat it
    // as a fourth generic dynamic nested receiver.
    static_assert(native::kSourceOverlay.sourceRequestedCount==15u);
    static_assert(native::kSourceOverlay.sourceAllocatedDescriptors==16u);
    static_assert(native::kSourceOverlay.byteStride==32u);
    static_assert(native::kSourceOverlay.originalOuterParentX86Offset==0x7A4u);
    static_assert(native::kSourceOverlay.originalSourceProxyA==0x24127150u);
    static_assert(native::kSourceOverlay.originalSourceProxyB==0x24127180u);
    assert(native::kSourceOverlay.auxTable=="DT_OverlayVars");
    assert(native::kSourceOverlay.networkProperty=="m_AnimOverlay");
    assert(!native::CanExposeToRetailEngine());
}

int main() {
    assert_native_offsets_and_all_original_aux_source_names();
    preserve_special_original_facts_without_fake_callbacks();
    std::puts("PASS Phase1699 three source auxiliary nested arrays: 112 child-local original offsets with names 000..047, distinct type4 viewmodel and 15+1 overlay facts; retail ABI gated");
    return 0;
}
