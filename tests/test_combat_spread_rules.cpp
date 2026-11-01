#include "sin_compat/combat_spread_rules.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string>
using namespace sin_compat;
int main() {
    // Entirely synthetic fixture; the owner-held game script is NOT checked in.
    const char* synthetic=R"(// Synthetic same hierarchy as shipped game data
    "AttackSpreadRules"
    {
      "class_names" {
         "player" "player_rule"
         "npc_sintek_grunt" "standard_rule"
         "npc_druglab_grunt_elite" "elite_rule"
      }
      "player_rule" {
        "weapon_class_names" { "*" "general" }
        "general" { "target_class_names" { "*" "all" } "all" "0.0" }
      }
      "standard_rule" {
        "weapon_class_names" { "weapon_special" "special" "*" "general" }
        "general" { "target_class_names" { "player" "aim_player" "*" "aim_other" }
          "aim_player" "0.13053" "aim_other" "0.04362" }
        "special" { "target_class_names" { "player" "aim_player" "*" "aim_other" }
          "aim_player" "0.08716" "aim_other" "0.04362" }
      }
      "elite_rule" { "weapon_class_names" { "*" "general" }
        "general" { "target_class_names" { "player" "aim_player" "*" "aim_other" }
          "aim_player" "0.11320" "aim_other" "0.04362" }
      }
    })";
    CombatSpreadRules rules;
    std::string error;
    assert(!rules.loaded());
    assert(!rules.lookup("player","weapon","enemy"));
    assert(rules.load(synthetic,&error));
    assert(error.empty() && rules.loaded());
    auto lookup=[&](const char* a,const char* w,const char* t){return rules.lookup(a,w,t);};
    assert(lookup("player","anything","npc_sintek_grunt").spread==0.0);
    assert(lookup("npc_sintek_grunt","weapon_smg","player").spread==0.13053);
    assert(lookup("npc_sintek_grunt","weapon_smg","npc_jessica").spread==0.04362);
    assert(lookup("npc_sintek_grunt","weapon_special","player").spread==0.08716);
    assert(lookup("npc_druglab_grunt_elite","weapon_chaingun","player").spread==0.11320);
    assert(lookup("npc_missing","weapon","player").status==SpreadLookup::Status::NoActorRule);
    assert(!rules.load("AttackSpreadRules { class_names { player x player y } }",&error));
    assert(error=="duplicate key" && !rules.loaded());
    assert(!rules.load("AttackSpreadRules { class_names { player x }",&error));
    assert(!rules.load("AttackSpreadRules {} extra",&error));
    assert(!rules.load("wrong { class_names {} }",&error));
    assert(!rules.load("AttackSpreadRules { class_names { \"bad\\q\" one } }",&error));
    assert(!rules.load(std::string(262145,'x'),&error));
    assert(rules.load(R"("AttackSpreadRules" { "class_names" { "npc" "rule" }
      "rule" { "weapon_class_names" { "*" "general" }
        "general" { "target_class_names" { "*" "other" } "other" "NaN" } }
    })",&error));
    assert(rules.lookup("npc","weapon","player").status==SpreadLookup::Status::InvalidValue);
}
