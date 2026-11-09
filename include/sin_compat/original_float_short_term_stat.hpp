#pragma once
// Independent C++17 reconstruction of SiN Episodes (2006) server.dll
// CFloatShortTermStat. Owner's exact PE32 RTTI and original x86 code verified.
// NOT the original thiscall/vtable or Windows instance layout.
#include "sin_compat/original_short_term_stat_gates.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace sin_compat::difficulty {

// The native source stores [vtable +0, dirty byte +4, vector ptr +8,
// count +0xC, descending ring cursor +0x10]. Model avoids raw native ptrs.
class OriginalFloatShortTermStat final : public IOriginalShortTermStatSource {
public:
    explicit OriginalFloatShortTermStat(std::size_t window)
        : samples_(window, 0.0f), next_(window ? window-1U : 0U) {}

    // Original CFloatShortTermStat virtual slot2 (0x222C9050), which
    // invokes ring sum at 0x222C64A0. The sum starts at next+1 and
    // visits every ring slot once, dividing by its configured count.
    float read_slot2() override {
        if (samples_.empty()) {
            // Original x87 with masked exceptions performs 0/0 here.
            return std::numeric_limits<float>::quiet_NaN();
        }
        float sum = 0.0f;
        for (std::size_t offset=0; offset<samples_.size(); ++offset) {
            sum += samples_[(next_+offset+1U)%samples_.size()];
        }
        return sum / static_cast<float>(samples_.size());
    }

    // Original vtable slot3 (0x222C9500): fill entire history, WITHOUT
    // altering cursor or the modified byte. This receives Phase150 gate
    // forwarded values, not a single history insertion.
    void write_slot3(float fill) override {
        for (float& v : samples_) v=fill;
    }

    // Original virtual slot9 (0x222C9530): write at current cursor,
    // decrement (wrap from zero to last), then set dirty byte +4.
    // Original binary has no empty-ring check; safe model rejects it.
    bool append_slot9(float sample) {
        if (samples_.empty()) return false;
        samples_[next_] = sample;
        next_ = next_ ? next_-1U : samples_.size()-1U;
        dirty_ = true;
        return true;
    }

    // Original slot4 (0x222C9090): read mean through virtual slot2,
    // rebuild ring with new length and fill every cell with old mean.
    // Length-parsing and malloc/free remain engine-owner responsibilities.
    void resize_slot4(std::size_t new_window) {
        const float old_mean = read_slot2();
        samples_.assign(new_window, old_mean);
        next_ = new_window ? new_window-1U : 0U;
    }

    // Inherited CShortTermStat original virtual slots 11/12
    // (0x222C8E40/0x222C8E50): read and consume modified byte.
    bool dirty_slot11() const noexcept { return dirty_; }
    bool consume_dirty_slot12() noexcept {
        const bool previous = dirty_;
        dirty_ = false;
        return previous;
    }

    std::size_t window_size() const noexcept {return samples_.size();}
    std::size_t descending_cursor() const noexcept {return next_;}
    const std::vector<float>& physical_ring() const noexcept {return samples_;}

private:
    std::vector<float> samples_;
    std::size_t next_ = 0;
    bool dirty_ = false;
};

} // namespace sin_compat::difficulty
