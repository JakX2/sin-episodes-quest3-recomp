#pragma once
// Authored host reference for authenticated original SiN 2006 server code.
// This is NOT a drop-in DLL or an implementation of the original engine ABI.
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace sin_compat::phase584 {
struct TranslationHost {
    std::function<int(int, bool&)> virtual_33c;
    std::function<int(int)> virtual_338;
    std::function<bool()> original_220eb8a0_available;
    std::function<int(int)> original_220eebd0_lookup;
    std::function<bool(int)> original_2202e600_check;
    std::function<void(int)> original_diagnostic;
};

// This is the observed *order* of 0x22049870; virtual contracts are injected.
inline int ResolveRequest(int raw, TranslationHost& host) {
    bool initial_flag = false;
    const int initial = host.virtual_33c(raw, initial_flag);
    int current = raw;
    int most_recent_33c = initial;
    int last_338_alternative = raw;
    bool last_flag = false;
    for (int round = 1; round <= 5; ++round) {
        const int alt = host.virtual_338(current);
        if (alt != current) last_338_alternative = alt;
        bool flag = false;
        const int translated = host.virtual_33c(alt, flag);
        most_recent_33c = translated;
        last_flag = flag;
        if (translated == current) break;
        current = translated;
    }
    if (current == 15) return 15;
    if (host.original_220eb8a0_available() && host.original_220eebd0_lookup(most_recent_33c) != -1)
        return most_recent_33c;
    if (last_flag && host.original_diagnostic) host.original_diagnostic(most_recent_33c);
    if (last_338_alternative != most_recent_33c && host.original_2202e600_check(last_338_alternative))
        return last_338_alternative;
    if (initial != last_338_alternative && host.original_2202e600_check(initial))
        return raw; // the exact native branch returns the *original request*
    if (raw != initial && host.original_2202e600_check(raw)) return raw;
    if (raw == 10) return 6;
    if (raw == 6) return 10;
    return raw;
}

struct LookupHost {
    std::function<std::optional<int>()> original_22139a10_entity;
    std::function<int(int)> entity_virtual_24;
    std::function<int(int)> entity_virtual_28;
    std::function<int(int,int,int)> original_220eb1c0_lookup;
};
inline int LookupAction(int current_action_field_3ec,int translated,LookupHost& host) {
    const auto entity = host.original_22139a10_entity();
    int mode = 0;
    if (entity && host.entity_virtual_24(*entity)==3) mode=host.entity_virtual_28(*entity);
    return host.original_220eb1c0_lookup(mode,translated,current_action_field_3ec);
}

struct TransitionState {
    int current_request_878 = 0;
    int translated_87c = -1;
    int selected_action_3ec = -1;
    bool selected_action_active_3e1 = false;
};
struct TransitionHost {
    std::function<bool(int)> virtual_4e0;
    std::function<void(int)> original_220405c0_reset;
    std::function<void(int)> original_220f2300_set;
    std::function<float(int)> original_220eede0_float;
    std::function<void(int,float)> original_222a5470_use;
    std::function<int(int)> virtual_630_with_out_context;
    std::function<void(int)> original_22140bd0;
    std::function<void(int)> virtual_4e4;
    std::function<void()> nav_virtual_4c;
};
// Four stack args: request, action_id, translated_id, context.
// The debug-print path and engine ABI side effects remain external.
inline void ApplyTransition(TransitionState& state,int request,int action_id,
                            int translated_id,int context,TransitionHost& host) {
    state.translated_87c = translated_id;
    if (action_id > -1) {
        const bool same_and_active=(state.selected_action_3ec==action_id && state.selected_action_active_3e1);
        if (!same_and_active) {
            if (!(host.virtual_4e0(state.current_request_878) && host.virtual_4e0(request)))
                host.original_220405c0_reset(0);
        }
        host.original_220f2300_set(action_id);
        const float f=host.original_220eede0_float(action_id);
        host.original_222a5470_use(context,f);
    } else {
        host.original_220f2300_set(0);
    }
    host.original_22140bd0(host.virtual_630_with_out_context(state.translated_87c));
    if (state.current_request_878!=request) host.virtual_4e4(request);
    state.current_request_878=request;
    host.nav_virtual_4c();
}
}