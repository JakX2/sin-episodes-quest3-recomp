# Phase 984: fix malformed alias fixture

Windows runs 45552165093, 45552260546, 45552345998 and 45552345879 fail at the alias assertion in Phase 980. Its escaped C++ literal produced two separators rather than one. Use a raw literal with one backslash per separator. Do not change path normalization or weaken malformed-path checks. Prior phases 976–979 all workflows passed. Remaining directory selection assertions not reached in the failing runs; CI pending.
