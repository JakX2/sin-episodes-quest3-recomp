// Phase186 real Windows x86 kernel32 ABI smoke. The tier0 IMemAlloc virtual
// +0x5C branch is synthetic; the Win32 calls below are the real OS APIs.
// The original 0x24372904 anchor address is remapped to a variable in this
// *test executable*, not claimed to be a live original SiN client.dll global.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <windows.h>
#include "sin_compat/original_client_debug_owner.hpp"
namespace original = sin_compat::original_client_debug_owner;
static int gAnchorForNativeVirtualQuery=0x186;

struct Win32TestHost {
    bool enabled=true;
    unsigned probe_calls=0, heap_calls=0, alloc_calls=0, query_calls=0, name_calls=0;
    bool allocator_probe_5c() { ++probe_calls;return enabled; }
    std::uintptr_t get_process_heap(){++heap_calls;return reinterpret_cast<std::uintptr_t>(GetProcessHeap());}
    std::uintptr_t heap_alloc(std::uintptr_t heap,std::uint32_t flags,std::uint32_t bytes){
        ++alloc_calls;
        return reinterpret_cast<std::uintptr_t>(HeapAlloc(reinterpret_cast<HANDLE>(heap),flags,bytes));
    }
    void virtual_query(std::uint32_t anchor, original::MemoryBasicInformation32& output,std::uint32_t size) {
        ++query_calls;
        assert(anchor==original::kVirtualQueryAnchor);
        assert(size==sizeof(MEMORY_BASIC_INFORMATION));
        MEMORY_BASIC_INFORMATION info{};
        assert(VirtualQuery(&gAnchorForNativeVirtualQuery,&info,sizeof(info))==sizeof(info));
        output.base_address=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(info.BaseAddress));
        output.allocation_base=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(info.AllocationBase));
        output.allocation_protect=info.AllocationProtect;
        output.region_size=static_cast<std::uint32_t>(info.RegionSize);
        output.state=info.State;output.protect=info.Protect;output.type=info.Type;
    }
    void get_module_filename(std::uint32_t allocation_base,std::uintptr_t buffer,std::uint32_t cap) {
        ++name_calls;
        assert(buffer!=0 && cap==260);
        const auto written=GetModuleFileNameA(reinterpret_cast<HMODULE>(static_cast<std::uintptr_t>(allocation_base)),
                                            reinterpret_cast<char*>(buffer),cap);
        assert(written>0 && written<cap);
    }
};
int main() {
    static_assert(sizeof(void*)==4,"Phase186 actual Win32 smoke must run as x86 PE32");
    static_assert(sizeof(MEMORY_BASIC_INFORMATION)==28,"Original Win32 MBI must be 28 bytes");
    static_assert(sizeof(original::MemoryBasicInformation32)==sizeof(MEMORY_BASIC_INFORMATION));
    Win32TestHost host;
    const auto mem=original::resolve_filename_owner(host);
    assert(mem!=0);
    assert(host.probe_calls==1 && host.heap_calls==1 && host.alloc_calls==1);
    assert(host.query_calls==1 && host.name_calls==1);
    const std::string filename(reinterpret_cast<const char*>(mem));
    assert(!filename.empty());
    char current_exe[MAX_PATH] = {};
    assert(GetModuleFileNameA(nullptr,current_exe,MAX_PATH)>0);
    assert(filename==current_exe);
    assert(HeapFree(GetProcessHeap(),0,reinterpret_cast<LPVOID>(mem))!=0);
    Win32TestHost disabled;disabled.enabled=false;
    assert(original::resolve_filename_owner(disabled)==0);
    assert(disabled.probe_calls==1 && disabled.heap_calls==0 && disabled.alloc_calls==0);
    assert(disabled.query_calls==0 && disabled.name_calls==0);
}