#pragma once
// Original 2006 client.dll stat-name registry, SHA-pin:
// 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
// Owner-derived DATA identifiers, not copied engine code or runnable retail ABI.
// Original registry 0x241FFB60 iterates 0x42 ordered string pointers at RVA
// 0x31E428. Each insertion stores EBX (loop 0..65) as the stat ID.
// Independent host lookup deliberately requires exact spelling.
// Four compiled identifiers are not present in the original scripts.
#include <array>
#include <cstddef>
#include <string_view>
namespace sin_independent::stat_name_ids {
inline constexpr std::array<std::string_view,66> kOriginalIds = {{
    "STAT_TOTAL_TIME",
    "STAT_ABSOLUTE_TIME",
    "STAT_EXPECTED_TIME",
    "STAT_TOTAL_TIME_IN_COMBAT",
    "STAT_AVG_TIME_LENGTH_IN_COMBAT",
    "STAT_TOTAL_SHOTS",
    "STAT_TOTAL_SHOTS_IN_COMBAT",
    "STAT_TOTAL_PRIMARY_SHOTS",
    "STAT_TOTAL_SECONDARY_SHOTS",
    "STAT_TOTAL_WEAPON_ACCURACY",
    "STAT_TOTAL_WEAPON_ACCURACY_IN_COMBAT",
    "STAT_NUM_GRENADES_THROWN",
    "STAT_NUM_MELEES",
    "STAT_TOTAL_RELOADS",
    "STAT_TOTAL_HEADSHOTS",
    "STAT_TOTAL_HEADSHOTS_IN_COMBAT",
    "STAT_TOTAL_DAMAGE_TAKEN",
    "STAT_TOTAL_HEALTH_GAINED",
    "STAT_AVG_HEALTH",
    "STAT_AVG_HEALTH_IN_COMBAT",
    "STAT_TOTAL_DEATHS_PER_MINUTE",
    "STAT_TOTAL_DEATHS_PER_HOUR",
    "STAT_TOTAL_DEATHS",
    "STAT_TOTAL_REBIRTHS",
    "STAT_TOTAL_TIME_AT_FULL_HEALTH",
    "STAT_TOTAL_TIME_BELOW_25_HEALTH",
    "STAT_TOTAL_NPC_KILLS",
    "STAT_TOTAL_KILLS_PER_MINUTE",
    "STAT_TOTAL_KILLS_PER_MINUTE_IN_COMBAT",
    "STAT_TOTAL_DPS",
    "STAT_TOTAL_DPS_IN_COMBAT",
    "STAT_DPS_TAKEN_IN_COMBAT",
    "STAT_TOTAL_JUMPS",
    "STAT_TOTAL_JUMPS_IN_COMBAT",
    "STAT_TOTAL_DUCKS",
    "STAT_TOTAL_DUCKS_IN_COMBAT",
    "STAT_TOTAL_SAVES",
    "STAT_TOTAL_LOADS",
    "STAT_NUM_FOOT_STEPS",
    "STAT_NUM_PAUSES",
    "STAT_NUM_ITEMS_USED",
    "STAT_NUM_NPCS_CAUGHT_ON_FIRE",
    "STAT_NUM_NPC_HELMETS_SHOT_OFF",
    "STAT_NUM_NPCS_DECAPITATED",
    "STAT_NUM_CONTAINERS_EXPLODED",
    "STAT_MOST_KILLS_BEFORE_DEATH",
    "STAT_NUM_SECRETS",
    "STAT_NUM_ITEM_CRATES_BROKEN",
    "STAT_CHALLENGE_RATING",
    "STAT_INSTANT_CHALLENGE_RATING",
    "STAT_MOST_KILLS_BEFORE_DAMAGE",
    "STAT_NUM_DD_TRIGGERS",
    "STAT_NUM_ITEMS_PICKED_UP",
    "STAT_NUM_ITEMS_USED_GENERIC",
    "STAT_NUM_FLAILS",
    "STAT_WEAPON_ACCURACY",
    "STAT_WEAPON_HEADSHOTS",
    "STAT_WEAPON_PRIMARY_SHOTS",
    "STAT_WEAPON_SECONDARY_SHOTS",
    "STAT_WEAPON_ACCURACY_IN_COMBAT",
    "STAT_NPC_KILLS",
    "STAT_WEAPON_NPC_KILLS",
    "STAT_LEVEL_KILLS",
    "STAT_LEVEL_DEATHS",
    "STAT_LEVEL_SECRETS",
    "STAT_GAME_PLAY_VAR_AVERAGES",
}};
inline constexpr std::size_t kCount=kOriginalIds.size();
inline constexpr std::string_view NameForId(std::size_t id) noexcept {
    return id<kCount?kOriginalIds[id]:std::string_view{};
}
inline constexpr int Lookup(std::string_view name) noexcept {
    for(std::size_t i=0;i<kCount;++i)
        if(kOriginalIds[i]==name)return static_cast<int>(i);
    return -1;
}
static_assert(kCount==66,"original 0x42 stat names");
static_assert(Lookup("STAT_TOTAL_TIME")==0,"first original stat ID");
static_assert(Lookup("STAT_EXPECTED_TIME")==2,"unlisted source stat still has ID");
static_assert(Lookup("STAT_NUM_FLAILS")==54,"compiled-only stat retains native ID");
static_assert(Lookup("STAT_GAME_PLAY_VAR_AVERAGES")==65,"last original stat ID");
} // namespace sin_independent::stat_name_ids
