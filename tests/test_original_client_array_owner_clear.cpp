#include "sin_compat/original_client_array_owner_clear.hpp"
#include <cassert>
using namespace sin_compat::original_client_array_owner_clear;
int main() {
    static_assert(kNativeVA==0x24105FE0u && kNativeBytes==39u);
    Owner disabled{{1,2,3},7,-1,0};
    clear_if_enabled(disabled);
    assert(disabled.allocation.size()==3 && disabled.count==7 && disabled.release_events==0);
    disabled.guard=0;
    clear_if_enabled(disabled);
    assert(disabled.allocation.empty() && disabled.count==0 && disabled.release_events==1);
    disabled.count=9;
    clear_if_enabled(disabled);
    assert(disabled.count==0 && disabled.release_events==1);
    Owner enabled_empty{{},15,1,0};
    clear_if_enabled(enabled_empty);
    assert(enabled_empty.count==0 && enabled_empty.release_events==0);
}
