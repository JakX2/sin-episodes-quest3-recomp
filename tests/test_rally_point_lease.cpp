#include "sin_compat/rally_point_lease.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <optional>
#include <set>

int main() {
    using sin_compat::RallyPointLease;
    using Token = RallyPointLease::OwnerToken;
    constexpr Token a = 0x00010004ULL;
    constexpr Token b = 0x00020004ULL;  // same index, different entity generation
    constexpr Token c = 0x00010005ULL;

    std::set<Token> live{a, b, c};
    auto is_live = [&live](Token token) { return live.count(token) != 0; };
    RallyPointLease point;
    assert(!point.current_owner().has_value());

    assert(point.acquire(a, is_live));
    assert(point.current_owner().value() == a);
    // Original lock fails even for same owner if currently live.
    assert(!point.acquire(a, is_live));
    assert(!point.acquire(b, is_live));
    assert(point.current_owner().value() == a);

    // Different owner cannot unlock a valid reservation.
    assert(!point.release(c, is_live));
    assert(point.current_owner().value() == a);
    assert(!point.release(std::nullopt, is_live));
    assert(point.release(a, is_live));
    assert(!point.current_owner().has_value());
    assert(point.release(b, is_live)); // already empty

    // An expired entity's slot can be reused without reviving its old handle.
    assert(point.acquire(a, is_live));
    live.erase(a);
    assert(point.acquire(b, is_live));
    assert(point.current_owner().value() == b);
    assert(!point.release(a, is_live));
    assert(point.release(b, is_live));

    // An expired handle may be cleared by anybody, just as in the x86 code.
    assert(point.acquire(c, is_live));
    live.erase(c);
    assert(point.release(a, is_live));
    assert(point.acquire(std::nullopt, is_live));
    assert(!point.current_owner().has_value());
}
