#include "sin_compat/original_server_voice_recipient_filter.hpp"
#include <cassert>
#include <cstdint>
using namespace sin_compat::original_server_voice_recipient_filter;

int main(){
    PlayerResolveWitness p{1,32,true,0U,true,true};
    assert(can_resolve_player(p));
    p.one_based_index=32;assert(can_resolve_player(p));
    p.one_based_index=0;assert(!can_resolve_player(p));
    p.one_based_index=33;assert(!can_resolve_player(p));
    p.one_based_index=1;p.edict_present=false;assert(!can_resolve_player(p));
    p.edict_present=true;p.edict_flags=2U;assert(!can_resolve_player(p));
    p.edict_flags=4U;assert(can_resolve_player(p));
    p.edict_owner_present=false;assert(!can_resolve_player(p));
    p.edict_owner_present=true;p.owner_virtual_resolves=false;
    assert(!can_resolve_player(p));
    p.owner_virtual_resolves=true;assert(can_resolve_player(p));
    RecipientListNormalPath recipients;
    assert(recipients.append_unique(1));
    assert(!recipients.append_unique(1));
    assert(recipients.append_unique(32));
    assert(recipients.append_unique(4));
    assert(!recipients.append_unique(32));
    assert(!recipients.append_unique(0)); // host extra safety
    assert(recipients.size()==3U);
    assert(recipients.recipient(0U)==1);
    assert(recipients.recipient(1U)==32);
    assert(recipients.recipient(2U)==4);
    recipients.clear(); assert(recipients.size()==0U);
    return 0;
}