#include "sin_compat/original_client_sound_name.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string_view>
using namespace sin_compat::original_client_sound_name;

int main() {
    Pool pool;
    int resolverCalls=0;
    auto resolver=[&](std::string_view name) {
        ++resolverCalls;
        return name=="ambient/voice.script";
    };
    assert(HasLiteralFileToken("Audio/BLAST.WAV"));
    assert(HasLiteralFileToken("sound.Mp3_suffix"));
    assert(HasLiteralFileToken("prefix.wav.trailing"));
    assert(HasLiteralFileToken(".wav"));
    assert(!HasLiteralFileToken("ambient.ogg"));
    assert(!HasLiteralFileToken("ambience.wavless"));
    assert(!HasLiteralFileToken("mp3"));
    assert(!HasLiteralFileToken(""));
    assert(!ContainsAsciiFold("short",".wav"));
    const auto one=Bind(pool,"Audio/BLAST.WAV","ui_string",resolver);
    assert(one.route==Route::LiteralFileToken);
    assert(one.internedName && one.internedAuxiliary);
    assert(resolverCalls==0);
    assert(std::strcmp(one.internedName,"Audio/BLAST.WAV")==0);
    const auto same=Bind(pool,"audio/blast.wav","UI_STRING",resolver);
    assert(same.route==Route::LiteralFileToken);
    assert(same.internedName==one.internedName);
    assert(same.internedAuxiliary==one.internedAuxiliary);
    assert(pool.Size()==2 && resolverCalls==0);
    const auto found=Bind(pool,"ambient/voice.script","metadata",resolver);
    assert(found.route==Route::EmitterResolved && found.emitterCalled &&
           found.emitterAccepted && resolverCalls==1);
    const auto denied=Bind(pool,"ambient/other.script","metadata",resolver);
    assert(denied.route==Route::EmitterRejected && denied.emitterCalled &&
           !denied.emitterAccepted && resolverCalls==2);
    assert(denied.internedName && pool.Find("AMBIENT/OTHER.SCRIPT")==denied.internedName);
    assert(denied.internedAuxiliary==found.internedAuxiliary);
    const auto mp3=Bind(pool,"audio-track.MP3.old","",resolver);
    assert(mp3.route==Route::LiteralFileToken && !mp3.emitterCalled &&
           mp3.internedAuxiliary==nullptr && resolverCalls==2);
    const char embedded[]={'a','b','\0','c'};
    auto invalid=Bind(pool,std::string_view(embedded,4),"bad",resolver);
    assert(invalid.route==Route::InvalidInput && !invalid.internedName &&
           resolverCalls==2);
    invalid=Bind(pool,"","",resolver);
    assert(invalid.route==Route::InvalidInput && resolverCalls==2);
    invalid=Bind(pool,"ok",std::string_view(embedded,4),resolver);
    assert(invalid.route==Route::InvalidInput && resolverCalls==2);
    assert(pool.Size()==5);
    // Pool pointers must survive insertion, but never be used after Clear.
    for(int i=0;i<320;++i) {
        auto r=Bind(pool,"other.wav"+std::to_string(i),"",resolver);
        assert(r.route==Route::LiteralFileToken && r.internedName);
    }
    assert(std::strcmp(one.internedName,"Audio/BLAST.WAV")==0);
    pool.Clear();
    assert(pool.Size()==0 && pool.Find("audio/blast.wav")==nullptr);
    const auto renewed=Bind(pool,"audio/blast.wav","",resolver);
    assert(renewed.route==Route::LiteralFileToken &&
           std::strcmp(renewed.internedName,"audio/blast.wav")==0);
    std::puts("PASS source sound-name route: ASCII Q_stristr tokens, "
              "emitter fallback, first-spelling pool ownership and clear");
}
