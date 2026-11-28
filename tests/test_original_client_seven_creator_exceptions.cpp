// Phase1721: authenticate all 149 original entity creator entrypoint
// strategies without creating an actual 2006 PE32 engine entity.
#include "sin_compat/original_client_seven_creator_exceptions.hpp"
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
#include <string_view>

namespace exceptions=sin_compat::original_client_seven_creator_exceptions;
namespace common=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(exceptions::SourceEntityCreatorFamilyComplete());
    static_assert(!exceptions::CanCreateRetailEntity());
    static_assert(!creators::CanPublishRetailCreatorPointers());
    std::ifstream originalClassTSV(argv[1]);
    assert(originalClassTSV.good());
    classes::Catalog catalog;
    assert(catalog.Load(originalClassTSV));
    assert(catalog.Size()==191u);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    struct Case final {
        std::size_t ordinal;
        std::string_view networkName;
        exceptions::CreatorPath route;
        std::uint32_t allocationBytes;
        std::uint32_t helperVA;
        std::uint16_t returnOffset;
        std::uint16_t callOffset;
    };
    constexpr std::array<Case,7> special{{
        {48u,"CInfoTeleporterCountdown",
         exceptions::CreatorPath::SharedZeroingHelper,1168u,
         0x240C4FF0u,8u,0x94u},
        {69u,"CRagdollPropAttached",
         exceptions::CreatorPath::SharedZeroingHelper,3408u,
         0x240C4FF0u,8u,0x94u},
        {75u,"CWorld",exceptions::CreatorPath::ExistingGlobalSingleton,
         0u,0x2433F3F4u,8u,0x94u},
        {120u,"CBaseEntity",
         exceptions::CreatorPath::DirectAllocatorManualZero,1152u,
         0x2416F8F0u,8u,0x94u},
        {143u,"CTEParticleSystem",
         exceptions::CreatorPath::RawAllocator,28u,
         0x2416FCF0u,4u,0x30u},
        {170u,"CTEBaseBeam",
         exceptions::CreatorPath::RawAllocator,76u,
         0x2416FCF0u,4u,0x30u},
        {190u,"CBaseTempEntity",
         exceptions::CreatorPath::RawAllocator,16u,
         0x2416FCF0u,4u,0x30u},
    }};
    const auto* cls=catalog.Head();
    std::size_t sourceEntities=0u, tempEvents=0u,commonRouted=0u,
                variantRouted=0u,allocationRequests=0u,
                singletonWorld=0u;
    for(std::size_t ordinal=0u;ordinal<creators::kOriginalClasses;++ordinal) {
        assert(cls!=nullptr);
        const auto va=creators::SourceCreatorVAForOrdinal(ordinal);
        const auto plan=exceptions::SourcePlanForEntity(ordinal,va);
        const auto ownedNode=image.classNodeTokens[ordinal];
        assert(image.arena.Read32(ownedNode+0u)==0u);
        assert(image.arena.Read32(ownedNode+4u)==0u);
        if(cls->originalTempEvent) {
            ++tempEvents;
            assert(plan.path==exceptions::CreatorPath::NotAnEntity);
        }else {
            ++sourceEntities;
            assert(plan.path!=exceptions::CreatorPath::NotAnEntity);
            assert(plan.originalCreatorVA==va);
            if(plan.path==exceptions::CreatorPath::SharedCommonZeroingHelper) {
                ++commonRouted;
                assert(plan.requestBytes>=1152u &&
                       plan.requestBytes<=10048u);
                assert(plan.originalAllocatorOrGlobalVA==0x240C4FF0u);
                assert(common::SourceAllocationForClass(ordinal,va)!=nullptr);
                // No original return-subobject offset is asserted for
                // the 142 creator BODIES; only their first 11 source
                // bytes are independently authenticated at Phase1718.
                assert(plan.returnedNetworkableSubobjectOffset==0u);
            }else {
                ++variantRouted;
                std::size_t found=0u;
                for(const auto& item:special) {
                    if(item.ordinal!=ordinal)continue;
                    assert(cls->networkName==item.networkName);
                    assert(plan.path==item.route);
                    assert(plan.requestBytes==item.allocationBytes);
                    assert(plan.originalAllocatorOrGlobalVA==item.helperVA);
                    assert(plan.returnedNetworkableSubobjectOffset==
                           item.returnOffset);
                    assert(plan.originalVirtualCallVtableByteOffset==
                           item.callOffset);
                    ++found;
                }
                assert(found==1u);
            }
            if(plan.path==exceptions::CreatorPath::ExistingGlobalSingleton) {
                assert(ordinal==75u && plan.requestBytes==0u);
                ++singletonWorld;
            }else ++allocationRequests;
        }
        cls=cls->next;
    }
    assert(cls==nullptr);
    assert(tempEvents==42u && sourceEntities==149u);
    assert(commonRouted==142u && variantRouted==7u);
    assert(allocationRequests==148u && singletonWorld==1u);
    const auto invalid=exceptions::SourcePlanForEntity(
        75u,creators::SourceCreatorVAForOrdinal(75u)+1u);
    assert(invalid.path==exceptions::CreatorPath::NotAnEntity);
    assert(invalid.requestBytes==0u);
    assert(exceptions::SourcePlanForEntity(191u,0u).path==
           exceptions::CreatorPath::NotAnEntity);
    assert(!exceptions::CanCreateRetailEntity());
    assert(!image.CanExposeToRetailEngine());
    std::puts("PASS Phase1721 original 149 entity creators: 142 common + "
              "six special allocations / CWorld singleton, 42 temp "
              "event getters excluded, all native creator pointers NULL; "
              "NO retail slot5");
    return 0;
}
