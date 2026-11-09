#include "sin_compat/source_map_runtime.hpp"

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <utility>

namespace sin_compat::map {
namespace {

bool valid_entity_ref(EntityRef entity) noexcept {
    return entity.index != std::numeric_limits<std::uint32_t>::max() &&
           entity.generation != std::numeric_limits<std::uint32_t>::max();
}

class BusyScope final {
public:
    explicit BusyScope(bool& busy) noexcept : busy_(busy) { busy_ = true; }
    ~BusyScope() { busy_ = false; }

    BusyScope(const BusyScope&) = delete;
    BusyScope& operator=(const BusyScope&) = delete;

private:
    bool& busy_;
};

} // namespace

bool SourceMapRuntime::safe_is_live(EntityRef entity) const noexcept {
    if (!valid_entity_ref(entity)) return false;
    try {
        return host_.is_live(entity);
    } catch (...) {
        return false;
    }
}

bool SourceMapRuntime::safe_remove(EntityRef entity) noexcept {
    if (!safe_is_live(entity)) return true;
    try {
        if (!host_.remove_entity(entity)) return !safe_is_live(entity);
        return !safe_is_live(entity);
    } catch (...) {
        return !safe_is_live(entity);
    }
}

bool SourceMapRuntime::rollback(std::vector<EntityRef>& created,
                                std::size_t& removed) noexcept {
    entities_.clear();
    bool complete = true;
    for (auto it = created.rbegin(); it != created.rend(); ++it) {
        if (!safe_is_live(*it)) continue;
        if (safe_remove(*it)) {
            ++removed;
        } else {
            entities_.push_back(*it);
            complete = false;
        }
    }
    std::reverse(entities_.begin(), entities_.end());
    loaded_ = !entities_.empty();
    return complete;
}

SourceMapLoadResult SourceMapRuntime::load_bsp(
    const std::uint8_t* bytes, std::size_t byte_count) {
    SourceMapLoadResult result;
    SourceBspEntityDocument document;
    SourceBspEntityParseResult parsed;
    try {
        parsed = parse_source_bsp_entities(bytes, byte_count, document);
    } catch (...) {
        result.code = SourceMapLoadCode::AllocationFailed;
        return result;
    }
    if (!parsed.ok()) {
        result.code = SourceMapLoadCode::ParseFailed;
        result.parse_status = parsed.status;
        result.error_offset = parsed.error_offset;
        return result;
    }
    result = load_document(document);
    result.parse_status = parsed.status;
    result.error_offset = parsed.error_offset;
    return result;
}

SourceMapLoadResult SourceMapRuntime::load_document(
    const SourceBspEntityDocument& document) {
    SourceMapLoadResult result;
    if (busy_) {
        result.code = SourceMapLoadCode::Reentrant;
        return result;
    }
    if (loaded_ || !entities_.empty()) {
        result.code = SourceMapLoadCode::AlreadyLoaded;
        return result;
    }
    BusyScope busy(busy_);

    if (document.entities.empty()) {
        result.code = SourceMapLoadCode::EmptyDocument;
        return result;
    }
    if (document.entities.size() > max_entities_) {
        result.code = SourceMapLoadCode::TooManyEntities;
        return result;
    }

    std::vector<std::string_view> classnames;
    std::vector<EntityRef> created;
    std::unordered_set<std::uint32_t> entity_indices;
    try {
        classnames.reserve(document.entities.size());
        created.reserve(document.entities.size());
        entity_indices.reserve(document.entities.size());
        entities_.reserve(document.entities.size());
    } catch (...) {
        result.code = SourceMapLoadCode::AllocationFailed;
        return result;
    }

    for (std::size_t i = 0; i < document.entities.size(); ++i) {
        const auto& entity = document.entities[i];
        const std::string* classname = nullptr;
        std::size_t classname_count = 0;
        for (const auto& property : entity.properties) {
            if (property.key == "classname") {
                ++classname_count;
                if (classname == nullptr) classname = &property.value;
            }
        }
        if (classname_count == 0U || classname == nullptr || classname->empty()) {
            result.code = SourceMapLoadCode::MissingClassname;
            result.entity_index = i;
            return result;
        }
        if (classname_count != 1U) {
            result.code = SourceMapLoadCode::DuplicateClassname;
            result.entity_index = i;
            return result;
        }
        classnames.emplace_back(*classname);
    }

    const auto fail = [&](SourceMapLoadCode code, std::size_t entity_index,
                          std::size_t property_index) {
        result.code = code;
        result.entity_index = entity_index;
        result.property_index = property_index;
        if (!rollback(created, result.entities_removed)) {
            result.code = SourceMapLoadCode::RollbackFailed;
        }
        return result;
    };

    // Ordinary Source map entities are constructed and keyvalued in map
    // order, then passed through the spawn and activation phases. The host
    // still owns special-case entity ordering and all engine ABI details.
    for (std::size_t i = 0; i < document.entities.size(); ++i) {
        EntityRef entity{};
        bool created_ok = false;
        try {
            created_ok = host_.create_entity(classnames[i], entity);
        } catch (...) {
            created_ok = false;
        }

        const bool valid = valid_entity_ref(entity);
        const bool live = valid && safe_is_live(entity);
        bool duplicate_index = false;
        if (valid && live) {
            duplicate_index = !entity_indices.insert(entity.index).second;
            if (std::find(created.begin(), created.end(), entity) == created.end()) {
                created.push_back(entity);
            }
        }
        result.entities_created = created.size();

        if (!created_ok) {
            return fail(SourceMapLoadCode::HostCreateFailed, i,
                        SourceMapLoadResult::no_index);
        }
        if (!valid || !live) {
            return fail(SourceMapLoadCode::InvalidHostEntity, i,
                        SourceMapLoadResult::no_index);
        }
        if (duplicate_index) {
            return fail(SourceMapLoadCode::DuplicateEntityIndex, i,
                        SourceMapLoadResult::no_index);
        }

        const auto& source = document.entities[i];
        for (std::size_t p = 0; p < source.properties.size(); ++p) {
            const auto& property = source.properties[p];
            if (property.key == "classname") continue;
            if (!safe_is_live(entity)) {
                return fail(SourceMapLoadCode::InvalidHostEntity, i, p);
            }
            bool accepted = false;
            try {
                accepted = host_.key_value(entity, property.key, property.value);
            } catch (...) {
                accepted = false;
            }
            if (!accepted) {
                return fail(SourceMapLoadCode::HostKeyValueFailed, i, p);
            }
            ++result.key_values_applied;
        }
    }

    for (std::size_t i = 0; i < created.size(); ++i) {
        if (!safe_is_live(created[i])) {
            return fail(SourceMapLoadCode::InvalidHostEntity, i,
                        SourceMapLoadResult::no_index);
        }
        bool spawned = false;
        try {
            spawned = host_.dispatch_spawn(created[i]);
        } catch (...) {
            spawned = false;
        }
        if (!spawned) {
            return fail(SourceMapLoadCode::HostSpawnFailed, i,
                        SourceMapLoadResult::no_index);
        }
        if (!safe_is_live(created[i])) {
            return fail(SourceMapLoadCode::InvalidHostEntity, i,
                        SourceMapLoadResult::no_index);
        }
        ++result.entities_spawned;
    }

    for (std::size_t i = 0; i < created.size(); ++i) {
        if (!safe_is_live(created[i])) {
            return fail(SourceMapLoadCode::InvalidHostEntity, i,
                        SourceMapLoadResult::no_index);
        }
        bool activated = false;
        try {
            activated = host_.activate(created[i]);
        } catch (...) {
            activated = false;
        }
        if (!activated) {
            return fail(SourceMapLoadCode::HostActivateFailed, i,
                        SourceMapLoadResult::no_index);
        }
        if (!safe_is_live(created[i])) {
            return fail(SourceMapLoadCode::InvalidHostEntity, i,
                        SourceMapLoadResult::no_index);
        }
        ++result.entities_activated;
    }

    entities_.swap(created);
    loaded_ = true;
    result.code = SourceMapLoadCode::Loaded;
    result.entities_created = entities_.size();
    return result;
}

SourceMapLoadResult SourceMapRuntime::unload() {
    SourceMapLoadResult result;
    if (busy_) {
        result.code = SourceMapLoadCode::Reentrant;
        return result;
    }
    if (entities_.empty()) {
        result.code = SourceMapLoadCode::NotLoaded;
        return result;
    }
    BusyScope busy(busy_);

    std::vector<EntityRef> remaining;
    try {
        remaining.reserve(entities_.size());
    } catch (...) {
        result.code = SourceMapLoadCode::AllocationFailed;
        return result;
    }

    for (auto it = entities_.rbegin(); it != entities_.rend(); ++it) {
        if (!safe_is_live(*it)) {
            ++result.entities_removed;
            continue;
        }
        if (safe_remove(*it)) {
            ++result.entities_removed;
        } else {
            remaining.push_back(*it);
        }
    }
    std::reverse(remaining.begin(), remaining.end());
    entities_.swap(remaining);
    loaded_ = !entities_.empty();
    result.code = loaded_ ? SourceMapLoadCode::PartialUnload
                          : SourceMapLoadCode::Unloaded;
    result.entities_created = entities_.size();
    return result;
}

} // namespace sin_compat::map
