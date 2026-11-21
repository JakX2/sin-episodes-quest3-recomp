#include "sin_compat/original_voice_ban_persistence.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
using sin_compat::original_voice_ban::Key32;
using sin_compat::original_voice_ban::KeyTable;
using sin_compat::original_voice_ban::bucket_for;
using sin_compat::original_voice_ban::save_v1;
using sin_compat::original_voice_ban::load_v1_strict;
int main() {
    static_assert(sizeof(Key32) == 32, "Original key is exactly 32 bytes");
    Key32 a{}, b{}, c{};
    a.data[0] = 5;               // bucket 5
    b.data[0] = 3; b.data[1] = 2; // same hash but distinct full 32 bytes
    c.data[31] = 2;              // bucket 2
    assert(bucket_for(a) == 5 && bucket_for(b) == 5 && bucket_for(c) == 2);
    KeyTable table;
    assert(table.insert_unique(a));
    assert(table.insert_unique(b));
    assert(!table.insert_unique(b));
    assert(table.insert_unique(c));
    auto encoded = save_v1(table);
    assert(encoded.size() == 4 + 3 * 32);
    assert((encoded[0] == 1 && encoded[1] == 0 && encoded[2] == 0 && encoded[3] == 0));
    // The native serializer visits buckets in ascending order and follows
    // next links within a bucket, so c precedes a and b.
    assert(encoded[4 + 31] == 2);
    assert(encoded[4 + 32] == 5);
    assert(encoded[4 + 64] == 3 && encoded[4 + 65] == 2);
    KeyTable parsed;
    assert(load_v1_strict(encoded, parsed));
    assert(parsed.count() == 3 && parsed.contains(a) && parsed.contains(b) && parsed.contains(c));
    assert(save_v1(parsed) == encoded);
    auto corrupt = encoded;
    corrupt[0] = 2;
    assert(!load_v1_strict(corrupt, parsed));
    assert(parsed.count() == 0); // native-style clear-first, host failure guard
    corrupt = encoded; corrupt.pop_back();
    assert(!load_v1_strict(corrupt, parsed));
    corrupt = encoded;
    assert(!load_v1_strict(corrupt, parsed, 2));
    KeyTable empty;
    assert(load_v1_strict({1,0,0,0},empty));
    assert(empty.count() == 0 && save_v1(empty).size() == 4);
    // Fully opaque keys: not string terminated, not usernames or Steam IDs.
    Key32 opaque{}; opaque.data.fill(0xff);
    assert(bucket_for(opaque) == static_cast<std::uint8_t>(32u * 255u));
    return 0;
}