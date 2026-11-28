// Phase1724: original SOURCE-only 191 whole native executable
// ClientClass creator body identities. No PE32 body bytes, executable
// creators, engine vtables or Source object constructors are linked.
#include "sin_compat/original_client_all_creator_body_fingerprints.hpp"
#include "sin_compat/original_client_class_catalog.hpp"
#include "sin_compat/original_client_native32_recvtable_image.hpp"
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

namespace bodies=sin_compat::original_client_all_creator_body_fingerprints;
namespace canonical=sin_compat::original_client_canonical_creator_bodies;
namespace special=sin_compat::original_client_seven_creator_exceptions;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(bodies::All191OriginalSourceBodiesIdentified());
    static_assert(!bodies::CanPublishRetailCreatorPointers());
    static_assert(special::SourceEntityCreatorFamilyComplete());
    std::ifstream registry(argv[1]);
    assert(registry.good());
    classes::Catalog catalog;
    assert(catalog.Load(registry));
    assert(catalog.Size()==191u);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    std::array<std::size_t,7> classification{};
    std::size_t totalExecutableBytes=0u;
    std::unordered_set<std::size_t> noncanonicalOrdinalSet;
    for(const auto& record:bodies::kOriginal24NoncanonicalMethods) {
        const auto ord=std::size_t(record.originalClassOrdinal);
        assert(noncanonicalOrdinalSet.insert(ord).second);
        const auto va=creators::SourceCreatorVAForOrdinal(ord);
        const auto plan=bodies::WholeSourceBodyForCreator(ord,va);
        assert(plan.kind==bodies::WholeSourceBody::OriginalNoncanonicalEntity);
        assert(plan.completeMethodBytes==record.completeOriginalMethodBytes);
        assert(plan.trailingInt3Bytes==record.originalInt3PaddingBytes);
        assert(record.originalRequestedBytes>=1152u);
        assert(record.originalRequestedBytes<=10048u);
    }
    assert(noncanonicalOrdinalSet.size()==24u);
    for(std::size_t i=0u;i<191u;++i) {
        const auto va=creators::SourceCreatorVAForOrdinal(i);
        const auto record=bodies::WholeSourceBodyForCreator(i,va);
        const auto category=static_cast<std::size_t>(record.kind);
        assert(category>0u && category<classification.size());
        assert(record.completeMethodBytes>=4u);
        assert(record.completeMethodBytes<=1000u);
        assert(record.originalSourceCreatorVA==va);
        ++classification[category];
        totalExecutableBytes+=record.completeMethodBytes;
        assert(bodies::WholeSourceBodyForCreator(i,va+1u).kind==
               bodies::WholeSourceBody::Unrecognized);
        // Original 20B ClientClass still has both creator pointers NULL:
        // fingerprinting source code NEVER creates working host callbacks.
        const auto nativeNode=image.classNodeTokens[i];
        assert(image.arena.Read32(nativeNode+0u)==0u);
        assert(image.arena.Read32(nativeNode+4u)==0u);
        if(creators::SourceCreatorIsTemp(i))
            assert(record.kind==
                   bodies::WholeSourceBody::OriginalStaticTempGetter6B);
    }
    assert(classification[1]==42u);
    assert(classification[2]==68u);
    assert(classification[3]==50u);
    assert(classification[4]==24u);
    assert(classification[5]==7u);
    assert(classification[6]==0u);
    assert(totalExecutableBytes==bodies::kOriginalConcatenatedExecutableBytes);
    assert(totalExecutableBytes==14868u);
    assert(bodies::WholeSourceBodyForCreator(191u,0u).kind==
           bodies::WholeSourceBody::Unrecognized);
    assert(!bodies::CanPublishRetailCreatorPointers());
    assert(!image.CanExposeToRetailEngine());
    std::puts("PASS Phase1724 ALL 191 source creator complete executable "
              "method fingerprints: 42 temp +68x60B +50x86B +24 complex "
              "+7 exceptional, 14868 original bytes SHA authenticated "
              "separately, native creator pointers NULL and NO slot5");
    return 0;
}
