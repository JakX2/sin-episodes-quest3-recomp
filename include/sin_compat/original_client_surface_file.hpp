#pragma once
// Phase182: original 2006 SiN client.dll KeyValues value-to-string and
// filesystem-to-surface-property dispatch; host-neutral C++17 reference.
// Functions, call order and 32-bit field offsets are original-binary verified.
// This is NOT a native Source-engine ABI, playable DLL or file parser.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <type_traits>

namespace sin_compat::original_client_surface_file {
inline constexpr std::uint32_t kGetValueStringVA=0x241667C0u;
inline constexpr std::uint32_t kReadSurfaceFileVA=0x24191EC0u;
inline constexpr std::uint32_t kSurfaceBridge4VA=0x24192050u;
inline constexpr std::uint32_t kSurfaceBridge8VA=0x24192070u;
inline constexpr std::uint32_t kSurfaceBridge12VA=0x24192090u;
inline constexpr std::uint32_t kSurfaceMethodsGlobalVA=0x243743ACu;
inline constexpr std::uint32_t kValueTagTableVA=0x241668A8u;
inline constexpr const char* kOriginalFileMode="rb";
inline constexpr const char* kOriginalManifest="scripts/surfaceproperties_manifest.txt";
inline constexpr std::uint32_t kNativeUtf8CodePage=65001u;
inline constexpr std::size_t kNativeWideConversionBufferSize=0x200;

// These five DWORD fields are read at the native offsets. Do not replace the
// DWORDs with host-size pointers: that would lose MSVC32 layout fidelity.
struct NativeValuePrefix32 {
    std::uint32_t name_symbol_00;
    std::uint32_t string_address_04;
    std::uint32_t wide_address_08;
    std::uint32_t scalar_bits_0c;
    std::uint32_t tag_10;
};
static_assert(sizeof(NativeValuePrefix32)==20,"native value prefix must be 20 bytes");
static_assert(offsetof(NativeValuePrefix32, tag_10)==0x10,"native tag +0x10");
static_assert(std::is_standard_layout<NativeValuePrefix32>::value,"fixed-layout value prefix");

// Host adaptation uses wider addresses, but preserves the same type cases.
struct ValueRecord {
    std::uint32_t tag=0;
    std::uintptr_t published_string=0;
    std::uint32_t wide_address=0;
    std::uint32_t scalar_bits=0;
};
inline std::string original_decimal(std::uint32_t bits) {
    return std::to_string(static_cast<std::int32_t>(bits));
}
inline std::string original_float_format(float value) {
    char buffer[96]{};
    std::snprintf(buffer,sizeof(buffer),"%f",static_cast<double>(value));
    return buffer;
}

// Native 0x241667C0 uses find-key(this,key,0), then dispatches by
// unsigned (tag-1), allowing only tags 1..5. Jump table: type 1 returns
// existing [node+4]; tags 2/4 render decimal [node+0xC]; tag 3 renders
// float32 [node+0xC]; tag 5 calls CP_UTF8 WideCharToMultiByte with 0x200
// output bytes. Successful conversion/formatting is handed to the original
// opaque key-value setter, after which [node+4] is returned.
// Host: ValueRecord* find(const char*,bool); void assign(ValueRecord&,string);
// optional<string> convert_utf8(uint32_t wchar_address, size_t capacity).
template<class Host>
std::uintptr_t get_value_string(Host& host,const char* name,
                                std::uintptr_t fallback) {
    ValueRecord* n=host.find(name,false);
    if(!n) return fallback;
    switch(n->tag) {
        case 1: return n->published_string;
        case 2:
        case 4:
            host.assign(*n,original_decimal(n->scalar_bits));
            return n->published_string;
        case 3: {
            float f;
            static_assert(sizeof(f)==sizeof(n->scalar_bits),"float32 expected");
            const std::uint32_t raw=n->scalar_bits;
            // memcpy avoids type-punning/strict-aliasing assumptions.
            std::memcpy(&f,&raw,sizeof(f));
            host.assign(*n,original_float_format(f));
            return n->published_string;
        }
        case 5: {
            std::optional<std::string> translated=host.convert_utf8(n->wide_address,
                                                    kNativeWideConversionBufferSize);
            if(translated) {
                host.assign(*n,*translated);
                return n->published_string;
            }
            return fallback;
        }
        default:return fallback;
    }
}

struct FileResult {
    bool opened=false;
    std::size_t size=0;
};

// Original 0x24191EC0: filesystem virtual +8 opens (path,"rb",0),
// virtual +0x1C queries byte length, +0 reads length bytes, +0xC closes;
// after close, append NUL and invoke surfaceprops virtual +4 with file path
// and the buffer. On missing handle only, log the original warning.
// Native uses a stack allocation and does NOT inspect read's return value.
// A finite maximum and checked heap buffer are safety substitutions here.
// Host: uintptr_t open(path,mode,uint32 flags); size_t size(handle);
// void read(handle,char*,size_t); void close(handle);
// void parse_surface(path,const char*); void warning(path,manifest).
template<class Host>
FileResult read_surface_file(Host& host,const char* filename,
                             std::size_t max_safe_size=16u*1024u*1024u) {
    const std::uintptr_t handle=host.open(filename,kOriginalFileMode,0);
    if(!handle) {
        host.warning(filename,kOriginalManifest);
        return {};
    }
    const std::size_t size=host.size(handle);
    if(size>max_safe_size) {
        host.close(handle); // deliberate host safety, not original native branch
        throw std::length_error("unsafe original stack-size request");
    }
    std::vector<char> bytes(size+1u);
    host.read(handle,bytes.data(),size); // no original short-read test
    host.close(handle);
    bytes[size]='\0';
    host.parse_surface(filename,bytes.data());
    return {true,size};
}

// Native +0x04/+0x08 wrappers test BOTH input pointers for zero before
// fetching global interface 0x243743AC, then virtual-dispatching.
// Native +0x0C wrapper DOES NOT perform either null test.
template<class Host>
void dispatch_if_both_method4(Host& h,std::uintptr_t a,std::uintptr_t b) {
    if(a && b) h.method4(a,b);
}
template<class Host>
void dispatch_if_both_method8(Host& h,std::uintptr_t a,std::uintptr_t b) {
    if(a && b) h.method8(a,b);
}
template<class Host>
void dispatch_unconditional_method12(Host& h,std::uintptr_t a,std::uintptr_t b) {
    h.method12(a,b);
}
} // namespace sin_compat::original_client_surface_file