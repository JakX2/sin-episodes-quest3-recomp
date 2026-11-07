// End-to-end original ai_goal_assault fanout across one real compiled native
// C ABI shared module and a synthetic level-owned NPC/point host.
// No original 2006 PE or SourceVR engine is linked or executed.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "sin_compat/assault_goal_fanout.hpp"
#include "sin_compat/assault_goal_configuration.hpp"
#include <optional>
#include <string_view>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace sin_compat;
namespace {

bool same(sin_assault_ref_v1 a, sin_assault_ref_v1 b) {
    return a.index == b.index && a.generation == b.generation;
}
struct Npc {
    sin_assault_ref_v1 ref;
    bool alive = true;
    bool heard_combat = false;
    unsigned resets = 0;
    unsigned provoked = 0;
};
struct Rally {
    uint32_t id;
    int priority;
    float x;
    sin_assault_ref_v1 owner{};
};
struct FakeLevel {
    std::vector<Npc> actors{
        {{10, 1}, true, false, 0, 0},
        {{11, 1}, true, false, 0, 0},
        {{12, 1}, true, false, 0, 0},
        {{13, 1}, true, false, 0, 0},
    };
    std::vector<Rally> points{
        {100, 9, 1.f, {0, 0}},
        {200, 5, 2.f, {0, 0}},
        {300, 3, 3.f, {0, 0}},
        {400, 1, 4.f, {0, 0}},
    };
    std::vector<std::pair<std::string, uint32_t>> events;
    unsigned combat_queries = 0;
    bool expire_on_init = false;

    Npc* npc(sin_assault_ref_v1 id) {
        for (auto& n : actors) if (same(n.ref, id)) return &n;
        return nullptr;
    }
    Rally* rally(uint32_t id) {
        for (auto& p : points) if (p.id == id) return &p;
        return nullptr;
    }
    std::vector<sin_assault_candidate_v1> candidates() const {
        std::vector<sin_assault_candidate_v1> result;
        for (const auto& p : points)
            result.push_back({p.id, p.priority, {p.x, 0.f, 0.f},
                              p.owner, 1});
        return result;
    }
    static int32_t live(void* user, sin_assault_ref_v1 id) {
        auto* level = static_cast<FakeLevel*>(user);
        const auto* actor = level->npc(id);
        return actor && actor->alive ? 1 : 0;
    }
    static int32_t lock(void* user, uint32_t id, sin_assault_ref_v1 actor) {
        auto* level = static_cast<FakeLevel*>(user);
        auto* p = level->rally(id);
        if (!p || !live(user, actor)) return 0;
        if (p->owner.index && live(user, p->owner)) return 0;
        p->owner = actor;
        return 1;
    }
    static int32_t resolve(void* user, uint32_t id) {
        return static_cast<FakeLevel*>(user)->rally(id) ? 1 : 0;
    }
    static int32_t unlock(void* user, uint32_t id, sin_assault_ref_v1 actor) {
        auto* level = static_cast<FakeLevel*>(user);
        auto* p = level->rally(id);
        if (!p || !same(p->owner, actor)) return 0;
        p->owner = {0, 0};
        return 1;
    }
    static int32_t initialize(void* user, sin_assault_ref_v1 id, uint32_t point) {
        auto* level = static_cast<FakeLevel*>(user);
        auto* n = level->npc(id);
        if (!n || !n->alive || !level->rally(point)) return 0;
        if (level->expire_on_init) n->alive = false;
        return 1;
    }
    static int32_t reset(void* user, sin_assault_ref_v1 id) {
        auto* level = static_cast<FakeLevel*>(user);
        auto* n = level->npc(id);
        if (!n || !n->alive) return 0;
        ++n->resets;
        level->events.emplace_back("reset", id.index);
        return 1;
    }
    static int32_t set_condition(void* user, sin_assault_ref_v1 id, uint32_t condition) {
        auto* level = static_cast<FakeLevel*>(user);
        auto* n = level->npc(id);
        if (!n || !n->alive || condition != 9001) return 0;
        ++n->provoked;
        level->events.emplace_back("provoked", id.index);
        return 1;
    }
    static int32_t has_condition(void* user, sin_assault_ref_v1 id, uint32_t condition) {
        auto* level = static_cast<FakeLevel*>(user);
        auto* n = level->npc(id);
        if (!n || !n->alive || condition != 9002) return -1;
        ++level->combat_queries;
        return n->heard_combat ? 1 : 0;
    }
    sin_assault_host_v1 callbacks() const {
        return {SIN_ASSAULT_ABI_VERSION_1, sizeof(sin_assault_host_v1),
                live, lock, resolve, unlock, initialize, reset,
                set_condition, has_condition};
    }
};

void expect_state(sin_assault_session_v1* s, int mode, int received, uint32_t rally) {
    sin_assault_state_v1 observed{};
    assert(sin_assault_get_state_v1(s, &observed) == SIN_ASSAULT_OK);
    assert(observed.configured_mode == mode);
    assert(observed.received_cue == received);
    assert(observed.rally_id == rally);
}

void run() {
    FakeLevel world;
    const auto cb = world.callbacks();
    const sin_assault_ref_v1 a{10, 1}, b{11, 1}, c{12, 1}, d{13, 1};
    const sin_assault_ref_v1 recycled{10, 2};
    auto* sa = sin_assault_create_v1(&cb, &world, a, 9001, 9002);
    auto* sb = sin_assault_create_v1(&cb, &world, b, 9001, 9002);
    auto* sc = sin_assault_create_v1(&cb, &world, c, 9001, 9002);
    auto* sd = sin_assault_create_v1(&cb, &world, d, 9001, 9002);
    assert(sa && sb && sc && sd);
    const float origin[3]{0.f, 0.f, 0.f};
    uint32_t selected = 0;
    auto bind = [&](sin_assault_session_v1* session, int mode) {
        const auto candidates = world.candidates();
        const int result = sin_assault_bind_v1(
            session, origin, candidates.data(),
            static_cast<uint32_t>(candidates.size()), mode, &selected);
        assert(result == SIN_ASSAULT_OK);
        return selected;
    };
    assert(bind(sa, 1) == 100);
    assert(bind(sb, 2) == 200);
    assert(bind(sc, 3) == 300);
    assert(same(world.rally(100)->owner, a));
    assert(same(world.rally(200)->owner, b));
    assert(same(world.rally(300)->owner, c));

    // Stable per-level handle snapshots and lookup emulate the original
    // ai_goal_assault actor fanout without cloning Windows EHANDLE bytes.
    auto resolve_session = [&](sin_assault_ref_v1 ref)->sin_assault_session_v1* {
        if (same(ref, a)) return sa;
        if (same(ref, b)) return sb;
        if (same(ref, c)) return sc;
        if (same(ref, d)) return sd;
        return nullptr;
    };
    const std::vector<sin_assault_ref_v1> original_goal_snapshot{
        a, {0, 0}, a, {99, 1}, b, c, d
    };
    world.events.clear();
    const auto report = begin_original_goal_assault_fanout(
        original_goal_snapshot, resolve_session);
    assert(report.visited == 7 && report.signaled == 3);
    assert(report.invalid_handles == 1 && report.duplicate_handles == 1);
    assert(report.missing_sessions == 1 && report.not_bound == 1);
    assert(report.expired_actors == 0 && report.host_failures == 0);
    const std::vector<std::pair<std::string, uint32_t>> expected{
        {"reset",10}, {"provoked",10},
        {"reset",11}, {"provoked",11},
        {"reset",12}, {"provoked",12}
    };
    assert(world.events == expected); // original per-actor effect ordering
    expect_state(sa, 1, 1, 100);
    expect_state(sb, 2, 1, 200);
    expect_state(sc, 3, 1, 300);
    uint32_t ready = 55;
    assert(sin_assault_check_cue_v1(sa, &ready) == SIN_ASSAULT_OK && ready == 1);
    assert(sin_assault_check_cue_v1(sb, &ready) == SIN_ASSAULT_OK && ready == 0);
    assert(world.combat_queries == 1);
    assert(sin_assault_check_cue_v1(sc, &ready) == SIN_ASSAULT_OK && ready == 1);
    assert(world.combat_queries == 1);
    expect_state(sc, 3, 3, 300);
    world.npc(b)->heard_combat = true;
    assert(sin_assault_check_cue_v1(sb, &ready) == SIN_ASSAULT_OK && ready == 1);
    assert(world.combat_queries == 2);

    // Original actor generation expires; a recycled same-index NPC is NOT
    // interchangeable, even though numeric index 10 still exists.
    world.npc(a)->alive = false;
    world.actors.push_back({recycled, true, false, 0, 0});
    const auto second = begin_original_goal_assault_fanout({a, b}, resolve_session);
    assert(second.visited == 2 && second.signaled == 1);
    assert(second.expired_actors == 1);
    assert(world.npc(a)->provoked == 1);
    expect_state(sa, 1, 1, 100);
    assert(sin_assault_finish_v1(sa) == SIN_ASSAULT_OK); // cleanup after death
    assert(sin_assault_destroy_v1(sa) == SIN_ASSAULT_OK);
    sa = nullptr;
    assert(!world.rally(100)->owner.index);

    auto* new_generation = sin_assault_create_v1(&cb, &world, recycled, 9001, 9002);
    assert(new_generation);
    assert(bind(new_generation, 1) == 100);
    assert(sin_assault_begin_v1(new_generation) == SIN_ASSAULT_OK);
    expect_state(new_generation, 1, 1, 100);
    assert(world.npc(recycled)->provoked == 1);

    // A host callback can expire an NPC midway through setup; new code must
    // roll back the acquired rally instead of committing old cue state.
    world.expire_on_init = true;
    const auto candidates = world.candidates();
    assert(sin_assault_bind_v1(sd, origin, candidates.data(),
        static_cast<uint32_t>(candidates.size()), 3, &selected)
        == SIN_ASSAULT_ACTOR_EXPIRED);
    assert(!world.rally(400)->owner.index);
    expect_state(sd, 1, 0, 0);
    world.expire_on_init = false;
    assert(sin_assault_begin_v1(sd) == SIN_ASSAULT_NOT_BOUND);

    // A user-supplied snapshot resolver can throw for one actor; fail closed
    // for that actor and continue to subsequent valid NPCs.
    const auto recover = begin_original_goal_assault_fanout(
        {{88, 1}, c}, [&](sin_assault_ref_v1 ref)->sin_assault_session_v1* {
            if (ref.index == 88) throw std::runtime_error("missing host binding");
            return sc;
        });
    assert(recover.visited == 2 && recover.host_failures == 1 && recover.signaled == 1);

    for (auto* session : {sb, sc, new_generation}) {
        assert(sin_assault_finish_v1(session) == SIN_ASSAULT_OK);
        assert(sin_assault_destroy_v1(session) == SIN_ASSAULT_OK);
    }
    assert(sin_assault_destroy_v1(sd) == SIN_ASSAULT_OK);
    for (const auto& point : world.points) assert(!point.owner.index);
}

void run_goal_configuration() {
    // Original ai_goal_assault goal-actor configuration happens BEFORE the
    // separate BeginAssault input. This fixture uses compiled shared C ABI.
    FakeLevel world;
    const auto cb = world.callbacks();
    const sin_assault_ref_v1 a{10,1}, b{11,1}, c{12,1}, d{13,1};
    auto* sa = sin_assault_create_v1(&cb,&world,a,9001,9002);
    auto* sb = sin_assault_create_v1(&cb,&world,b,9001,9002);
    auto* sc = sin_assault_create_v1(&cb,&world,c,9001,9002);
    auto* sd = sin_assault_create_v1(&cb,&world,d,9001,9002);
    assert(sa && sb && sc && sd);
    unsigned resolved = 0, collected = 0;
    const auto session = [&](sin_assault_ref_v1 ref)->sin_assault_session_v1* {
        ++resolved;
        if (same(ref,a)) return sa;
        if (same(ref,b)) return sb;
        if (same(ref,c)) return sc;
        if (same(ref,d)) return sd;
        return nullptr;
    };
    const auto gather = [&](sin_assault_ref_v1 ref, std::string_view name)
        -> std::optional<AssaultGoalActorBindingSnapshot> {
        ++collected;
        assert(world.npc(ref));
        if (name == "snapshot-failed") return std::nullopt;
        if (name == "throw-resolver") throw std::runtime_error("host lookup");
        AssaultGoalActorBindingSnapshot out;
        out.npc_origin = {{0.f,0.f,0.f}};
        if (name == "goal_rally" || name == "direct_rally")
            out.matching_rallypoints = world.candidates();
        return out; // empty is a valid, reported "no matching rally" result
    };
    const std::vector<sin_assault_ref_v1> actors{a, {0,0}, a, {99,1}, b};
    // Invalid map configuration fails before dereferencing *any* host handle.
    auto result = configure_original_goal_assault_fanout(
        actors, "", 1, session, gather);
    assert(result.invalid_configuration && result.visited == 5);
    assert(resolved == 0 && collected == 0);
    result = configure_original_goal_assault_fanout(
        actors, "goal_rally", 99, session, gather);
    assert(result.invalid_configuration && resolved == 0);
    result = configure_original_goal_assault_fanout(
        actors, "goal_rally", 1, session, gather);
    assert(!result.invalid_configuration && result.visited == 5);
    assert(result.bound == 2 && result.invalid_handles == 1);
    assert(result.duplicate_handles == 1 && result.missing_sessions == 1);
    assert(result.actor_results.size() == 3);
    assert(result.actor_results[0].status == SIN_ASSAULT_OK);
    assert(result.actor_results[0].rally_id == 100);
    assert(result.actor_results[1].status == SIN_ASSAULT_INVALID_ARGUMENT);
    assert(result.actor_results[2].status == SIN_ASSAULT_OK);
    assert(result.actor_results[2].rally_id == 200);
    assert(resolved == 3 && collected == 2);
    expect_state(sa,1,0,100);
    expect_state(sb,1,0,200);
    assert(world.npc(a)->resets == 0 && world.npc(a)->provoked == 0);
    // Only the later BeginAssault input signals and provokes them.
    world.events.clear();
    const auto began = begin_original_goal_assault_fanout({a,b},session);
    assert(began.signaled == 2 && began.host_failures == 0);
    const std::vector<std::pair<std::string,uint32_t>> expected{
        {"reset",10},{"provoked",10},{"reset",11},{"provoked",11}};
    assert(world.events == expected);
    expect_state(sa,1,1,100);
    expect_state(sb,1,1,200);

    // Second use of a bound session must not allocate a second reservation.
    result=configure_original_goal_assault_fanout(
        {a}, "goal_rally", 2, session, gather);
    assert(result.already_bound == 1 && result.bound == 0);
    expect_state(sa,1,1,100);

    // The *direct MobGrunt Assault(string)* path always passes cue mode 3,
    // and it must NOT implicitly fire an ai_goal_assault BeginAssault cue.
    const auto direct = configure_original_mobgrunt_assault_input(
        c,"direct_rally",session,gather);
    assert(direct.bound == 1 && direct.actor_results.size() == 1);
    assert(direct.actor_results[0].rally_id == 300);
    expect_state(sc,3,0,300);
    assert(world.npc(c)->provoked == 0 && world.npc(c)->resets == 0);
    uint32_t ready=0;
    const unsigned before=world.combat_queries;
    assert(sin_assault_check_cue_v1(sc,&ready)==SIN_ASSAULT_OK && ready==1);
    assert(world.combat_queries == before);
    expect_state(sc,3,3,300);

    // Distinguish a valid empty candidate set from unavailable level data.
    result=configure_original_goal_assault_fanout(
        {d},"no_matching_rally",1,session,gather);
    assert(result.no_candidates==1 && result.unavailable_snapshots==0);
    expect_state(sd,1,0,0);
    result=configure_original_goal_assault_fanout(
        {d},"snapshot-failed",1,session,gather);
    assert(result.unavailable_snapshots==1 && result.no_candidates==0);
    expect_state(sd,1,0,0);

    // Actor slot generation is checked inside the C ABI before a lock.
    world.npc(d)->alive=false;
    result=configure_original_goal_assault_fanout(
        {d},"goal_rally",3,session,gather);
    assert(result.expired_actors==1 && result.bound==0);
    expect_state(sd,1,0,0);
    world.npc(d)->alive=true;

    // A collector throwing for one actor cannot suppress the next actor.
    result=configure_original_goal_assault_fanout(
        {d,{98,1}},"throw-resolver",3,session,gather);
    assert(result.host_failures==1 && result.missing_sessions==1);
    expect_state(sd,1,0,0);

    for (auto* s : {sa,sb,sc}) {
        assert(sin_assault_finish_v1(s)==SIN_ASSAULT_OK);
        assert(sin_assault_destroy_v1(s)==SIN_ASSAULT_OK);
    }
    assert(sin_assault_destroy_v1(sd)==SIN_ASSAULT_OK);
    for (auto& rally : world.points) assert(!rally.owner.index);
}
} // namespace
int main() {
    run();
    run_goal_configuration();
    std::cout << "PASS: original ai_goal_assault C ABI fanout across multiple generation-safe mock NPCs and rally ownership\n";
}
