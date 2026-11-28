// Phase1722: original 2006 PE32 118 complete canonical entity creator
// SOURCE-ONLY ABI plans. Do not execute source constructors, import 2006
// entity vtables, or publish retail native ClientClass factory pointers.
#include "sin_compat/original_client_canonical_creator_bodies.hpp"
#include "sin_compat/original_client_shared_constructor_scalars.hpp"
#include "sin_compat/original_client_seven_creator_exceptions.hpp"
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

namespace canonical=sin_compat::original_client_canonical_creator_bodies;
namespace common=sin_compat::original_client_entity_allocation_prologues;
namespace allcreators=sin_compat::original_client_seven_creator_exceptions;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(canonical::SourceCanonicalInventoryConsistent());
    static_assert(allcreators::SourceEntityCreatorFamilyComplete());
    static_assert(!canonical::CanPublishRetailCreatorPointers());
    static_assert(!canonical::CanCreateRetailEntity());
    std::ifstream originalClasses(argv[1]);
    assert(originalClasses.good());
    classes::Catalog catalog;
    assert(catalog.Load(originalClasses));
    assert(catalog.Size()==creators::kOriginalClasses);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    std::size_t short60=0u,long86=0u,unclassifiedCommon=0u,
                exceptions=0u,tempEvents=0u;
    std::unordered_set<std::uint32_t> constructorVAs60,constructorVAs86;
    std::array<bool,creators::kOriginalClasses> visited{};
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        const auto nativeCreator=creators::SourceCreatorVAForOrdinal(i);
        const auto plan=canonical::SourceCanonicalBodyPlan(i,nativeCreator);
        const auto* commonAllocation=common::SourceAllocationForClass(
            i,nativeCreator);
        const auto strategy=allcreators::SourcePlanForEntity(
            i,nativeCreator);
        const auto node=image.classNodeTokens[i];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
        assert(!visited[i]);
        visited[i]=true;
        if(creators::SourceCreatorIsTemp(i)) {
            ++tempEvents;
            assert(plan.family==canonical::Family::Unclassified);
            assert(strategy.path==allcreators::CreatorPath::NotAnEntity);
            continue;
        }
        if(commonAllocation==nullptr) {
            ++exceptions;
            assert(plan.family==canonical::Family::Unclassified);
            assert(strategy.path!=allcreators::CreatorPath::NotAnEntity);
            continue;
        }
        if(plan.family==canonical::Family::Unclassified) {
            ++unclassifiedCommon;
            assert(plan.originalConstructorVA==0u);
            assert(plan.originalRequestedAllocationBytes==0u);
            continue;
        }
        assert(plan.originalRequestedAllocationBytes==
               commonAllocation->sourceObjectByteCount);
        assert(plan.originalConstructorVA>=0x24001000u);
        assert(plan.originalConstructorVA<0x24275fbau);
        assert(plan.originalReturnSubobjectOffset==8u);
        assert(plan.originalVirtualCallVtableByteOffset==0x94u);
        if(plan.family==canonical::Family::ConstructorReturnsObject60B) {
            ++short60;
            assert(plan.originalFullMethodBytes==60u);
            assert(plan.originalMethodPaddingBytes==4u);
            assert(plan.originalVtableWrites==0u);
            constructorVAs60.insert(plan.originalConstructorVA);
        }else if(plan.family==canonical::Family::InlineFourVtables86B) {
            ++long86;
            assert(plan.originalFullMethodBytes==86u);
            assert(plan.originalMethodPaddingBytes==10u);
            assert(plan.originalVtableWrites==4u);
            constructorVAs86.insert(plan.originalConstructorVA);
        }else assert(false);
    }
    assert(short60==68u && long86==50u);
    assert(unclassifiedCommon==24u && exceptions==7u && tempEvents==42u);
    assert(constructorVAs60.size()==68u);
    assert(constructorVAs86.size()==6u);
    // Phase1726: independently specified little-endian final scalar bytes.
    namespace scalar=sin_compat::original_client_shared_constructor_scalars;
    std::size_t projected=0u;
    std::unordered_set<std::uint32_t> unionTargets=constructorVAs60;
    unionTargets.insert(constructorVAs86.begin(),constructorVAs86.end());
    assert(unionTargets.size()==71u);
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        const auto va=creators::SourceCreatorVAForOrdinal(i);
        const auto plan=canonical::SourceCanonicalBodyPlan(i,va);
        if(plan.originalConstructorVA!=scalar::kConstructorVA) {
            std::vector<std::uint8_t> untouched(0x820u,0xa5u);
            const auto before=untouched;
            assert(scalar::ApplyToSyntheticStorage(i,va,untouched)==
                   scalar::Status::UnrecognizedSource);
            assert(untouched==before);
            continue;
        }
        ++projected;
        std::vector<std::uint8_t> bytes(plan.originalRequestedAllocationBytes,0xa5u);
        auto expected=bytes;
        for(const auto offset:std::array<std::size_t,6>{{
                0x788u,0x80cu,0x7e0u,0x7e4u,0x7d8u,0x7dcu}})
            for(std::size_t j=0;j<4u;++j)expected[offset+j]=0xffu;
        for(const auto offset:std::array<std::size_t,4>{{
                0x78cu,0x81cu,0x7d0u,0x79cu}})
            for(std::size_t j=0;j<4u;++j)expected[offset+j]=0u;
        for(const auto offset:std::array<std::size_t,2>{{0x7ecu,0x7f0u}}) {
            expected[offset]=0; expected[offset+1]=0;
            expected[offset+2]=0x82u; expected[offset+3]=0x42u;
        }
        for(const auto offset:std::array<std::size_t,2>{{0x7f4u,0x7f8u}}) {
            expected[offset]=0; expected[offset+1]=0;
            expected[offset+2]=0x80u; expected[offset+3]=0x44u;
        }
        expected[0x7fcu]=0;
        expected[0x810u]=expected[0x811u]=0xffu;
        assert(scalar::ApplyToSyntheticStorage(i,va+1u,bytes)==
               scalar::Status::UnrecognizedSource);
        assert(bytes==std::vector<std::uint8_t>(bytes.size(),0xa5u));
        auto shortBytes=bytes; shortBytes.pop_back();
        const auto shortBefore=shortBytes;
        assert(scalar::ApplyToSyntheticStorage(i,va,shortBytes)==
               scalar::Status::WrongStorageSize);
        assert(shortBytes==shortBefore);
        auto longBytes=bytes; longBytes.push_back(0xa5u);
        const auto longBefore=longBytes;
        assert(scalar::ApplyToSyntheticStorage(i,va,longBytes)==
               scalar::Status::WrongStorageSize);
        assert(longBytes==longBefore);
        assert(scalar::ApplyToSyntheticStorage(i,va,bytes)==
               scalar::Status::AppliedSyntheticScalarState);
        assert(bytes==expected);
        assert(scalar::ApplyToSyntheticStorage(i,va,bytes)==
               scalar::Status::AppliedSyntheticScalarState);
        assert(bytes==expected);
    }
    assert(projected==34u);
    assert(!scalar::CanPublishRetailCreatorPointers());

    // Exact 2006 source class tuple and source constructor identity must
    // both match before reporting any authentic original method plan.
    for(const auto& item:canonical::k60ByteConstructors) {
        const auto ordinal=std::size_t(item.originalClassOrdinal);
        const auto va=creators::SourceCreatorVAForOrdinal(ordinal);
        const auto plan=canonical::SourceCanonicalBodyPlan(ordinal,va);
        assert(plan.family==canonical::Family::ConstructorReturnsObject60B);
        assert(plan.originalConstructorVA==item.originalConstructorVA);
        assert(canonical::SourceCanonicalBodyPlan(ordinal,va+1u).family==
               canonical::Family::Unclassified);
    }
    for(const auto& item:canonical::k86ByteConstructors) {
        const auto ordinal=std::size_t(item.originalClassOrdinal);
        const auto va=creators::SourceCreatorVAForOrdinal(ordinal);
        const auto plan=canonical::SourceCanonicalBodyPlan(ordinal,va);
        assert(plan.family==canonical::Family::InlineFourVtables86B);
        assert(plan.originalConstructorVA==item.originalConstructorVA);
        assert(canonical::SourceCanonicalBodyPlan(ordinal,va+1u).family==
               canonical::Family::Unclassified);
    }
    assert(canonical::SourceCanonicalBodyPlan(191u,0u).family==
           canonical::Family::Unclassified);
    assert(!image.CanExposeToRetailEngine());
    assert(!canonical::CanPublishRetailCreatorPointers());
    std::puts("PASS Phase1722 source-authenticated 118 COMPLETE native "
              "creator methods (68x60B +50x86B), six shared inline base "
              "ctors, 68 unique external ctors, +8 networkable return, "
              "virtual +0x94, 24 complex common methods deferred, "
              "7 exceptional/42 temp getters excluded, NO retail slot5");
    return 0;
}
