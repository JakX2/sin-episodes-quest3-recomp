// Phase1723: verify original 50 inline factory virtual-method links.
// SOURCE-ONLY metadata, never execute original 75-byte PE32 network
// initializer or original 4-byte interface adjustment thunk.
#include "sin_compat/original_client_inline_factory_vtable_init.hpp"
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

namespace vt=sin_compat::original_client_inline_factory_vtable_init;
namespace canonical=sin_compat::original_client_canonical_creator_bodies;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(vt::SourceInlineVtableRoutesComplete());
    static_assert(!vt::CanInvokeRetailVirtualMethods());
    static_assert(!canonical::CanCreateRetailEntity());
    std::ifstream classTSV(argv[1]);
    assert(classTSV.good());
    classes::Catalog catalog;
    assert(catalog.Load(classTSV));
    assert(catalog.Size()==creators::kOriginalClasses);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    std::size_t active=0u,excluded=0u;
    std::size_t targets34=0u,targets13=0u,targets3=0u;
    std::unordered_set<std::size_t> seen;
    for(std::size_t ordinal=0u;ordinal<creators::kOriginalClasses;++ordinal) {
        const auto creator=creators::SourceCreatorVAForOrdinal(ordinal);
        const auto parent=canonical::SourceCanonicalBodyPlan(ordinal,creator);
        const auto dependency=vt::SourceVirtualInitDependencyFor(
            ordinal,creator);
        const auto ownedClass=image.classNodeTokens[ordinal];
        assert(image.arena.Read32(ownedClass+0u)==0u);
        assert(image.arena.Read32(ownedClass+4u)==0u);
        if(parent.family!=canonical::Family::InlineFourVtables86B) {
            assert(dependency.status==
                   vt::Status::NotAnAuthenticatedInlineSourceCreator);
            assert(dependency.originalPrimaryVirtual94VA==0u);
            ++excluded;
            continue;
        }
        ++active;
        assert(seen.insert(ordinal).second);
        assert(dependency.status==vt::Status::RecognizedSourceDependency);
        assert(dependency.originalPrimaryVirtual94VA==0x240C8FE0u);
        assert(dependency.originalSecondaryVirtual0VA==0x24054EC0u);
        assert(dependency.secondaryThisObjectOffset==4u);
        assert(dependency.returnedObjectSubobjectOffset==8u);
        assert(dependency.mutatedOwnerFieldOffset==0x50u);
        assert(parent.originalFullMethodBytes==86u);
        assert(parent.originalVtableWrites==4u);
        switch(dependency.originalPrimaryVirtual44VA) {
            case 0x24062070u:++targets34;break;
            case 0x24054E80u:++targets13;break;
            case 0x240982D0u:++targets3;break;
            default: assert(false);
        }
        // A wrong source x86 label must never become a valid native
        // engine callback, even when the class ordinal is known.
        assert(vt::SourceVirtualInitDependencyFor(
            ordinal,creator+1u).status==
            vt::Status::NotAnAuthenticatedInlineSourceCreator);
    }
    assert(active==50u && excluded==141u);
    assert(seen.size()==50u);
    assert(targets34==34u && targets13==13u && targets3==3u);
    assert(vt::SourceVirtualInitDependencyFor(191u,0u).status==
           vt::Status::NotAnAuthenticatedInlineSourceCreator);
    assert(vt::SourceVirtualInitDependencyFor(0u,0u).status==
           vt::Status::NotAnAuthenticatedInlineSourceCreator);
    assert(!image.CanExposeToRetailEngine());
    assert(!vt::CanInvokeRetailVirtualMethods());
    std::puts("PASS Phase1723 50 complete source-original inline factories "
              "all share native virtual+0x94 75B initializer and "
              "secondary+0 4B this-minus-4 adjustor; 34/13/3 "
              "primary+0x44 source groups, original creator pointers NULL; "
              "NO retail slot5");
    return 0;
}
