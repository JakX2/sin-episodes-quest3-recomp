#pragma once
// Completely fabricated, independently authored SOURCE-INTERFACE-SHAPED stub.
// NOT a copy of Valve headers or a real SDK build; compile contract only.
enum Activity { ACT_INVALID = -1 };
class CBaseEntity {
public:
    virtual ~CBaseEntity() = default;
};
class CBaseCombatWeapon;
class CBaseCombatCharacter : public CBaseEntity {
public:
    CBaseCombatWeapon* active_weapon = nullptr;
    CBaseCombatWeapon* GetActiveWeapon() const { return active_weapon; }
};
