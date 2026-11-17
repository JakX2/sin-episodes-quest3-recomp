# SiN server reciprocal contact cleanup (Phase 320)

## Evidence and scope

Verified the owner-original PE32 server.dll SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Compared the two partner notification routines and their unlink helpers
against the owner's SDK 2004 physics_main_shared.cpp. Observed:

| Operation | Touch RVA | Ground RVA |
| --- | --- | --- |
| Partner notification | 0x21FF00 | 0x2200D0 |
| Single contact removal | 0x21F120 | 0x21F2E0 |

The partner routines check presence, obtain the relevant data object,
resolve candidate handles through the entity registry, remove the first
matching entity and then conditionally destroy the empty sentinel.
Touch requires flags bit 0 (node +0x10) for its EndTouch-style virtual
call (vtable byte offset 0x168), and requires a live resolved entity.
The receiving entity is the partner; the argument is the departing
entity. Unlink updates both neighbors before decrementing the counter
and returning the node to its pool. Touch honors the global cleanup
suppression flag when deciding whether to destroy an empty list.

Important SiN-specific difference: the ground helper at RVA 0x21F2E0
calls a nonvirtual helper at RVA 0x14C730 with mask 1 on the resolved
entity when both entities are live. The target clears mask 1 in the
entity's flag word at +0xBC, calls virtual slot byte offset 0x268 when
the word changes, and notifies another subsystem with old/new flags.
The SDK identifies bit 0 as FL_ONGROUND. The original ground helper
does not show a virtual EndGroundContact dispatch. Its list destruction
decision does not use the touch-cleanup suppression flag.

## Independent implementation

server_reciprocal_contact_core.h adds generation-checked matching,
two-way neighbor updates, conditional EndTouch, an explicit remove-flags
hook, allocator accounting, and kind-specific empty-root policy.
The native representation is not a binary overlay. Its circular-list
validation and refusal of self-contact and incomplete hooks are independent
safeguards. Callbacks must not mutate the list or destroy live entities.

The reciprocal regression tests flags, stale handles, malformed lists,
callback order, root cleanup and repeat removal. A separate paired fixture
wires bulk cleanup to the reciprocal remover with shared counters and
actually deletes both contact nodes. It checks the suppressed empty
touch root is retained while the empty ground root is destroyed.

Still missing: concrete game entity lifecycle, original flag-change
notification and EndTouch implementation, allocator and data-object
registry integration. These cores are not exported to the retail engine;
the paired fixture is native synthetic integration, not boot evidence.
