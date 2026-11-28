#include "sin_compat/original_client_game_string_pool.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
using namespace sin_compat::original_client_game_string_pool;

int main() {
    static_assert(kOriginalInitialNodes*kOriginalNodeBytes==0xc00u);
    static_assert(kOriginalGrowNodes==32u);
    static_assert(!std::is_copy_constructible_v<Pool>);
    static_assert(!std::is_move_constructible_v<Pool>);
    Pool pool;
    assert(pool.Size()==0);
    assert(!pool.Find("missing"));
    assert(!pool.Intern(""));
    assert(!pool.Intern(nullptr));
    assert(!pool.Find(nullptr));
    assert(!pool.Intern(std::string_view("a\0b",3)));
    assert(pool.Size()==0);

    // Independent copy, first spelling and case-folded pointer identity.
    char mutableName[]="SinTEK_Enemy";
    const char* original=pool.Intern(mutableName);
    assert(original && original!=mutableName);
    mutableName[0]='X';
    assert(std::strcmp(original,"SinTEK_Enemy")==0);
    assert(pool.Find("sintek_enemy")==original);
    assert(pool.Intern("SINTEK_ENEMY")==original);
    assert(pool.Size()==1);

    // Prefix distinction, unsigned non-ASCII bytes, no host locale.
    const auto* prefix=pool.Intern("SinTEK");
    const auto* suffix=pool.Intern("SinTEK_Enemy2");
    const char highA[]={static_cast<char>(0xc0),'A',0};
    const char highB[]={static_cast<char>(0xe0),'a',0};
    const char highAFold[]={static_cast<char>(0xc0),'a',0};
    const auto* high=pool.Intern(highA);
    const auto* other=pool.Intern(highB);
    assert(prefix && suffix && prefix!=original && suffix!=original);
    assert(high && other && high!=other);
    assert(pool.Find(highAFold)==high);
    assert(!pool.Find("not-registered"));
    assert(pool.Size()==5);

    // Fill the complete source 16-bit index domain, including the
    // original 256-node first capacity boundary and subsequent growth.
    // Previously returned pointers must survive all inserts, regardless
    // of host STL tree growth or string length. Existing duplicates
    // remain findable at full capacity; new insertions refuse.
    const auto initial=pool.Size();
    for(std::size_t i=0;i<kMaximumUniqueStrings-initial;++i) {
        const std::string name="entity_string_"+std::to_string(i);
        const char* value=pool.Intern(name);
        assert(value && name==value);
        if(i==255u || i==287u || i%1024u==0u) {
            assert(pool.Find("SINTEK_enemy")==original);
            assert(std::strcmp(original,"SinTEK_Enemy")==0);
        }
    }
    assert(pool.Size()==kMaximumUniqueStrings);
    assert(!pool.Intern("overflow"));
    assert(!pool.Find("overflow"));
    assert(pool.Intern("sintek_ENEMY")==original);
    assert(pool.Find("entity_string_256"));

    // Source slot5 frees string ownership and resets the lookup tree.
    // Never access old pointers after clear; allocator reuse is legal.
    pool.Clear();
    assert(pool.Size()==0 && !pool.Find("SinTEK_Enemy"));
    assert(!pool.Find("entity_string_256"));
    pool.Clear();
    const auto* recreated=pool.Intern("sintek_enemy");
    assert(recreated && std::strcmp(recreated,"sintek_enemy")==0);
    assert(pool.Find("SINTEK_ENEMY")==recreated && pool.Size()==1);

    // Independent pool objects must not share string ownership.
    Pool otherPool;
    assert(!otherPool.Find("SINTEK_ENEMY"));
    const auto* independentlyOwned=otherPool.Intern("SINTEK_ENEMY");
    assert(independentlyOwned && independentlyOwned!=recreated);
    otherPool.Clear();
    assert(pool.Find("SINTEK_ENEMY")==recreated);
    std::puts("PASS CGameStringPool: first spelling/pointer identity, "
              "C-locale case fold, owned allocation, lookup-only misses, "
              "65535 unique capacity, stable pointers and level clear");
}
