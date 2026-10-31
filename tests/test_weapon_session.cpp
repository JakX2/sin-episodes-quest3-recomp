#include "sin_compat/weapon_session.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace sin_compat;

class FakeEngine final : public IWeaponSessionHost {
public:
    struct Live {std::uint32_t generation;};
    std::map<std::uint32_t, Live> live{{8, {3}}, {14, {7}}};
    EntityRef owner{8, 3}, weapon{14, 7}, selected=weapon;
    EntityRef attached_owner=owner;
    std::array<EntityRef, 2> slots{};
    int viewmodel_index=0, creates=0, removes=0, binds=0, plays=0;
    int weapon_hidden=0, viewmodel_hidden=0, fovs=0;
    bool player=true, model_owner_matches=true, allow_create=true;
    bool allow_config=true, allow_store=true;
    std::vector<int> cleared;
    std::vector<std::string> events;
    std::vector<int> played_sequences;
    std::string model_name;

    bool is_live(EntityRef e) const override {
        const auto it=live.find(e.index);
        return it != live.end() && it->second.generation==e.generation;
    }
    EntityRef owner_of(EntityRef e) const override {
        assert(e==weapon); return attached_owner;
    }
    EntityRef selected_weapon_of(EntityRef e) const override {
        assert(e==owner); return selected;
    }
    void set_weapon_nodraw(EntityRef e, bool hidden) override {
        assert(e==weapon && hidden && is_live(e)); ++weapon_hidden;
        events.push_back("hide_weapon");
    }
    bool is_player_owner(EntityRef e) const override {return e==owner && player;}
    int viewmodel_index_of(EntityRef e) const override {
        assert(e==weapon); return viewmodel_index;
    }
    bool is_valid_viewmodel_index(EntityRef e, int index) const override {
        return e==owner && index>=0 && index<2;
    }
    EntityRef viewmodel_of(EntityRef e, int index) const override {
        assert(e==owner && index>=0 && index<2); return slots[index];
    }
    bool is_viewmodel_owned_by(EntityRef e, EntityRef o) const override {
        assert(o==owner); return model_owner_matches && is_live(e);
    }
    void set_viewmodel_nodraw(EntityRef e, bool hidden) override {
        assert(is_live(e) && hidden); ++viewmodel_hidden;
        events.push_back("hide_viewmodel");
    }
    bool is_player(EntityRef e) const override {return e==owner && player;}
    int viewmodel_slot_count(EntityRef e) const override {assert(e==owner);return 2;}
    EntityRef viewmodel_at(EntityRef e, int slot) const override {
        assert(e==owner && slot>=0 && slot<2);return slots[slot];
    }
    EntityRef create_named_entity(const char* classname) override {
        assert(std::string(classname)=="viewmodel");
        ++creates;
        if (!allow_create) return {};
        EntityRef created{static_cast<std::uint32_t>(30+creates), 1};
        live[created.index]={created.generation};
        return created;
    }
    bool configure_viewmodel(EntityRef e, EntityRef p, int slot) override {
        assert(is_live(e) && p==owner && slot>=0 && slot<2);
        return allow_config;
    }
    bool store_viewmodel_handle(EntityRef p, int slot, EntityRef model) override {
        assert(p==owner && slot>=0 && slot<2 && is_live(model));
        if (!allow_store) return false;
        slots[slot]=model;
        return true;
    }
    void remove_viewmodel(EntityRef model) override {
        assert(is_live(model));
        live.erase(model.index);
        ++removes;
        events.push_back("remove"+std::to_string(model.index));
    }
    void clear_viewmodel_handle(EntityRef p, int slot) override {
        assert(p==owner && slot>=0 && slot<2);
        slots[slot]={};cleared.push_back(slot);
    }
    int resolve_sequence(int activity) override {return activity>=0 ? activity+100 : -1;}
    void bind_viewmodel(const std::string& model, const std::array<float,3>&) override {
        ++binds;model_name=model;
    }
    void play_sequence(int sequence) override {
        ++plays;played_sequences.push_back(sequence);
    }
    void set_flatscreen_fov(int, float) override {++fovs;}
};

static WeaponConfig fixture() {
    WeaponConfig c;
    c.viewmodel_name="synthetic/model_only.mdl";
    c.zoom_fov=55; // synthetic only, NOT original SiN game data
    c.ammo.clip_size=6;
    c.ammo.clip2_size=-1;
    return c;
}

int main() {
    FakeEngine h;
    WeaponSession session(h,h.owner,h.weapon,WeaponKind::Magnum,fixture(),true);

    // Fresh owner: no viewmodel, then create exactly once and reuse.
    assert(session.ensure_viewmodel()==EnsureViewModelResult::Created);
    assert(session.ensure_viewmodel()==EnsureViewModelResult::AlreadyPresent);
    assert(h.creates==1 && h.slots[0].index==31);
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::Applied);
    assert(h.weapon_hidden==1 && h.viewmodel_hidden==1);
    assert((h.events == std::vector<std::string>{"hide_weapon","hide_viewmodel"}));

    // The original player has a second slot. The bridge reads the live index.
    h.viewmodel_index=1;
    assert(session.ensure_viewmodel()==EnsureViewModelResult::Created);
    assert(h.creates==2 && h.slots[1].index==32);
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::Applied);
    assert(h.viewmodel_hidden==2);

    // Bad slot never indexes the array; unrelated / stale viewmodels not hidden.
    h.viewmodel_index=2;
    assert(session.ensure_viewmodel()==EnsureViewModelResult::InvalidSlot);
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::Applied);
    assert(h.weapon_hidden==3 && h.viewmodel_hidden==2);
    h.viewmodel_index=0;
    h.live[31].generation=2; // stale 31:1, recycled as 31:2
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::Applied);
    assert(h.viewmodel_hidden==2);
    h.live[31].generation=1;
    h.model_owner_matches=false;
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::Applied);
    assert(h.viewmodel_hidden==2);
    h.model_owner_matches=true;

    // A different selected weapon is forbidden to receive effects or ammo.
    h.selected={88,4};h.live[88]={4};
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::IgnoredNotSelected);
    assert(session.ensure_viewmodel()==EnsureViewModelResult::InvalidPlayer);
    assert(!session.on_primary_attack());
    assert(!session.on_reload_begin());
    const int old_hide=h.weapon_hidden;
    h.selected=h.weapon;

    // VR button-press zoom does not change the HMD FOV; losing selection
    // deactivates ADS, and a fresh press is needed to enter again.
    assert(session.on_usercmd(0x80000u,false).entered_zoom);
    assert(session.runtime().is_zoomed() && h.fovs==0);
    h.selected={88,4};
    assert(session.on_usercmd(0u,false).exited_zoom);
    assert(!session.runtime().is_zoomed());
    h.selected=h.weapon;
    assert(session.on_usercmd(0x80000u,false).entered_zoom);
    assert(h.fovs==0 && h.weapon_hidden==old_hide);

    // Reload transfer is engine-event-driven AND exactly once per cycle.
    session.runtime().ammo().clip=2;
    session.runtime().ammo().reserve=5;
    assert(session.on_reload_begin());
    assert(session.runtime().ammo().clip==2);
    h.selected={88,4};
    assert(session.on_reload_transfer_event().primary==0);
    assert(session.runtime().ammo().clip==2);
    h.selected=h.weapon;
    const auto first=session.on_reload_transfer_event();
    assert(first.primary==4 && session.runtime().ammo().clip==6);
    session.runtime().ammo().clip=3; // simulate correction before duplicate event
    assert(session.on_reload_transfer_event().primary==0);
    assert(session.runtime().ammo().clip==3);
    session.on_reload_end();
    assert(!session.runtime().reload_active());
    assert(session.on_reload_begin());
    assert(session.on_reload_transfer_event().primary==1);
    session.on_reload_end();

    // Holster cancels outstanding reload and ADS. No early transfer.
    session.runtime().ammo().clip=1;
    session.runtime().ammo().reserve=4;
    assert(session.on_reload_begin());
    const int reserve_before=session.runtime().ammo().reserve;
    assert(session.on_holster());
    assert(session.runtime().is_holstered());
    assert(!session.runtime().reload_active());
    assert(session.runtime().ammo().reserve==reserve_before);
    assert(session.on_reload_transfer_event().primary==0);

    // A player-level cleanup removes both models in reverse slot order.
    session.on_player_cleanup();
    assert(h.removes==2);
    assert((h.cleared == std::vector<int>{1,0}));
    assert(h.slots[0]==EntityRef{} && h.slots[1]==EntityRef{});
    session.on_player_cleanup();
    assert(h.removes==2);

    // Handle reuse and dead player cannot trigger additional events.
    h.live[8].generation=4;
    assert(session.ensure_viewmodel()==EnsureViewModelResult::InvalidPlayer);
    assert(session.on_hide_weapon_input()==HideWeaponInputResult::IgnoredInvalidOwner);
    assert(!session.on_reload_begin());

    // Native-port safety: a deselection during an active reload must cancel
    // its pending event rather than grant ammunition to the next owner.
    FakeEngine interrupted;
    WeaponSession detached(interrupted, interrupted.owner, interrupted.weapon,
                           WeaponKind::Magnum, fixture(), true);
    detached.runtime().ammo().clip=1;
    detached.runtime().ammo().reserve=5;
    assert(detached.on_reload_begin());
    interrupted.selected={55, 2};
    interrupted.live[55]={2};
    detached.on_usercmd(0u, false);
    assert(!detached.runtime().reload_active());
    interrupted.selected=interrupted.weapon;
    assert(detached.on_reload_transfer_event().primary==0);
    assert(detached.runtime().ammo().clip==1);

    // Flatscreen-only: a lost selection must restore the old camera FOV.
    // This synthetic shim has no actual headset and does not test an HMD.
    FakeEngine desktop;
    WeaponSession flat(desktop, desktop.owner, desktop.weapon,
                       WeaponKind::Magnum, fixture(), false);
    assert(flat.on_usercmd(0x80000u,false).entered_zoom);
    assert(desktop.fovs==1);
    desktop.selected={99,1};
    desktop.live[99]={1};
    assert(flat.on_usercmd(0u,false).exited_zoom);
    assert(desktop.fovs==2);
    assert(!flat.runtime().is_zoomed());
}
