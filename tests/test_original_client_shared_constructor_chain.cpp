// Phase1723 original 2006 PE32 constructor dependency SOURCE graph.
// All three original VAs are labels, not callable native constructors.
// Never install original function pointers in independent ClientClass.
#include "sin_compat/original_client_shared_constructor_chain.hpp"
#include "sin_compat/original_client_native32_recvtable_image.hpp"
#include "sin_compat/original_client_class_catalog.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <unordered_set>

namespace shared=sin_compat::original_client_shared_constructor_chain;
namespace source=sin_compat::original_client_full_entity_creator_bodies;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(shared::SourceChainInventoryConsistent());
    static_assert(!shared::CanRunOriginalConstructorChain());
    std::ifstream originalCatalog(argv[1]);
    assert(originalCatalog.good());
    classes::Catalog catalog;
    assert(catalog.Load(originalCatalog));
    assert(catalog.Size()==191u);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    // Original x86 A -> B -> C first CALLs; the first original
    // internal CALL inside C -> an unresolved fourth source method.
    constexpr auto& three=shared::kOriginalFunctionChain;
    static_assert(three.size()==3u);
    assert(three[0].originalFunctionVA==0x24092BD0u);
    assert(three[0].completeExecutableBytes==206u);
    assert(three[0].firstCallOffset==3u);
    assert(three[0].firstCallTargetVA==three[1].originalFunctionVA);
    assert(three[0].trailingInt3Bytes==2u);
    assert(three[1].originalFunctionVA==0x240BD7C0u);
    assert(three[1].completeExecutableBytes==861u);
    assert(three[1].firstCallOffset==7u);
    assert(three[1].firstCallTargetVA==three[2].originalFunctionVA);
    assert(three[1].trailingInt3Bytes==3u);
    assert(three[2].originalFunctionVA==0x240CCE30u);
    assert(three[2].completeExecutableBytes==573u);
    assert(three[2].firstCallOffset==86u);
    assert(three[2].firstCallTargetVA==
           shared::kUnreconstructedNestedConstructorVA);
    assert(three[2].trailingInt3Bytes==3u);

    std::unordered_set<std::uint32_t> targets;
    std::size_t directA=0u,directC=0u,unclassified=0u;
    std::array<bool,191u> sourceCreatorMapped{};
    for(const auto& record:source::kSourceCompleteCreatorBodies) {
        const auto ord=std::size_t(record.originalClassOrdinal);
        assert(ord<191u);
        assert(!creators::SourceCreatorIsTemp(ord));
        assert(creators::SourceCreatorVAForOrdinal(ord)==record.originalCreatorVA);
        sourceCreatorMapped[ord]=true;
        targets.insert(record.originalConstructorTargetVA);
        const auto route=shared::RouteForSourceConstructorTarget(
            record.originalConstructorTargetVA);
        if(route==shared::SourceConstructorRoute::DirectAThroughBToC) {
            assert(record.originalConstructorTargetVA==
                   shared::kNativeSharedConstructorA);
            ++directA;
        }else if(route==shared::SourceConstructorRoute::DirectC) {
            assert(record.originalConstructorTargetVA==
                   shared::kNativeSharedConstructorC);
            ++directC;
        }else {
            assert(route==shared::SourceConstructorRoute::UnsupportedOriginalTarget);
            ++unclassified;
        }
        // Even a source-authenticated chain of COMPLETE original x86
        // constructor functions does not imply host allocators,
        // native vtables or cross-platform executable callbacks.
        const auto node=image.classNodeTokens[ord];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
    }
    assert(directA==34u && directC==26u);
    assert(unclassified==82u && directA+directC==60u);
    assert(targets.size()==73u);
    assert(shared::RouteForSourceConstructorTarget(
        0x240BD7C0u)==shared::SourceConstructorRoute::UnsupportedOriginalTarget);
    assert(shared::RouteForSourceConstructorTarget(
        0u)==shared::SourceConstructorRoute::UnsupportedOriginalTarget);
    assert(shared::RouteForSourceConstructorTarget(
        0xFFFFFFFFu)==shared::SourceConstructorRoute::UnsupportedOriginalTarget);

    // All class registry positions stay original, 42 temp-event
    // creator addresses source-backed, and 7 special creators
    // outside this 142 common-source method family.
    std::size_t seven=0u,tempEvents=0u;
    for(std::size_t i=0u;i<191u;++i) {
        if(creators::SourceCreatorIsTemp(i)) {
            ++tempEvents;
            assert(!sourceCreatorMapped[i]);
        }else if(!sourceCreatorMapped[i])++seven;
        const auto node=image.classNodeTokens[i];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
    }
    assert(tempEvents==42u && seven==7u);
    assert(!image.CanExposeToRetailEngine());
    assert(!shared::CanRunOriginalConstructorChain());
    std::puts("PASS Phase1723 native source 206B->861B->573B "
              "constructor chain; 34 original direct A + 26 direct C "
              "source callers, 73 target families, no executable "
              "ClientClass creators or retail slot5");
    return 0;
}
