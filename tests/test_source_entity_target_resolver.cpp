#include "sin_compat/entity_io_runtime.hpp"
#include "sin_compat/source_entity_target_resolver.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace sin_compat;

class Directory : public IEntityTargetDirectory {
public:
    struct Row {
        EntityRef ref;
        std::optional<std::string> name;
        std::string classname;
    };

    EntityRef add(std::uint32_t index, std::uint32_t generation,
                  std::optional<std::string> name, std::string classname) {
        const EntityRef ref{index, generation};
        generations[index] = generation;
        rows.push_back(Row{ref, std::move(name), std::move(classname)});
        return ref;
    }

    void recycle(EntityRef old_ref, std::uint32_t new_generation) {
        generations[old_ref.index] = new_generation;
    }

    std::vector<EntityRef> entities_in_engine_order() const override {
        std::vector<EntityRef> result;
        result.reserve(rows.size());
        for (const auto& row : rows) result.push_back(row.ref);
        return result;
    }

    bool is_live(EntityRef entity) const override {
        const auto it = generations.find(entity.index);
        return it != generations.end() && it->second == entity.generation;
    }

    bool get_target_name(EntityRef entity, std::string& value) const override {
        const Row* row = find(entity);
        if (!row || !row->name.has_value()) return false;
        value = *row->name;
        return true;
    }

    bool get_classname(EntityRef entity, std::string& value) const override {
        const Row* row = find(entity);
        if (!row) return false;
        value = row->classname;
        return true;
    }

    EntityRef player_by_index_one() const override {
        return player_one;
    }

    EntityRef find_client_in_pvs(EntityRef origin) const override {
        last_pvs_origin = origin;
        return pvs_player;
    }

    EntityRef find_picker_entity(EntityRef player) const override {
        last_picker_player = player;
        return picked_entity;
    }

    EntityRef player_one{};
    EntityRef pvs_player{};
    EntityRef picked_entity{};
    mutable EntityRef last_pvs_origin{};
    mutable EntityRef last_picker_player{};

protected:
    const Row* find(EntityRef entity) const {
        if (!is_live(entity)) return nullptr;
        for (const auto& row : rows) {
            if (row.ref == entity) return &row;
        }
        return nullptr;
    }

    std::vector<Row> rows;
    std::map<std::uint32_t, std::uint32_t> generations;
};

class OutputWorld final : public Directory, public IEntityOutputHost {
public:
    OutputWorld() : resolver(*this) {}

    bool is_live(EntityRef entity) const override {
        return Directory::is_live(entity);
    }

    std::vector<EntityRef> resolve_targets(
        const std::string& target, EntityRef activator,
        EntityRef caller) override {
        const SourceTargetResolveContext context{caller, activator, caller, {}};
        return resolver.resolve(target, context).targets;
    }

    void accept_input(EntityRef target, const std::string& input,
                      const std::string& parameter, EntityRef activator,
                      EntityRef caller) override {
        delivered.push_back(Delivery{target, input, parameter, activator, caller});
    }

    SourceEntityTargetResolver resolver;
    struct Delivery {
        EntityRef target;
        std::string input;
        std::string parameter;
        EntityRef activator;
        EntityRef caller;
    };
    std::vector<Delivery> delivered;
};

static EntityOutputConnection link(std::string target, std::string input) {
    return {std::move(target), std::move(input), {}, 0.0, -1};
}

int main() {
    Directory directory;
    const EntityRef named_first =
        directory.add(7, 1, std::string("Door"), "prop_dynamic");
    const EntityRef named_second =
        directory.add(3, 1, std::string("door"), "npc");
    const EntityRef class_only_door =
        directory.add(4, 1, std::nullopt, "door");
    const EntityRef prefix_name =
        directory.add(9, 1, std::string("door_extra"), "prop_dynamic");
    const EntityRef func_door_a =
        directory.add(12, 1, std::nullopt, "func_door");
    const EntityRef func_door_b =
        directory.add(5, 1, std::nullopt, "func_door");
    const EntityRef npc_class_a =
        directory.add(16, 1, std::nullopt, "npc_sintek_grunt");
    const EntityRef npc_class_b =
        directory.add(2, 1, std::nullopt, "npc_sintek_jetpack");

    SourceEntityTargetResolver resolver(directory);

    // Duplicate targetnames are all returned in host entity-list order, not
    // sorted by entity index. Matching is ASCII case-insensitive.
    auto exact = resolver.resolve("DOOR");
    assert((exact.targets == std::vector<EntityRef>{named_first, named_second}));
    assert(exact.matched_targetname);
    assert(!exact.used_classname_fallback);

    // The pinned NamesMatch implementation short-circuits at the first '*'.
    // Any query suffix after that wildcard is ignored by the actual code.
    auto wildcard = resolver.resolve("door*ignored");
    assert((wildcard.targets ==
            std::vector<EntityRef>{named_first, named_second, prefix_name}));
    assert(wildcard.matched_targetname);
    assert(!wildcard.used_classname_fallback);

    auto all_named = resolver.resolve("*");
    assert((all_named.targets == std::vector<EntityRef>{
        named_first, named_second, prefix_name}));

    // Classname fallback is used only when the targetname search found none.
    auto class_fallback = resolver.resolve("func_door");
    assert((class_fallback.targets ==
            std::vector<EntityRef>{func_door_a, func_door_b}));
    assert(class_fallback.used_classname_fallback);
    assert(!class_fallback.matched_targetname);

    auto class_wildcard = resolver.resolve("npc_*");
    assert((class_wildcard.targets ==
            std::vector<EntityRef>{npc_class_a, npc_class_b}));
    assert(class_wildcard.used_classname_fallback);

    // A targetname hit suppresses classname fallback even if other entities
    // have a matching classname. A direct target is delivered after name hits.
    SourceTargetResolveContext direct_context;
    direct_context.direct_entity = class_only_door;
    auto name_and_direct = resolver.resolve("door", direct_context);
    assert((name_and_direct.targets ==
            std::vector<EntityRef>{named_first, named_second, class_only_door}));
    assert(name_and_direct.matched_direct_entity);
    assert(!name_and_direct.used_classname_fallback);

    // A direct entity alone also suppresses classname fallback.
    direct_context = {};
    direct_context.direct_entity = prefix_name;
    auto direct_only = resolver.resolve("func_door", direct_context);
    assert((direct_only.targets == std::vector<EntityRef>{prefix_name}));
    assert(direct_only.matched_direct_entity);
    assert(!direct_only.used_classname_fallback);

    // A stale direct entity is ignored, allowing the classname fallback.
    const EntityRef stale_direct_ref =
        directory.add(25, 1, std::string("stale_direct"), "prop_dynamic");
    directory.recycle(stale_direct_ref, 2);
    direct_context.direct_entity = stale_direct_ref;
    auto stale_direct = resolver.resolve("func_door", direct_context);
    assert((stale_direct.targets ==
            std::vector<EntityRef>{func_door_a, func_door_b}));
    assert(!stale_direct.matched_direct_entity);
    assert(stale_direct.used_classname_fallback);

    // Stale entity-list entries are skipped without changing the relative
    // order of the remaining duplicate targetname matches.
    directory.recycle(named_second, 2);
    auto after_recycle = resolver.resolve("door");
    assert((after_recycle.targets == std::vector<EntityRef>{named_first}));
    assert(!after_recycle.used_classname_fallback);

    // Procedural targets use the original Source SDK 2013 token spellings.
    const EntityRef player_one =
        directory.add(1, 1, std::string("player_one"), "player");
    const EntityRef searcher =
        directory.add(18, 1, std::string("searcher"), "logic_relay");
    const EntityRef activator =
        directory.add(19, 1, std::string("activator"), "player");
    const EntityRef caller =
        directory.add(20, 1, std::string("caller"), "logic_relay");
    const EntityRef pvs_player =
        directory.add(21, 1, std::string("pvs"), "player");
    const EntityRef picker =
        directory.add(22, 1, std::string("picked"), "prop_physics");
    directory.player_one = player_one;
    directory.pvs_player = pvs_player;
    directory.picked_entity = picker;

    SourceTargetResolveContext context{searcher, activator, caller, {}};
    assert((resolver.resolve("!activator", context).targets ==
            std::vector<EntityRef>{activator}));
    assert((resolver.resolve("!caller", context).targets ==
            std::vector<EntityRef>{caller}));
    assert((resolver.resolve("!self", context).targets ==
            std::vector<EntityRef>{searcher}));
    assert((resolver.resolve("!player", context).targets ==
            std::vector<EntityRef>{player_one}));
    assert((resolver.resolve("!pvsplayer", context).targets ==
            std::vector<EntityRef>{pvs_player}));
    assert(directory.last_pvs_origin == searcher);
    assert((resolver.resolve("!picker", context).targets ==
            std::vector<EntityRef>{picker}));
    assert(directory.last_picker_player == player_one);

    // !pvsplayer falls back from searching entity to activator, then player 1.
    context.searching_entity = {};
    (void)resolver.resolve("!pvsplayer", context);
    assert(directory.last_pvs_origin == activator);
    context.activator = {};
    (void)resolver.resolve("!pvsplayer", context);
    assert(directory.last_pvs_origin == player_one);

    // Unknown procedural names do not become ordinary targetname searches.
    assert(resolver.resolve("!not_a_source_token", context).targets.empty());
    assert(resolver.resolve("").targets.empty());

    // Exercise the resolver through the Phase 74 delayed-output host seam.
    OutputWorld world;
    const EntityRef source =
        world.add(30, 1, std::string("source"), "logic_auto");
    const EntityRef target_a =
        world.add(31, 1, std::string("door"), "prop_dynamic");
    const EntityRef target_b =
        world.add(32, 1, std::string("door"), "prop_dynamic");
    const EntityRef class_fallback_entity =
        world.add(33, 1, std::nullopt, "func_door");
    const EntityRef player =
        world.add(34, 1, std::string("player"), "player");

    EntityIoRuntime io(world);
    assert(io.connect(source, "OnOpen", link("door", "Open")));
    assert(io.fire(source, "OnOpen", player) == 1);
    auto named_delivery = io.advance_to(0.0);
    assert(named_delivery.inputs_delivered == 2);
    assert(world.delivered[0].target == target_a);
    assert(world.delivered[1].target == target_b);
    assert(world.delivered[0].activator == player);
    assert(world.delivered[0].caller == source);
    assert(world.delivered[0].target != class_fallback_entity);

    assert(io.connect(source, "OnClassFallback", link("func_door", "Open")));
    assert(io.fire(source, "OnClassFallback", player) == 1);
    auto class_delivery = io.advance_to(0.0);
    assert(class_delivery.inputs_delivered == 1);
    assert(world.delivered.back().target == class_fallback_entity);

    assert(io.connect(source, "OnActivator", link("!activator", "Use")));
    assert(io.fire(source, "OnActivator", target_a) == 1);
    auto procedural_delivery = io.advance_to(0.0);
    assert(procedural_delivery.inputs_delivered == 1);
    assert(world.delivered.back().target == target_a);
}
