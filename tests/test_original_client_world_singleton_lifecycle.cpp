// Phase1725: source-only original CWorld client startup, network
// creator and client shutdown preflight. There is NO original
// CWorld constructor or heap allocation implementation here.
#include "sin_compat/original_client_world_singleton_lifecycle.hpp"
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

namespace world=sin_compat::original_client_world_singleton_lifecycle;
namespace special=sin_compat::original_client_seven_creator_exceptions;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(!world::CanPublishRetailCWorldCreator());
    static_assert(!special::CanCreateRetailEntity());
    static_assert(world::PlanNativeWorldStartup()==
                  world::Status::RequiresOriginalAllocationAndConstructor);
    std::ifstream classTSV(argv[1]);
    assert(classTSV.good());
    classes::Catalog catalog;
    assert(catalog.Load(classTSV));
    assert(catalog.Size()==creators::kOriginalClasses);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    const auto originalVA=creators::SourceCreatorVAForOrdinal(75u);
    assert(originalVA==world::kWorldOriginalCreatorVA);
    assert(world::kWorldNativeSingletonGlobalVA==0x2433F3F4u);
    const auto worldSource=special::SourcePlanForEntity(75u,originalVA);
    assert(worldSource.path==special::CreatorPath::ExistingGlobalSingleton);
    assert(worldSource.requestBytes==0u);
    assert(worldSource.returnedNetworkableSubobjectOffset==8u);
    assert(worldSource.originalVirtualCallVtableByteOffset==0x94u);
    assert(world::kNativeWorldRequestBytes==1200u);
    assert(world::kNativeWorldPrimaryVirtual94VA==0x240FCB10u);
    assert(world::kNativeWorldPrimaryVirtual94VA!=0x240C8FE0u);
    assert(world::kNativeWorldSecondaryVirtual0VA==0x24054EC0u);

    // In the real 2006 PE32 original the CWorld creator dereferences
    // the global singleton BEFORE checking it for null. A host
    // implementation must NOT reproduce an invalid original access.
    const world::HostOwnedWorldBinding unbound{};
    assert(world::PlanSourceCWorldClientClassCreator(unbound)==
           world::Status::RefusedUnboundWorldObject);
    assert(world::PlanNativeWorldShutdown(unbound)==
           world::Status::WorldGlobalWouldBeClearedWithoutDestroy);

    for(const auto token:std::array<std::uint32_t,7>{{
            0u,0x2433F3F4u,0x240FC920u,0xFFFFFFFFu,
            0x10000001u,0x10000004u,0x20000000u}}) {
        const world::HostOwnedWorldBinding invalid{true,token};
        assert(world::PlanSourceCWorldClientClassCreator(invalid)==
               world::Status::RefusedUnsafeSyntheticWorldToken);
        assert(world::PlanNativeWorldShutdown(invalid)==
               world::Status::RefusedUnsafeSyntheticWorldToken);
    }
    const world::HostOwnedWorldBinding synthetic{true,0x10002000u};
    assert(world::PlanSourceCWorldClientClassCreator(synthetic)==
           world::Status::RequiresOriginalWorldNetworkInitialization);
    assert(world::PlanNativeWorldShutdown(synthetic)==
           world::Status::RequiresOriginalVirtualDestroy);
    // Source-only plan never modifies an actual global nor the owned
    // native32 20-byte ClientClass creator callback function pointers.
    const auto node=image.classNodeTokens[75u];
    assert(image.arena.Read32(node+0u)==0u);
    assert(image.arena.Read32(node+4u)==0u);
    assert(!image.CanExposeToRetailEngine());
    assert(!world::CanPublishRetailCWorldCreator());
    std::puts("PASS Phase1725 source CWorld singleton original init/shutdown "
              "preflight, 1200B bootstrap and separate world vtable+94 "
              "source target; unbound/old PE32 pointer refusal, "
              "native factory pointers NULL; NO retail slot5");
    return 0;
}
