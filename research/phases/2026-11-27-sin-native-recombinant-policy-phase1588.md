# Phase 1588: SiN-exclusive Recombinant Major/Minor policies

Priority pivot: independent work from verified phase1579, not the pending inherited-script regression stack. Do not reconstruct generic HL2 tasks or re-enable adaptive difficulty (phase663 owner choice). Original server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 verified from private bin.zip. RTTI CNPC_BaseMutant 225372bc, CNPC_RecombinantMajor 22537720, CNPC_RecombinantMinor 22537abc; primary tables 2249f65c, 224a0d8c, 224a2974. Original game class strings npc_recombinant_major and npc_recombinant_minor, distinctive NPC_Leperkin.Pain and NPC_gib_hopper.Pain sound names. This is original class-specific code, not an inherited Source NPC recreation.

- Major +43c 22376fb0: local request1->84,8->85; 16/32/33/41 ->80 unless current conditions 66 then33 and current schedule80, then73. Minor +43c 2237b0b0: 4->83,23->85,29->5,41->80,76->88. Others delegate original inherited 2205f1c0.
- Major +338 22378ee0: activity6 becomes10 in schedule73, then state byte +bf &8 maps1->111 and6/113->112, then serial-checked weapon +78c enables named blunt idle/walk/run activities (registered global slots 2259d1c0/bc/b8) or64->65. Minor +338 2237ae50:6->10; otherwise inherited 2203cfc0.
- Major +5e0 22376ec0: activities1/6=25,10=35,default20. Minor 2237ae90: named ACT_LEAP_ATTACK or default30;6/10=20,42/43=15.
- Major +598 223787b0 suppresses pain if +bf&8; else emits NPC_Leperkin.Pain. Minor 2237c150 with same flag draws original [0,10], only zero emits NPC_gib_hopper.Pain; without flag no draw.
- All three tables +fc 22044820 and +120 220017e0 authenticate combat NPC classification, independent of Grunt family.

Native server-only API uses existing condition mapper, serial registry and activity dictionary; inherited schedule/activity functions and actual sound emission remain owner-bound. Registered custom names are looked up, not assigned guessed global IDs. No original executable bytes committed. No full actor constructors or scheduler yet. Windows loaded regression tests all branches, stale weapon handle, original short-circuit order, dictionary missing and non-mutant rejection.

Original code/table region SHA256 (VA/bytes/digest):
22376fb0/177/ee92d6314087b5c6dc01553139008456670f0369b25101220f96ab2c47330807
2237b0b0/173/869bad5698345e46725c102afd95eb2433d5fdc2da3ed13109d492e7a64f25e5
22378ee0/248/489f48e6e025176489850c089cde388fcc76908a50cf1ad949a349feb88453ad
2237ae50/26/60e3c93f0e3cad3d98d02f27064e83f8e02f4f99083a258bc95ff9ebc6d9fb1e
22376ec0/111/0420651a23c44e3de7c6636e815a2abd19a75b163965102b9b63a502e72c505e
2237ae90/119/f9c04eca990404c5a912ffad9112c0801290814d41403feb82589955f7cc36de
223787b0/24/eb286dce190841529ab452b77a950655ccc1a846ae445c1ba66bed1d5089b270
2237c150/49/266b9daa164be26ce635885ab4d1aedbbcaba56ab02336026aa32e41606e1def

CI pending.
