// Read-only Win32 x86 interface-object/vtable observation of owner-original SiN.
// This DOES NOT call any original virtual methods or launch the retail engine.
// --synthetic mode uses entirely fake DLLs to exercise the inspection mechanics.
// --owner mode SHA256 authenticates the entire actual 2006 DLL before LoadLibrary.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>

#pragma comment(lib, "Bcrypt.lib")
#if !defined(_M_IX86)
#error "This original interface object probe must be built for native Windows x86"
#endif

namespace {
using Factory = void* (__cdecl *)(const char*, int*);

struct Pin {
    const char* name;
    std::uint32_t singleton_rva;
    std::uint32_t vtable_rva;
    std::uint32_t first_method_rva;
    std::uint32_t checked_prefix_slots;
};

// RVA metadata from previously SHA-reviewed 2006 PE32 singleton factory
// and initialized vtable evidence (Phases 178 / 226). These are NOT
// original Source SDK method definitions, and not every virtual slot is known.
constexpr Pin kServerPins[] = {
    {"ServerGameDLL003",     0x00510650u, 0x00442EFCu, 0x001E7640u, 31u},
    {"ServerGameEnts001",    0x00510654u, 0x00443030u, 0x001E9360u,  7u},
    {"ServerGameClients003", 0x00510658u, 0x00443054u, 0x001E63F0u, 15u},
};
constexpr Pin kClientPins[] = {
    {"GameClientExports001", 0x002FA4B0u, 0x00296B18u, 0x000FE9B0u,  4u},
    {"VClient011",           0x002FA4B8u, 0x00296C6Cu, 0x000FE0E0u, 48u},
};
constexpr char kServerHash[] =
    "568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650";
constexpr char kClientHash[] =
    "6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d";
constexpr LONGLONG kServerSize = 6049792LL;
constexpr LONGLONG kClientSize = 3592192LL;

bool hash_is_original(const wchar_t* path, const char* expected, LONGLONG expected_size) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hasher = nullptr;
    LARGE_INTEGER length{};
    BYTE block[65536]{};
    BYTE digest[32]{};
    bool valid = false;
    do {
        if (!GetFileSizeEx(file, &length) || length.QuadPart != expected_size) break;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
                                        nullptr, 0) < 0) break;
        if (BCryptCreateHash(algorithm, &hasher, nullptr, 0, nullptr, 0, 0) < 0) break;
        bool failed = false;
        for (;;) {
            DWORD read = 0;
            if (!ReadFile(file, block, sizeof(block), &read, nullptr)) {
                failed = true;
                break;
            }
            if (!read) break;
            if (BCryptHashData(hasher, block, read, 0) < 0) {
                failed = true;
                break;
            }
        }
        if (failed || BCryptFinishHash(hasher, digest, sizeof(digest), 0) < 0) break;
        char actual[65]{};
        for (unsigned index = 0; index < sizeof(digest); ++index)
            std::snprintf(actual + 2u * index, 3, "%02x", digest[index]);
        valid = std::strcmp(actual, expected) == 0;
    } while (false);
    if (hasher != nullptr) BCryptDestroyHash(hasher);
    if (algorithm != nullptr) BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return valid;
}

bool image_region(HMODULE module, const void* address, std::size_t length,
                  bool require_executable) {
    if (!address || !length) return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(address, &mbi, sizeof(mbi))) return false;
    const auto start = reinterpret_cast<std::uintptr_t>(address);
    const auto region = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
    if (start < region || length > mbi.RegionSize - (start - region)) return false;
    if (mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE ||
        mbi.AllocationBase != module || (mbi.Protect & PAGE_GUARD))
        return false;
    switch (mbi.Protect & 0xFFu) {
        case PAGE_EXECUTE:
            return require_executable;
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        case PAGE_READONLY:
        case PAGE_READWRITE:
        case PAGE_WRITECOPY:
            return !require_executable;
        default:
            return false;
    }
}

bool to_rva(HMODULE module, const void* pointer, std::uint32_t& value) {
    const auto start = reinterpret_cast<std::uintptr_t>(module);
    const auto target = reinterpret_cast<std::uintptr_t>(pointer);
    if (target < start) return false;
    const auto offset = target - start;
    if (offset > 0xffffffffu) return false;
    value = static_cast<std::uint32_t>(offset);
    return true;
}

void* read_pointer(const void* slot) {
    void* output = nullptr;
    std::memcpy(&output, slot, sizeof(output));
    return output;
}

struct ISynthetic {
    virtual unsigned long __thiscall token() = 0;
protected:
    ~ISynthetic() = default;
};

int inspect(HMODULE module, Factory factory, const Pin& pin,
            bool synthetic, std::uint32_t token) {
    int code = -7;
    void* object = factory(pin.name, &code);
    if (object == nullptr || code != 0) {
        std::fprintf(stderr, "FAIL factory %s: returned no interface object (status=%d)\n",
                     pin.name, code);
        return 1;
    }
    if (!image_region(module, object, sizeof(void*), false)) {
        std::fprintf(stderr, "FAIL factory %s: object is not readable image memory\n", pin.name);
        return 1;
    }
    const void* vtable = read_pointer(object);
    if (!image_region(module, vtable, pin.checked_prefix_slots * sizeof(void*), false)) {
        std::fprintf(stderr, "FAIL factory %s: vtable outside readable module image\n", pin.name);
        return 1;
    }
    if (!synthetic) {
        std::uint32_t actual = 0;
        if (!to_rva(module, object, actual) || actual != pin.singleton_rva ||
            !to_rva(module, vtable, actual) || actual != pin.vtable_rva) {
            std::fprintf(stderr, "FAIL factory %s: original singleton/vptr RVA mismatch\n",
                         pin.name);
            return 1;
        }
    }
    for (std::uint32_t slot = 0; slot < pin.checked_prefix_slots; ++slot) {
        const auto offset = slot * sizeof(void*);
        const auto* address = static_cast<const unsigned char*>(vtable) + offset;
        const void* method = read_pointer(address);
        if (!image_region(module, method, 1, true)) {
            std::fprintf(stderr, "FAIL factory %s: virtual pointer slot %lu is not executable in module\n",
                         pin.name, static_cast<unsigned long>(slot));
            return 1;
        }
        if (!synthetic && slot == 0) {
            std::uint32_t actual = 0;
            if (!to_rva(module, method, actual) || actual != pin.first_method_rva) {
                std::fprintf(stderr, "FAIL factory %s: first method RVA mismatch\n", pin.name);
                return 1;
            }
        }
    }
    if (synthetic) {
        // This direct vcall is safe ONLY for our separately built fake object,
        // whose one synthetic virtual method has this known test signature.
        if (static_cast<ISynthetic*>(object)->token() != token) {
            std::fprintf(stderr, "FAIL synthetic %s: first virtual method returned wrong token\n",
                         pin.name);
            return 1;
        }
    }
    std::printf("PASS %s %s: object/vptr ownership, %lu executable prefix slot(s)%s\n",
                synthetic ? "synthetic" : "owner-original", pin.name,
                static_cast<unsigned long>(pin.checked_prefix_slots),
                synthetic ? ", one safe synthetic vcall" : ", no virtual methods invoked");
    return 0;
}

int run(bool synthetic, bool server, const wchar_t* path) {
    const char* hash = server ? kServerHash : kClientHash;
    const LONGLONG bytes = server ? kServerSize : kClientSize;
    if (!synthetic && !hash_is_original(path, hash, bytes)) {
        std::fputs("FAIL full original DLL size/SHA256 authentication; refusing LoadLibrary\n", stderr);
        return 2;
    }
    wchar_t absolute[32768]{};
    const DWORD length = GetFullPathNameW(path, static_cast<DWORD>(sizeof(absolute) /
                                                   sizeof(absolute[0])), absolute, nullptr);
    if (!length || length >= sizeof(absolute) / sizeof(absolute[0])) {
        std::fputs("FAIL invalid owner/synthetic module path\n", stderr);
        return 3;
    }
    HMODULE module = LoadLibraryExW(absolute, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (module == nullptr) {
        std::fprintf(stderr, "FAIL LoadLibraryExW, Win32 error=%lu\n",
                     static_cast<unsigned long>(GetLastError()));
        return 4;
    }
    auto entry = GetProcAddress(module, "CreateInterface");
    if (entry == nullptr || !image_region(module,
             reinterpret_cast<const void*>(entry), 1, true)) {
        std::fputs("FAIL original/synthetic module missing its own CreateInterface\n", stderr);
        FreeLibrary(module);
        return 5;
    }
    auto factory = reinterpret_cast<Factory>(entry);
    int result = 0;
    if (synthetic) {
        const Pin pin{server ? "ServerGameDLL003" : "VClient011", 0, 0, 0, 1};
        result = inspect(module, factory, pin, true, server ? 0x53494e53u : 0x53494e43u);
    } else {
        const Pin* pins = server ? kServerPins : kClientPins;
        const auto count = server ? sizeof(kServerPins) / sizeof(kServerPins[0])
                                  : sizeof(kClientPins) / sizeof(kClientPins[0]);
        for (std::size_t index = 0; index < count && result == 0; ++index)
            result = inspect(module, factory, pins[index], false, 0);
    }
    if (!FreeLibrary(module)) {
        std::fputs("FAIL FreeLibrary of inspected module\n", stderr);
        return 6;
    }
    return result;
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 4 ||
        (std::wcscmp(argv[1], L"--owner") != 0 &&
         std::wcscmp(argv[1], L"--synthetic") != 0) ||
        (std::wcscmp(argv[2], L"server") != 0 &&
         std::wcscmp(argv[2], L"client") != 0)) {
        std::fputs("Usage: sin_pc_owner_vptr_probe.exe --synthetic|--owner server|client PATH_TO_DLL\n"
                   "OWNER mode verifies full original SHA256 and never calls original virtual methods.\n",
                   stderr);
        return 64;
    }
    return run(std::wcscmp(argv[1], L"--synthetic") == 0,
               std::wcscmp(argv[2], L"server") == 0, argv[3]);
}
