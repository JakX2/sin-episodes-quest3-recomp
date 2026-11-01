#pragma once
#include "cbase.h"
#include <string>
class CBaseCombatWeapon {
public:
    CBaseCombatCharacter* owner = nullptr;
    CBaseCombatCharacter* GetOwner() const { return owner; }
};
