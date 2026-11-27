# Phase 1612: Major/Minor original activity-transition overrides

Retail server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. The class vtable +0x4e4 paths differ:
* Major 2237a410 (151 bytes): if incoming private ACT_FRENZY, emits NPC_Leperkin.Frenzy (0,0), draws float [.9,1.1] then calls inherited 22191a70, which conditionally updates shared actor+3c0 and animation observer flags. Treat as owner-managed setter, not duplicate Source method. Activity29 writes actor+c14=1,c15=0 then clears both if prior actor+878==27; otherwise if prior==29 clears both and incoming27 sets c14=1,c15=0 (including prior29 case). Other activities preserve latch unless leaving 29.
* Minor 2237dde0 (68 bytes): activity1 draws uniform [0,.75], activity10 draws [0,.25], passes result to inherited 220405c0, which updates shared actor+3e8 and animator dirty flags. Other activities no-op. No inherited playback, model, or physics recreated.

The native API resolves Major ACT_FRENZY from the original generation-verified activity registration rather than an invented ID. Host retains original sound/RNG setters. Loaded regression covers frenzy order, both latch transitions, unchanged transitions, exact random ranges, unknown activities, wrong class and stale registry generation.

Original bytes/digests:
2237a410/151/b16f43162e575003434adcb9b2338198f740e973d1914c69c7e542f688537609
2237dde0/68/87670503806dcfa320000d9ca9b0dc712caf58ece82c22b0a213133533643928
Shared owners:
22191a70/74/11443ae00624322fec5867e63ea123c4dcc04e70b812fd39f0238e5a0f5c2362
220405c0/74/c29922726417ba239a53115070d53d158916bcdd6113e22651a359a5be872926

No executable bytes copied, no generic Source subsystem duplicated; CI pending.
