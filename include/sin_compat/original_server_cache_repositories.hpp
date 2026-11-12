#pragma once
// Phase188: original 2006 SiN Episodes server cached repository loaders.
// Behavior reconstructed from two 576-byte original x86 functions and their
// 80-byte reload wrappers. Host interfaces are intentionally synthetic:
// no Source IFileSystem, record serializer, or engine-compatible DLL is here.
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace sin_compat::original_server_cache_repositories {

inline constexpr std::uint32_t kSceneObject = 0x22503EE0u;
inline constexpr std::uint32_t kModelSoundsObject = 0x224F0388u;
inline constexpr std::uint32_t kSceneInit = 0x221AD3E0u;
inline constexpr std::uint32_t kModelSoundsInit = 0x221498C0u;
inline constexpr std::uint32_t kSceneReload = 0x221AD620u;
inline constexpr std::uint32_t kModelSoundsReload = 0x22149B00u;
inline constexpr std::uint32_t kFileSystemSlot = 0x225721A4u;
inline constexpr std::string_view kPathID = "MOD";
inline constexpr std::string_view kOpenMode = "rb";
inline constexpr std::uint32_t kCacheSystemVersion = 2u;

struct RepositoryConfig {
    std::uint32_t singleton_va;
    std::string_view filename;
    std::uint32_t expected_version;
    std::uint32_t init_va;
    std::uint32_t reload_va;
};
inline constexpr RepositoryConfig kScene = {
    kSceneObject, "scene.cache", 3u, kSceneInit, kSceneReload};
inline constexpr RepositoryConfig kModelSounds = {
    kModelSoundsObject, "modelsounds.cache", 2u, kModelSoundsInit, kModelSoundsReload};

// The original object fields are at +0x34 filename, +0x38 repository version,
// +0x3C needs-rebuild flag, +0x3D already-initialized byte, +0x40 optional
// metadata callback, +0x44 cached metadata checksum, +0x4E reload gate.
struct State {
    bool initialized = false;
    bool cache_invalid_or_missing = false;
    bool reload_gate = false;
    std::uint32_t cached_metadata = 0;
};

enum class Rejection {
    cache_system_version,
    repository_version,
    metadata_checksum,
};

// Observed original x86 prefix and decision/cleanup order:
// - initialized=true BEFORE checking the filename, including failure;
// - existing initialized=true fast path returns success immediately;
// - nonexistent cache is not fatal, marks needs-rebuild and returns true;
// - the file header consists of original 4-byte cache system version (=2),
//   per-repository version (scene=3/model sounds=2), metadata checksum,
//   and signed record count;
// - read operations have no native short-read check;
// - invalid header closes the file, invalidates it via filesystem slot +0x44,
//   marks needs-rebuild, returns true; there is no rollback of initialized.
// Native record payload decoding is NOT reconstructed: process_record is
// an opaque callback for this still-unported loop, not a file-parser claim.
// Host contract:
//   void no_filename();
//   uint32_t compute_metadata(const RepositoryConfig&);
//   uintptr_t open_file(std::string_view,std::string_view,std::string_view);
//   uint32_t read_dword(uintptr_t);
//   void process_record(const RepositoryConfig&,uintptr_t,uint32_t);
//   void reject(const RepositoryConfig&,Rejection);
//   void close_file(uintptr_t);
//   void invalidate_file(std::string_view,std::string_view);
template<class Host>
bool initialize(const RepositoryConfig& config, State& state, Host& host,
                std::size_t max_records = 65536) {
    if (state.initialized) return true;
    state.initialized = true;
    if (config.filename.empty()) {
        host.no_filename();
        return false;
    }
    state.cached_metadata = host.compute_metadata(config);
    const std::uintptr_t file = host.open_file(config.filename,kOpenMode,kPathID);
    if (!file) {
        state.cache_invalid_or_missing = true;
        return true;
    }
    bool rejected = false;
    if (host.read_dword(file) != kCacheSystemVersion) {
        host.reject(config,Rejection::cache_system_version);
        rejected = true;
    } else if (host.read_dword(file) != config.expected_version) {
        host.reject(config,Rejection::repository_version);
        rejected = true;
    } else if ([&] { const auto from_file=host.read_dword(file);
                   return state.cached_metadata != 0u &&
                          from_file != state.cached_metadata; }()) {
        host.reject(config,Rejection::metadata_checksum);
        rejected = true;
    } else {
        const auto n=static_cast<std::int32_t>(host.read_dword(file));
        if (n > 0) {
            // Host-only guard. The native routine uses the signed file count
            // with no safety cap, and delegates actual record parsing.
            if (static_cast<std::size_t>(n)>max_records)
                throw std::length_error("host-only cache record limit exceeded");
            for (std::int32_t i=0;i<n;++i)
                host.process_record(config,file,static_cast<std::uint32_t>(i));
        }
    }
    host.close_file(file);
    if (rejected) {
        host.invalidate_file(config.filename,kPathID);
        state.cache_invalid_or_missing = true;
    }
    return true;
}

// Original 0x221AD620 / 0x22149B00 wrappers:
// if initialized, clear initialized flag, then take either cache-invalid
// reset+clear path or optional reload-gate check+rebuild+clear path. Both
// tail-JMP back into the corresponding initializer. No original parser's
// internal implementation is presumed here.
// Host adds:
//   void reset_invalid_cache(const RepositoryConfig&);
//   void clear_repository_data(const RepositoryConfig&);
//   bool reload_check(const RepositoryConfig&);
//   void rebuild_repository(const RepositoryConfig&);
template<class Host>
bool reload(const RepositoryConfig& config, State& state, Host& host,
            std::size_t max_records = 65536) {
    if (state.initialized) {
        state.initialized = false;
        if (state.cache_invalid_or_missing) {
            host.reset_invalid_cache(config);
            host.clear_repository_data(config);
        } else {
            if (state.reload_gate && !host.reload_check(config))
                host.rebuild_repository(config);
            host.clear_repository_data(config);
        }
    }
    return initialize(config,state,host,max_records);
}
} // namespace sin_compat::original_server_cache_repositories