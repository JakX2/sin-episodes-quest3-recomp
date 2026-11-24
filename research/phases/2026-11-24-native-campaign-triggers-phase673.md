# Phase 673: compile recovered SiN campaign triggers into native server.dll

Reuses the original-authenticated OriginalTriggerEntityFactory and
FixedDifficultyTriggerHost directly, rather than copying their
logic into a second implementation. The map registry now attaches
trigger_dynamic_difficulty and trigger_secret, routes the recovered
map fields and releases factory records with map entity lifetime.
Both factory and fixed host are level-owned and destroyed before
the campaign policy they reference. Class-specific targetname is
captured by the behavior; a real shell host must also apply its
base targetname key.

The versioned SiN_GetCampaignTriggerApi accepts explicit host
callbacks during spawn, touch and removal retry. This supplies
native base trigger spawn, liveness, player predicate, special
action, event publication and deferred removal. The caller supplies
actual observed native flags (+0x120, +0xb4, +0xb8), origin and
engine removal gate. The generic map spawn refuses triggers because
those host fields/callbacks are mandatory. Phase672's ordinary BSP
host adapter remains limited to env_global/info_challenge until a
trigger-aware engine shell host exists.

The original trigger implementation preserves event names/fields,
targetname fallback, warning, secret descriptor 11/12, player gate,
special-action gate, low-byte EndOfGame and queued deletion even
when publication fails. FixedDifficultyTriggerHost redirects only
manager/accounting to the existing DLL-owned campaign; it never
calls an adaptive manager. No callback or context is retained.
Reentrant mutation during foreign callback dispatch is refused.

Loaded-module tests exercise both exact classnames, recovered keys,
native-field observations, rejected special action, nonplayer,
publish/removal failure, retry without duplicate events, secret
independence and completion on EndOfGame=257 but not 256. The
original engine's actual CBaseTrigger, collision, factory,
datamap, event manager and delete queue remain host work.
No retail execution or playable claim.
