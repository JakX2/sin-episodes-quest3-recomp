// Phase1716: the 191 creator function pointers are original SOURCE labels,
// not actual runtime callbacks. Assert source metadata and retain NULL
// creator fields in the safe host-owned native32 ClientClass image.
#include "sin_compat/original_client_creator_source_map.hpp"
#include "sin_compat/original_client_class_catalog.hpp"
#include "sin_compat/original_client_native32_recvtable_image.hpp"
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
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

static std::array<std::string_view,7> SourceCSVFields(
    const std::string& line) {
    std::array<std::string_view,7> fields{};
    std::size_t start=0u;
    for(std::size_t i=0u;i<fields.size();++i) {
        const auto separator=line.find(',',start);
        if((i==6u && separator!=std::string::npos) ||
           (i<6u && separator==std::string::npos))
            throw std::invalid_argument("invalid source creator CSV field count");
        fields[i]=std::string_view(line.data()+start,
                     (separator==std::string::npos?line.size():separator)-start);
        if(fields[i].empty())throw std::invalid_argument("empty source creator field");
        start=separator+1u;
    }
    return fields;
}
static std::uint32_t Number(std::string_view source,int radix) {
    if(radix==16) {
        if(source.size()<3u || source.substr(0u,2u)!="0x")
            throw std::invalid_argument("expected PE32 source VA in 0x form");
        source.remove_prefix(2u);
    }
    std::uint32_t out=0u;
    const auto status=std::from_chars(
        source.data(),source.data()+source.size(),out,radix);
    if(status.ec!=std::errc{} || status.ptr!=source.data()+source.size())
        throw std::invalid_argument("malformed original source ordinal/VA");
    return out;
}

int main(int argc,char** argv) {
    assert(argc==3);
    static_assert(creators::SourceCreatorInventoryConsistent());
    static_assert(!creators::CanPublishRetailCreatorPointers());
    std::ifstream classTSV(argv[1]);
    assert(classTSV.good());
    classes::Catalog catalog;
    assert(catalog.Load(classTSV));
    assert(catalog.Size()==creators::kOriginalClasses);

    std::ifstream csv(argv[2]);
    assert(csv.good());
    std::string line;
    assert(std::getline(csv,line));
    if(!line.empty() && line.back()=='\r')line.pop_back();
    assert(line=="ordinal,network_class,kind,creator_va,crt_index,"
                 "source_node_va,source_recvtable_va");
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());
    std::unordered_set<std::uint32_t> functionLabels;
    std::size_t entities=0u,tempEvents=0u,records=0u;
    const auto* cls=catalog.Head();
    for(;cls;cls=cls->next) {
        assert(records<creators::kOriginalClasses);
        assert(std::getline(csv,line));
        if(!line.empty() && line.back()=='\r')line.pop_back();
        const auto fields=SourceCSVFields(line);
        const auto ordinal=Number(fields[0],10);
        const auto originalVA=Number(fields[3],16);
        const auto classCRT=Number(fields[4],10);
        const auto classNode=Number(fields[5],16);
        const auto classTable=Number(fields[6],16);
        assert(ordinal==records && fields[1]==cls->networkName);
        assert(classCRT==cls->sourceCRTIndex);
        assert(classNode==cls->originalNodeVA);
        assert(classTable==cls->originalRecvTableVA);
        assert(fields[2]==(cls->originalTempEvent?"T":"E"));
        assert(creators::SourceCreatorIsTemp(records)==cls->originalTempEvent);
        assert(creators::SourceCreatorVAForOrdinal(records)==originalVA);
        assert(originalVA>=creators::kOriginalPECodeStart);
        assert(originalVA<creators::kOriginalPECodeEndExclusive);
        assert(functionLabels.insert(originalVA).second);
        if(cls->originalTempEvent)++tempEvents;
        else ++entities;

        // The metadata image represents a 20-byte native ClientClass.
        // Neither the original event creator (+4), nor the original
        // entity creator (+0), is a usable host callback. Both MUST be
        // null even though their source address labels are known.
        const auto ownedNode=image.classNodeTokens[records];
        assert(image.arena.Read32(ownedNode+0u)==0u);
        assert(image.arena.Read32(ownedNode+4u)==0u);
        assert(ownedNode!=originalVA);
        ++records;
    }
    assert(records==191u);
    assert(tempEvents==42u && entities==149u);
    assert(functionLabels.size()==191u);
    assert(!std::getline(csv,line)); // exact inventory count
    assert(!image.CanExposeToRetailEngine());
    assert(!creators::CanPublishRetailCreatorPointers());
    std::puts("PASS Phase1716 191 source-only ClientClass creator labels, "
              "149 entity / 42 temp-event, original CRT order and "
              "owned 20-byte ClientClass creator pointers remain NULL; "
              "NO retail slot5");
    return 0;
}
