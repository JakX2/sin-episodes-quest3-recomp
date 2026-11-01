#include "sin_compat/dynamic_difficulty_activation.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <limits>
using namespace sin_compat;
struct Setting : IGameplayDifficultySettingSource {
    bool ok = true;
    float value = 0.25f;
    bool read_setting(float& out) const override { out=value; return ok; }
};
int main() {
    Setting setting;
    GameplayDifficultyVariables model;
    GameplayDifficultyActivation activation(model,setting);
    assert(activation.initialize_from_setting()==GameplayActivationStatus::InvalidModel);
    assert(!activation.initialized());
    assert(model.configure({{"damage_scale",0.5f,0,10,0,10},
                            {"ammo",1,1.5f,0.5f,1.5f,0.5f}}));
    assert(activation.initialize_from_setting()==GameplayActivationStatus::Applied);
    assert(activation.initialized() && activation.cached_setting()==0.25f);
    assert(model.find("damage_scale")->current==2.5f);
    assert(model.find("ammo")->current==1.25f);
    assert(activation.restore_if_uninitialized()==GameplayActivationStatus::AlreadyInitialized);
    assert(model.find("damage_scale")->current==2.5f);
    setting.value=0.75f;
    assert(activation.initialize_from_setting()==GameplayActivationStatus::Applied);
    assert(activation.cached_setting()==0.75f);
    assert(model.find("damage_scale")->current==7.5f);
    assert(model.find("ammo")->current==0.75f);
    assert(activation.seed_cached_state(0.4f,false));
    setting.value=0.95f;
    assert(activation.restore_if_uninitialized()==GameplayActivationStatus::Applied);
    assert(activation.restore_half_marker()==0.5f);
    assert(activation.cached_setting()==0.4f);
    assert(model.find("damage_scale")->current==4.0f);
    assert(model.find("ammo")->current==1.1f);
    assert(activation.restore_if_uninitialized()==GameplayActivationStatus::AlreadyInitialized);
    setting.ok=false;
    assert(activation.initialize_from_setting()==GameplayActivationStatus::MissingSetting);
    assert(activation.cached_setting()==0.4f);
    setting.ok=true;
    setting.value=std::numeric_limits<float>::quiet_NaN();
    assert(activation.initialize_from_setting()==GameplayActivationStatus::InvalidSetting);
    assert(!activation.seed_cached_state(INFINITY,false));
    assert(activation.initialized());
    assert(activation.seed_cached_state(1.0f,false));
    assert(activation.restore_if_uninitialized()==GameplayActivationStatus::Applied);
    assert(model.find("damage_scale")->current==10.0f);
    assert(model.configure({{"huge",1,3.0e38f,-3.0e38f,0,1}}));
    assert(activation.seed_cached_state(10.0f,false));
    assert(activation.restore_if_uninitialized()==GameplayActivationStatus::InterpolationFailed);
    assert(!activation.initialized());
}
