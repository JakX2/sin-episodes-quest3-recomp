#pragma once
// Original SiN Episodes (2006) CShortTermStatGate / Conjugate, clean-room
// C++17 behavior model from the owner's SHA-256-pinned server.dll.
// NOT an x86 class layout, thiscall adapter, or native engine game entity.
#include "sin_compat/original_stat_advisor_sample_runtime.hpp"

namespace sin_compat::difficulty {

// Original gates' +0x08 points to an engine-owned stat object.
// Vtable slot 2 (+0x08) produces an x87 float; slot 3 (+0x0C)
// consumes an incoming normalized float. No concrete Source ABI guessed.
struct IOriginalShortTermStatSource {
    virtual ~IOriginalShortTermStatSource() = default;
    virtual float read_slot2() = 0;
    virtual void write_slot3(float normalized_sample) = 0;
};

// RTTI: CShortTermStatGate at 0x22522CA0; vtable 0x2245E4A8.
// 2006 object offsets: scale +0x04, source pointer +0x08, cached
// scalar +0x0C, gain +0x10 (20-byte allocation). The fields below
// are *not* claimed to reside at those offsets in this C++ model.
class OriginalShortTermStatGate : public IOriginalAdvisorListener {
public:
    OriginalShortTermStatGate(IOriginalShortTermStatSource& stat,
                              float output_scale, float gain)
        : stat_(stat), output_scale_(output_scale), gain_(gain) {
        // Original constructor 0x222C6600 reads source slot2 immediately,
        // clamps gain*value to [0,1] and stores scale*clamped at +0x0C.
        refresh();
    }

    // Original shared gate vtable slot2 -> 0x222C6370. It stores the
    // incoming float at +0x0C, then calls source slot3 with
    // incoming/(gain*output_scale). Do not silently clamp this input.
    void forward_slot2(float incoming_sample) {
        cached_ = incoming_sample;
        stat_.write_slot3(incoming_sample / (gain_ * output_scale_));
    }

    // Advisor fanout 0x222C6750 invokes gate vtable slot3 (+0x0C).
    // The gate's original 0x222C63A0/0x222C6420 ignore the caller
    // float argument and instead re-read the underlying source slot2.
    void receive_sample(float ignored_sample) override {
        (void)ignored_sample;
        refresh();
    }

    // Original shared slot1 -> 0x222C6670: FLD DWORD PTR [this+0x0C].
    float read_current_value() override { return cached_; }

    void refresh() {
        float normalized = stat_.read_slot2() * gain_;
        // Original x87 FCOM/FNSTSW clamps ordered values. Unordered
        // NaN propagates (unlike an unconditional NaN-to-zero policy).
        if (normalized > 1.0f)
            normalized = 1.0f;
        else if (normalized < 0.0f)
            normalized = 0.0f;
        cached_ = output_scale_ * transform(normalized);
    }

    virtual ~OriginalShortTermStatGate() = default;

protected:
    virtual float transform(float normalized) const noexcept {
        return normalized;
    }

private:
    IOriginalShortTermStatSource& stat_;
    float output_scale_;
    float gain_;
    float cached_ = 0.0f;
};

// RTTI: CShortTermStatGateConjugate at 0x22522CC4;
// primary vtable 0x2245E4C8. Same slot1/slot2, but its slot3
// 0x222C6420 caches scale*(1-clamp(gain*source.slot2())).
// Constructor 0x222C66A0 calls base constructor *then* refreshes
// under the conjugate vtable, so source slot2 is read twice.
class OriginalShortTermStatGateConjugate final
    : public OriginalShortTermStatGate {
public:
    OriginalShortTermStatGateConjugate(IOriginalShortTermStatSource& stat,
                                      float output_scale, float gain)
        : OriginalShortTermStatGate(stat, output_scale, gain) {
        refresh();
    }

protected:
    float transform(float normalized) const noexcept override {
        return 1.0f - normalized;
    }
};

} // namespace sin_compat::difficulty
