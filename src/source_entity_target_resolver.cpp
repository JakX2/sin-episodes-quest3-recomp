#include "sin_compat/source_entity_target_resolver.hpp"

namespace sin_compat {
namespace {

unsigned char ascii_fold(unsigned char value) {
    if (value >= static_cast<unsigned char>('A') &&
        value <= static_cast<unsigned char>('Z')) {
        return static_cast<unsigned char>(
            value + static_cast<unsigned char>('a' - 'A'));
    }
    return value;
}

} // namespace

bool SourceEntityTargetResolver::names_match(
    const std::string& query, const std::string& candidate) {
    std::size_t position = 0;
    while (position < candidate.size() && position < query.size()) {
        const auto candidate_char =
            static_cast<unsigned char>(candidate[position]);
        const auto query_char = static_cast<unsigned char>(query[position]);
        if (ascii_fold(candidate_char) != ascii_fold(query_char)) break;
        ++position;
    }

    if (position == query.size() && position == candidate.size()) return true;

    // The pinned Source implementation checks the query's current character
    // after the comparison loop. Its actual behavior is a prefix wildcard at
    // the first '*', even though the adjacent comment describes trailing '*'.
    return position < query.size() && query[position] == '*';
}

EntityRef SourceEntityTargetResolver::resolve_procedural(
    const std::string& target,
    const SourceTargetResolveContext& context) const {
    // FindEntityProcedural uses exact FStrEq comparisons for these tokens.
    if (target == "!activator") {
        return directory_.is_live(context.activator) ? context.activator
                                                     : EntityRef{};
    }
    if (target == "!caller") {
        return directory_.is_live(context.caller) ? context.caller
                                                  : EntityRef{};
    }
    if (target == "!self") {
        return directory_.is_live(context.searching_entity)
            ? context.searching_entity : EntityRef{};
    }
    if (target == "!player") {
        const EntityRef player = directory_.player_by_index_one();
        return directory_.is_live(player) ? player : EntityRef{};
    }
    if (target == "!pvsplayer") {
        EntityRef origin{};
        if (directory_.is_live(context.searching_entity)) {
            origin = context.searching_entity;
        } else if (directory_.is_live(context.activator)) {
            origin = context.activator;
        } else {
            origin = directory_.player_by_index_one();
        }
        if (!directory_.is_live(origin)) return {};
        const EntityRef player = directory_.find_client_in_pvs(origin);
        return directory_.is_live(player) ? player : EntityRef{};
    }
    if (target == "!picker") {
        const EntityRef player = directory_.player_by_index_one();
        if (!directory_.is_live(player)) return {};
        const EntityRef picked = directory_.find_picker_entity(player);
        return directory_.is_live(picked) ? picked : EntityRef{};
    }
    return {};
}

SourceTargetResolution SourceEntityTargetResolver::resolve(
    const std::string& target,
    const SourceTargetResolveContext& context) const {
    SourceTargetResolution result;

    if (!target.empty() && target.front() == '!') {
        const EntityRef procedural = resolve_procedural(target, context);
        if (directory_.is_live(procedural)) {
            result.targets.push_back(procedural);
            result.matched_procedural = true;
        }
    } else if (!target.empty()) {
        for (const EntityRef entity : directory_.entities_in_engine_order()) {
            if (!directory_.is_live(entity)) continue;
            std::string name;
            if (!directory_.get_target_name(entity, name)) continue;
            if (!names_match(target, name)) continue;
            result.targets.push_back(entity);
            result.matched_targetname = true;
        }
    }

    // CEventQueue::ServiceEvents checks a direct entity after the name search.
    // Preserve this order even when the direct reference duplicates a name hit.
    if (directory_.is_live(context.direct_entity)) {
        result.targets.push_back(context.direct_entity);
        result.matched_direct_entity = true;
    }

    if (!result.targets.empty() || target.empty()) return result;

    // Source's event service falls back to classname only when targetname /
    // procedural lookup and the direct-entity path found nothing.
    result.used_classname_fallback = true;
    for (const EntityRef entity : directory_.entities_in_engine_order()) {
        if (!directory_.is_live(entity)) continue;
        std::string classname;
        if (!directory_.get_classname(entity, classname)) continue;
        if (!names_match(target, classname)) continue;
        result.targets.push_back(entity);
    }

    return result;
}

} // namespace sin_compat
