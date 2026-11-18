# Phase 332: post-Connect server math startup and initial game-event loading

## Actual owner observation

Phase 330 executed the original engine's sound emitter Connect with the
independent server DLL, after all sixteen required factory acquisitions.
The callback returned true. The terminal diagnostic exit was 0x53494E46.
The original server was restored and the original client was unchanged.
This proves Connect entry and return, not successful DLLInit or gameplay.
The exact owner trace is retained as a regression.

## Original evidence

The private original server.dll was SHA-256 verified as
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
The original instructions following the sound-emitter success branch are:

| Original RVA | Operation |
| --- | --- |
| 0x1E77F0..0x1E7815 | Store globals pointer, call MathLib_Init |
| 0x26DCC0 | MathLib_Init, including CPU-dependent function selection |
| 0x268910 | 256-entry sine table |
| 0x26CDA0 | Gamma, screen, texture and lighting maps |
| 0x26CD30 | 256-entry base-two exponent table |
| 0x1E782A | Cache the engine, physics and filesystem factories via 0x263C30 |
| 0x1E783F | Call acquired GAMEEVENTSMANAGER002 virtual slot 1 with resource/gameevents.res |

The math arguments are gamma 2.2f, texture gamma 2.2f, brightness
0, overbright 2, and four true CPU feature permissions. SDK 2004
mathlib.cpp corroborates the table roles and parameter ordering.
Direct original math inspection confirms 256-entry sine, texture gamma,
texture-to-linear, linear-to-gamma, gamma-to-linear and exponent tables;
1024-entry linear-to-screen and linear-to-texture maps; and two 4096-entry
lighting maps. The scalar reconstruction is numerically equivalent in
intent, NOT a claim of bit-exact x87 intermediates, control-word
behavior or optimized function selection. It uses independently owned
storage and bounds its supported parameter range. The native regression
checks every sine/color/exponent/lighting entry and monotonic 1024-entry
maps, invalid inputs, gamma cap and overbright changes.

The original event call obtains the manager from the initial engine
factory, calls its slot 1 with the original resource path, and does
not immediately test the return. SDK event-manager interfaces place
LoadEventsFromFile after the virtual destructor and return an int.
The older bundled SDK header declares version 001; the actual acquired
retail version and call-site slot are verified independently by the
original binary. No assumption of full interface parity is made.

## Bounded owner diagnostic

The independently owned post-Connect state retains the opaque globals
and three factories, initializes its scalar tables, and invokes only
the initial gameevents file load. The new terminal probe requires
-EnableServerEventProbe in addition to -EnableServerSoundProbe and
-EnableServerFactoryProbe in Server-only mode. The previous sound-only
mode remains unchanged. Positive, zero and negative signed return
values are recorded verbatim. Exit 0x53494E48 means the event call
returned; it does not establish that any particular events loaded.
The classifier requires ordered same-process sound success, math,
factory caching, event entry, signed return and terminal stop.

The production DLL is tested using independently owned synthetic child
interfaces for callback order, three signed event results, the prior
sound-only mode, missing dependencies, and sound failure. No original
game process is executed by CI. The owner must wait for a passing
Windows build before attempting this diagnostic in a confirmed
disposable copy.

The next original operations after this callback include a local
registration helper (0x1E6450), lookup of sv_cheats, additional
registration functions, resource/ModEvents.res, game-system setup and
later allocations. They remain unimplemented. GetAllServerClasses
remains guarded. No original binary or proprietary code is committed.
