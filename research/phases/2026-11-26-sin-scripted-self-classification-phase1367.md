# Phase 1367: Original self-attacker classification across all Grunt families

Drive SE1/bin.zip extracted server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Fresh PE32 original +d0 table reads and objdump:

| Vtable | +d0 target | MOV EAX immediate / RET |
|---|---|---|
|2248df04|22355d60|11|
|224954a4|22358410|6|
|2249aaa4|2236a3a0|11|
|224a416c|22358410|6|
|2248c514|22354270|11|
|2248e83c|22356030|11|
|2249964c|22358410|6|
|224a4a4c|223807d0|6|

The six-byte leaves are b8 0b 00 00 00 c3 or b8 06 00 00 00 c3. Extend native SetHealth self-attacker classification to all subclass routes; leave supplied overrides untouched and standalone arbitrary-attacker DeathThink explicit. Loaded test now removes the fake Elite classifier and exercises forced immediate mode3 on all four Jetpack tables, then delayed mode4. Hosted effects remain explicit. CI pending.
