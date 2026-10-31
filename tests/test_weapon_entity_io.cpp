#include "sin_compat/weapon_entity_io.hpp"

#include <cassert>
#include <cstdint>
#include <map>
#include <utility>

using namespace sin_compat;

class FakeIOHost final : public IWeaponEntityIOHost {
public:
    std::map<std::uint32_t, std::uint32_t> generations;
    EntityRef owner{};
    EntityRef selected{};
    int nodraw_calls = 0;
    EntityRef applied_to{};
    bool last_hidden = false;

    bool is_live(EntityRef ref) const override {
        auto found = generations.find(ref.index);
        return found != generations.end() && found->second == ref.generation;
    }
    EntityRef owner_of(EntityRef) const override { return owner; }
    EntityRef selected_weapon_of(EntityRef) const override { return selected; }
    void set_weapon_nodraw(EntityRef weapon, bool hidden) override {
        ++nodraw_calls;
        applied_to = weapon;
        last_hidden = hidden;
    }
};

int main() {
    const EntityRef weapon{14, 7};
    const EntityRef owner{8, 3};
    const EntityRef other{15, 2};
    FakeIOHost host;
    host.generations = {{weapon.index, weapon.generation}, {owner.index, owner.generation},
                        {other.index, other.generation}};
    host.owner = owner;
    host.selected = weapon;

    assert(process_hide_weapon_input(host, weapon) == HideWeaponInputResult::Applied);
    assert(host.nodraw_calls == 1 && host.last_hidden && host.applied_to == weapon);

    // Invalid or recycled weapon IDs must never mutate an entity.
    assert(process_hide_weapon_input(host, {weapon.index, 8}) ==
           HideWeaponInputResult::IgnoredInvalidWeapon);
    assert(host.nodraw_calls == 1);

    // The same numeric owner slot with a stale generation is not a valid owner.
    host.owner = {owner.index, 4};
    assert(process_hide_weapon_input(host, weapon) ==
           HideWeaponInputResult::IgnoredInvalidOwner);
    assert(host.nodraw_calls == 1);
    host.owner = owner;

    // An unselected weapon, or a stale selected-weapon handle, does nothing.
    host.selected = other;
    assert(process_hide_weapon_input(host, weapon) ==
           HideWeaponInputResult::IgnoredNotSelected);
    host.selected = {weapon.index, 99};
    assert(process_hide_weapon_input(host, weapon) ==
           HideWeaponInputResult::IgnoredNotSelected);
    assert(host.nodraw_calls == 1);
    host.selected = weapon;

    // If the host invalidates the owning entity, the handler declines safely.
    host.generations.erase(owner.index);
    assert(process_hide_weapon_input(host, weapon) ==
           HideWeaponInputResult::IgnoredInvalidOwner);
    assert(host.nodraw_calls == 1);
    host.generations[owner.index] = owner.generation;
    assert(process_hide_weapon_input(host, weapon) == HideWeaponInputResult::Applied);
    assert(host.nodraw_calls == 2);
}
