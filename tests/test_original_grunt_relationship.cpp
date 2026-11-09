#include "sin_compat/original_grunt_relationship.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
using namespace sin_compat::ai::grunt;

namespace {
struct Target {
    OriginalGruntEntityToken token;
    bool npc;
    bool player;
    std::int32_t priority;
};
class FakeHost final : public IOriginalGruntRelationshipHost {
public:
    std::vector<Target> targets;
    std::vector<OriginalGruntEntityToken> ordered_tokens;
    OriginalGruntEntityToken enemy = 0;
    std::int32_t parent_relation = 7;
    std::vector<std::string> calls;
    std::vector<OriginalSharedAiCondition> conditions;
    std::vector<OriginalGruntEntityToken> deferred_tails;
    std::size_t parent_gather_calls = 0;

    const Target& target(OriginalGruntEntityToken token) const {
        for (const auto& value : targets) if (value.token == token) return value;
        assert(false && "unknown fake target");
        return targets.front();
    }
    std::int32_t parent_relation_type(
        OriginalGruntEntityToken token) const noexcept override {
        const_cast<FakeHost*>(this)->calls.push_back("parent_relation:" + std::to_string(token));
        return parent_relation;
    }
    bool target_is_npc(OriginalGruntEntityToken token) const noexcept override {
        const_cast<FakeHost*>(this)->calls.push_back("is_npc:" + std::to_string(token));
        return target(token).npc;
    }
    bool target_is_player(OriginalGruntEntityToken token) const noexcept override {
        const_cast<FakeHost*>(this)->calls.push_back("is_player:" + std::to_string(token));
        return target(token).player;
    }
    std::vector<OriginalGruntEntityToken>
    relationship_targets_in_original_order() const override {
        const_cast<FakeHost*>(this)->calls.push_back("enumerate_targets");
        return ordered_tokens;
    }
    OriginalGruntEntityToken current_enemy() const noexcept override {
        const_cast<FakeHost*>(this)->calls.push_back("current_enemy");
        return enemy;
    }
    std::int32_t relation_priority(
        OriginalGruntEntityToken token) const noexcept override {
        const_cast<FakeHost*>(this)->calls.push_back("priority:" + std::to_string(token));
        return target(token).priority;
    }
    void set_ai_condition(OriginalSharedAiCondition condition) noexcept override {
        conditions.push_back(condition);
        calls.push_back("set_condition:" + std::to_string(static_cast<unsigned>(condition)));
    }
    void delegate_parent_condition_gather() noexcept override {
        ++parent_gather_calls;
        calls.push_back("parent_gather");
    }
    void defer_unrecovered_target_condition_tail(
        OriginalGruntEntityToken token) noexcept override {
        deferred_tails.push_back(token);
        calls.push_back("unresolved_tail:" + std::to_string(token));
    }
};
bool has_condition(const FakeHost& host, OriginalSharedAiCondition condition) {
    for (auto value : host.conditions) if (value == condition) return true;
    return false;
}

void test_disabled_relation_uses_parent() {
    FakeHost h; h.targets.push_back({1U,true,false,5});
    assert(original_grunt_relation_type(h,false,1U)==h.parent_relation);
    assert((h.calls==std::vector<std::string>{"parent_relation:1"}));
}
void test_enabled_npc_short_circuits_player() {
    FakeHost h; h.targets.push_back({1U,true,false,0});
    assert(original_grunt_relation_type(h,true,1U)==disposition_hate);
    assert((h.calls==std::vector<std::string>{"is_npc:1"}));
}
void test_enabled_player_checks_npc_then_player() {
    FakeHost h; h.targets.push_back({2U,false,true,0});
    assert(original_grunt_relation_type(h,true,2U)==disposition_hate);
    assert((h.calls==std::vector<std::string>{"is_npc:2","is_player:2"}));
}
void test_enabled_other_is_neutral() {
    FakeHost h; h.targets.push_back({3U,false,false,0});
    assert(original_grunt_relation_type(h,true,3U)==disposition_neutral);
    assert((h.calls==std::vector<std::string>{"is_npc:3","is_player:3"}));
}
void test_disabled_gather_delegates_to_parent() {
    FakeHost h; h.ordered_tokens={1U};
    const auto r=gather_original_grunt_conditions(h,false);
    assert(r.disposition==OriginalGruntConditionGatherDisposition::ParentDelegated);
    assert(r.targets_visited==0U && h.parent_gather_calls==1U);
    assert(h.conditions.empty());
    assert((h.calls==std::vector<std::string>{"parent_gather"}));
}
void test_enabled_gather_preserves_order_enemy_and_priority_bands() {
    FakeHost h;
    h.targets={{1U,true,false,-1},{2U,false,true,11},
               {3U,false,false,5},{4U,true,false,10}};
    h.ordered_tokens={1U,2U,3U,4U}; h.enemy=2U;
    const auto r=gather_original_grunt_conditions(h,true);
    assert(r.disposition==OriginalGruntConditionGatherDisposition::PartialHostilityMode);
    assert(r.targets_visited==4U && r.neutral_targets_skipped==1U);
    assert(r.unresolved_tails_deferred==3U && h.parent_gather_calls==0U);
    assert(has_condition(h,OriginalSharedAiCondition::SeePlayer));
    assert(has_condition(h,OriginalSharedAiCondition::SeeEnemy));
    assert(has_condition(h,OriginalSharedAiCondition::SeeDislike));
    assert(has_condition(h,OriginalSharedAiCondition::SeeNemesis));
    assert(has_condition(h,OriginalSharedAiCondition::SeeHate));
    assert((h.deferred_tails==std::vector<OriginalGruntEntityToken>{1U,2U,4U}));
    const std::vector<std::string> prefix={
        "enumerate_targets","current_enemy",
        "is_player:1","is_npc:1","is_player:1","priority:1","unresolved_tail:1",
        "is_player:2","is_npc:2","is_player:2"};
    assert(h.calls.size()>=prefix.size());
    for(std::size_t i=0;i<prefix.size();++i) assert(h.calls[i]==prefix[i]);
}
}
int main() {
    test_disabled_relation_uses_parent();
    test_enabled_npc_short_circuits_player();
    test_enabled_player_checks_npc_then_player();
    test_enabled_other_is_neutral();
    test_disabled_gather_delegates_to_parent();
    test_enabled_gather_preserves_order_enemy_and_priority_bands();
    std::puts("PASS: original SiN grunt hostility-mode relation and condition gathering (mock host only)");
    return 0;
}
