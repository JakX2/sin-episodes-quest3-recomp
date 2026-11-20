# Phase 484: native Win32 soundscape resource bridge

## New recovered engine contract

The SHA-authenticated original 2006 client's shared KeyValues
LoadFromFile function at VA 0x24164A50 invokes the engine
IFileSystem::Open virtual call at byte slot +0x08 (slot 2) with
read mode "rb" (constant at original rdata 0x24289C0C).
The original C_SoundscapeSystem::Init at VA 0x240F4160 calls
LoadFromFile for scripts/soundscapes_manifest.txt with path id
"GAME" and loads the manifest's "file" resource children.

This is supported by **private original PE32 code inspection** and
the public archived Source SDK 2004 IFileSystem interface.
It is separate from proof of successful reconstructed game startup.

## New code

client_soundscape_engine_fs_bridge.h composes the previous owned
Soundscape manifest loader (Phase 482) and nested KeyValues catalog
(Phase 483) with the already implemented and Win32-native-tested
VFileSystem012 read-only helper from client_engine_filesystem_read.h.

The bridge resolves/validates engine method slots for Read=0,
Open=2, Close=3 and Size(handle)=7. It reads the manifest and
all referenced soundscape scripts with mode "rb" and path id "GAME"
rather than guessing text mode or silently accepting truncated reads.

It publishes a new complete **owned** snapshot only after all
file handles close, every resource is bounded, the KeyValues-like
syntax is parsed, and the catalog's cross-file name graph is built.
Previously valid state is kept on invalid interface, missing file,
malformed manifest, malformed soundscape text or short reads.

## Native MSVC Windows test

test_client_soundscape_engine_fs_bridge.cpp builds a fake
VFileSystem012-compatible object with a native x86 virtual table,
and uses the existing VerifiedMethod executable/memory guards.
It checks exact filesystem modes/search path, original message
resource paths, partial 9-byte binary reads, required handle closes,
two-file catalog publication, bad path rejection, malformed KeyValues
rollback, missing dependency, empty/oversized and zero-progress reads,
and successful reinitialization.

The synthetic fixture includes NO original game script text, real
engine pointer or proprietary DLL. Public CI and Linux/Android
reference builds are not owner retail game execution.

## What is still not implemented

The bridge is a **testable boundary**, not wired to the normal
distributed client.dll or live C_SoundscapeSystem. It doesn't create
original KeyValues objects, register soundscape records through the
original linked hierarchy, play wave files, resolve DSP effect data,
select soundscapes by trigger or implement shutdown.

Further work is required on ownership of the engine's actual
IFileSystem handle, real Source soundscape runtime interfaces,
client callback ordering, and the other pending game systems.

The production independent VClient011::Init still intentionally
fails closed; genuine owner Windows retail startup checkpoint
remains **0x53494E74**. No Windows game boot, Android or Quest 3
gameplay is claimed by synthetic ABI tests.
