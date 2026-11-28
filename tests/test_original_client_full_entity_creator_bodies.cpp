// Phase1722: COMPLETE original x86 142 common entity creator body
// SOURCE metadata; no original executable byte or real entity factory.
// Original 2006 PE32 networkable callback return is object base +8
// after a native virtual method at vtable byte offset +0x94.
#include "sin_compat/original_client_full_entity_creator_bodies.hpp"
#include "sin_compat/original_client_native32_recvtable_image.hpp"
#include "sin_compat/original_client_class_catalog.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <stdexcept>
#include <unordered_set>

namespace whole=sin_compat::original_client_full_entity_creator_bodies;
namespace common=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

static std::array<std::string_view,12> ParseCSVLine(
    const std::string& line) {
    std::array<std::string_view,12> result{};
    std::size_t begin=0u;
    for(std::size_t i=0u;i<result.size();++i) {
        const auto sep=line.find(',',begin);
        if((i==result.size()-1u && sep!=std::string::npos) ||
           (i!=result.size()-1u && sep==std::string::npos))
            throw std::invalid_argument("original full creator metadata CSV shape");
        result[i]=std::string_view(
            line.data()+begin,
            (sep==std::string::npos?line.size():sep)-begin);
        if(result[i].empty())
            throw std::invalid_argument("source creator CSV empty property");
        begin=sep+1u;
    }
    return result;
}
static std::uint32_t Number(std::string_view data,bool hex) {
    if(hex) {
        if(data.size()<3u || data.substr(0u,2u)!="0x")
            throw std::invalid_argument("source x86 VA not prefixed");
        data.remove_prefix(2u);
    }
    std::uint32_t result=0u;
    const auto parsed=std::from_chars(
        data.data(),data.data()+data.size(),result,hex?16:10);
    if(parsed.ec!=std::errc{} ||
       parsed.ptr!=data.data()+data.size())
        throw std::invalid_argument("source original constructor value invalid");
    return result;
}
int main(int argc,char** argv) {
    assert(argc==3);
    static_assert(whole::SourceFullBodyInventoryConsistent());
    static_assert(!whole::CanPublishRetailCreatorPointers());
    static_assert(!whole::CanCreateRetailEntity());
    std::ifstream nativeClasses(argv[1]);
    assert(nativeClasses.good());
    classes::Catalog catalog;
    assert(catalog.Load(nativeClasses));
    assert(catalog.Size()==191u);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    std::array<const classes::Entry*,191> cls{};
    auto* pointer=catalog.Head();
    for(auto& record:cls) {
        assert(pointer!=nullptr);
        record=pointer;
        pointer=pointer->next;
    }
    assert(pointer==nullptr);
    std::ifstream csv(argv[2]);
    assert(csv.good());
    std::string line;
    assert(std::getline(csv,line));
    if(!line.empty() && line.back()=='\r')line.pop_back();
    assert(line=="class_ordinal,network_class,creator_va,"
                 "allocation_request_bytes,complete_method_bytes,"
                 "original_constructor_call_va,original_constructor_target_va,"
                 "original_virtual_call_byte_offset,original_return_subobject_offset,"
                 "original_creator_family,native_int3_padding_bytes,"
                 "complete_function_sha256");

    std::unordered_set<std::uint32_t> targets;
    std::size_t originalWholeBytes=0u,regular=0u,stamped=0u,
                adjacent=0u,index=0u;
    for(const auto& record:whole::kSourceCompleteCreatorBodies) {
        assert(std::getline(csv,line));
        if(!line.empty() && line.back()=='\r')line.pop_back();
        const auto row=ParseCSVLine(line);
        assert(Number(row[0],false)==record.originalClassOrdinal);
        assert(record.originalClassOrdinal<cls.size());
        const auto& original=*cls[record.originalClassOrdinal];
        assert(!original.originalTempEvent);
        assert(row[1]==original.networkName);
        assert(Number(row[2],true)==record.originalCreatorVA);
        assert(Number(row[3],false)==record.originalRequestedBytes);
        assert(Number(row[4],false)==record.completeExecutableBytes);
        const auto call=Number(row[5],true);
        assert(Number(row[6],true)==record.originalConstructorTargetVA);
        assert(Number(row[7],true)==whole::kNativeVirtualCallByteOffset);
        assert(Number(row[8],false)==whole::kOriginalReturnedSubobjectOffset);
        assert(Number(row[10],false)==record.originalTrailingInt3Bytes);
        assert(row[11].size()==64u);
        assert(creators::SourceCreatorVAForOrdinal(
            record.originalClassOrdinal)==record.originalCreatorVA);
        const auto* previous=common::SourceAllocationForClass(
            record.originalClassOrdinal,record.originalCreatorVA);
        assert(previous!=nullptr);
        assert(previous->sourceObjectByteCount==record.originalRequestedBytes);
        assert(record.originalConstructorTargetVA>=0x24001000u &&
               record.originalConstructorTargetVA<0x24275FBAu);
        const bool shortBody=
            record.methodFamily==whole::CreatorBodyFamily::ConstructThenVirtual;
        assert(call==record.originalCreatorVA+(shortBody?20u:23u));
        if(shortBody) {
            assert(row[9]=="ctor_then_virtual");
            assert(record.completeExecutableBytes==60u);
            ++regular;
        }else {
            assert(row[9]=="vtable_stamped_then_virtual");
            ++stamped;
        }
        if(record.originalTrailingInt3Bytes==0u) {
            ++adjacent;
            assert(record.completeExecutableBytes==96u);
        }
        targets.insert(record.originalConstructorTargetVA);
        originalWholeBytes+=record.completeExecutableBytes;
        const auto node=image.classNodeTokens[record.originalClassOrdinal];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
        ++index;
    }
    assert(!std::getline(csv,line));
    assert(index==142u);
    assert(regular==68u && stamped==74u);
    assert(adjacent==12u && targets.size()==73u);
    assert(originalWholeBytes==10770u);

    // Noncommon seven source entity creators are separately whole-function
    // verified in Phase1721; 42 source temp-event getter methods in
    // Phase1717. All 191 class creators remain UNCALLABLE placeholders.
    std::size_t missing=0u,events=0u;
    for(std::size_t i=0u;i<cls.size();++i) {
        if(creators::SourceCreatorIsTemp(i)) {
            ++events;
            continue;
        }
        bool commonSource=false;
        for(const auto& record:whole::kSourceCompleteCreatorBodies)
            if(record.originalClassOrdinal==i)commonSource=true;
        if(!commonSource)++missing;
        const auto node=image.classNodeTokens[i];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
    }
    assert(missing==7u && events==42u);
    assert(!whole::CanPublishRetailCreatorPointers());
    assert(!image.CanExposeToRetailEngine());
    std::puts("PASS Phase1722 all 142 WHOLE original PE32 common entity "
              "creator methods, 68+74 native templates, 73 target "
              "constructors, +0x94 virtual/+8 networkable return, "
              "10,770 original bytes source-authenticated; "
              "no retail executable slot5");
    return 0;
}
