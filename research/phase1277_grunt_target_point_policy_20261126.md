# Phase 1277 — Grunt target point policy and schedule 106

Original binary: server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.

- 2235b3d0 +790: bit/c64/c90/target/c84 early gates, adaptive +188, original player/NPC classification and NPC +53c(0).
- 2235b498/2235b4e4: first or second enemy-memory cache plus target +37c scaled by .75 with binary32 stores. Second route reloads memory, eye +1c4, hull extents +/-12, mask 46004003, fraction !=1 adjusts candidate by plane normal times 16.
- 2235b5e6: 2213c760 visible-point check mask 4081. Success writes fcc/fd0/fd4. Failure copies invalid global vector and sets c84=now+1. Early refusals do not mutate either.
- 2235d809/2235d8c3: native slot 9 selection then schedule 106; rejection 88 or existing 41 fallback. No generic engine physics or model work.
- Existing ABI unchanged; additive Scene API provides acquisition and both high-level selector paths, as well as directly testing the target policy. Bounded entity extents and known class tokens only; unknown types and missing external leaves fail closed. Scene bindings represent the original adaptive, NPC eligibility, eye and trace operations, not retail runtime hooks.
- Schedule 41's greater-than-256 weapon-scene fallback is still unresolved. This phase does not claim completion of that branch.

Regression fixture covers first/second caches, hull hit/clear, invalid-point retry and exact deadline, disabled policy, NPC eligibility, free/occupied squad slot 9, and real schedule acquisition. Native runtime CI required.
