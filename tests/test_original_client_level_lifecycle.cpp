#include "sin_compat/original_client_level_lifecycle.hpp"
#include <cassert>
#include <cstddef>
#include <string_view>
#include <vector>
using namespace sin_compat::original_client_level_lifecycle;
int main() {
    static_assert(kClientCallbackVtableByteOffset == 0x34);
    static_assert(kClientCallbackCountOffset == 0x10);
    static_assert(kPreEntity == 0x240FDB40u && kPostEntity == 0x240FDC00u);
    // Exact original branch: 0 and 1 always choose command 0, even when
    // virtual +0x148 would return false; only >1 evaluates that virtual.
    assert(prediction_command(0, false)=="cl_predict 0");
    assert(prediction_command(1, false)=="cl_predict 0");
    assert(prediction_command(2, false)=="cl_predict 1");
    assert(prediction_command(2, true)=="cl_predict 0");
    assert(prediction_command(255, false)=="cl_predict 1");
    assert(prediction_command(-1, false)=="cl_predict 0");
    bool active=false;
    assert(enter_level_once(active) && active);
    assert(!enter_level_once(active) && active);
    assert(exit_level_once(active) && !active);
    assert(!exit_level_once(active) && !active);
    std::size_t n=0;
    std::vector<std::size_t> seen;
    for_each_client_callback_reverse(n,[&](std::size_t i){seen.push_back(i);});
    assert(seen.empty() && n==0);
    n=1;
    for_each_client_callback_reverse(n,[&](std::size_t i){seen.push_back(i);});
    assert((seen==std::vector<std::size_t>{0}) && n==0);
    seen.clear(); n=4;
    for_each_client_callback_reverse(n,[&](std::size_t i){seen.push_back(i);});
    assert((seen==std::vector<std::size_t>{3,2,1,0}) && n==0);
    return 0;
}