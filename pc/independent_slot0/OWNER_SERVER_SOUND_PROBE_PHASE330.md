# Phase 330: actual server DLLInit entry, sound connection, class registry

## Phase 328 observed owner result

The original client was retained. The independent server accepted
ServerGameDLL003, ServerGameEnts001 and ServerGameClients003, rejected
HLTVDirector001, returned the pre-init GetGameDescription, then entered
DLLInit. All 15 required engine acquisitions and the one filesystem
acquisition returned non-null. The bounded diagnostic returned false.
The engine subsequently queried PluginHelpersCheck001 (rejected),
called the description again and reached slot 10. The guard exited
with 0x53494E45. Restoration was VERIFIED_COMPLETE.

This is real retail entry and acquisition evidence, not successful
DLLInit or gameplay. Module sampling missed the DLL, but the in-process
session trace is explicit. The exact event sequence is replayed in CI.

## Server-class registry

The SHA-pinned original server GetAllServerClasses slot 10 (RVA 0x1E7C30)
returns a global class-list pointer at RVA 0x589050. The constructor at
RVA 0x1290 initializes a 20-byte PE32 class record: name +0, send table
+4, next +8, engine-controlled ID +12, baseline +16 (initial 0xFFFF).
It inserts in case-insensitive alphabetical order, corroborated by
the owner's SDK 2004 public/server_class.h.

Direct local read-only analysis identified 164 unique constructor
call sites with distinct class names, class storage and send-table
storage. Examples include CSE1Proxy, CKeypad, CPropKeypad, CPropPhone,
CPropSinVehicle, CNPC_EliteGrunt, CNPC_GruntBase, CNPC_Hologram,
CWeaponAssaultRifle, CWeaponChaingun, CWeaponMagnum and CPlayerHand.
The SHA-gated class inventory inspector recovers all 164 records,
validates mapped storage and exact canonical inventory digest, and
outputs metadata only. It does not construct any tables or properties.
No metadata-only or empty placeholder list is returned to the engine.
Slot 10 is now named GetAllServerClasses and retains its terminal guard.

## Next bounded retail diagnostic

Both original game DLLs call the sound-emitter Connect after the initial
acquisitions. The original server performs this at RVA 0x1E77E6, passing
the engine factory; a false return branches to DLLInit failure. SDK
2004 gameinterface.cpp explicitly notes that both game DLLs may connect
and the sound emitter handles this. Phase 327 verified the native
callback ABI and ordering against independent synthetic fixtures.

Phase 330 enables the production independent server to call this one
real engine callback ONLY in the explicit server-only launcher mode
with both -EnableServerFactoryProbe and -EnableServerSoundProbe.
The default remains off. As soon as Connect returns, log the result
and terminate the disposable probe process without returning a fabricated
DLLInit success or invoking unfinished game systems. Codes:

- 0x53494E46: Connect returned true, terminal diagnostic stop.
- 0x53494E47: Connect returned false, terminal diagnostic stop.
- 0x53494E45: unsupported callback, including missing ABI input.

The classifier requires the same-process entry, result and terminal
events, and still declares no boot proof. The callback has real engine
side effects. Only use a confirmed disposable game copy. No guessed
Disconnect or full DLLShutdown is attempted. The launcher restores the
original server and verifies the retained original client.

The distributed server DLL itself is exercised in isolated synthetic
child processes for both Connect outcomes, default disabled, missing
last dependency, and missing factory opt-in. CI never runs the original
game. The production client remains acquisition-only. The owner should
not attempt the next executable test until native CI passes.

Phase 329 also had an invalid synthetic corruption fixture whose last
byte was already zero; the parent fix flips that byte instead.

No original game binaries or proprietary implementation code committed.
