#include "sin_compat/legacy_sound_vgui_prebridge.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <array>
#include <cstring>
#include <string>
using namespace sin_compat::phase99;
int main() {
    static_assert(original_key_count == 256);
    static_assert(original_key_nonzero_count == 99);
    static_assert(old_name_to_token_slot == 3 && native_name_to_token_slot == 7);
    static_assert(old_token_to_name_slot == 6 && native_token_to_name_slot == 10);
    LegacyKeyTranslator keys;
    assert(!keys.ready());
    assert(keys.translate(48) == 0);
    std::array<std::uint32_t,256> mock{};
    for (std::size_t i=0; i<99; ++i) mock[i]=static_cast<std::uint32_t>(i+1);
    assert(!keys.install(mock,false));
    assert(keys.install(mock,true));
    assert(keys.ready());
    assert(keys.translate(-1)==0 && keys.translate(256)==0);
    assert(keys.translate(0)==1 && keys.translate(98)==99 && keys.translate(99)==0);
    auto duplicate=mock; duplicate[1]=1;
    assert(!keys.install(duplicate,true));
    assert(!keys.ready());
    assert(keys.install(mock,true));
    keys.reset();
    assert(!keys.ready() && keys.translate(2)==0);

    VerifiedSoundTokenCrosswalk bridge;
    using R=CrosswalkResult;
    assert(bridge.bind(-1,300,10,true,"door","door")==R::LegacyIndexInvalid);
    assert(bridge.bind(300,300,10,true,"door","door")==R::LegacyIndexInvalid);
    assert(bridge.bind(2,300,-1,true,"door","door")==R::NativeIndexInvalid);
    assert(bridge.bind(2,300,65535,true,"door","door")==R::NativeIndexInvalid);
    assert(bridge.bind(2,300,65536,true,"door","door")==R::NativeIndexInvalid);
    assert(bridge.bind(2,300,10,false,"door","door")==R::NativeValidationFailed);
    assert(bridge.bind(2,300,10,true,"door","Door")==R::NameMismatch);
    assert(bridge.bind(2,300,10,true,"door","")==R::NameMismatch);
    assert(bridge.bind(2,300,10,true,"door","door")==R::Added);
    assert(bridge.bind(2,300,10,true,"door","door")==R::AlreadyPresent);
    assert(bridge.bind(2,300,11,true,"door","door")==R::MappingConflict);
    assert(bridge.bind(3,300,10,true,"wind","wind")==R::MappingConflict);
    assert(bridge.bind(3,300,11,true,"door","door")==R::MappingConflict);
    const char* stable=bridge.name_from_old_token(2);
    assert(std::strcmp(stable,"door")==0);
    assert(bridge.native_from_old_token(2)==10);
    assert(bridge.old_from_native_token(10)==2);
    for(int i=3;i<200;i++) {
      auto r=bridge.bind(i,300,i+20,true,"n"+std::to_string(i),"n"+std::to_string(i));
      assert(r==R::Added);
    }
    assert(std::strcmp(stable,"door")==0);
    assert(bridge.size()==198);
    assert(bridge.name_from_old_token(555)[0]=='\0');
    assert(bridge.native_from_old_token(555)==missing_token);
    assert(bridge.old_from_native_token(65535u)==missing_token);
    assert(bridge.bind(299,300,65534,true,"limit","limit")==R::Added);
    bridge.reset();assert(bridge.size()==0 && bridge.native_from_old_token(2)==missing_token);
}
