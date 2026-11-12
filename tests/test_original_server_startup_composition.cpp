#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "sin_compat/original_server_startup_composition.hpp"

namespace post = sin_compat::original_server_post_emitter;
namespace cache = sin_compat::original_server_cache_repositories;
namespace composed = sin_compat::original_server_startup_composition;

struct CacheImage {
    bool present = true;
    std::vector<std::uint32_t> words;
    std::size_t cursor = 0;
};

struct Host {
    CacheImage scene{true, {2u, 3u, 77u, 2u}, 0};
    CacheImage sounds{true, {2u, 2u, 77u, 0u}, 0};
    std::vector<std::string> calls;
    std::uintptr_t failing_system = 0;
    std::uint32_t metadata = 77u;
    static constexpr std::uintptr_t kSceneHandle = 0xA1u;
    static constexpr std::uintptr_t kSoundsHandle = 0xB2u;

    CacheImage& image(std::uintptr_t handle) {
        if (handle == kSceneHandle) return scene;
        assert(handle == kSoundsHandle);
        return sounds;
    }
    static std::string name(std::uintptr_t handle) {
        if (handle == kSceneHandle) return "scene.cache";
        assert(handle == kSoundsHandle);
        return "modelsounds.cache";
    }
    void duplicate_system(post::SystemNode& node) {
        calls.push_back("duplicate:" + std::to_string(node.identity));
    }
    bool init_system(post::SystemNode& node) {
        calls.push_back("system:" + std::to_string(node.identity));
        return node.identity != failing_system;
    }
    void no_filename() { calls.push_back("no-filename"); }
    std::uint32_t compute_metadata(const cache::RepositoryConfig& cfg) {
        calls.push_back("metadata:" + std::string(cfg.filename));
        return metadata;
    }
    std::uintptr_t open_file(std::string_view filename,
                             std::string_view mode, std::string_view path_id) {
        assert(mode == "rb" && path_id == "MOD");
        calls.push_back("open:" + std::string(filename));
        if (filename == "scene.cache") return scene.present ? kSceneHandle : 0;
        assert(filename == "modelsounds.cache");
        return sounds.present ? kSoundsHandle : 0;
    }
    std::uint32_t read_dword(std::uintptr_t handle) {
        CacheImage& file = image(handle);
        calls.push_back("read:" + name(handle));
        assert(file.cursor < file.words.size());
        return file.words[file.cursor++];
    }
    void process_record(const cache::RepositoryConfig& cfg,
                        std::uintptr_t handle, std::uint32_t index) {
        assert(std::string_view(name(handle)) == cfg.filename);
        calls.push_back("record:" + std::string(cfg.filename) + ":" + std::to_string(index));
    }
    void reject(const cache::RepositoryConfig& cfg, cache::Rejection reason) {
        calls.push_back("reject:" + std::string(cfg.filename) + ":" +
                        std::to_string(static_cast<int>(reason)));
    }
    void close_file(std::uintptr_t handle) {
        calls.push_back("close:" + name(handle));
    }
    void invalidate_file(std::string_view filename, std::string_view path_id) {
        assert(path_id == "MOD");
        calls.push_back("invalidate:" + std::string(filename));
    }
    std::size_t count(const std::string& word) const {
        return static_cast<std::size_t>(std::count(calls.begin(), calls.end(), word));
    }
    std::size_t at(const std::string& word) const {
        const auto found = std::find(calls.begin(), calls.end(), word);
        assert(found != calls.end());
        return static_cast<std::size_t>(found - calls.begin());
    }
};

int main() {
    // Ordered original-known gate subset: dedup, systems, scene, model sounds.
    {
        Host h;
        post::SystemNode a{11u, nullptr}, b{22u, nullptr}, duplicate{11u, nullptr};
        a.next = &b;
        b.next = &duplicate;
        post::Registry registry{&a, {}, false};
        cache::State scene, sounds;
        assert(composed::run_required_gates(registry, scene, sounds, h));
        assert(registry.pending == nullptr && registry.init_flag);
        assert(registry.installed.size() == 2u);
        assert(h.at("duplicate:11") < h.at("system:11"));
        assert(h.at("system:11") < h.at("system:22"));
        assert(h.at("system:22") < h.at("metadata:scene.cache"));
        assert(h.at("close:scene.cache") < h.at("metadata:modelsounds.cache"));
        assert(h.count("record:scene.cache:0") == 1u);
        assert(h.count("record:scene.cache:1") == 1u);
        assert(h.count("close:modelsounds.cache") == 1u);
        assert(scene.initialized && sounds.initialized);
        assert(!scene.cache_invalid_or_missing && !sounds.cache_invalid_or_missing);
    }
    // The two missing and corrupt cache paths must remain nonfatal.
    {
        Host h; h.scene.present = false;
        post::Registry registry{}; cache::State scene, sounds;
        assert(composed::run_required_gates(registry, scene, sounds, h));
        assert(scene.cache_invalid_or_missing && !sounds.cache_invalid_or_missing);
        assert(h.count("close:scene.cache") == 0u);
        assert(h.at("open:scene.cache") < h.at("open:modelsounds.cache"));
    }
    {
        Host h;
        h.scene.words[0] = 17u;
        h.sounds.words[1] = 99u;
        post::Registry registry{}; cache::State scene, sounds;
        assert(composed::run_required_gates(registry, scene, sounds, h));
        assert(scene.cache_invalid_or_missing && sounds.cache_invalid_or_missing);
        assert(h.at("close:scene.cache") < h.at("invalidate:scene.cache"));
        assert(h.at("invalidate:scene.cache") < h.at("open:modelsounds.cache"));
        assert(h.at("close:modelsounds.cache") < h.at("invalidate:modelsounds.cache"));
        assert(h.count("record:scene.cache:0") == 0u);
    }
    // A failed system prevents both repositories, leaving native-known
    // cleared pending head and initialized systems flag intact.
    {
        Host h; h.failing_system = 22u;
        post::SystemNode a{11u, nullptr}, b{22u, nullptr};
        a.next = &b;
        post::Registry registry{&a, {}, false}; cache::State scene, sounds;
        assert(!composed::run_required_gates(registry, scene, sounds, h));
        assert(registry.pending == nullptr && registry.init_flag);
        assert(registry.installed.size() == 2u);
        assert(h.count("system:11") == 1u && h.count("system:22") == 1u);
        assert(h.count("metadata:scene.cache") == 0u);
        assert(h.count("metadata:modelsounds.cache") == 0u);
        assert(!scene.initialized && !sounds.initialized);
    }
    // Model re-entry is a no-I/O fast path; not a native double-DLLInit claim.
    {
        Host h;
        post::Registry registry{}; cache::State scene, sounds;
        assert(composed::run_required_gates(registry, scene, sounds, h));
        assert(composed::run_required_gates(registry, scene, sounds, h));
        assert(h.count("open:scene.cache") == 1u);
        assert(h.count("open:modelsounds.cache") == 1u);
    }
    return 0;
}
