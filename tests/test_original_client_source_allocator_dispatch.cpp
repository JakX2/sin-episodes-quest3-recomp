// Phase1720: source-only 2006 PE32 allocator VTABLE route decisions.
// No original global pointers, object allocation, heap allocator,
/// Windows import or native callback is ever executed.
#include "sin_compat/original_client_source_allocator_dispatch.hpp"
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

namespace dispatch=sin_compat::original_client_source_allocator_dispatch;
namespace source=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(source::SourceAllocationInventoryConsistent());
    static_assert(!dispatch::CanDispatchRetailAllocator());
    static_assert(!dispatch::CanPublishRetailCreatorPointers());
    std::ifstream classTSV(argv[1]);
    assert(classTSV.good());
    classes::Catalog catalog;
    assert(catalog.Load(classTSV));
    assert(catalog.Size()==creators::kOriginalClasses);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    std::size_t visited=0u;
    for(const auto& item:source::kOriginalAllocationPrologues) {
        const auto ordinal=std::size_t(item.originalClassOrdinal);
        const auto sourceVA=item.originalCreatorVA;
        // Real code lazy-init flag0 triggers 0x2416F7F0, which
        // may call native engine allocator virtual +0x5C and
        // original Windows-import routines. The fake host must
        // not assume a context pointer already exists.
        auto noEngine=dispatch::PlanOriginalSourceAllocation(
            ordinal,sourceVA,{false,true,0u});
        assert(noEngine.selected==
               dispatch::Route::RefusedUnboundEngineAllocator);
        auto unresolved=dispatch::PlanOriginalSourceAllocation(
            ordinal,sourceVA,{true,false,0u});
        assert(unresolved.selected==
               dispatch::Route::RequiresOriginalContextResolver);

        auto direct=dispatch::PlanOriginalSourceAllocation(
            ordinal,sourceVA,{true,true,0u});
        assert(direct.selected==dispatch::Route::DirectVirtualSlot4);
        assert(direct.originalSourceRequestedBytes==
               item.sourceObjectByteCount);
        assert(direct.originalVtableByteOffset==4u);
        assert(direct.syntheticHostContextToken==0u);

        auto withContext=dispatch::PlanOriginalSourceAllocation(
            ordinal,sourceVA,{true,true,0x10002000u});
        assert(withContext.selected==dispatch::Route::ContextVirtualSlot0);
        assert(withContext.originalSourceRequestedBytes==
               item.sourceObjectByteCount);
        assert(withContext.originalVtableByteOffset==0u);
        assert(withContext.syntheticHostContextToken==0x10002000u);

        // These are actual original 2006 source PE32 address labels,
        // never valid independent engine heap/context addresses.
        for(auto bad:std::array<std::uint32_t,7>{{
              0x2416F8F0u,0x24372908u,0xFFFFFFFFu,
              0x0FFFFFFCu,0x10000001u,0x1FFFFFFFu,0x20000000u}}) {
            const auto outcome=dispatch::PlanOriginalSourceAllocation(
                ordinal,sourceVA,{true,true,bad});
            assert(outcome.selected==
                dispatch::Route::RefusedOriginalOrUnsafeContextPointer);
        }
        // The modeled dispatch is decision-only and MUST NOT
        // modify the host-owned native32 class or pointer fields.
        const auto classNode=image.classNodeTokens[ordinal];
        assert(image.arena.Read32(classNode+0u)==0u);
        assert(image.arena.Read32(classNode+4u)==0u);
        ++visited;
    }
    assert(visited==142u);

    // All 7 excluded original entities and 42 temp-event getters
    // require their own source-specific native lifecycle: refuse
    // rather than fabricating a compatible original allocator route.
    std::size_t rejected=0u;
    for(std::size_t i=0u;i<creators::kOriginalClasses;++i) {
        const auto va=creators::SourceCreatorVAForOrdinal(i);
        if(source::SourceAllocationForClass(i,va)!=nullptr)continue;
        const auto result=dispatch::PlanOriginalSourceAllocation(
            i,va,{true,true,0u});
        assert(result.selected==dispatch::Route::RefusedUnrecognizedSource);
        assert(result.originalSourceRequestedBytes==0u);
        ++rejected;
    }
    assert(rejected==49u);
    {
        const auto wrong=dispatch::PlanOriginalSourceAllocation(
            0u,creators::SourceCreatorVAForOrdinal(0u)+1u,
            {true,true,0x10002000u});
        assert(wrong.selected==dispatch::Route::RefusedUnrecognizedSource);
    }
    assert(!image.CanExposeToRetailEngine());
    assert(!dispatch::CanDispatchRetailAllocator());
    std::puts("PASS Phase1720 142 source allocator route plans, lazy resolver "
              "required when uninitialized, vtable+4 vs vtable+0 only "
              "with host-bound context; 49 unsupported creator routes; "
              "no executable original heap or retail slot5");
    return 0;
}
