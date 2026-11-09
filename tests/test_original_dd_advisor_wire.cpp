#include "sin_compat/original_dd_advisor_wire.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>
using namespace sin_compat::original_dd_advisor;
namespace {
struct Op { bool is_float; std::uint32_t raw; };
struct Buffer final : Writer, Reader {
    std::vector<Op> ops;
    std::size_t position = 0;
    std::size_t limit = 1000;
    bool write_float32_bits(std::uint32_t v) override {
        if (ops.size() >= limit) return false;
        ops.push_back({true, v});
        return true;
    }
    bool write_signed16(std::int16_t v) override {
        if (ops.size() >= limit) return false;
        ops.push_back({false, static_cast<std::uint16_t>(v)});
        return true;
    }
    bool read_float32_bits(std::uint32_t& v) override {
        if (position >= ops.size() || !ops[position].is_float) return false;
        v = ops[position++].raw;
        return true;
    }
    bool read_signed16(std::int16_t& v) override {
        if (position >= ops.size() || ops[position].is_float) return false;
        auto x = static_cast<std::int32_t>(ops[position++].raw);
        v = static_cast<std::int16_t>(x < 32768 ? x : x - 65536);
        return true;
    }
};
void test_thresholds() {
    for (int mode = -8; mode <= 9; ++mode)
        for (int code = -8; code <= 9; ++code)
            for (std::size_t i = 0; i < 6U; ++i) {
                int expected = code;
                if (i < 4U && mode < 2 && code < 2 && mode <= code) expected = 2;
                if (i < 4U && mode > 2 && code > 2 && mode >= code) expected = 2;
                assert(transmitted_code(i, mode, code) == expected);
            }
    assert(transmitted_code(0, 0, 1) == 2);
    assert(transmitted_code(0, 1, 0) == 0);
    assert(transmitted_code(0, 3, 3) == 2);
    assert(transmitted_code(0, 3, 4) == 4);
    assert(transmitted_code(0, 2, 1) == 1);
    assert(transmitted_code(4, 0, 1) == 1);
    assert(transmitted_code(5, 3, 3) == 3);
    assert(wire_signed16(0x12345) == 0x2345);
    assert(wire_signed16(-1) == -1);
    assert(wire_signed16(0x8000) == -32768);
}
ServerSnapshot sample() {
    ServerSnapshot s;
    s.raw_mode = 3;
    for (std::size_t i = 0; i < kAdvisorCount; ++i) {
        s.advisors[i].score = float_from_bits(i == 0 ? 0x80000000U
            : i == 1 ? 0x7FC01234U
            : 0x3F800000U + static_cast<std::uint32_t>(i));
        s.advisors[i].raw_code = static_cast<std::int32_t>(i + 1U);
        for (std::size_t c = 0; c < kChannelFields; ++c)
            s.advisors[i].channels[c] = float_from_bits(
                0x3F800000U + static_cast<std::uint32_t>(i * 4U + c));
    }
    return s;
}
void test_scores() {
    auto s = sample();
    Buffer stream;
    assert(write_scores(s, stream));
    assert(stream.ops.size() == 12U);
    for (std::size_t i = 0; i < kAdvisorCount; ++i) {
        assert(stream.ops[2U*i].is_float);
        assert(stream.ops[2U*i].raw == float_bits(s.advisors[i].score));
        assert(!stream.ops[2U*i+1U].is_float);
        assert(stream.ops[2U*i+1U].raw == static_cast<std::uint16_t>(
            wire_signed16(transmitted_code(i, s.raw_mode, s.advisors[i].raw_code))));
    }
    ClientSnapshot c;
    assert(read_scores(stream, c));
    assert(!c.dirty && stream.position == 12U);
    for (std::size_t i = 0; i < kAdvisorCount; ++i) {
        assert(float_bits(c.scores[i]) == float_bits(s.advisors[i].score));
        assert(c.codes[i] == transmitted_code(i, s.raw_mode, s.advisors[i].raw_code));
    }
}
void test_channels() {
    auto s = sample();
    Buffer stream;
    assert(write_channels(s, stream));
    assert(stream.ops.size() == 24U);
    ClientSnapshot c;
    assert(read_channels(stream, c));
    assert(!c.dirty && stream.position == 24U);
    for (std::size_t i = 0; i < kAdvisorCount; ++i)
        for (std::size_t j = 0; j < kChannelFields; ++j) {
            auto bits = float_bits(s.advisors[i].channels[j]);
            assert(stream.ops[i*4U+j].is_float && stream.ops[i*4U+j].raw == bits);
            assert(float_bits(c.channels[j][i]) == bits);
        }
}
void test_truncated_packets_are_noncommitting() {
    auto s = sample();
    Buffer stream;
    stream.limit = 3;
    assert(!write_scores(s, stream) && stream.ops.size() == 3U);
    ClientSnapshot c;
    c.codes[0] = 123;
    assert(!read_scores(stream, c));
    assert(c.dirty && c.codes[0] == 123);
    Buffer channels;
    channels.limit = 7;
    assert(!write_channels(s, channels));
    c.channels[0][0] = 93.0f;
    assert(!read_channels(channels, c));
    assert(c.dirty && c.channels[0][0] == 93.0f);
    Buffer wrong_type;
    wrong_type.ops.push_back({false, 8U});
    assert(!read_scores(wrong_type, c));
}
}
int main() {
    test_thresholds();
    test_scores();
    test_channels();
    test_truncated_packets_are_noncommitting();
    return 0;
}
