#define SIN_SCENE_C_API_EXPORT
#include "sin_compat/scene_actor_c_api.h"
#include "sin_compat/scene_actor_bridge.hpp"
#include <type_traits>
#include <new>
#include <string>
#include <cstdint>

namespace {
using namespace sin_compat;
static_assert(static_cast<int>(SceneActorResult::Ok) == SIN_SCENE_OK);
static_assert(static_cast<int>(SceneActorResult::InvalidState) == SIN_SCENE_INVALID_STATE);
static_assert(static_cast<int>(SceneActorResult::InvalidArgument) == SIN_SCENE_INVALID_ARGUMENT);
static_assert(static_cast<int>(SceneActorResult::HostFailed) == SIN_SCENE_HOST_FAILED);
static_assert(static_cast<int>(SceneActorResult::UnsupportedInput) == SIN_SCENE_UNSUPPORTED_INPUT);
static_assert(static_cast<int>(SceneActorResult::WrongInputType) == SIN_SCENE_WRONG_INPUT_TYPE);
static_assert(static_cast<int>(SceneActorResult::NoPendingThrow) == SIN_SCENE_NO_PENDING_THROW);
static_assert(sizeof(SceneActorRef) == sizeof(sin_scene_actor_ref_v1));

sin_scene_actor_ref_v1 from_cpp(SceneActorRef x) noexcept { return {x.index, x.generation}; }
SceneActorRef from_c(sin_scene_actor_ref_v1 x) noexcept { return {x.index, x.generation}; }

bool complete(const sin_scene_callbacks_v1 &cb) noexcept {
    return cb.version == SIN_SCENE_ABI_VERSION_1 && cb.struct_size == sizeof(cb) &&
        cb.create_from_existing_base && cb.set_model && cb.set_health &&
        cb.set_targetname && cb.choreography_capable && cb.play_scene &&
        cb.holster_player_weapon && cb.throw_item_at_player && cb.remove_actor;
}

struct CSceneHost final : ISceneActorHost {
    explicit CSceneHost(const sin_scene_callbacks_v1 &c, void *u) : cb(c), userdata(u) {}
    SceneActorRef create_from_existing_base(const char *base, const char *original) override {
        return from_c(cb.create_from_existing_base(userdata,base,original));
    }
    bool set_model(SceneActorRef r,const char *model) override {
        return cb.set_model(userdata,from_cpp(r),model)!=0;
    }
    bool set_health(SceneActorRef r,int hp) override {
        return cb.set_health(userdata,from_cpp(r),hp)!=0;
    }
    bool set_targetname(SceneActorRef r,const char *name) override {
        return cb.set_targetname(userdata,from_cpp(r),name)!=0;
    }
    bool choreography_capable(SceneActorRef r) override {
        return cb.choreography_capable(userdata,from_cpp(r))!=0;
    }
    bool play_scene(SceneActorRef r,const char *file,const char *token) override {
        return cb.play_scene(userdata,from_cpp(r),file,token)!=0;
    }
    bool holster_player_weapon() override { return cb.holster_player_weapon(userdata)!=0; }
    bool throw_item_at_player(SceneActorRef r,const char *item,int primary,int secondary) override {
        return cb.throw_item_at_player(userdata,from_cpp(r),item,primary,secondary)!=0;
    }
    void remove_actor(SceneActorRef r) override { cb.remove_actor(userdata,from_cpp(r)); }
    const sin_scene_callbacks_v1 cb;
    void *const userdata;
};
}

struct sin_scene_session_v1 {
    bool in_api = false;
    CSceneHost host;
    sin_compat::SceneActorBridge bridge;
    sin_scene_session_v1(const sin_scene_callbacks_v1 &cb,void *ctx,sin_compat::SceneActorKind kind)
      : host(cb,ctx),bridge(host,kind){}
};
namespace {
class CallGuard {
public:
    explicit CallGuard(sin_scene_session_v1 *s) noexcept : s_(s) {
        if(s && !s->in_api) {s->in_api=true; valid_=true;}
    }
    ~CallGuard() noexcept { if(valid_)s_->in_api=false; }
    explicit operator bool() const noexcept {return valid_;}
    CallGuard(const CallGuard&)=delete;
    CallGuard& operator=(const CallGuard&)=delete;
private:
    sin_scene_session_v1 *s_ = nullptr;
    bool valid_ = false;
};
}
extern "C" {
uint32_t sin_scene_abi_version_v1(void){return SIN_SCENE_ABI_VERSION_1;}
sin_scene_session_v1 *sin_scene_create_v1(const sin_scene_callbacks_v1 *cb,void *ctx,int32_t kind) {
    if(!cb || !complete(*cb) || kind<SIN_SCENE_ACTOR_JC || kind>SIN_SCENE_ACTOR_GENERIC)return nullptr;
    try {
        return new sin_scene_session_v1(*cb,ctx,static_cast<sin_compat::SceneActorKind>(kind));
    }catch(...){return nullptr;}
}
void sin_scene_destroy_v1(sin_scene_session_v1 *s) {
    if(s && !s->in_api)delete s;
}
int32_t sin_scene_spawn_v1(sin_scene_session_v1 *s,const char *targetname,const char *model_override){
    CallGuard g(s);if(!g)return -1;
    try{
        sin_compat::SceneActorSpawnOptions opts{targetname?targetname:"",model_override?model_override:""};
        return static_cast<int32_t>(s->bridge.spawn(opts));
    }catch(...){return -1;}
}
int32_t sin_scene_begin_v1(sin_scene_session_v1 *s,const char *file,const char *token){
    CallGuard g(s);if(!g)return -1;
    if(!file || !token)return SIN_SCENE_INVALID_ARGUMENT;
    try{return static_cast<int32_t>(s->bridge.begin_scene(file,token));}catch(...){return -1;}
}
int32_t sin_scene_input_v1(sin_scene_session_v1 *s,const char *name,int32_t type,
                            const char *str,int32_t value){
    CallGuard g(s);if(!g)return -1;
    if(!name || type<SIN_SCENE_INPUT_VOID || type>SIN_SCENE_INPUT_INTEGER)return SIN_SCENE_INVALID_ARGUMENT;
    if(type==SIN_SCENE_INPUT_STRING && !str)return SIN_SCENE_INVALID_ARGUMENT;
    try{
        sin_compat::SceneActorInput in;
        in.type=static_cast<sin_compat::SceneActorInput::Type>(type);
        in.integer_value=value;
        if(type==SIN_SCENE_INPUT_STRING)in.string_value=str;
        return static_cast<int32_t>(s->bridge.input(name,in));
    }catch(...){return -1;}
}
int32_t sin_scene_animation_event_v1(sin_scene_session_v1 *s,const char *symbol){
    CallGuard g(s);if(!g)return -1;
    if(!symbol)return SIN_SCENE_INVALID_ARGUMENT;
    try{return static_cast<int32_t>(s->bridge.animation_event(symbol));}catch(...){return -1;}
}
int32_t sin_scene_get_actor_v1(sin_scene_session_v1 *s,sin_scene_actor_ref_v1 *out){
    CallGuard g(s);if(!g)return -1;
    if(!out)return SIN_SCENE_INVALID_ARGUMENT;
    *out=from_cpp(s->bridge.actor());
    return (out->index && out->generation)?SIN_SCENE_OK:-1;
}
}