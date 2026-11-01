#pragma once
#include "cbase.h"
#include "basecombatweapon.h"
#include <string>
class CBaseViewModel : public CBaseEntity {
public:
    CBaseCombatWeapon* associated_weapon = nullptr;
    int select_calls = 0;
    int model_calls = 0;
    int play_calls = 0;
    int last_activity = -1;
    int last_sequence = -1;
    bool sequence_available = true;
    std::string bound_model;
    int SelectWeightedSequence(Activity activity) {
        ++select_calls;
        last_activity = static_cast<int>(activity);
        return sequence_available ? last_activity + 200 : -1;
    }
    CBaseCombatWeapon* GetWeapon() const { return associated_weapon; }
    void SetWeaponModel(const char* path, CBaseCombatWeapon* weapon) {
        ++model_calls;
        bound_model = path;
        associated_weapon = weapon;
    }
    void SendViewModelMatchingSequence(int id) {
        ++play_calls;
        last_sequence = id;
    }
};
