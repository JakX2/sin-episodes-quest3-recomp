#include "sin_compat/difficulty_message_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace sin_compat::difficulty;

// Typed sequence fixture: tests *call order*, not an invented byte protocol.
struct Scalar {
    char type;
    std::uint32_t bits;
    bool operator==(const Scalar& x) const { return type == x.type && bits == x.bits; }
};
static std::uint32_t bits(float x) {
    std::uint32_t b;
    std::memcpy(&b, &x, sizeof b);
    return b;
}
static float unbits(std::uint32_t b) {
    float v;
    std::memcpy(&v, &b, sizeof v);
    return v;
}
static Scalar f(float x) { return {'f', bits(x)}; }
static Scalar i(std::int16_t x) { return {'i', static_cast<std::uint32_t>(x)}; }

struct Fixture final : TypedReader, TypedWriter {
    std::vector<Scalar> cells;
    std::size_t cursor = 0;
    std::size_t fail_after = static_cast<std::size_t>(-1);
    bool read_f32(float& v) override {
        if (cursor >= cells.size() || cursor >= fail_after || cells[cursor].type != 'f') return false;
        v = unbits(cells[cursor++].bits); return true;
    }
    bool read_i16(std::int16_t& v) override {
        if (cursor >= cells.size() || cursor >= fail_after || cells[cursor].type != 'i') return false;
        std::uint32_t b = cells[cursor++].bits;
        v = static_cast<std::int16_t>(static_cast<std::uint16_t>(b)); return true;
    }
    bool write_f32(float v) override {
        if (cells.size() >= fail_after) return false;
        cells.push_back(f(v)); return true;
    }
    bool write_i16(std::int16_t v) override {
        if (cells.size() >= fail_after) return false;
        cells.push_back(i(v)); return true;
    }
};

int main() {
    assert(registrations.size() == 9);
    assert(registrations[0].name == "GamePlayVarValue" && registrations[0].declared_length == 6);
    assert(registrations[1].name == "Progression" && registrations[1].declared_length == 4);
    int hooks = 0;
    for (const auto& r : registrations) {
        if (r.client_hook_verified_in_dd_init) ++hooks;
        if (r.name != "GamePlayVarValue" && r.name != "Progression") assert(r.declared_length == -1);
    }
    assert(hooks == 8);
    assert(!registrations[5].client_hook_verified_in_dd_init);

    DecisionOrdinaryRecord original{};
    original.first = 0.25f;
    original.integers = {{-42, 0, 32767}};
    original.floats = {{1.5f, -2.75f, 0.125f, 13.0f}};
    Fixture send;
    assert(write_decision_ordinary(send, original));
    assert(send.cells.size() == 8);
    assert(send.cells[0] == f(0.25f));
    assert(send.cells[1] == i(-42) && send.cells[2] == i(0) && send.cells[3] == i(32767));
    for (std::size_t n = 4; n < 8; ++n) assert(send.cells[n].type == 'f');
    DecisionPrefix decoded{};
    assert(read_decision_known_prefix(send, decoded));
    assert(decoded.kind == DecisionPrefixKind::OrdinaryRecord && send.cursor == 8);
    assert(decoded.ordinary.first == original.first);
    assert(decoded.ordinary.integers == original.integers);
    assert(decoded.ordinary.floats == original.floats);

    // Binary-grounded x87 threshold handling: 0 is ordinary, not special.
    for (float marker : {-2.0f, -1.0f, -0.5f}) {
        Fixture special;
        special.cells = {f(marker), i(99)}; // Remaining tail is unknown.
        DecisionPrefix prefix{};
        assert(read_decision_known_prefix(special, prefix));
        assert(special.cursor == 1);
        assert(prefix.kind == (marker < -1.0f
            ? DecisionPrefixKind::LessThanMinusOne
            : DecisionPrefixKind::NegativeAtLeastMinusOne));
        DecisionOrdinaryRecord invalid = original;
        invalid.first = marker;
        Fixture no_write;
        assert(!write_decision_ordinary(no_write, invalid) && no_write.cells.empty());
    }
    {
        DecisionOrdinaryRecord zero = original;
        zero.first = 0.0f;
        Fixture plain;
        assert(write_decision_ordinary(plain, zero));
        DecisionPrefix got{};
        assert(read_decision_known_prefix(plain, got));
        assert(got.kind == DecisionPrefixKind::OrdinaryRecord && plain.cursor == 8);
    }
    // Failure must not expose a partially decoded record, or read wrong types.
    for (std::size_t cutoff = 0; cutoff < 8; ++cutoff) {
        Fixture truncated;
        truncated.cells = send.cells;
        truncated.fail_after = cutoff;
        DecisionPrefix prior{};
        prior.ordinary.first = 88.0f;
        assert(!read_decision_known_prefix(truncated, prior));
        assert(prior.ordinary.first == 88.0f);
    }
    Fixture bad_type;
    bad_type.cells = send.cells;
    bad_type.cells[2] = f(7.0f);
    DecisionPrefix ignored{};
    assert(!read_decision_known_prefix(bad_type, ignored));
    Fixture failed_writer;
    failed_writer.fail_after = 3;
    assert(!write_decision_ordinary(failed_writer, original));
    assert(failed_writer.cells.size() == 3); // Writes cannot be rolled back.

    SuccessModifiers modifiers{};
    for (std::size_t n = 0; n < success_pair_count; ++n)
        modifiers.pairs[n] = {{static_cast<float>(n), -static_cast<float>(n) - 0.25f}};
    Fixture success;
    assert(write_success_modifiers(success, modifiers));
    assert(success.cells.size() == 20);
    for (std::size_t n = 0; n < success_pair_count; ++n) {
        assert(success.cells[2 * n] == f(modifiers.pairs[n][0]));
        assert(success.cells[2 * n + 1] == f(modifiers.pairs[n][1]));
    }
    SuccessModifiers readback{};
    assert(read_success_modifiers(success, readback));
    assert(success.cursor == 20 && readback.pairs == modifiers.pairs);
    for (std::size_t cutoff = 0; cutoff < 20; ++cutoff) {
        Fixture missing;
        missing.cells = success.cells;
        missing.fail_after = cutoff;
        SuccessModifiers prior{};
        prior.pairs[0][0] = 123.0f;
        assert(!read_success_modifiers(missing, prior));
        assert(prior.pairs[0][0] == 123.0f);
    }
    Fixture success_write_failure;
    success_write_failure.fail_after = 19;
    assert(!write_success_modifiers(success_write_failure, modifiers));
    assert(success_write_failure.cells.size() == 19);
}
