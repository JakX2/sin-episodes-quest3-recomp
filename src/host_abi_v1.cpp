#define SIN_COMPAT_ABI_EXPORT
#include "sin_compat/host_abi_v1.h"
#include "sin_compat/weapon_session.hpp"
#include <array>
#include <memory>
#include <new>
#include <string>
#include <utility>

namespace {
using namespace sin_compat;
sin_entity_ref_v1 to_c(EntityRef r) noexcept { return {r.index, r.generation}; }
EntityRef from_c(sin_entity_ref_v1 r) noexcept { return {r.index, r.generation}; }

class Callbacks final : public IWeaponSessionHost {
public:
    Callbacks(sin_host_callbacks_v1 cb, void *context) : c_(cb), u_(context) {}
    bool is_live(EntityRef e) const override { return c_.is_live(u_,to_c(e))!=0; }
    EntityRef owner_of(EntityRef e) const override {return from_c(c_.owner_of(u_,to_c(e)));}
    EntityRef selected_weapon_of(EntityRef e) const override {return from_c(c_.selected_weapon_of(u_,to_c(e)));}
    void set_weapon_nodraw(EntityRef e,bool b) override {c_.set_weapon_nodraw(u_,to_c(e),b);}
    bool is_player_owner(EntityRef e) const override {return c_.is_player_owner(u_,to_c(e))!=0;}
    int viewmodel_index_of(EntityRef e) const override {return c_.viewmodel_index_of(u_,to_c(e));}
    bool is_valid_viewmodel_index(EntityRef e,int i) const override {return c_.is_valid_viewmodel_index(u_,to_c(e),i)!=0;}
    EntityRef viewmodel_of(EntityRef e,int i) const override {return from_c(c_.viewmodel_of(u_,to_c(e),i));}
    bool is_viewmodel_owned_by(EntityRef a,EntityRef b) const override {return c_.is_viewmodel_owned_by(u_,to_c(a),to_c(b))!=0;}
    void set_viewmodel_nodraw(EntityRef e,bool b) override {c_.set_viewmodel_nodraw(u_,to_c(e),b);}
    bool is_player(EntityRef e) const override {return c_.is_player(u_,to_c(e))!=0;}
    int viewmodel_slot_count(EntityRef e) const override {return c_.viewmodel_slot_count(u_,to_c(e));}
    EntityRef viewmodel_at(EntityRef e,int i) const override {return from_c(c_.viewmodel_at(u_,to_c(e),i));}
    EntityRef create_named_entity(const char *n) override {return from_c(c_.create_named_entity(u_,n));}
    bool configure_viewmodel(EntityRef a,EntityRef b,int i) override {return c_.configure_viewmodel(u_,to_c(a),to_c(b),i)!=0;}
    bool store_viewmodel_handle(EntityRef a,int i,EntityRef b) override {return c_.store_viewmodel_handle(u_,to_c(a),i,to_c(b))!=0;}
    void remove_viewmodel(EntityRef e) override {c_.remove_viewmodel(u_,to_c(e));}
    void clear_viewmodel_handle(EntityRef e,int i) override {c_.clear_viewmodel_handle(u_,to_c(e),i);}
    int resolve_sequence(int i) override {return c_.resolve_sequence(u_,i);}
    void bind_viewmodel(const std::string& n,const std::array<float,3>& a) override {c_.bind_viewmodel(u_,n.c_str(),a.data());}
    void play_sequence(int i) override {c_.play_sequence(u_,i);}
    void set_flatscreen_fov(int i,float d) override {c_.set_flatscreen_fov(u_,i,d);}
private:
    sin_host_callbacks_v1 c_; void* u_;
};

bool complete(const sin_host_callbacks_v1 &c) {
    return c.version==SIN_COMPAT_ABI_VERSION_1 && c.struct_size==sizeof(c) &&
        c.is_live && c.owner_of && c.selected_weapon_of && c.set_weapon_nodraw &&
        c.is_player_owner && c.viewmodel_index_of && c.is_valid_viewmodel_index &&
        c.viewmodel_of && c.is_viewmodel_owned_by && c.set_viewmodel_nodraw &&
        c.is_player && c.viewmodel_slot_count && c.viewmodel_at &&
        c.create_named_entity && c.configure_viewmodel && c.store_viewmodel_handle &&
        c.remove_viewmodel && c.clear_viewmodel_handle && c.resolve_sequence &&
        c.bind_viewmodel && c.play_sequence && c.set_flatscreen_fov;
}
}
struct sin_session_v1 {
    Callbacks callbacks;
    WeaponSession session;
    sin_session_v1(const sin_host_callbacks_v1& cb,void* ctx,EntityRef player,
                   EntityRef weapon,WeaponKind kind,WeaponConfig cfg,bool vr)
        : callbacks(cb,ctx),session(callbacks,player,weapon,kind,std::move(cfg),vr) {}
};
extern "C" {
uint32_t sin_abi_version_v1(void) {return SIN_COMPAT_ABI_VERSION_1;}
sin_session_v1 *sin_session_create_v1(const sin_host_callbacks_v1 *cb,void *ctx,
    sin_entity_ref_v1 player,sin_entity_ref_v1 weapon,int32_t kind,
    const sin_weapon_config_v1 *cfg,int vr) {
    if(!cb || !cfg || !cfg->viewmodel_name || kind<0 || kind>2 ||
       cb->version!=SIN_COMPAT_ABI_VERSION_1 || cb->struct_size!=sizeof(sin_host_callbacks_v1))return nullptr;
    if (!complete(*cb)) return nullptr;
    try {
        WeaponConfig w;w.viewmodel_name=cfg->viewmodel_name;
        w.origin_offset={cfg->origin_offset[0],cfg->origin_offset[1],cfg->origin_offset[2]};
        w.zoom_fov=cfg->zoom_fov;w.ammo.clip_size=cfg->clip_size;w.ammo.clip2_size=cfg->clip2_size;
        return new sin_session_v1(*cb,ctx,from_c(player),from_c(weapon),
                 static_cast<WeaponKind>(kind),std::move(w),vr!=0);
    } catch (...) {return nullptr;}
}
void sin_session_destroy_v1(sin_session_v1 *s){delete s;}
int32_t sin_session_ensure_viewmodel_v1(sin_session_v1 *s){if(!s)return -1;try{return static_cast<int32_t>(s->session.ensure_viewmodel());}catch(...){return -1;}}
int32_t sin_session_hide_weapon_v1(sin_session_v1 *s){if(!s)return -1;try{return static_cast<int32_t>(s->session.on_hide_weapon_input());}catch(...){return -1;}}
int32_t sin_session_usercmd_v1(sin_session_v1 *s,uint32_t b,int block){if(!s)return -1;try{auto r=s->session.on_usercmd(b,block!=0);return (int)r.entered_zoom | ((int)r.exited_zoom<<1) | ((int)r.automatically_restored<<2) | ((int)r.was_interrupted<<3);}catch(...){return -1;}}
int32_t sin_session_reload_begin_v1(sin_session_v1 *s,int32_t a,int32_t b){if(!s)return -1;try{return s->session.on_reload_begin(a,b);}catch(...){return -1;}}
int32_t sin_session_reload_event_v1(sin_session_v1 *s,int32_t *p,int32_t *q){if(!s||!p||!q)return -1;try{const auto t=s->session.on_reload_transfer_event();*p=t.primary;*q=t.secondary;return 0;}catch(...){return -1;}}
int32_t sin_session_reload_end_v1(sin_session_v1 *s){if(!s)return -1;try{s->session.on_reload_end();return 0;}catch(...){return -1;}}
int32_t sin_session_set_ammo_v1(sin_session_v1 *s,sin_ammo_v1 a){if(!s||a.clip<0||a.clip2<0||a.reserve<0||a.reserve2<0)return -1;try{auto &w=s->session.runtime().ammo();if(w.reload_active)return -1;w.clip=a.clip;w.clip2=a.clip2;w.reserve=a.reserve;w.reserve2=a.reserve2;return 0;}catch(...){return -1;}}
int32_t sin_session_get_ammo_v1(sin_session_v1 *s,sin_ammo_v1 *a){if(!s||!a)return -1;try{auto &w=s->session.runtime().ammo();*a={w.clip,w.clip2,w.reserve,w.reserve2};return 0;}catch(...){return -1;}}
int32_t sin_session_holster_v1(sin_session_v1 *s){if(!s)return -1;try{return s->session.on_holster();}catch(...){return -1;}}
int32_t sin_session_cleanup_v1(sin_session_v1 *s){if(!s)return -1;try{s->session.on_player_cleanup();return 0;}catch(...){return -1;}}
}
