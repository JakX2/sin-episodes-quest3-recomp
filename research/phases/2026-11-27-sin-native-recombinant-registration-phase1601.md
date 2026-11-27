# Phase 1601: exact mutant private activity registrations

Original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Major registration prefix 2237a4b0..2237a5c5; Minor prefix 2237dec0..2237e040. Both call 220e6fe0, store returned ID in a named global slot then publish via 22022a60. Seven Major and ten Minor registration operations, sixteen unique names (ACT_FLY_ATTACK shared as private name; original private registrar returns already registered private ID, rejects shared name collision). No absolute original slot accessed by native code.

|Family|Activity|Original slot|Store VA|
|---|---|---|---|
|Major|ACT_FLY_ATTACK|2259d1d0|2237a519|
|Major|ACT_FRENZY|2259d1cc|2237a533|
|Major|ACT_BIG_SLASH|2259d1c8|2237a54d|
|Major|ACT_BITE|2259d1c4|2237a567|
|Major|ACT_IDLE_BLUNT_WEAPON|2259d1c0|2237a581|
|Major|ACT_WALK_BLUNT_WEAPON|2259d1bc|2237a59e|
|Major|ACT_RUN_BLUNT_WEAPON|2259d1b8|2237a5b8|
|Minor|ACT_LEAP_ATTACK|2259d45c|2237df46|
|Minor|ACT_FLY_ATTACK|2259d458|2237df60|
|Minor|ACT_FLY_LAND|2259d454|2237df7a|
|Minor|ACT_HIT_FLYING|2259d450|2237df94|
|Minor|ACT_WALK_BACK|2259d44c|2237dfae|
|Minor|ACT_SPIT|2259d448|2237dfcb|
|Minor|ACT_FLY_ATTACK_POUNCE_OFF|2259d444|2237dfe5|
|Minor|ACT_FLY_AWAY|2259d440|2237dfff|
|Minor|ACT_FLY_LAND_AWAY|2259d43c|2237e019|
|Minor|ACT_SIDESTEP|2259d438|2237e033|

Use existing activity dictionary, generation-aware per-class registration result, original order, optional host publication. Partial capacity failure is explicit (registered prefix remains, ready false), not a claim of transactional registry mutation. New helper binds Minor +2259d450 ACT_HIT_FLYING and +2259d44c ACT_WALK_BACK to the task bindings. Session rejects a mismatch instead of accepting guessed numeric activity IDs. Phase1600 loaded regression now uses actual native registrations, not hand-assigned test IDs. Separate loaded test checks duplicate, order, generation, wrong class, shared collision and bounded capacity.

Region VA/bytes/SHA256:
2237a4b0/277/9f089a80a2df1fce6715444975438dd53a2823e2344c5a90087f76590db54862
2237dec0/384/0820af73bba44004b2b105bbda637a528f24b377bde3be6e9767bb6628317d56

No original executable bytes committed; CI pending. No new inherited Source activity code.
