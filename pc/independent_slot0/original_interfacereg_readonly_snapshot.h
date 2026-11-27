#pragma once
// Original 2006 PE32 client/server.dll InterfaceReg observer.
// This is an original-free, NO-EXECUTION, NO-WRITE linked-record validator.
// The production game does not use this header to register any interfaces.
// All remote reads must be supplied by an explicitly authenticated host.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sin_independent {
namespace original_interfacereg_snapshot {

constexpr std::size_t kMaxRecords=64;
constexpr std::size_t kMaxNameBytes=96;
constexpr std::size_t kRecordBytes=12;

enum class Status : unsigned char {
    InvalidInput, HeadUnreadable, Empty, InvalidRecord,
    Cycle, RecordUnreadable, InvalidCreator, InvalidNamePointer,
    NameUnreadable, NameInvalid, DuplicateName, TooManyRecords,
    HeadChanged, CountMismatch, PriorityMissing, VerifiedUnpinned
};

struct Layout {
    std::uintptr_t module_base=0;
    std::uint32_t image_size=0;
    std::uint32_t registry_head_rva=0;
    std::uint32_t text_first=0,text_end=0;
    std::uint32_t rdata_first=0,rdata_end=0;
    std::uint32_t data_first=0,data_end=0;
    std::size_t expected_count=0;
    const char* const* priority_names=nullptr;
    std::size_t priority_count=0;
};

// Read-only PE section extents are from the complete SHA-pinned original
// 2006 PE32 images, NOT from HL2 SDK layouts or observed source strings
// alone. IMAGE_BASE relocation is handled by the module_base argument.
// The 55/10 total InterfaceReg manifests predate this observer (Phase 177).
inline constexpr const char* kClientPriority[]={
    "VClient011", "VClientEntityList003", "VClientPrediction001",
    "GameClientExports001", "ClientLeafSystem001",
    "IEffects001", "VCENTERPRINT002", "GameMovement001"
};
inline constexpr const char* kServerPriority[]={
    "ServerGameDLL003", "ServerGameEnts001", "ServerGameClients003",
    "GameMovement001", "IEffects001", "PlayerInfoManager001",
    "PlayerInfoManager002", "BotManager001",
    "HLTVDirector001", "PluginHelpersCheck001"
};
inline Layout OriginalClientLayout(std::uintptr_t relocated_base) noexcept {
    return {relocated_base,0x3D9000,0x369378,
            0x1000,0x275FBA,0x276000,0x2F29FD,
            0x2F3000,0x38F320,55,
            kClientPriority,sizeof(kClientPriority)/sizeof(kClientPriority[0])};
}
inline Layout OriginalServerLayout(std::uintptr_t relocated_base) noexcept {
    return {relocated_base,0x628000,0x573C5C,
            0x1000,0x3D111A,0x3D2000,0x4D5626,
            0x4D6000,0x5A0B24,10,
            kServerPriority,sizeof(kServerPriority)/sizeof(kServerPriority[0])};
}

struct Reader {
    void* context=nullptr;
    bool (*dword)(void*,std::uintptr_t,std::uint32_t*) noexcept=nullptr;
    bool (*byte)(void*,std::uintptr_t,std::uint8_t*) noexcept=nullptr;
};

struct Result {
    Status status=Status::InvalidInput;
    std::size_t entries=0;
    std::size_t priority_found=0;
    bool accepted() const noexcept {return status==Status::VerifiedUnpinned;}
};

// Evaluate the linked registry using actual ORIGINAL module section ranges.
// Refuse any native pointer that would read outside the authenticated module.
// This checks only identity and linked-list shape, NOT unchanging backing:
// even two successful snapshots may miss an ABA between observations.
inline Result Observe(const Layout& l,const Reader& reader) noexcept {
    constexpr auto top=std::uint64_t{0x100000000};
    if(!reader.dword||!reader.byte||
       l.module_base==0||l.module_base>=top||l.image_size==0||
       std::uint64_t(l.module_base)+l.image_size>top||
       l.registry_head_rva>l.image_size||
       l.image_size-l.registry_head_rva<4||
       l.text_first>=l.text_end||l.rdata_first>=l.rdata_end||
       l.data_first>=l.data_end||
       l.text_end>l.rdata_first||l.rdata_end>l.data_first||
       l.data_end>l.image_size||
       l.expected_count==0||l.expected_count>kMaxRecords||
       l.priority_count>l.expected_count||
       (l.priority_count&&!l.priority_names))
        return {};
    for(std::size_t i=0;i<l.priority_count;++i) {
        if(!l.priority_names[i]||!l.priority_names[i][0]||
           std::strlen(l.priority_names[i])>=kMaxNameBytes)
            return {};
        for(std::size_t j=0;j<i;++j)
            if(std::strcmp(l.priority_names[i],l.priority_names[j])==0)
                return {};
    }
    const auto head_address=l.module_base+l.registry_head_rva;
    std::uint32_t head=0;
    if(!reader.dword(reader.context,head_address,&head))
        return {Status::HeadUnreadable};
    if(!head)return {Status::Empty};
    std::array<std::uint32_t,kMaxRecords> visited{};
    std::array<std::array<char,kMaxNameBytes>,kMaxRecords> names{};
    std::array<bool,kMaxRecords> priority{};
    Result result{Status::InvalidRecord};
    std::uint32_t current=head;

    auto within=[&](std::uint32_t value,std::uint32_t first,
                    std::uint32_t end,std::size_t bytes) noexcept {
        if(value<l.module_base)return false;
        const auto offset=std::uint64_t(value)-l.module_base;
        return offset>=first&&offset<end&&bytes<=end-offset;
    };

    while(current) {
        if(result.entries>=kMaxRecords) {
            result.status=Status::TooManyRecords;return result;
        }
        if(!within(current,l.data_first,l.data_end,kRecordBytes)) {
            result.status=Status::InvalidRecord;return result;
        }
        for(std::size_t i=0;i<result.entries;++i)
            if(visited[i]==current) {
                result.status=Status::Cycle;return result;
            }
        visited[result.entries]=current;
        std::uint32_t creator=0,name_ptr=0,next=0;
        if(!reader.dword(reader.context,current,&creator)||
           !reader.dword(reader.context,current+4,&name_ptr)||
           !reader.dword(reader.context,current+8,&next)) {
            result.status=Status::RecordUnreadable;return result;
        }
        if(!within(creator,l.text_first,l.text_end,1)) {
            result.status=Status::InvalidCreator;return result;
        }
        if(!within(name_ptr,l.rdata_first,l.rdata_end,1)) {
            result.status=Status::InvalidNamePointer;return result;
        }
        auto& name=names[result.entries];
        bool terminated=false;
        for(std::size_t pos=0;pos<kMaxNameBytes;++pos) {
            if(!within(name_ptr,l.rdata_first,l.rdata_end,pos+1)) {
                result.status=Status::InvalidNamePointer;return result;
            }
            std::uint8_t c=0;
            if(!reader.byte(reader.context,name_ptr+pos,&c)) {
                result.status=Status::NameUnreadable;return result;
            }
            if(c==0) {
                terminated=true;break;
            }
            if(c<0x21||c>0x7e) {
                result.status=Status::NameInvalid;return result;
            }
            name[pos]=static_cast<char>(c);
        }
        if(!terminated||name[0]==0) {
            result.status=Status::NameInvalid;return result;
        }
        for(std::size_t i=0;i<result.entries;++i)
            if(std::strcmp(name.data(),names[i].data())==0) {
                result.status=Status::DuplicateName;return result;
            }
        for(std::size_t i=0;i<l.priority_count;++i) {
            if(std::strcmp(name.data(),l.priority_names[i])==0)
                priority[i]=true;
        }
        ++result.entries;
        current=next;
    }
    std::uint32_t head_after=0;
    if(!reader.dword(reader.context,head_address,&head_after)||
       head_after!=head) {
        result.status=Status::HeadChanged;return result;
    }
    for(std::size_t i=0;i<l.priority_count;++i)
        if(priority[i])++result.priority_found;
    if(result.entries!=l.expected_count) {
        result.status=Status::CountMismatch;return result;
    }
    if(result.priority_found!=l.priority_count) {
        result.status=Status::PriorityMissing;return result;
    }
    result.status=Status::VerifiedUnpinned;
    return result;
}

inline const char* StatusName(Status status) noexcept {
    switch(status) {
        case Status::InvalidInput:return "invalid_input";
        case Status::HeadUnreadable:return "head_unreadable";
        case Status::Empty:return "empty";
        case Status::InvalidRecord:return "invalid_record";
        case Status::Cycle:return "cycle";
        case Status::RecordUnreadable:return "record_unreadable";
        case Status::InvalidCreator:return "invalid_creator";
        case Status::InvalidNamePointer:return "invalid_name_pointer";
        case Status::NameUnreadable:return "name_unreadable";
        case Status::NameInvalid:return "name_invalid";
        case Status::DuplicateName:return "duplicate_name";
        case Status::TooManyRecords:return "too_many_records";
        case Status::HeadChanged:return "head_changed";
        case Status::CountMismatch:return "count_mismatch";
        case Status::PriorityMissing:return "priority_missing";
        case Status::VerifiedUnpinned:return "verified_unpinned";
    }
    return "invalid_status";
}

} // namespace original_interfacereg_snapshot
} // namespace sin_independent
