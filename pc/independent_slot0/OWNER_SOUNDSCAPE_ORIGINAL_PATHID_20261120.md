# Phase 494: SHA-authenticated original soundscape per-resource filesystem ABI

## Purpose

The user-retail filesystem test from Phase 486/493 invokes a native
2006 Source VFileSystem012 instance. Its original-version bridge
incorrectly used pathID="GAME" for BOTH the manifest and every
individual soundscape script. The private 2006 DLL shows two distinct
calls. This change corrects an actual live engine interface argument,
not merely a synthetic parser or a metadata report.

## Authenticated original client code

The original user-owned 2006 client.dll SHA256 is:

6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Read-only disassembly and full SHA authentication identify:

| Original method | VA | Length | Complete SHA-256 |
| --- | --- | ---: | --- |
| C_SoundscapeSystem::Init | 0x240F4160 | 343 bytes | 0fbb82377e54b7119c2c0e1a0b107c8958b354010993e03a481829cb2a2d661a |
| Original per-file resource helper | 0x240F4070 | 233 bytes | 69d75f737e839fd49af874f8cda11ad9b8b95feb51b2e6d084c34a2eac5c9bfa |

Both use original KeyValues::LoadFromFile helper at VA 0x24164A50.
Inspect the *argument order*, rather than assuming a shared search path:

1. C_SoundscapeSystem::Init at 0x240F41BC pushes the rdata
   address 0x24293B58 ("GAME"). It then pushes 0x24293D08
   ("scripts/soundscapes_manifest.txt") and the original
   IFileSystem* from global 0x2433F6BC, and calls
   0x24164A50 at 0x240F41C9. The underlying filesystem
   Open mode is "rb".
2. The original per-file helper at 0x240F409C explicitly
   pushes **zero** for pathID, then pushes its individual
   file path and the same original IFileSystem* before
   calling 0x24164A50 at 0x240F40A6. That is "rb"/nullptr,
   not "rb"/"GAME".
3. Optional map-specific soundscape resources are formed
   from source literal "scripts/soundscapes_%s.txt".
   After manifest traversal, the original checks existence
   using filesystem virtual byte slot +0x28 with a **null
   pathID** at 0x240F4274; if found, it invokes the
   per-file helper with the null pathID.
4. The per-file helper can fail to load the document and
   simply delete its temporary KeyValues object. Its
   caller does not test a success return value. The
   original Init explicitly writes AL=1 at 0x240F4290
   and at 0x240F42B2, including a failed-manifest
   diagnostic branch. Returning true is NOT proof of
   available audio definitions.

All string addresses, key-name "file", exact selected instruction
windows, direct call targets and entire method SHA-256s are
authenticated by the private mode of the independent auditor
audit_sin_original_soundscape_pathid.py, using the owner's
original SHA-matched client.dll from the connected Drive.
Its public --self-test uses only fabricated instructions and
corrupted-marker rejection, without any proprietary code.

## Actual implementation change

client_soundscape_engine_fs_bridge.h now calls the already-verified
VFileSystem012 ReadOne function with ("rb", "GAME") for the
manifest and ("rb", nullptr) for every referenced script.

This changes an actual engine argument in the opt-in owner-retail
soundscape probe, but DOES NOT broaden its terminal diagnostics into
normal successful gameplay or invoke original KeyValues.

The strict diagnostic intentionally continues to require the exact
seven retail file reads, 301 definitions, 242 references and three
unresolved references before it can signal prospective 0x53494E76.
That is **stricter than** the original Init's permissive true-return
behavior. Do not claim this strict probe is a playable Source startup
implementation.

## Tests and next verification

The two Windows x86 VFileSystem012 synthetic-engine test objects
were updated to **refuse a manifest null pathID or script "GAME"
pathID**. They also assert exact accepted manifest and script read
counts. All previous malformed-read, missing script, original-count,
rollback and session-proof checks remain required.

An additional original-free Python CTest verifies selected 343-byte
Init and 233-byte resource-helper opcode windows, original call
targets and two true-return instructions. It is suitable for
public GitHub CI. The original full SHA validation runs privately
against the owner-supplied original bin.zip archive.

The next meaningful retail engine milestone remains the same:
a disposable-owner Windows run of RUN_SOUNDSCAPE_CATALOG_TEST.bat
with prospective 0x53494E76/77 outcome and Phase 493 detailed
read/parse diagnostics. The latest actually observed owner
checkpoint is **0x53494E74**. Normal VClient011::Init remains 0.
