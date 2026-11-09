#pragma once

#include "sin_compat/source_bsp_entities.hpp"
#include "sin_compat/weapon_entity_io.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace sin_compat::map {

enum class SourceMapLoadCode : std::uint8_t {
    Loaded,
    Unloaded,
    PartialUnload,
    NotLoaded,
    AlreadyLoaded,
    Reentrant,
    EmptyDocument,
    TooManyEntities,
    MissingClassname,
    DuplicateClassname,
    HostCreateFailed,
    InvalidHostEntity,
    DuplicateEntityIndex,
    HostKeyValueFailed,
    HostSpawnFailed,
    HostActivateFailed,
    ParseFailed,
    AllocationFailed,
    RollbackFailed
};

struct SourceMapLoadResult {
    static constexpr std::size_t no_index = static_cast<std::size_t>(-1);

    SourceMapLoadCode code = SourceMapLoadCode::EmptyDocument;
    SourceBspEntityParseStatus parse_status = SourceBspEntityParseStatus::Ok;
    std::size_t error_offset = 0;
    std::size_t entity_index = no_index;
    std::size_t property_index = no_index;
    std::size_t entities_created = 0;
    std::size_t key_values_applied = 0;
    std::size_t entities_spawned = 0;
    std::size_t entities_activated = 0;
    std::size_t entities_removed = 0;

    bool ok() const noexcept { return code == SourceMapLoadCode::Loaded; }
};

// The native engine adapter owns every entity and every ABI-sensitive call.
// create_entity must use the host's real entity factory for the classname;
// key_value must preserve supplied order and accept unknown-but-legal map
// keys according to that engine's normal KeyValue contract. remove_entity
// must invalidate the generation-checked handle before returning true.
class ISourceMapEntityHost {
public:
    virtual ~ISourceMapEntityHost() = default;

    virtual bool is_live(EntityRef entity) const = 0;
    virtual bool create_entity(std::string_view classname, EntityRef& entity) = 0;
    virtual bool key_value(EntityRef entity, std::string_view key,
                           std::string_view value) = 0;
    virtual bool dispatch_spawn(EntityRef entity) = 0;
    virtual bool activate(EntityRef entity) = 0;
    virtual bool remove_entity(EntityRef entity) = 0;
};

// Instantiates the ordinary entity subset of a parsed Source BSP entity lump.
// It deliberately does not emulate Source's special worldspawn, node/light,
// point_template, parent-priority, or precache paths. Those remain explicit
// responsibilities of the selected native host.
class SourceMapRuntime final {
public:
    explicit SourceMapRuntime(ISourceMapEntityHost& host,
                              std::size_t max_entities = 65536U)
        : host_(host), max_entities_(max_entities) {}

    SourceMapRuntime(const SourceMapRuntime&) = delete;
    SourceMapRuntime& operator=(const SourceMapRuntime&) = delete;
    SourceMapRuntime(SourceMapRuntime&&) = delete;
    SourceMapRuntime& operator=(SourceMapRuntime&&) = delete;

    SourceMapLoadResult load_bsp(const std::uint8_t* bytes,
                                 std::size_t byte_count);
    SourceMapLoadResult load_document(const SourceBspEntityDocument& document);
    SourceMapLoadResult unload();

    bool loaded() const noexcept { return loaded_; }
    const std::vector<EntityRef>& entities() const noexcept { return entities_; }

private:
    bool safe_is_live(EntityRef entity) const noexcept;
    bool safe_remove(EntityRef entity) noexcept;
    bool rollback(std::vector<EntityRef>& created,
                  std::size_t& removed) noexcept;

    ISourceMapEntityHost& host_;
    std::size_t max_entities_;
    std::vector<EntityRef> entities_;
    bool loaded_ = false;
    bool busy_ = false;
};

} // namespace sin_compat::map
