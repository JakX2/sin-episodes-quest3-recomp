#include "sin_compat/original_client_base_interpolation_registrations.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
namespace reg = sin_compat::original_client_base_interpolation_registrations;
void check(const reg::Record& r, unsigned word, unsigned value, unsigned interp) {
    assert(r.sourceWord == word && r.valueOffset == value && r.interpolatorOffset == interp);
}
int main() {
    reg::List empty;
    assert(empty.Size() == 0 && empty.LogicalCapacity() == 0);
    assert(!empty.Insert(1, reg::kVector));
    assert(empty.LogicalCapacity() == 0);
    auto list = reg::BuildBaseRegistrations();
    assert(list.Size() == 2 && list.LogicalCapacity() == 3);
    check(list.At(0), 2, 0x220, 0x22c);
    check(list.At(1), 2, 0x25c, 0x268);
    assert(list.Insert(1, {7, 8, 9}));
    assert(list.Size() == 3 && list.LogicalCapacity() == 3);
    check(list.At(0), 2, 0x220, 0x22c);
    check(list.At(1), 7, 8, 9);
    check(list.At(2), 2, 0x25c, 0x268);
    assert(list.Insert(0, {10, 11, 12}));
    assert(list.Size() == 4 && list.LogicalCapacity() == 6);
    check(list.At(0), 10, 11, 12);
    check(list.At(1), 2, 0x220, 0x22c);
    check(list.At(2), 7, 8, 9);
    check(list.At(3), 2, 0x25c, 0x268);
    assert(!list.Insert(5, reg::kQAngle));
    assert(list.Size() == 4 && list.LogicalCapacity() == 6);
    auto independent = reg::BuildBaseRegistrations();
    assert(independent.Size() == 2 && independent.LogicalCapacity() == 3);
    std::puts("PASS base interpolation registrations: original offsets, order, 12-byte records, insertion and logical growth");
}
