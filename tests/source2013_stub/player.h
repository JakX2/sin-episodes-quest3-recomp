#pragma once
#include "cbase.h"
#include "baseviewmodel.h"
class CBasePlayer : public CBaseCombatCharacter {
public:
    CBaseViewModel* viewmodel = nullptr;
    int requested_vm_index = -1;
    bool last_observer_flag = true;
    int set_fov_calls = 0;
    int last_fov = -1;
    float last_duration = -1;
    CBaseEntity* last_fov_requester = nullptr;
    CBaseViewModel* GetViewModel(int index = 0, bool observer = true) {
        requested_vm_index = index;
        last_observer_flag = observer;
        return index == 0 ? viewmodel : nullptr;
    }
    bool SetFOV(CBaseEntity* requester, int fov,
                float duration = 0.0f, int zoom_start = 0) {
        (void)zoom_start;
        ++set_fov_calls;
        last_fov = fov;
        last_duration = duration;
        last_fov_requester = requester;
        return true;
    }
};
