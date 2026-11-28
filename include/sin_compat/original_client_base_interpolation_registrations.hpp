#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace sin_compat::original_client_base_interpolation_registrations {
// Source records contain a word and two PE32 pointers. For the host witness,
// use owner-relative offsets instead of pointers; never expose this to retail.
struct Record final {
    std::uint32_t sourceWord;
    std::uint32_t valueOffset;
    std::uint32_t interpolatorOffset;
};
static_assert(sizeof(Record) == 12);
inline constexpr Record kVector{2u, 0x220u, 0x22cu};
inline constexpr Record kQAngle{2u, 0x25cu, 0x268u};

// Model the source zero-grow-size list only. The original memory allocator,
// grow-size sentinel paths and physical object layout are not reproduced.
class List final {
public:
    bool Insert(std::size_t index, Record value) {
        if (index > records_.size()) return false;
        if (records_.size() == logicalCapacity_) {
            if (logicalCapacity_ > records_.max_size() / 2u)
                throw std::length_error("interpolation list capacity overflow");
            const auto next = logicalCapacity_ == 0 ? 3u : logicalCapacity_ * 2u;
            records_.reserve(next);
            logicalCapacity_ = next;
        }
        records_.insert(records_.begin() + static_cast<std::ptrdiff_t>(index), value);
        return true;
    }
    std::size_t Size() const noexcept { return records_.size(); }
    std::size_t LogicalCapacity() const noexcept { return logicalCapacity_; }
    const Record& At(std::size_t index) const { return records_.at(index); }
private:
    std::vector<Record> records_;
    std::size_t logicalCapacity_ = 0;
};
inline List BuildBaseRegistrations() {
    List result;
    result.Insert(result.Size(), kVector);
    result.Insert(result.Size(), kQAngle);
    return result;
}
} // namespace sin_compat::original_client_base_interpolation_registrations
