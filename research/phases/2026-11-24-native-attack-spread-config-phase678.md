# Phase 678: native combat spread configuration

The owner-provided original server.dll (SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650) was inspected together with scripts/rules_combat.txt (2920 bytes, SHA-256 0d4c47cfa543435deb7098b4218e62707a02971351957260959ffd6016667f1e). No original script or executable is redistributed.

The original lookup at 0x222d7ee0 resolves the AttackSpreadRules class_names, weapon_class_names and target_class_names tables in order. Helper 0x222d5470 looks for an exact mapping then wildcard. The original rules constructor at 0x222d7e30 references scripts/rules_combat.txt and the GAME path ID. An unresolved rule uses 0.13053f; the result is multiplied by metric 12 and written into three vector components.

The independent module exposes SiN_GetAttackSpreadApi for bounded text loading, clearing and querying. Its parser reuses the owned stats lexer, rejects duplicate case-folded keys and invalid reachable references, and commits a replacement only after validation. These strict validation rules are port choices rather than claims about all original KeyValues behavior. Missing lookups use the authenticated fallback. Unlike the retail cache, the port recomputes the scaled value at each query so a skill change is reflected immediately.

Tests cover synthetic exact and wildcard mappings, case folding, changes of fixed skill, transactional rejection, absent configuration, and a separate optional owner-file argument to the loaded-module test for six authentic-script lookups. The owner file is not needed in CI. The actual engine filesystem loading, original cache and NPC callsite bindings are still outstanding.
