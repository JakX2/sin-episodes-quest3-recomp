// Phase1719: native source 40B zeroing allocator *SUCCESS PATH* only.
// Synthetic host-owned bytes: never actual original 2006 PE32 engine
// allocation, constructor, vtable, entity, executable callback or slot5.
#include "sin_compat/original_client_source_zero_init_allocator.hpp"
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
#include <vector>

namespace zero=sin_compat::original_client_source_zero_init_allocator;
namespace sizes=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(sizes::SourceAllocationInventoryConsistent());
    static_assert(!zero::CanCreateRetailEntity());
    static_assert(!zero::CanPublishRetailCreatorPointers());
    std::ifstream classTSV(argv[1]);
    assert(classTSV.good());
    classes::Catalog catalog;
    assert(catalog.Load(classTSV));
    assert(catalog.Size()==creators::kOriginalClasses);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    // Verify source x86 REP STOSD, then REP STOSB tail semantics on an
    // explicitly owned test vector, including arbitrary odd byte
    // counts. No allocation failure behavior is asserted here.
    for(std::size_t count=0u;count<=129u;++count) {
        std::vector<std::uint8_t> original(200u,0xa5u);
        assert(zero::ZeroOriginalSizeIntoOwnedBuffer(count,original)
               ==zero::Status::PreparedOwnedZeroBytes);
        for(std::size_t i=0u;i<count;++i)
            assert(original[i]==0u);
        for(std::size_t i=count;i<original.size();++i)
            assert(original[i]==0xa5u);
    }
    {
        std::vector<std::uint8_t> tooShort(129u,0x6eu);
        const auto before=tooShort;
        assert(zero::ZeroOriginalSizeIntoOwnedBuffer(
            130u,tooShort)==zero::Status::RefusedBadOwnedMemory);
        assert(tooShort==before);
    }

    // All 142 actual original 2006 ClientClass entity creator prologues
    // request allocation via the same zero-initializing helper.
    // Allocate fake HOST-owned byte arrays only, at exact original
    // request size, then prove no constructors or callbacks were run.
    std::size_t verified=0u;
    std::array<bool,creators::kOriginalClasses> observed{};
    for(const auto& record:sizes::kOriginalAllocationPrologues) {
        const auto originalVA=record.originalCreatorVA;
        const auto ordinal=std::size_t(record.originalClassOrdinal);
        assert(ordinal<creators::kOriginalClasses);
        assert(!creators::SourceCreatorIsTemp(ordinal));
        assert(creators::SourceCreatorVAForOrdinal(ordinal)==originalVA);
        assert(!observed[ordinal]);
        observed[ordinal]=true;
        std::vector<std::uint8_t> bytes(37u,0x5cu);
        const auto state=zero::PrepareSyntheticOriginalEntityZeroedBytes(
            ordinal,originalVA,bytes);
        assert(state==zero::Status::PreparedOwnedZeroBytes);
        assert(bytes.size()==record.sourceObjectByteCount);
        for(auto byte:bytes)assert(byte==0u);
        // Preflight must not synthesize the original vtable or expose
        // any native executable original creator callback address.
        const auto nativeNode=image.classNodeTokens[ordinal];
        assert(image.arena.Read32(nativeNode+0u)==0u);
        assert(image.arena.Read32(nativeNode+4u)==0u);
        ++verified;
    }
    assert(verified==142u);

    const std::array<std::size_t,7> special{{
        48u,69u,75u,120u,143u,170u,190u}};
    for(const auto ordinal:special) {
        assert(!creators::SourceCreatorIsTemp(ordinal));
        assert(!observed[ordinal]);
        std::vector<std::uint8_t> old(127u,0xbeu);
        const auto saved=old;
        assert(zero::PrepareSyntheticOriginalEntityZeroedBytes(
            ordinal,creators::SourceCreatorVAForOrdinal(ordinal),old)
            ==zero::Status::UnrecognizedSource);
        assert(old==saved);
    }
    // Wrong source callback VA or an original temporary-event creator
    // must not be used to manufacture a fake entity even if its desired
    // synthetic buffer would fit in memory.
    {
        std::vector<std::uint8_t> old(13u,0x55u);
        const auto prior=old;
        assert(zero::PrepareSyntheticOriginalEntityZeroedBytes(
            0u,creators::SourceCreatorVAForOrdinal(0u)+1u,old)
            ==zero::Status::UnrecognizedSource);
        assert(zero::PrepareSyntheticOriginalEntityZeroedBytes(
            191u,0u,old)==zero::Status::UnrecognizedSource);
        for(std::size_t i=0u;i<191u;++i)
            if(creators::SourceCreatorIsTemp(i)) {
                assert(zero::PrepareSyntheticOriginalEntityZeroedBytes(
                    i,creators::SourceCreatorVAForOrdinal(i),old)
                    ==zero::Status::UnrecognizedSource);
            }
        assert(old==prior);
    }
    assert(!image.CanExposeToRetailEngine());
    assert(!zero::CanCreateRetailEntity());
    std::puts("PASS Phase1719 original source 40B zero-init allocator "
              "success behavior, 142 source-created owned-zero buffers, "
              "7 special/42 event creators refused, no retail slot5");
    return 0;
}
