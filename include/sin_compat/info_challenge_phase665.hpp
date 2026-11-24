#pragma once
// Original 2006 CInfoChallenge input/output dispatch and separately
// defined fixed-mode reporting policy. Engine entity creation, Source
// variant_t and COutputEvent remain host operations, never fabricated.
#include "fixed_difficulty_phase663.hpp"
#include <cmath>
#include <cstdint>
#include <string_view>

namespace sin_compat::difficulty::info_challenge_phase665 {
inline constexpr std::string_view kClassname="info_challenge";
inline constexpr std::uint32_t kOriginalObjectBytes=0x3e0u;
inline constexpr std::uint32_t kOriginalSnapshotOffset=0x3dcu;
inline constexpr std::uint32_t kOriginalInitialOutputOffset=0x394u;
inline constexpr std::uint32_t kOriginalCurrentOutputOffset=0x3acu;
inline constexpr std::uint32_t kOriginalDifferenceOutputOffset=0x3c4u;

enum class Input : std::uint8_t { Initial, Current, Difference };
enum class Code : std::uint8_t {
    Completed, UnsupportedInput, NoSnapshot, ProviderFailure,
    InvalidValue, OutputFailure, HostException, Reentrant
};
inline const char* output_name(Input input) noexcept {
    switch(input) {
        case Input::Initial:return "OnInitialChallenge";
        case Input::Current:return "OnCurrentChallenge";
        case Input::Difference:return "OnChallengeDifference";
    }
    return "";
}
inline std::uint32_t output_offset(Input input) noexcept {
    switch(input) {
        case Input::Initial:return kOriginalInitialOutputOffset;
        case Input::Current:return kOriginalCurrentOutputOffset;
        case Input::Difference:return kOriginalDifferenceOutputOffset;
    }
    return 0u;
}

class IChallengeValues {
public:
    virtual ~IChallengeValues()=default;
    // Distinct native source functions:
    // spawn snapshot 0x222D3630 (unscaled aggregate);
    // initial output 0x222BC4A0 singleton +8 (raw float);
    // current output 0x222D3730 (scaled aggregate).
    virtual bool read_spawn_value(float& value)=0;
    virtual bool read_initial_value(float& value)=0;
    virtual bool read_current_value(float& value)=0;
};
class IChallengeOutput {
public:
    virtual ~IChallengeOutput()=default;
    // The native output helper uses the info_challenge itself as BOTH
    // activator and caller, ignoring the inputdata_t activator/caller.
    virtual bool fire_float(std::uint32_t output_offset,
        std::string_view output_name,float value,
        std::uint32_t activator,std::uint32_t caller)=0;
};

struct Result {
    Code code=Code::UnsupportedInput;
    float value=0.f;
    std::uint32_t field_offset=0u;
};
struct Snapshot {
    float value=0.f;
    bool valid=false;
};

class InfoChallenge final {
public:
    InfoChallenge(std::uint32_t self,IChallengeValues& values,
            IChallengeOutput& output) noexcept
        :self_(self),values_(values),output_(output){}
    // Native original snapshot vslot +0x64 writes one float at this+0x3dc.
    // A successful call replaces any previous snapshot.
    Code capture_snapshot() noexcept {
        if(busy_)return Code::Reentrant;
        Guard guard(busy_);
        try {
            float value=0.f;
            if(!values_.read_spawn_value(value))return Code::ProviderFailure;
            if(!std::isfinite(value))return Code::InvalidValue;
            snapshot_={value,true};
            return Code::Completed;
        } catch(...) { return Code::HostException; }
    }
    Snapshot snapshot() const noexcept {return snapshot_;}
    // New host-owned archive boundary, NOT an original Source save format.
    bool restore_snapshot(Snapshot saved) noexcept {
        if(busy_ || (saved.valid && !std::isfinite(saved.value)))return false;
        snapshot_=saved;
        return true;
    }
    Result accept_input(std::string_view input) noexcept {
        if(input=="GetInitialChallenge")return accept(Input::Initial);
        if(input=="GetCurrentChallenge")return accept(Input::Current);
        if(input=="GetChallengeDifference")return accept(Input::Difference);
        return {};
    }
    Result accept(Input input) noexcept {
        Result r{};
        if(busy_){r.code=Code::Reentrant;return r;}
        r.field_offset=output_offset(input);
        if(!r.field_offset)return r;
        Guard guard(busy_);
        try {
            if(input==Input::Difference&&!snapshot_.valid){
                r.code=Code::NoSnapshot;return r;
            }
            const bool read=input==Input::Initial?
                values_.read_initial_value(r.value):
                values_.read_current_value(r.value);
            if(!read){r.code=Code::ProviderFailure;return r;}
            // Native x87 computes (current - stored_snapshot)*100 without
            // an intermediate f32 store. A double intermediate avoids
            // extra float32 rounding for ordinary finite input values.
            if(input==Input::Difference)
                r.value=static_cast<float>((static_cast<double>(r.value)-
                        static_cast<double>(snapshot_.value))*100.0);
            if(!std::isfinite(r.value)){
                r.code=Code::InvalidValue;return r;
            }
            if(!output_.fire_float(r.field_offset,output_name(input),
                                  r.value,self_,self_)){
                r.code=Code::OutputFailure;return r;
            }
            r.code=Code::Completed;
        }catch(...){r.code=Code::HostException;}
        return r;
    }
private:
    struct Guard {
        bool& active;
        explicit Guard(bool& a) noexcept:active(a){active=true;}
        ~Guard(){active=false;}
    };
    std::uint32_t self_;
    IChallengeValues& values_;
    IChallengeOutput& output_;
    Snapshot snapshot_{};
    bool busy_=false;
};

// New port policy, explicitly NOT the original adaptive aggregate units.
// All three raw input values are normalized to 0, 0.5, or 1 so a fixed
// skill cannot produce a spurious difference from mismatched units.
// Initial records game-start skill; current observes only explicit skill
// selection. Difference output retains native multiply-by-100 semantics.
// Maps querying GetInitial/GetCurrent get normalized values in this mode;
// do not install globally until real BSP output consumers are audited.
class FixedChallengeValues final:public IChallengeValues {
public:
    explicit FixedChallengeValues(const fixed_phase663::FixedDifficultyRuntime& p) noexcept
        :policy_(p),initial_(normalize(p.skill())){}
    static float normalize(int skill) noexcept {
        return skill==1?0.f:skill==3?1.f:0.5f;
    }
    bool read_spawn_value(float& v) override {
        v=normalize(policy_.skill());return true;
    }
    bool read_initial_value(float& v) override {
        v=initial_;return true;
    }
    bool read_current_value(float& v) override {
        v=normalize(policy_.skill());return true;
    }
private:
    const fixed_phase663::FixedDifficultyRuntime& policy_;
    float initial_;
};
} // namespace sin_compat::difficulty::info_challenge_phase665
