# Phase 1609: Major melee hit policy and projectile name clarification

Original server.dll authenticated SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Major +774 22379fd0 queries enemy, resolves +c44; if both live invokes 223791a0 object toss and returns selected object (full toss implementation still owner boundary). Otherwise original hull min/max using +610, shared hit query +3b4 with reach, min/max, damage, type4, scale1, mode0. On miss NPC_Leperkin.AttackMiss, on hit AttackHit. Player +120 true -> skip view/impulse if +bd&40, else +434 angular and 222453e0 impulse. Nonplayer +ec token and 2228be90 eligibility -> left/right attachment for side0/1, both for2, no attachments otherwise. Each successful attachment re-queries +ec and calls 22149eb0 with amount min(damage,30); missing left in both path still checks right. All actual hull/hit query, object throw, player effects and blood effects owner-hosted. Tests verify direct branch, stale serials, no enemy, player flag, hit/miss, both attachments and absent attachment behavior. Correct phase1607 name callback: 22138160 pools string then writes entity+0c, not a particle renderer. Rename source field entity_name, no binary struct layout change. No generic model/physics/animation reimplementation.

VA/bytes/SHA256:
22379fd0/880/db583a231250ebd946006b83b66265f3224a600d0764aae730bcf8582388937d
22138160/30/914cdb6ec251a223cbfd29d49d29b9004550736daa36e73d5a56d373c4c15681

CI pending. No original executable bytes committed.
