// Phase1718: inspect exact original entity allocation request sizes,
// never execute original 0x240C4FF0 or construct partially initialized
// Source entities. Only original-source metadata is modeled.
#include "sin_compat/original_client_entity_allocation_prologues.hpp"
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
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace sizes=sin_compat::original_client_entity_allocation_prologues;
namespace creators=sin_compat::original_client_creator_source_map;
namespace classes=sin_compat::original_client_class_catalog;
namespace native=sin_compat::original_client_native32_recvtable_image;

static std::array<std::string_view,8> CsvFields(const std::string& line) {
    std::array<std::string_view,8> fields{};
    std::size_t next=0u;
    for(std::size_t i=0u;i<fields.size();++i) {
        const auto sep=line.find(',',next);
        if((i==7u && sep!=std::string::npos) ||
           (i!=7u && sep==std::string::npos))
            throw std::invalid_argument("original allocator CSV field count");
        fields[i]=std::string_view(
            line.data()+next,
            (sep==std::string::npos?line.size():sep)-next);
        if(fields[i].empty())
            throw std::invalid_argument("original source creator field empty");
        next=sep+1u;
    }
    return fields;
}
static std::uint32_t Number(std::string_view value,int base) {
    if(base==16) {
        if(value.size()<3u || value.substr(0u,2u)!="0x")
            throw std::invalid_argument("non-hex original source address");
        value.remove_prefix(2u);
    }
    std::uint32_t out=0u;
    const auto parsed=std::from_chars(
        value.data(),value.data()+value.size(),out,base);
    if(parsed.ec!=std::errc{} ||
       parsed.ptr!=value.data()+value.size())
        throw std::invalid_argument("original source decimal/hex error");
    return out;
}

int main(int argc,char** argv) {
    assert(argc==3);
    static_assert(sizes::SourceAllocationInventoryConsistent());
    static_assert(!sizes::CanCreateRetailEntity());
    static_assert(!creators::CanPublishRetailCreatorPointers());

    std::ifstream originalClasses(argv[1]);
    assert(originalClasses.good());
    classes::Catalog catalog;
    assert(catalog.Load(originalClasses));
    assert(catalog.Size()==191u);
    const auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());

    std::ifstream csv(argv[2]);
    assert(csv.good());
    std::string line;
    assert(std::getline(csv,line));
    if(!line.empty() && line.back()=='\r')line.pop_back();
    assert(line=="class_ordinal,network_class,creator_va,"
                 "object_size_bytes,original_allocator_va,"
                 "original_size_push_va,original_allocator_call_va,crt_index");
    std::array<bool,creators::kOriginalClasses> seen{};
    std::unordered_set<std::uint32_t> sizeGroups,sourceCreatorVAs;
    std::uint32_t maxBytes=0u,minBytes=0xffffffffu;
    std::size_t records=0u;
    const auto* cls=catalog.Head();
    std::array<const classes::Entry*,creators::kOriginalClasses> all{};
    for(std::size_t i=0u;i<all.size();++i) {
        assert(cls!=nullptr);
        all[i]=cls;
        cls=cls->next;
    }
    assert(cls==nullptr);
    while(std::getline(csv,line)) {
        if(!line.empty() && line.back()=='\r')line.pop_back();
        assert(!line.empty());
        const auto row=CsvFields(line);
        const auto ordinal=Number(row[0],10);
        assert(ordinal<creators::kOriginalClasses);
        const auto request=Number(row[3],10);
        const auto creator=Number(row[2],16);
        const auto allocator=Number(row[4],16);
        const auto push=Number(row[5],16);
        const auto call=Number(row[6],16);
        const auto crt=Number(row[7],10);
        assert(all[ordinal]!=nullptr);
        const auto& original=*all[ordinal];
        assert(!original.originalTempEvent);
        assert(!seen[ordinal]);
        seen[ordinal]=true;
        assert(row[1]==original.networkName);
        assert(original.sourceCRTIndex==crt);
        assert(creators::SourceCreatorVAForOrdinal(ordinal)==creator);
        assert(sizes::SourceAllocationForClass(ordinal,creator)!=nullptr);
        assert(sizes::SourceAllocationForClass(ordinal,creator)
                    ->sourceObjectByteCount==request);
        assert(allocator==sizes::kOriginalAllocatorVA);
        assert(push==creator+1u && call==creator+6u);
        assert(request>=sizes::kSmallestAllocation &&
               request<=sizes::kLargestAllocation &&
               (request&3u)==0u);
        assert(sourceCreatorVAs.insert(creator).second);
        sizeGroups.insert(request);
        if(request<minBytes)minBytes=request;
        if(request>maxBytes)maxBytes=request;
        // A known original allocation SIZE is not a valid native
        // creator function: the host-owned native32 class +0/+4
        // remain null, even for this authenticated source prologue.
        const auto node=image.classNodeTokens[ordinal];
        assert(image.arena.Read32(node+0u)==0u);
        assert(image.arena.Read32(node+4u)==0u);
        assert(node!=creator);
        ++records;
    }
    assert(!csv.bad());
    assert(records==142u);
    assert(sizeGroups.size()==69u);
    assert(minBytes==1152u && maxBytes==10048u);
    const std::array<std::size_t,7> special{{
        48u,69u,75u,120u,143u,170u,190u}};
    std::size_t unknown=0u;
    for(std::size_t ordinal=0u;ordinal<all.size();++ordinal) {
        if(all[ordinal]->originalTempEvent) {
            assert(!seen[ordinal]);
            continue;
        }
        if(!seen[ordinal]) {
            assert(unknown<special.size());
            assert(ordinal==special[unknown]);
            ++unknown;
            assert(sizes::SourceAllocationForClass(
                ordinal,creators::SourceCreatorVAForOrdinal(ordinal))==nullptr);
        }
    }
    assert(unknown==7u);
    assert(sizes::SourceAllocationForClass(191u,0u)==nullptr);
    assert(sizes::SourceAllocationForClass(
        0u,creators::SourceCreatorVAForOrdinal(0u)+1u)==nullptr);
    assert(!sizes::CanCreateRetailEntity());
    assert(!image.CanExposeToRetailEngine());
    std::puts("PASS Phase1718 142 original entity allocator prologues "
              "and byte counts / 69 sizes / 7 explicit exceptions; "
              "191 native creator callbacks remain NULL; NO retail slot5");
    return 0;
}
