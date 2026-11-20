// Phase 499 native independent C_SoundscapeSystem original 2006 object
// constructor/slot7 field ownership. No 2006 original code executed.
#include "client_soundscape_original_object_ctor.h"
#include <cstdint>
#include <cstdio>

namespace o=sin_independent::soundscape_original_object;
static int Fail(int n) {
    std::fprintf(stderr,"original soundscape known object field fixture failed %d\n",n);
    return n;
}
int main(){
    static_assert(o::kOriginalCtorBytes==120);
    static_assert(o::kOriginalVtableSlots.size()==12);
    static_assert(o::kOriginalVtableSlots[0]==0x240F4160u);
    static_assert(o::kOriginalVtableSlots[1]==0x240F3EE0u);
    static_assert(o::kOriginalVtableSlots[3]==0x240F3C40u);
    static_assert(o::kOriginalVtableSlots[10]==0x240F4D30u);
    static_assert(o::kOwnedKnownDwords==50);
    o::KnownDwordModel state{};
    for(auto& value:state.words)value=0xCDCDCDCDu;
    o::ApplyOriginalConstructor(state);
    if(state.At(0)!=o::kOriginalVtableVA ||
       state.At(4)!=0xFFFFFFFFu ||
       state.At(0x30)!=o::kOriginalSubobjectVtableVA ||
       state.At(0x9C)!=0xFFFFFFFFu ||
       state.revision!=1 ||
       state.slot7_invocations!=0)
        return Fail(1);
    for(const auto offset:o::kOriginalEarlyZeroOffsets)
        if(state.At(offset)!=0)return Fail(2);
    for(const auto offset:o::kOriginalLaterZeroOffsets)
        if(state.At(offset)!=0)return Fail(3);
    // No unsupported invented field writes: fields not visibly
    // initialized by original 120B constructor remain opaque.
    for(const auto offset:{
        0x34u,0x40u,0x54u,0x64u,0x84u,0x94u,0x98u
    })if(state.At(offset)!=0xCDCDCDCDu)return Fail(4);
    if(state.At(0xC8u)!=0u || state.At(0x09u)!=0u)
        return Fail(5); // outside modeled span or unaligned access
    // Original 12-byte slot7 writes exactly this+4. It performs
    // no owner allocations or a source-only direct ConVar call.
    o::ApplyOriginalSlot7GlobalPlus4(state,0x12345678u);
    if(state.At(4)!=0x12345678u ||state.At(0)!=o::kOriginalVtableVA ||
       state.At(0x9C)!=0xFFFFFFFFu ||
       state.revision!=2 ||state.slot7_invocations!=1)return Fail(6);
    o::ApplyOriginalSlot7GlobalPlus4(state,0);
    if(state.At(4)!=0 ||state.revision!=3 ||
       state.slot7_invocations!=2)return Fail(7);
    if(!o::OriginalVtableSlotIsSingleRet(4) ||
       !o::OriginalVtableSlotIsSingleRet(6) ||
       !o::OriginalVtableSlotIsSingleRet(8) ||
       !o::OriginalVtableSlotIsSingleRet(9) ||
       o::OriginalVtableSlotIsSingleRet(0) ||
       o::OriginalVtableSlotIsSingleRet(7) ||
       o::OriginalVtableSlotIsSingleRet(10) ||
       o::OriginalVtableSlotIsSingleRet(11) ||
       o::OriginalVtableSlotIsSingleRet(12))return Fail(8);
    // A synthetic re-construction resets known constructor fields
    // and slot7 count without changing unsupported base fields.
    state.words[0xC4u/4u]=0xABu;
    state.words[0x94u/4u]=0x99112233u;
    o::ApplyOriginalConstructor(state);
    if(state.At(0xC4)!=0 ||
       state.At(0x94)!=0x99112233u ||
       state.At(4)!=0xFFFFFFFFu ||
       state.slot7_invocations!=0 ||
       state.revision!=4)return Fail(9);
    // Original slot10's first instructions refuse expensive work
    // when +0xD0 BYTE==0; only then read the required global pointer.
    // This test intentionally does NOT call the remaining update body.
    if(o::CheckOriginalSlot10EarlyGate(
           false,false)!=o::Slot10EarlyGate::SkippedObjectFlagZero ||
       o::CheckOriginalSlot10EarlyGate(
           false,true)!=o::Slot10EarlyGate::SkippedObjectFlagZero ||
       o::CheckOriginalSlot10EarlyGate(
           true,false)!=o::Slot10EarlyGate::SkippedGlobalMissing ||
       o::CheckOriginalSlot10EarlyGate(
           true,true)!=o::Slot10EarlyGate::EnterUnreconstructedUpdate)
        return Fail(10);
    static_assert(o::kOriginalSlot10VA==0x240F4D30u);
    static_assert(o::kOriginalSlot10FlagOffset==0xD0u);
    static_assert(o::kOriginalSlot10GlobalVA==0x24338CF0u);
    // Exhaustive 50-DWORD write-set oracle, not just sampled opaque offsets.
    // This detects accidental clobbering of any presently unknown field.
    for(std::uint32_t seed=0;seed<64u;++seed){
        o::KnownDwordModel exhaustive{};
        for(std::size_t i=0;i<exhaustive.words.size();++i)
            exhaustive.words[i]=0xA5A50000u ^
                (seed*0x01010101u) ^ static_cast<std::uint32_t>(i*0x9E3779B9u);
        const auto before=exhaustive.words;
        o::ApplyOriginalConstructor(exhaustive);
        for(std::size_t i=0;i<exhaustive.words.size();++i){
            const auto offset=static_cast<std::uint32_t>(i*4u);
            std::uint32_t expected=before[i];
            if(offset==0u)expected=o::kOriginalVtableVA;
            if(offset==4u || offset==0x9Cu)expected=o::kUninitializedSentinel;
            if(offset==0x30u)expected=o::kOriginalSubobjectVtableVA;
            for(const auto zero:o::kOriginalEarlyZeroOffsets)
                if(offset==zero)expected=0u;
            for(const auto zero:o::kOriginalLaterZeroOffsets)
                if(offset==zero)expected=0u;
            if(exhaustive.words[i]!=expected)return Fail(11);
        }
        const auto constructed=exhaustive.words;
        o::ApplyOriginalSlot7GlobalPlus4(exhaustive,seed);
        for(std::size_t i=0;i<exhaustive.words.size();++i)
            if(exhaustive.words[i]!=(i==1u?seed:constructed[i]))
                return Fail(12);
        if(exhaustive.revision!=2u || exhaustive.slot7_invocations!=1u)
            return Fail(13);
        o::ApplyOriginalConstructor(exhaustive);
        if(exhaustive.revision!=3u || exhaustive.slot7_invocations!=0u ||
           exhaustive.At(4u)!=o::kUninitializedSentinel)
            return Fail(14);
    }
    std::puts("PASS 2006 original C_SoundscapeSystem 120-byte constructor: 12-source slots, exactly 20 zeroed DWORDs, original -1 fields, nested vtable, raw slot7 global+4 copy, opaque fields preserved");
    return 0;
}
