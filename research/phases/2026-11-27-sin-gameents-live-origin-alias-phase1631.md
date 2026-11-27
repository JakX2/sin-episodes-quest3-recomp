# Phase 1631: Preserve original live dirty-origin reads in ServerGameEnts001 touch

## Scope and provenance

This is a correctness correction to the already implemented original-guided `ServerGameEnts001::MarkEntitiesAsTouching` entry, **not an additional original function recovered**. Prior phases 1627 and 1630 established the 299-byte original method `0x221e9380`, whole-method SHA-256 `8352791af429bd1cf0d040a0765b42ec959181b34d67f98f818a8c039389f1c1`, in the authenticated `server.dll` SHA-256 `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. No new original code bytes are counted.

Direct owner-local x86 disassembly confirms that the two origin-dirty checks **are not simultaneous snapshots**:

| Original VA | Instruction pattern | Authenticated order |
| --- | --- | --- |
| `0x221e93ce` | `call 0x22285a10` | Initialize 0x54-byte trace first |
| `0x221e93d3` | `mov ecx,[esi+0xb8]` | Read **second** actor flags *after* trace initialization |
| `0x221e93e6` | `call 0x2213fa60` | Refresh second actor only if bit 11 set |
| `0x221e93eb` | `mov edx,[edi+0xb8]` | Read **first** actor flags **after second refresh returns** |
| `0x221e93fb` | `call 0x2213fa60` | Refresh first actor if its **current** bit 11 is set |
| `0x221e9400..0x221e943c` | x87 reads at `+0x2bc/+0x2c0/+0x2c4` | Read both actors' **current** origins after both refreshes |
| `0x221e949e` | `call 0x22221080` | Hand off original trace to owner contact service |

The preceding entry model resolved both edicts into independent `GameEntsTouchEntity` views before trace initialization and treated each view's dirty flag as a frozen value. This differs from retail when the two edicts alias one entity or a second actor's refresh mutates the first actor's dirty flag. This is a genuine semantic discrepancy even though all previous synthetic tests passed.

## Implemented correction

`GameEntsTouchEntryHooks` gains an **optional trailing** `sync_live_origin` callback. This preserves existing five-field aggregate callers while allowing an actual owner to reread flags and three world-origin floats from live state at the original inspection points. The wrapper now optionally rereads second state immediately after trace initialization, first state after second refresh, and both live origins after the refresh sequence, before building the midpoint. It also copies the refreshed view for exact `first.opaque == second.opaque` aliases, so older synthetic callbacks without the optional hook will not refresh a single actor twice using stale independent snapshots.

`server_gameents_touch_contact_bridge.h` supplies the live-state callback from **existing typed `TouchEntity`** owners, not guessed original `edict_t` or `CBaseEntity` layouts. It validates that the view resolves to one of the two actors and records the number of live reads. The existing midpoint prepublication check, 0x54-byte trace publisher, reciprocal links, release semantics and host-services remain untouched.

## Regression scenarios

- Standalone entry: both edicts resolve to the **same actor**; a dirty-origin refresh clears the live state, and the original first-actor check then skips refresh. Both callback-equipped and legacy callback-free variants assert **one** refresh (not two) and one handoff.
- Integrated contact bridge: identical edict inputs are recognized and ignored by `MarkTouchPair` after one original dirty-origin refresh; no contact nodes are allocated.
- Integrated bridge: refreshing the **second** actor clears the formerly dirty first actor, so first refresh is skipped, and the midpoint uses the new first origin.
- Integrated bridge: refreshing the **second** actor marks the formerly clean first actor dirty, so first refresh is performed, and the midpoint uses both live results.
- Existing 0x54 trace, owner publication, reciprocal allocation and failure tests continue to apply. No fake Source physics, surface/material, prop, model-loading or rendering work is added.

This still does **not** export the incomplete `ServerGameEnts001` interface to retail Source. Live-state views are host-provided, and passing Windows regression tests are not proof of a retail engine boot or playable Android/VR port.
