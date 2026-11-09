#include "sin_compat/source_map_runtime.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace sin_compat;
using namespace sin_compat::map;

namespace {

SourceBspEntityDocument make_document() {
    SourceBspEntityDocument document;
    document.bsp_version = 19U;

    SourceMapEntity world;
    world.properties = {
        {"classname", "worldspawn"},
        {"message", "SiN map test"}
    };
    document.entities.push_back(std::move(world));

    SourceMapEntity relay;
    relay.properties = {
        {"classname", "logic_relay"},
        {"targetname", "relay"},
        {"targetname", "relay_duplicate"},
        {"OnTrigger", "door,Open,,0,-1"}
    };
    document.entities.push_back(std::move(relay));
    return document;
}

void put_u32(std::vector<std::uint8_t>& bytes, std::size_t offset,
             std::uint32_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::vector<std::uint8_t> make_bsp(const std::string& entity_text) {
    constexpr std::size_t header_size = 1036U;
    std::vector<std::uint8_t> bytes(header_size + entity_text.size(), 0U);
    bytes[0] = 'V';
    bytes[1] = 'B';
    bytes[2] = 'S';
    bytes[3] = 'P';
    put_u32(bytes, 4U, 19U);
    put_u32(bytes, 8U, static_cast<std::uint32_t>(header_size));
    put_u32(bytes, 12U, static_cast<std::uint32_t>(entity_text.size()));
    put_u32(bytes, 16U, 0U);
    put_u32(bytes, 20U, 0U);
    if (!entity_text.empty()) {
        std::memcpy(bytes.data() + header_size, entity_text.data(),
                    entity_text.size());
    }
    return bytes;
}

struct FakeHost final : ISourceMapEntityHost {
    std::map<std::uint32_t, EntityRef> live;
    std::vector<std::string> events;
    int create_calls = 0;
    int key_value_calls = 0;
    int spawn_calls = 0;
    int activate_calls = 0;
    int fail_create_at = -1;
    int fail_key_value_at = -1;
    int fail_spawn_at = -1;
    int fail_activate_at = -1;
    int fail_remove_index = -1;
    bool duplicate_second_index = false;
    bool reenter_on_key_value = false;
    SourceMapRuntime* reenter_runtime = nullptr;
    SourceMapLoadCode reentry_code = SourceMapLoadCode::Loaded;

    bool is_live(EntityRef entity) const override {
        const auto it = live.find(entity.index);
        return it != live.end() && it->second == entity;
    }

    bool create_entity(std::string_view classname, EntityRef& entity) override {
        ++create_calls;
        if (create_calls == fail_create_at) return false;
        if (duplicate_second_index && create_calls == 2) {
            entity = EntityRef{1U, 2U};
        } else {
            entity = EntityRef{static_cast<std::uint32_t>(create_calls), 1U};
        }
        live[entity.index] = entity;
        events.emplace_back("create:" + std::string(classname));
        return true;
    }

    bool key_value(EntityRef entity, std::string_view key,
                   std::string_view value) override {
        ++key_value_calls;
        events.emplace_back("kv:" + std::to_string(entity.index) + ":" +
                            std::string(key) + ":" + std::string(value));
        if (reenter_on_key_value && reenter_runtime != nullptr) {
            reentry_code = reenter_runtime->unload().code;
            reenter_on_key_value = false;
        }
        return key_value_calls != fail_key_value_at;
    }

    bool dispatch_spawn(EntityRef entity) override {
        ++spawn_calls;
        events.emplace_back("spawn:" + std::to_string(entity.index));
        return spawn_calls != fail_spawn_at;
    }

    bool activate(EntityRef entity) override {
        ++activate_calls;
        events.emplace_back("activate:" + std::to_string(entity.index));
        return activate_calls != fail_activate_at;
    }

    bool remove_entity(EntityRef entity) override {
        events.emplace_back("remove:" + std::to_string(entity.index));
        if (static_cast<int>(entity.index) == fail_remove_index) return false;
        const auto it = live.find(entity.index);
        if (it == live.end() || it->second != entity) return false;
        live.erase(it);
        return true;
    }
};

void test_load_order_and_duplicate_keyvalue_preservation() {
    FakeHost host;
    SourceMapRuntime runtime(host);
    const auto result = runtime.load_document(make_document());
    assert(result.ok());
    assert(result.entities_created == 2U);
    assert(result.key_values_applied == 4U);
    assert(result.entities_spawned == 2U);
    assert(result.entities_activated == 2U);
    assert(runtime.loaded() && runtime.entities().size() == 2U);

    assert(host.events[0] == "create:worldspawn");
    assert(host.events[1] == "kv:1:message:SiN map test");
    assert(host.events[2] == "create:logic_relay");
    assert(host.events[3] == "kv:2:targetname:relay");
    assert(host.events[4] == "kv:2:targetname:relay_duplicate");
    assert(host.events[5] == "kv:2:OnTrigger:door,Open,,0,-1");
    assert(host.events[6] == "spawn:1");
    assert(host.events[7] == "spawn:2");
    assert(host.events[8] == "activate:1");
    assert(host.events[9] == "activate:2");

    const auto second_load = runtime.load_document(make_document());
    assert(second_load.code == SourceMapLoadCode::AlreadyLoaded);
}

void test_bsp_entrypoint_and_parse_failure() {
    FakeHost host;
    SourceMapRuntime runtime(host);
    const auto bytes = make_bsp(
        "{ \"classname\" \"worldspawn\" }\n"
        "{ \"classname\" \"info_player_start\" }\n");
    const auto loaded = runtime.load_bsp(bytes.data(), bytes.size());
    assert(loaded.ok() && loaded.parse_status == SourceBspEntityParseStatus::Ok);
    assert(loaded.entities_created == 2U);
    assert(runtime.unload().code == SourceMapLoadCode::Unloaded);
    assert(host.live.empty());

    std::vector<std::uint8_t> bad = bytes;
    bad[0] = 'X';
    const auto rejected = runtime.load_bsp(bad.data(), bad.size());
    assert(rejected.code == SourceMapLoadCode::ParseFailed);
    assert(rejected.parse_status == SourceBspEntityParseStatus::BadMagic);
    assert(host.create_calls == 2);
}

void test_document_validation_precedes_host_side_effects() {
    FakeHost host;
    SourceMapRuntime runtime(host);

    SourceBspEntityDocument missing;
    missing.entities.push_back(SourceMapEntity{});
    assert(runtime.load_document(missing).code ==
           SourceMapLoadCode::MissingClassname);
    assert(host.create_calls == 0);

    SourceBspEntityDocument duplicate;
    SourceMapEntity entity;
    entity.properties = {{"classname", "worldspawn"},
                         {"classname", "logic_relay"}};
    duplicate.entities.push_back(std::move(entity));
    assert(runtime.load_document(duplicate).code ==
           SourceMapLoadCode::DuplicateClassname);
    assert(host.create_calls == 0);
}

void test_host_failure_rolls_back_created_entities() {
    const auto run_failure = [](int create_failure, int key_failure,
                                int spawn_failure, int activate_failure,
                                SourceMapLoadCode expected) {
        FakeHost host;
        host.fail_create_at = create_failure;
        host.fail_key_value_at = key_failure;
        host.fail_spawn_at = spawn_failure;
        host.fail_activate_at = activate_failure;
        SourceMapRuntime runtime(host);
        const auto result = runtime.load_document(make_document());
        assert(result.code == expected);
        assert(host.live.empty());
        assert(!runtime.loaded());
        assert(result.entities_removed == result.entities_created);
    };

    run_failure(2, -1, -1, -1, SourceMapLoadCode::HostCreateFailed);
    run_failure(-1, 2, -1, -1, SourceMapLoadCode::HostKeyValueFailed);
    run_failure(-1, -1, 2, -1, SourceMapLoadCode::HostSpawnFailed);
    run_failure(-1, -1, -1, 2, SourceMapLoadCode::HostActivateFailed);
}

void test_duplicate_entity_indices_are_rejected_and_cleaned() {
    FakeHost host;
    host.duplicate_second_index = true;
    SourceMapRuntime runtime(host);
    const auto result = runtime.load_document(make_document());
    assert(result.code == SourceMapLoadCode::DuplicateEntityIndex);
    assert(host.live.empty());
    assert(!runtime.loaded());
}

void test_failed_rollback_keeps_retryable_handles() {
    FakeHost host;
    host.fail_spawn_at = 2;
    host.fail_remove_index = 1;
    SourceMapRuntime runtime(host);
    const auto failed = runtime.load_document(make_document());
    assert(failed.code == SourceMapLoadCode::RollbackFailed);
    assert(runtime.loaded() && runtime.entities().size() == 1U);
    assert(host.live.size() == 1U);

    host.fail_remove_index = -1;
    const auto unloaded = runtime.unload();
    assert(unloaded.code == SourceMapLoadCode::Unloaded);
    assert(host.live.empty() && !runtime.loaded());
}

void test_reentrancy_guard_and_reverse_unload() {
    FakeHost host;
    SourceMapRuntime runtime(host);
    host.reenter_runtime = &runtime;
    host.reenter_on_key_value = true;
    const auto loaded = runtime.load_document(make_document());
    assert(loaded.ok());
    assert(host.reentry_code == SourceMapLoadCode::Reentrant);

    const auto unloaded = runtime.unload();
    assert(unloaded.code == SourceMapLoadCode::Unloaded);
    assert(host.events[host.events.size() - 2U] == "remove:2");
    assert(host.events.back() == "remove:1");
}

void test_entity_limit_is_checked_before_creation() {
    FakeHost host;
    SourceMapRuntime runtime(host, 1U);
    const auto result = runtime.load_document(make_document());
    assert(result.code == SourceMapLoadCode::TooManyEntities);
    assert(host.create_calls == 0);
}

} // namespace

int main() {
    test_load_order_and_duplicate_keyvalue_preservation();
    test_bsp_entrypoint_and_parse_failure();
    test_document_validation_precedes_host_side_effects();
    test_host_failure_rolls_back_created_entities();
    test_duplicate_entity_indices_are_rejected_and_cleaned();
    test_failed_rollback_keeps_retryable_handles();
    test_reentrancy_guard_and_reverse_unload();
    test_entity_limit_is_checked_before_creation();
    std::puts("PASS: Source BSP entity text reaches generation-safe host create/keyvalue/spawn/activate lifecycle (mock host only)");
    return 0;
}
