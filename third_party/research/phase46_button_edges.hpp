#pragma once
#include <cstdint>
namespace sin_phase46 {
constexpr std::uint32_t kZoomMask = 0x00080000u;
struct ButtonEdges {
    std::uint32_t current = 0;
    std::uint32_t previous = 0;
    std::uint32_t pressed = 0;
    std::uint32_t released = 0;
    // The C++ analogue of 0x2223D240. Not Ritual's original source.
    void update(std::uint32_t incoming) {
        previous = current;
        const auto changed = previous ^ incoming;
        pressed = changed & incoming;
        released = changed & ~incoming;
        current = incoming;
    }
};
}
