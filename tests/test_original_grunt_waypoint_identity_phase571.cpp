#include "sin_compat/original_grunt_waypoint_identity_phase571.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace sin_compat;
using phase571::Point;

int main() {
    { // Matching incoming head with one remaining successor: skip, release, attach.
        Point prefix{}, drop{}, successor{}, last{};
        prefix.identity_0x10 = drop.identity_0x10 = 17;
        prefix.flags_0x20 = 8;
        drop.forward = &successor; successor.backward = &drop;
        successor.forward = &last; last.backward = &successor;
        drop.flags_0x20 = 8;
        int released = 0;
        auto r = phase571::splice_identity(&prefix, &drop, [&](Point* p) {
            assert(p == &drop);
            assert(successor.backward == nullptr);
            ++released;
        });
        assert(r.status == phase571::Status::Ok && r.released_incoming);
        assert(released == 1 && prefix.forward == &successor);
        assert(successor.backward == &prefix && last.backward == &successor);
        assert((prefix.flags_0x20 & 8u) == 0 && (last.flags_0x20 & 8u));
    }
    { // Matching incoming tail: release and make prefix terminal.
        Point prefix{}, drop{};
        prefix.identity_0x10 = drop.identity_0x10 = 20;
        std::vector<Point*> freed;
        auto r = phase571::splice_identity(&prefix, &drop, [&](Point* p){freed.push_back(p);});
        assert(r.status == phase571::Status::Ok && r.released_incoming);
        assert(freed.size() == 1 && freed[0] == &drop);
        assert(prefix.forward == nullptr && (prefix.flags_0x20 & 8u));
        assert(r.final_node == &prefix);
    }
    { // Matching interior incoming: detach the prior part of the old chain.
        Point newprefix{}, prior{}, dropped{}, next{};
        newprefix.identity_0x10 = dropped.identity_0x10 = 9;
        prior.forward = &dropped; dropped.backward = &prior;
        dropped.forward = &next; next.backward = &dropped;
        int releases = 0;
        auto r = phase571::splice_identity(&newprefix, &dropped,
                        [&](Point* p) { assert(p == &dropped); ++releases; });
        assert(r.status == phase571::Status::Ok && releases == 1);
        assert(prior.forward == nullptr && next.backward == &newprefix);
        assert(newprefix.forward == &next && (next.flags_0x20 & 8u));
    }
    { // Identity mismatch: retain incoming head and sever prior segment.
        Point prefix{}, prior{}, incoming{}, last{};
        prefix.identity_0x10 = 6;
        incoming.identity_0x10 = 7;
        prior.forward = &incoming; incoming.backward = &prior;
        incoming.forward = &last; last.backward = &incoming;
        int releases = 0;
        auto r = phase571::splice_identity(&prefix, &incoming,
                                            [&](Point*) { ++releases; });
        assert(r.status == phase571::Status::Ok && !r.released_incoming);
        assert(releases == 0 && prior.forward == nullptr);
        assert(prefix.forward == &incoming && incoming.backward == &prefix);
        assert((last.flags_0x20 & 8u) && (prefix.flags_0x20 & 8u) == 0);
    }
    { // -1 sentinel bypasses equality, even when incoming has id -1.
        Point prefix{}, incoming{};
        prefix.identity_0x10 = incoming.identity_0x10 = -1;
        auto r = phase571::splice_identity(&prefix, &incoming);
        assert(r.status == phase571::Status::Ok && !r.released_incoming);
        assert(prefix.forward == &incoming && incoming.backward == &prefix);
    }
    { // Two-segment introduced prefix; matched to old head.
        Point first{}, tail{}, old{}, follower{};
        first.forward = &tail; tail.backward = &first;
        tail.identity_0x10 = old.identity_0x10 = 2;
        old.forward = &follower; follower.backward = &old;
        phase570::Holder holder{&old};
        auto r = phase571::prepend_identity(holder, &first);
        assert(r.status == phase571::Status::Ok && r.released_incoming);
        assert(holder.first == &first && tail.forward == &follower);
        assert(follower.backward == &tail);
    }
    { // Reject unsupported native null dereference before modifying anything.
        Point prefix{}; prefix.identity_0x10 = 1; prefix.flags_0x20 = 8;
        auto r = phase571::splice_identity(&prefix, nullptr);
        assert(r.status == phase571::Status::UnsupportedNullNativePath);
        assert((prefix.flags_0x20 & 8u) && prefix.forward == nullptr);
    }
    { // Identity -1 plus empty old list simply marks prefix terminal.
        Point prefix{}; prefix.identity_0x10 = -1;
        auto r = phase571::splice_identity(&prefix, nullptr);
        assert(r.status == phase571::Status::Ok && r.final_node == &prefix);
        assert(prefix.flags_0x20 & 8u);
    }
    { // Aliasing and cycles refused before any original-memory writes.
        Point prefix{}, second{};
        prefix.forward = &second; second.backward = &prefix;
        auto r = phase571::splice_identity(&prefix, &second);
        assert(r.status == phase571::Status::UnsupportedAliasing);
        assert(prefix.forward == &second);
        second.forward = &prefix; prefix.backward = &second;
        r = phase571::splice_identity(&prefix, nullptr);
        assert(r.status == phase571::Status::UnsupportedGraph);
    }
    { // Recovered native constructors write +0x18/+0x1C zero after REP STOSD.
        Point n{}; n.raw_0x18 = 99; n.raw_0x1C = 97;
        phase571::default_node_220B2A60(n, {1,2,3});
        assert(n.position.x == 1 && n.raw_0x18 == 0 && n.raw_0x1C == 0);
        assert(n.identity_0x10 == -1 && n.cached_distance_0x14 == -1.f);
        phase571::explicit_node_220B2AB0(n, {4,5,6}, 13, 3, 0x12, 27);
        assert(n.position.y == 5 && n.raw_0x0C == 13 && n.kind_0x24 == 3);
        assert(n.flags_0x20 == 0x12 && n.identity_0x10 == 27);
        assert(n.cached_distance_0x14 == -1.f && n.raw_0x18 == 0 && n.raw_0x1C == 0);
    }
    return 0;
}