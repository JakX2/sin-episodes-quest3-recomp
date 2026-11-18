#pragma once
// Independently authored shared protocol metadata. Names, order and sizes
// are facts authenticated against both owner-original game DLLs.
// Not a decoder, dispatcher, or implementation of the message payloads.
#include <cstddef>
#include <cstring>
namespace sin_independent {
struct UserMessageSpec {const char* name;int bytes;};
inline constexpr UserMessageSpec kUserMessages[]={
    {"Train",1}, // 0
    {"HudText",-1}, // 1
    {"SayText",-1}, // 2
    {"TextMsg",-1}, // 3
    {"HudMsg",-1}, // 4
    {"ResetHUD",1}, // 5
    {"GameTitle",0}, // 6
    {"ItemPickup",-1}, // 7
    {"ShowMenu",-1}, // 8
    {"Shake",13}, // 9
    {"Fade",10}, // 10
    {"VGUIMenu",-1}, // 11
    {"Damage",18}, // 12
    {"VoiceMask",9}, // 13
    {"RequestState",0}, // 14
    {"CloseCaption",6}, // 15
    {"HintText",-1}, // 16
    {"SquadMemberDied",0}, // 17
    {"AmmoDenied",2}, // 18
    {"AmmoPickup",4}, // 19
    {"ArenaMode",1}, // 20
    {"ArenaUpdate",12}, // 21
    {"DynamicDecision",2}, // 22
    {"CreditsMsg",1}, // 23
    {"HUDCamera",2}, // 24
    {"NewGameInfo",-1}, // 25
    {"Cheating",1}, // 26
    {"StaticStat",-1}, // 27
    {"DynamicStat",-1}, // 28
    {"GamePlayVarValue",6}, // 29
    {"Progression",4}, // 30
    {"DDInfo",-1}, // 31
    {"DDAdvisorScores",-1}, // 32
    {"DDAdvisorChannels",-1}, // 33
    {"DDAdvisorsUpdate",-1}, // 34
    {"DDDecisionMade",-1}, // 35
    {"DDDecisionsPerf",-1}, // 36
    {"DDSuccessModifiers",-1}, // 37
    {"PoliceRadioMessage",-1}, // 38
};
inline constexpr std::size_t kUserMessageCount=sizeof(kUserMessages)/sizeof(kUserMessages[0]);
static_assert(kUserMessageCount==39,"original shared protocol count");
inline int FindUserMessage(const char* name) noexcept {
    if(!name)return -1;
    for(std::size_t i=0;i<kUserMessageCount;++i)
        if(std::strcmp(name,kUserMessages[i].name)==0)return static_cast<int>(i);
    return -1;
}
// Original GetUserMessageInfo returns false for invalid indices without
// modifying outputs. Positive buffer lengths use bounded, NUL-terminated
// copying (Q_strncpy semantics). Null/nonpositive buffers are additionally
// refused by the independent implementation.
inline bool GetUserMessageInfo(int index,char* name,int capacity,int& bytes) noexcept {
    if(index<0 || static_cast<std::size_t>(index)>=kUserMessageCount ||
       !name || capacity<=0)return false;
    const auto& spec=kUserMessages[index];
    auto length=std::strlen(spec.name);
    const auto limit=static_cast<std::size_t>(capacity-1);
    if(length>limit)length=limit;
    std::memcpy(name,spec.name,length);
    name[length]='\0';
    bytes=spec.bytes;
    return true;
}
} // namespace sin_independent
