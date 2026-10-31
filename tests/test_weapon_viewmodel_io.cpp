#include "sin_compat/weapon_viewmodel_io.hpp"

#include <cassert>
#include <cstdint>
#include <map>
#include <vector>

using namespace sin_compat;

class FakeViewModelHost final : public IWeaponViewModelHost {
public:
    EntityRef weapon{14, 7};
    EntityRef owner{8, 3};
    EntityRef viewmodel{22, 9};
    EntityRef selected = weapon;
    EntityRef resolved_owner = owner;
    EntityRef resolved_viewmodel = viewmodel;
    std::map<std::uint32_t, std::uint32_t> live = {{14, 7}, {8, 3}, {22, 9}};
    bool player_owner = true;
    bool correct_model_owner = true;
    int model_index = 1;
    int model_array_length = 2;
    int weapon_calls = 0;
    int model_calls = 0;
    std::vector<int> call_order;

    bool is_live(EntityRef ref) const override {
        auto found = live.find(ref.index);
        return found != live.end() && found->second == ref.generation;
    }
    EntityRef owner_of(EntityRef) const override { return resolved_owner; }
    EntityRef selected_weapon_of(EntityRef) const override { return selected; }
    void set_weapon_nodraw(EntityRef ref, bool value) override {
        assert(ref == weapon && value);
        ++weapon_calls;
        call_order.push_back(1);
    }
    bool is_player_owner(EntityRef who) const override {
        assert(who == owner);
        return player_owner;
    }
    int viewmodel_index_of(EntityRef ref) const override {
        assert(ref == weapon);
        return model_index;
    }
    bool is_valid_viewmodel_index(EntityRef who, int index) const override {
        assert(who == owner);
        return index >= 0 && index < model_array_length;
    }
    EntityRef viewmodel_of(EntityRef who, int index) const override {
        assert(who == owner && index == model_index);
        return resolved_viewmodel;
    }
    bool is_viewmodel_owned_by(EntityRef vm, EntityRef who) const override {
        assert(vm == resolved_viewmodel && who == owner);
        return correct_model_owner;
    }
    void set_viewmodel_nodraw(EntityRef ref, bool value) override {
        assert(ref == viewmodel && value);
        ++model_calls;
        call_order.push_back(2);
    }
};

int main() {
    FakeViewModelHost host;
    auto invoke = [&] { return process_hide_weapon_input_with_viewmodel(host, host.weapon); };
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.weapon_calls == 1 && host.model_calls == 1);
    assert((host.call_order == std::vector<int>{1, 2}));

    // Unselected: neither weapon nor viewmodel may change.
    host.selected = {99, 1};
    assert(invoke() == HideWeaponInputResult::IgnoredNotSelected);
    assert(host.weapon_calls == 1 && host.model_calls == 1);
    host.selected = host.weapon;

    // Stale owner and weapon generations are rejected before changing effects.
    host.resolved_owner = {8, 4};
    assert(invoke() == HideWeaponInputResult::IgnoredInvalidOwner);
    host.resolved_owner = host.owner;
    host.live[14] = 8;
    assert(invoke() == HideWeaponInputResult::IgnoredInvalidWeapon);
    host.live[14] = 7;

    // NPC owner, missing/invalid viewmodels: world weapon effect still applies.
    host.player_owner = false;
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 1 && host.weapon_calls == 2);
    host.player_owner = true;
    host.model_index = -1;
    assert(invoke() == HideWeaponInputResult::Applied);
    host.model_index = 2;  // first index beyond player array
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 1 && host.weapon_calls == 4);
    host.model_index = 1;

    // A recycled model handle must never receive visibility changes.
    host.live[22] = 10;
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 1 && host.weapon_calls == 5);
    host.live[22] = 9;

    // Resolver returning an unrelated entity must not mutate that model.
    host.correct_model_owner = false;
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 1 && host.weapon_calls == 6);
    host.correct_model_owner = true;
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 2 && host.weapon_calls == 7);

    // Both of the *original SiN* two registered m_hViewModel indices are
    // permitted by this fake host. The engine adapter sets its own bounds.
    host.model_index = 0;
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 3 && host.weapon_calls == 8);
    host.model_index = 2; // index 2 is beyond the two-slot mock array
    assert(invoke() == HideWeaponInputResult::Applied);
    assert(host.model_calls == 3 && host.weapon_calls == 9);
}
