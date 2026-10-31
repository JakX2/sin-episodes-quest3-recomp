#include "sin_compat/weapon_viewmodel_lifecycle.hpp"

#include <cassert>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace sin_compat;

class FakeLifecycleHost final : public IWeaponViewModelLifecycleHost {
public:
    EntityRef player{8, 3};
    EntityRef next_created{22, 9};
    EntityRef slots[2]{};
    std::map<std::uint32_t, std::uint32_t> living{{8, 3}};
    int count = 2;
    bool player_type = true;
    bool allow_config = true;
    bool allow_store = true;
    int creates = 0;
    std::vector<int> config_slots;
    std::vector<int> cleared_slots;
    std::vector<std::uint32_t> removed;
    std::string last_class;

    bool is_live(EntityRef e) const override {
        const auto it = living.find(e.index);
        return it != living.end() && it->second == e.generation && e.index != 0 && e.index != UINT32_MAX;
    }
    bool is_player(EntityRef e) const override {return e == player && player_type;}
    int viewmodel_slot_count(EntityRef) const override {return count;}
    EntityRef viewmodel_at(EntityRef, int slot) const override {
        assert(slot >= 0 && slot < 2);
        return slots[slot];
    }
    EntityRef create_named_entity(const char* classname) override {
        last_class = classname;
        ++creates;
        if (next_created.index != 0 && next_created.index != UINT32_MAX) living[next_created.index] = next_created.generation;
        return next_created;
    }
    bool configure_viewmodel(EntityRef entity, EntityRef who, int slot) override {
        assert(is_live(entity) && who == player && slot >= 0 && slot < 2);
        config_slots.push_back(slot);
        return allow_config;
    }
    bool store_viewmodel_handle(EntityRef who, int slot, EntityRef e) override {
        assert(who == player && slot >= 0 && slot < 2 && is_live(e));
        if (!allow_store) return false;
        slots[slot] = e;
        return true;
    }
    void remove_viewmodel(EntityRef model) override {
        assert(is_live(model));
        removed.push_back(model.index);
        living.erase(model.index);
    }
    void clear_viewmodel_handle(EntityRef who, int slot) override {
        assert(who == player && slot >= 0 && slot < 2);
        cleared_slots.push_back(slot);
        slots[slot] = {};
    }
};

int main() {
    FakeLifecycleHost h;
    auto ensure = [&](int slot) {return ensure_viewmodel_slot(h, h.player, slot);};
    assert(ensure(-1) == EnsureViewModelResult::InvalidSlot);
    assert(ensure(2) == EnsureViewModelResult::InvalidSlot);
    assert(h.creates == 0);
    h.count = 100000; // Defensive protection against a broken host.
    assert(ensure(0) == EnsureViewModelResult::InvalidSlot);
    clear_viewmodel_slots(h, h.player);
    h.count = 2;

    h.player_type = false;
    assert(ensure(0) == EnsureViewModelResult::InvalidPlayer);
    h.player_type = true;
    h.living[8] = 4;
    assert(ensure(0) == EnsureViewModelResult::InvalidPlayer);
    h.living[8] = 3;

    h.next_created = {22, 9};
    assert(ensure(0) == EnsureViewModelResult::Created);
    assert(h.creates == 1 && h.last_class == "viewmodel");
    assert(h.slots[0] == h.next_created);
    assert(ensure(0) == EnsureViewModelResult::AlreadyPresent);
    assert(h.creates == 1);

    // A stale EHANDLE is treated as missing; the new generation replaces it.
    h.next_created = {23, 7};
    h.slots[1] = {23, 6};
    assert(ensure(1) == EnsureViewModelResult::Created);
    assert(h.slots[1] == h.next_created);

    // Reverse-order cleanup, both handles invalidated.
    clear_viewmodel_slots(h, h.player);
    assert((h.cleared_slots == std::vector<int>{1, 0}));
    assert((h.removed == std::vector<std::uint32_t>{23, 22}));
    assert(h.slots[0] == EntityRef{} && h.slots[1] == EntityRef{});
    clear_viewmodel_slots(h, h.player);
    assert(h.removed.size() == 2); // no double-remove

    // Failed creation never commits an invalid entity.
    h.next_created = {};
    assert(ensure(0) == EnsureViewModelResult::CreationFailed);
    assert(h.slots[0] == EntityRef{});

    // A configure/commit failure rolls back the newly allocated entity.
    h.next_created = {30, 5};
    h.allow_config = false;
    assert(ensure(0) == EnsureViewModelResult::ConfigurationFailed);
    assert(!h.is_live({30, 5}));
    h.allow_config = true;
    h.allow_store = false;
    assert(ensure(0) == EnsureViewModelResult::HandleCommitFailed);
    assert(!h.is_live({30, 5}));
    assert(h.slots[0] == EntityRef{});
    h.allow_store = true;

    // Stale slot is cleared without attempting to remove a recycled entity.
    h.slots[0] = {41, 1};
    h.living[41] = 2;
    const auto count_before = h.removed.size();
    clear_viewmodel_slots(h, h.player);
    assert(h.removed.size() == count_before);
    assert(h.slots[0] == EntityRef{});
}
