#pragma once

// Independently authored Source-style target resolution reference.
// The directory adapter owns engine entity iteration and engine-specific queries.
// This is not an original SiN ABI or a SourceVR engine adapter.

#include "sin_compat/weapon_entity_io.hpp"

#include <string>
#include <vector>

namespace sin_compat {

struct SourceTargetResolveContext {
    EntityRef searching_entity{};
    EntityRef activator{};
    EntityRef caller{};
    EntityRef direct_entity{};
};

struct SourceTargetResolution {
    std::vector<EntityRef> targets;
    bool matched_targetname = false;
    bool matched_procedural = false;
    bool matched_direct_entity = false;
    bool used_classname_fallback = false;
};

class IEntityTargetDirectory {
public:
    virtual ~IEntityTargetDirectory() = default;

    // Must preserve the engine's entity-list iteration order. Do not sort by
    // entity index: duplicate-name delivery follows the host's list order.
    virtual std::vector<EntityRef> entities_in_engine_order() const = 0;
    virtual bool is_live(EntityRef entity) const = 0;

    // Return false for a NULL_STRING/no targetname. An empty but present
    // string remains distinguishable from a missing targetname.
    virtual bool get_target_name(EntityRef entity, std::string& value) const = 0;
    virtual bool get_classname(EntityRef entity, std::string& value) const = 0;

    // Mirrors UTIL_PlayerByIndex(1), not "first player in the entity list".
    virtual EntityRef player_by_index_one() const = 0;
    virtual EntityRef find_client_in_pvs(EntityRef origin) const = 0;
    virtual EntityRef find_picker_entity(EntityRef player) const = 0;
};

class SourceEntityTargetResolver final {
public:
    explicit SourceEntityTargetResolver(const IEntityTargetDirectory& directory)
        : directory_(directory) {}

    // Mirrors the public Source SDK 2013 CEventQueue target order:
    // targetname/procedural matches, then a direct entity, then classname
    // fallback only if neither earlier stage found a target.
    SourceTargetResolution resolve(
        const std::string& target,
        const SourceTargetResolveContext& context = {}) const;

    // Source SDK 2013 NamesMatch behavior for non-null name/class strings.
    // ASCII case-insensitive exact match, or a prefix match when the query
    // reaches '*'. The pinned implementation returns at the first '*', even
    // if additional query characters follow it.
    static bool names_match(const std::string& query,
                            const std::string& candidate);

private:
    EntityRef resolve_procedural(
        const std::string& target,
        const SourceTargetResolveContext& context) const;

    const IEntityTargetDirectory& directory_;
};

} // namespace sin_compat
