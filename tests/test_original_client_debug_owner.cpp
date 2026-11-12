#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "sin_compat/original_client_debug_owner.hpp"
#include "sin_compat/original_client_memalloc_bridge.hpp"
namespace n = sin_compat::original_client_debug_owner;
namespace prev = sin_compat::original_client_memalloc_bridge;
struct Fixture {
    struct Event { std::string name; std::uintptr_t a=0,b=0,c=0,d=0; };
    std::vector<Event> log;
    bool enabled=true;
    std::uintptr_t heap=0x12345000u, buffer=0x98760000u;
    std::uint32_t allocation_base=0x400000u;
    int resolves=0;
    bool allocator_probe_5c() { log.push_back({"probe"}); return enabled; }
    std::uintptr_t get_process_heap() { log.push_back({"heap"}); return heap; }
    std::uintptr_t heap_alloc(std::uintptr_t h,std::uint32_t flags,std::uint32_t bytes) {
        log.push_back({"alloc",h,flags,bytes}); return buffer;
    }
    void virtual_query(std::uint32_t anchor,n::MemoryBasicInformation32& mbi,std::uint32_t sz) {
        log.push_back({"query",anchor,sz}); mbi.base_address=0x1000;
        mbi.allocation_base=allocation_base;
    }
    void get_module_filename(std::uint32_t h,std::uintptr_t p,std::uint32_t nbytes) {
        log.push_back({"filename",h,p,nbytes});
    }
    std::uintptr_t resolve_filename_owner() { ++resolves;log.push_back({"resolve"});return buffer; }
    std::uintptr_t allocate_plain(std::uint32_t size) { log.push_back({"plain",size});return 0x1100u+size; }
    std::uintptr_t allocate_with_owner(std::uint32_t size,std::uint32_t owner,std::uint32_t line) {
        log.push_back({"debug",size,owner,line});return 0x2200u+size;
    }
    std::uintptr_t pair_plain(std::uint32_t edi,std::uint32_t esi) {
        log.push_back({"pair_plain",edi,esi});return 0x3300u+edi+esi;
    }
    std::uintptr_t pair_with_owner(std::uint32_t edi,std::uint32_t esi,std::uint32_t owner,std::uint32_t line) {
        log.push_back({"pair_debug",edi,esi,owner,line});return 0x4400u+edi+esi;
    }
};
int main() {
    static_assert(sizeof(n::MemoryBasicInformation32)==28);
    static_assert(offsetof(n::MemoryBasicInformation32,allocation_base)==4);
    static_assert(n::kModuleFilenameCapacity==260 && n::kMemoryBasicInfoBytes==28);
    assert(n::kMemAllocIAT==prev::kTier0MemAllocIAT);
    assert(n::kResolverVA==prev::kDebugOwnerResolverVA);
    assert(n::kPrimaryFlagsVA==prev::kDebugStateFlags);
    assert(n::kPrimaryCachedFilenameVA==prev::kDebugOwnerSlot);

    Fixture f;
    f.enabled=false;
    assert(n::resolve_filename_owner(f)==0);
    assert(f.log.size()==1 && f.log[0].name=="probe");
    f.log.clear(); f.enabled=true;
    assert(n::resolve_filename_owner(f)==f.buffer);
    assert(f.log.size()==5);
    assert(f.log[0].name=="probe");
    assert(f.log[1].name=="heap");
    assert(f.log[2].name=="alloc" && f.log[2].a==f.heap && f.log[2].b==0 && f.log[2].c==260);
    assert(f.log[3].name=="query" && f.log[3].a==n::kVirtualQueryAnchor && f.log[3].b==28);
    assert(f.log[4].name=="filename" && f.log[4].a==f.allocation_base && f.log[4].b==f.buffer && f.log[4].c==260);
    assert(f.log[4].a!=0x1000u); // AllocationBase, NOT BaseAddress.

    // Native does not check whether HeapAlloc or GetModuleFileNameA succeeds:
    f.log.clear();f.buffer=0;
    assert(n::resolve_filename_owner(f)==0);
    assert(f.log.size()==5 && f.log.back().b==0);

    Fixture c;c.buffer=0xAA11u;
    n::CacheState32 primary{0xF0u,0};
    assert(n::allocate_single(c,primary,23)==0x2217u);
    assert(primary.flags==0xF1u && primary.filename==0xAA11u);
    assert(c.resolves==1 && c.log[0].name=="resolve" && c.log[1].name=="debug");
    assert(c.log[1].a==23 && c.log[1].b==0xAA11u && c.log[1].c==0);
    assert(n::allocate_single(c,primary,42)==0x222au);
    assert(c.resolves==1);
    n::CacheState32 secondary{0x80u,0};
    assert(n::dispatch_pair(c,secondary,8,9)==0x4411u);
    assert(c.resolves==2 && secondary.flags==0x81u && secondary.filename==0xAA11u);
    assert(c.log.back().name=="pair_debug");
    assert(c.log.back().a==8 && c.log.back().b==9 && c.log.back().c==0xAA11u && c.log.back().d==0);
    assert(primary.filename==secondary.filename); // equal returned value, independent caches

    Fixture none;none.buffer=0;
    n::CacheState32 a{0x80000000u,0};
    assert(n::allocate_single(none,a,7)==0x1107u);
    assert(n::allocate_single(none,a,8)==0x1108u);
    assert(none.resolves==1 && a.flags==0x80000001u && a.filename==0);
    n::CacheState32 b{};
    assert(n::dispatch_pair(none,b,4,5)==0x3309u);
    assert(n::dispatch_pair(none,b,8,9)==0x3311u);
    assert(none.resolves==2 && b.flags==1);
    assert(none.log.back().name=="pair_plain" && none.log.back().a==8 && none.log.back().b==9);

    // Connecting original resolver to Phase185 allocator bridge by forwarding
    // an actual debug owner returned by the host's original OS sequence.
    Fixture combined;combined.buffer=0x5500;
    struct Composed {
        Fixture& f;
        std::uintptr_t resolve_debug_owner(){return n::resolve_filename_owner(f);}
        std::uintptr_t ordinary_alloc(std::uint32_t sz){return f.allocate_plain(sz);}
        std::uintptr_t debug_alloc(std::uint32_t sz,std::uintptr_t owner,std::uint32_t line){return f.allocate_with_owner(sz,static_cast<std::uint32_t>(owner),line);}
    } adapter{combined};
    prev::AllocationBridge bridge;
    assert(bridge.allocate(adapter,15)==0x220fu);
    assert(bridge.allocate(adapter,16)==0x2210u);
    assert(combined.log.size()==7); // five original resolver callbacks + two allocations
    assert(combined.log[5].name=="debug" && combined.log[5].b==combined.buffer);
}