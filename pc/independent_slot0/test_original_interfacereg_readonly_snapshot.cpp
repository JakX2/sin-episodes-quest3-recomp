// Phase 1653: never load proprietary game files. Reconstruct only PE32
// virtual-address DWORD/byte memory with synthetic typed callbacks.
#include "original_interfacereg_readonly_snapshot.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace sin_independent::original_interfacereg_snapshot;

static int Fail(int n,const char* why) {
    std::fprintf(stderr,"FAIL original PE32 InterfaceReg snapshot %d: %s\n",
                 n,why);
    return n;
}
struct FakeMemory {
    Layout layout{};
    std::map<std::uintptr_t,std::uint8_t> bytes{};
    std::size_t records=0;
    std::uintptr_t denied=0;
    bool mutate_head=false;
    unsigned head_reads=0;

    static bool Byte(void* context,std::uintptr_t address,
                     std::uint8_t* out) noexcept {
        auto& self=*static_cast<FakeMemory*>(context);
        if(!out||address==self.denied)return false;
        const auto it=self.bytes.find(address);
        if(it==self.bytes.end())return false;
        *out=it->second;
        return true;
    }
    static bool Dword(void* context,std::uintptr_t address,
                      std::uint32_t* out) noexcept {
        auto& self=*static_cast<FakeMemory*>(context);
        if(!out)return false;
        if(address==self.layout.module_base+
                       self.layout.registry_head_rva) {
            ++self.head_reads;
            if(self.mutate_head&&self.head_reads>=2) {
                *out=0;return true;
            }
        }
        std::uint32_t value=0;
        for(std::size_t i=0;i<4;++i) {
            std::uint8_t byte=0;
            if(!Byte(context,address+i,&byte))return false;
            value|=std::uint32_t(byte)<<(8u*i);
        }
        *out=value;
        return true;
    }
    Reader Callbacks() noexcept {
        return {this,&Dword,&Byte};
    }
    void Put32(std::uintptr_t address,std::uint32_t value) {
        for(std::size_t i=0;i<4;++i)
            bytes[address+i]=static_cast<std::uint8_t>(value>>(i*8u));
    }
    void PutString(std::uintptr_t address,const std::string& value) {
        for(std::size_t i=0;i<value.size();++i)
            bytes[address+i]=static_cast<std::uint8_t>(value[i]);
        bytes[address+value.size()]=0;
    }
    std::uintptr_t Node(std::size_t index) const noexcept {
        return layout.module_base+layout.data_first+0x100+
               index*kRecordBytes;
    }
    std::uintptr_t Name(std::size_t index) const noexcept {
        return layout.module_base+layout.rdata_first+0x100+
               index*100u;
    }
    static FakeMemory Build(Layout config,
                            const std::vector<std::string>& names) {
        FakeMemory f{};
        f.layout=config;
        f.records=names.size();
        f.Put32(config.module_base+config.registry_head_rva,
                names.empty()?0:static_cast<std::uint32_t>(
                    f.Node(0)));
        for(std::size_t i=0;i<names.size();++i) {
            const auto node=f.Node(i);
            f.Put32(node,static_cast<std::uint32_t>(
                config.module_base+config.text_first+0x100+
                i*16));
            f.Put32(node+4,static_cast<std::uint32_t>(f.Name(i)));
            f.Put32(node+8,i+1<names.size()?
                    static_cast<std::uint32_t>(f.Node(i+1)):0u);
            f.PutString(f.Name(i),names[i]);
        }
        return f;
    }
    Result Scan() noexcept {
        return Observe(layout,Callbacks());
    }
};

static std::vector<std::string> Names(
    const char* const* priority,std::size_t priority_count,
    std::size_t expected_count) {
    std::vector<std::string> names;
    for(std::size_t i=0;i<priority_count;++i)
        names.emplace_back(priority[i]);
    for(std::size_t i=priority_count;i<expected_count;++i)
        names.emplace_back("OriginalFactory_"+std::to_string(i));
    return names;
}

int main() {
    // All bases are fake, original-free address tokens. Readbacks are made
    // through std::map, not through reinterpret_cast dereferences.
    const auto client=OriginalClientLayout(0x25000000u);
    const auto server=OriginalServerLayout(0x26000000u);
    if(client.expected_count!=55||client.priority_count!=8||
       client.registry_head_rva!=0x369378||
       server.expected_count!=10||server.priority_count!=10||
       server.registry_head_rva!=0x573C5C)
        return Fail(1,"original 2006 PE32 section/registry manifests");
    const auto client_names=Names(kClientPriority,8,55);
    const auto server_names=Names(kServerPriority,10,10);
    auto a=FakeMemory::Build(client,client_names);
    auto b=FakeMemory::Build(server,server_names);
    auto ar=a.Scan(),br=b.Scan();
    if(!ar.accepted()||ar.entries!=55||ar.priority_found!=8||
       !br.accepted()||br.entries!=10||br.priority_found!=10||
       a.head_reads!=2||b.head_reads!=2||
       std::string(StatusName(ar.status))!="verified_unpinned")
        return Fail(2,"complete original client 55/server 10 manifests");

    auto empty=FakeMemory::Build(client,{});
    if(empty.Scan().status!=Status::Empty)
        return Fail(3,"empty module registry accepted");
    auto bad_config=FakeMemory::Build(client,client_names);
    bad_config.layout.module_base=0;
    if(bad_config.Scan().status!=Status::InvalidInput)
        return Fail(4,"invalid PE32 module base");
    bad_config=FakeMemory::Build(client,client_names);
    bad_config.layout.image_size=0x200;
    if(bad_config.Scan().status!=Status::InvalidInput)
        return Fail(5,"section endpoints beyond mapped PE32 image");
    bad_config=FakeMemory::Build(client,client_names);
    bad_config.layout.priority_names=nullptr;
    if(bad_config.Scan().status!=Status::InvalidInput)
        return Fail(6,"missing trusted priority list");
    bad_config=FakeMemory::Build(client,client_names);
    bad_config.layout.module_base=0xFFFF0000u;
    if(bad_config.Scan().status!=Status::InvalidInput)
        return Fail(7,"overflowing relocated PE32 module end");

    auto changed=FakeMemory::Build(client,client_names);
    changed.mutate_head=true;
    if(changed.Scan().status!=Status::HeadChanged)
        return Fail(8,"registry head changed during read");
    auto denied=FakeMemory::Build(client,client_names);
    denied.denied=client.module_base+client.registry_head_rva;
    if(denied.Scan().status!=Status::HeadUnreadable)
        return Fail(9,"unreadable original head accepted");
    denied=FakeMemory::Build(client,client_names);
    denied.denied=denied.Node(0)+4;
    if(denied.Scan().status!=Status::RecordUnreadable)
        return Fail(10,"unreadable original name pointer accepted");
    denied=FakeMemory::Build(client,client_names);
    denied.denied=denied.Name(0);
    if(denied.Scan().status!=Status::NameUnreadable)
        return Fail(11,"unreadable original version C string accepted");

    auto invalid=FakeMemory::Build(client,client_names);
    invalid.Put32(client.module_base+client.registry_head_rva,0xdead0000u);
    if(invalid.Scan().status!=Status::InvalidRecord)
        return Fail(12,"out-of-image registry head followed");
    invalid=FakeMemory::Build(client,client_names);
    invalid.Put32(invalid.Node(0),0x41414141u);
    if(invalid.Scan().status!=Status::InvalidCreator)
        return Fail(13,"creator outside original .text followed");
    invalid=FakeMemory::Build(client,client_names);
    invalid.Put32(invalid.Node(0)+4,0x41414141u);
    if(invalid.Scan().status!=Status::InvalidNamePointer)
        return Fail(14,"interface name outside original .rdata followed");
    invalid=FakeMemory::Build(client,client_names);
    invalid.Put32(invalid.Node(0)+8,0x41414141u);
    if(invalid.Scan().status!=Status::InvalidRecord)
        return Fail(15,"linked next outside original .data followed");
    invalid=FakeMemory::Build(client,client_names);
    invalid.PutString(invalid.Name(0)," Bad_Interface");
    if(invalid.Scan().status!=Status::NameInvalid)
        return Fail(16,"untrusted printable-name check bypassed");
    invalid=FakeMemory::Build(client,client_names);
    invalid.PutString(invalid.Name(1),client_names[0]);
    if(invalid.Scan().status!=Status::DuplicateName)
        return Fail(17,"duplicate InterfaceReg names not rejected");
    invalid=FakeMemory::Build(client,client_names);
    invalid.Put32(invalid.Node(54)+8,
                  static_cast<std::uint32_t>(invalid.Node(1)));
    if(invalid.Scan().status!=Status::Cycle)
        return Fail(18,"cyclic original registry not detected");
    invalid=FakeMemory::Build(client,client_names);
    invalid.layout.expected_count=54;
    if(invalid.Scan().status!=Status::CountMismatch)
        return Fail(19,"missing registration count not detected");
    invalid=FakeMemory::Build(client,client_names);
    static const char* const unregistered[]={"VClient011","NotOriginal"};
    invalid.layout.priority_names=unregistered;
    invalid.layout.priority_count=2;
    if(invalid.Scan().status!=Status::PriorityMissing)
        return Fail(20,"missing priority interface not detected");

    // Exhaust the 64-record safety bound with a fully valid 65-node
    // synthetic original layout, no reads outside our std::map.
    auto oversized_names=Names(kClientPriority,8,65);
    auto oversized=FakeMemory::Build(client,oversized_names);
    if(oversized.Scan().status!=Status::TooManyRecords)
        return Fail(21,"runaway registry exceeded 64-record budget");

    // Refuse a non-terminated <=96-byte name rather than scanning memory
    // beyond the authenticated original section or into the next object.
    auto long_name=FakeMemory::Build(client,client_names);
    for(std::size_t i=0;i<kMaxNameBytes;++i)
        long_name.bytes[long_name.Name(0)+i]='X';
    if(long_name.Scan().status!=Status::NameInvalid)
        return Fail(22,"unterminated original name boundary");

    std::puts("PASS owner-read-only original InterfaceReg snapshot: PE32 section bounds, client 55/server 10, eight/ten authentic priorities, callback-only memory, cycle/ABA-head/refusal, duplicate names and 64-node budget");
    return 0;
}
