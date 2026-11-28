// Phase1717: all 42 original 2006 PE32 static temp-event creators.
// Host-owned synthetic return TOKENS ONLY, never original static objects
// or genuinely executable 2006 class factory callbacks.
#include "sin_compat/original_client_temp_event_static_creators.hpp"
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

namespace getters=sin_compat::original_client_temp_event_static_creators;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

int main(int argc,char** argv) {
    assert(argc==2);
    static_assert(getters::SourceGettersConsistent());
    static_assert(creators::SourceCreatorInventoryConsistent());
    static_assert(!getters::CanPublishRetailCreatorPointers());
    std::ifstream classesFile(argv[1]);
    assert(classesFile.good());
    classes::Catalog catalog;
    assert(catalog.Load(classesFile));
    assert(catalog.Size()==191u);

    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());
    std::vector<std::uint8_t> owned(8192u,0xA5u);
    std::uint32_t output=0xABCDEF01u;
    std::size_t verified=0u;
    for(const auto& item:getters::kSourceStaticGetters) {
        const auto n=std::size_t(item.classOrdinal);
        assert(n<catalog.Size());
        assert(creators::SourceCreatorIsTemp(n));
        assert(creators::SourceCreatorVAForOrdinal(n)==item.originalCreatorVA);
        assert(getters::SourceGetterForClass(n,item.originalCreatorVA)==&item);
        const auto resultToken=getters::kSyntheticHostBase+
            getters::kSyntheticHostMinOffset+
            static_cast<std::uint32_t>(4u*verified);
        const auto saved=owned;
        output=0xABCDEF01u;
        assert(getters::ApplySourceOriginalGetter(
            n,item.originalCreatorVA,false,resultToken,owned,output)==
            getters::Status::RefusedUnboundHostObject);
        assert(output==0xABCDEF01u);
        assert(getters::ApplySourceOriginalGetter(
            n,item.originalCreatorVA,true,item.originalStaticObjectVA,
            owned,output)==getters::Status::RefusedBadOwnedMemory);
        assert(output==0xABCDEF01u);

        assert(getters::ApplySourceOriginalGetter(
            n,item.originalCreatorVA,true,resultToken,owned,output)==
            getters::Status::Applied);
        assert(output==resultToken);
        assert(owned==saved); // no original static data touched
        // Every independent ClientClass still has NULL original
        // executable entity/event creator fields, regardless of its
        // now source-attested static temp-event singleton location.
        const auto node=image.classNodeTokens[n];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
        assert(resultToken!=item.originalStaticObjectVA);
        ++verified;
    }
    assert(verified==42u);
    assert(getters::SourceGetterForClass(0u,
        creators::SourceCreatorVAForOrdinal(0u))==nullptr);
    const auto& first=getters::kSourceStaticGetters[0u];
    const auto& second=getters::kSourceStaticGetters[1u];
    output=0x22334455u;
    const auto saved=owned;
    assert(getters::ApplySourceOriginalGetter(
        first.classOrdinal,second.originalCreatorVA,true,
        getters::kSyntheticHostBase+0x1400u,owned,output)==
        getters::Status::UnrecognizedSource);
    assert(getters::ApplySourceOriginalGetter(
        191u,first.originalCreatorVA,true,
        getters::kSyntheticHostBase+0x1400u,owned,output)==
        getters::Status::UnrecognizedSource);
    assert(output==0x22334455u && saved==owned);

    for(const auto bad:std::array<std::uint32_t,7>{{
            0u,first.originalStaticObjectVA,0xFFFFFFFFu,
            0x10000001u,0x100013FCu,0x10001000u,0x10002000u}}) {
        assert(getters::ApplySourceOriginalGetter(
            first.classOrdinal,first.originalCreatorVA,true,bad,
            owned,output)==getters::Status::RefusedBadOwnedMemory);
        assert(output==0x22334455u);
    }
    // A token whose array has no allocated byte must not pass.
    {
        const std::vector<std::uint8_t> tiny(10u,0xCAu);
        assert(getters::ApplySourceOriginalGetter(
            first.classOrdinal,first.originalCreatorVA,true,
            getters::kSyntheticHostBase+0x1400u,tiny,output)==
            getters::Status::RefusedBadOwnedMemory);
    }
    assert(owned==saved);
    assert(!image.CanExposeToRetailEngine());
    assert(!getters::CanPublishRetailCreatorPointers());
    std::puts("PASS Phase1717 all 42 original static temp-event getters "
              "return only explicitly-bound synthetic owned tokens, "
              "native +0/+4 creator pointers remain NULL; NO slot5");
    return 0;
}
