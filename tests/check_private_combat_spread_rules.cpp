// Optional source-data comparison utility, NOT part of automatic CI.
// Reads the player's locally installed rules_combat.txt. Never redistribute it.
#include "sin_compat/combat_spread_rules.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
using namespace sin_compat;
int main(int argc,char**argv) {
    if(argc!=2) {
        std::cerr<<"usage: sin_combat_spread_private_audit /private/rules_combat.txt\n";
        return 2;
    }
    std::ifstream f(argv[1]);
    if(!f) return 3;
    const std::string text((std::istreambuf_iterator<char>(f)),{});
    CombatSpreadRules rules;
    std::string error;
    if(!rules.load(text,&error)) {std::cerr<<error<<'\n';return 1;}
    const struct {
        const char* attacker;
        const char* weapon;
        const char* target;
        double expected;
    } expected[]={
        {"player","weapon_magnum","npc_sintek_grunt",0.0},
        {"npc_sintek_grunt","weapon_assault_rifle","player",0.13053},
        {"npc_sintek_grunt","weapon_assault_rifle","npc_druglab_grunt",0.04362},
        {"npc_druglab_grunt_assault_rifle","weapon_assault_rifle","player",0.08716},
        {"npc_druglab_grunt_scattergun","weapon_scattergun","player",0.08716},
        {"npc_druglab_jetpack","weapon_assault_rifle","player",0.11320},
        {"npc_druglab_grunt_elite","weapon_chaingun","player",0.11320},
    };
    for(const auto& row:expected) {
        const auto found=rules.lookup(row.attacker,row.weapon,row.target);
        if(!found || found.spread!=row.expected) {
            std::cerr<<"unexpected rule for "<<row.attacker<<" -> "<<row.target<<'\n';
            return 1;
        }
    }
    std::cout<<"7 owner-file combat spread rules verified; no content copied.\n";
}
