#pragma once
// Read-only legacy VGUI slot ownership inspection. No virtual calls.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
namespace sin_independent::vgui_layout {
inline bool Readable(const void* p,std::size_t n) noexcept {
    if(!p || !n)return false;
    MEMORY_BASIC_INFORMATION m{};
    if(VirtualQuery(p,&m,sizeof(m))!=sizeof(m) || m.State!=MEM_COMMIT ||
       (m.Protect & (PAGE_NOACCESS|PAGE_GUARD)))return false;
    const auto prot=m.Protect & 0xffu;
    if(prot!=PAGE_READONLY && prot!=PAGE_READWRITE && prot!=PAGE_WRITECOPY &&
       prot!=PAGE_EXECUTE_READ && prot!=PAGE_EXECUTE_READWRITE &&
       prot!=PAGE_EXECUTE_WRITECOPY)return false;
    const auto addr=reinterpret_cast<std::uintptr_t>(p);
    const auto base=reinterpret_cast<std::uintptr_t>(m.BaseAddress);
    return addr>=base && n<=m.RegionSize && addr-base<=m.RegionSize-n;
}
inline bool OwnedBy(HMODULE expected,const void* p) noexcept {
    HMODULE found=nullptr;
    return expected && p && GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(p),&found) && expected==found;
}
inline bool Executable(const void* p) noexcept {
    MEMORY_BASIC_INFORMATION m{};
    if(!p || VirtualQuery(p,&m,sizeof(m))!=sizeof(m) ||
       m.State!=MEM_COMMIT || (m.Protect & (PAGE_NOACCESS|PAGE_GUARD)))
        return false;
    const auto prot=m.Protect & 0xffu;
    return prot==PAGE_EXECUTE || prot==PAGE_EXECUTE_READ ||
        prot==PAGE_EXECUTE_READWRITE || prot==PAGE_EXECUTE_WRITECOPY;
}
inline bool Verify(HMODULE module,void* object,std::size_t highSlot) noexcept {
    if(!module || !Readable(object,sizeof(void*)))return false;
    void** table=nullptr;
    std::memcpy(&table,object,sizeof(table));
    if(!table || !OwnedBy(module,table) ||
       !Readable(table,(highSlot+1)*sizeof(void*)))return false;
    for(auto slot : {std::size_t{0},highSlot}) {
        void* method=nullptr;
        std::memcpy(&method,table+slot,sizeof(method));
        if(!OwnedBy(module,method) || !Executable(method))return false;
    }
    return true;
}
} // namespace sin_independent::vgui_layout
