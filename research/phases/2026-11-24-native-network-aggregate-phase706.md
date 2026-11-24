# Phase 706: native network aggregate

Phase705 introduced 15 class records over separately allocated network
graphs. This phase replaces manual composition with an optional single
owner allocation per role. Individual versioned APIs remain available.

Build order: BaseEntity, BaseAnimating, OverlayVars, BaseFlex,
BaseCombatCharacter/inventory, Local, LocalPlayerExclusive, BasePlayer,
derived SiN player/hand, weapon inheritance, class records. All pointers
refer to embedded members; no intermediate graph handles escape.
Server weapon resolved initialization is factored into one shared helper
used by both individual and aggregate creation, preserving validation
of the entity registry, edict array and verified engine vslot18.

Each aggregate owns exactly 35 SendTable or RecvTable records plus their
property storage, names, callback contexts, and 15 class records. The
caller supplies the compatible CollisionProperty table, entity registry
and engine, clock and transform sampling, layer reserve/resize,
FOV sampling, velocity invalidation, effects and interpolation updates,
and client creation functions as appropriate. No host service is called
during build except range/method identity validation. No original binary
address is used. The output remains unchanged on any failure, even a late
missing creator. Destruction preflights every owned table, including
nested-only Array3 and dynamic layer tables, and class ID/baseline fields;
on refusal no owned storage is freed. The caller must detach all external
references and stop use before release. Mutable pointers are ABI views,
not ownership transfers.

Paired PE32 fixture exercises both role exports, early and late failures,
35 distinct table identities, correct class roots, direct callback
transfer of SiN accuracy through class -> local nested table, refusal of
nested table or class engine markers, corrupted view rejection, recreation,
and borrowed collision lifetime. No complete class catalog publication,
retail constructor, engine initialization success or gameplay claim.
