# Phase 496: original soundscape virtual slot 3, real-call-capable CVar lookup

## Original source evidence

A new small engine-facing method was recovered from the user's
SHA-authenticated original 2006 client.dll:

- Whole retail client SHA256:
  6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d
- C_SoundscapeSystem original vtable: 0x24293C34.
- Original vtable slot 3: **0x240F3C40**.
- Exact method size: **61 bytes**.
- Complete SHA256:
  **8a2bce6161d8de0973141bfae7d6c151091ee75933291d1c1d61b15d65b82040**.
- Original Source engine CVar interface singleton:
  **0x2433F690**.
- Original cached ConVar pointers:
  **0x2433E4AC** (snd_soundmixer);
  **0x2433E4A8** (dsp_volume).

Read-only source disassembly establishes the precise behavior:

1. Check if original global pointer 0x2433E4AC is nonzero.
   If so, skip the first lookup.
2. Otherwise load ICvar* from global 0x2433F690,
   push name "snd_soundmixer" (original rdata 0x24293C70),
   invoke the original interface virtual call at **byte +0x0C**
   (slot 3, FindVar) and cache its returned pointer at 0x2433E4AC.
3. Check global 0x2433E4A8 and, if it is null, repeat
   FindVar("dsp_volume") (original rdata 0x24293C64) through
   exactly the same **virtual slot 3**, caching the result.
4. Return void. No ConVar::SetValue, registration, write to an
   engine-owned ConVar object, sound channel playback, or DSP
   update appears in this 61-byte method body.

Source function's literal and instruction windows, entire method SHA
and original system vtable slot3 are verified by private mode of
audit_sin_original_soundscape_cvar_slot3.py. Public --self-test
fabricates the opcode windows and tests mutations without original
game bytes.

## Reconstructed independent executable C++ method

client_soundscape_original_convar_cache.h now contains
OwnedCache::Resolve(void* engine).

The actual lookup is NOT a mock event plan. It uses the same
native x86 method-boundary verifier from
client_known_gamesystem_init.h to validate object readability,
vtable presence, executable method pointer and same-module ownership.
It then invokes **only original virtual slot 3** through the
already tested Source 2004 VEngineCvar002 FindVar signature.

OwnedCache caches returned pointers **without dereferencing,
destroying or mutating them**. It retries only those still null
and never registers new ConVars. It preflights the interface
before changing local state, preserves previously acquired
references on invalid pointer/throwing fake callbacks, and
explicitly resets on a DIFFERENT engine object. That last reset is
a conservative independent lifetime policy, NOT an assertion
about the original static global lifetime.

Source ConVar lookup ordering and non-mutating results are retained.
If the original game does not currently expose either name, the
result simply reports one/both unavailable, matching the absence of
a fatal branch in the original source method.

## Native evidence and remaining real retail gate

test_client_soundscape_original_convar_cache.cpp uses an
independently authored Windows MSVC x86 vtable object. It tests:

- Exact original this-vtable slot 3 -> engine ICvar slot 3.
- snd_soundmixer lookup then dsp_volume lookup in source order.
- Both values present; second call skips all cached names.
- Missing dsp_volume then later retry only missing name.
- New engine object identity does not reuse former pointers.
- Both names absent; original-style nonfatal partial results.
- Null interface and throwing synthetic callbacks leave prior
  cached values intact.
- Reset restores empty owner state.

Public CI does not use original ConVar memory, game audio, or DLL.
The code is an independent **engine-call-capable adapter**, but
the current production independent client.dll does NOT invoke it
during normal game system startup.

The original 2006 game's lifecycle slot3 is not necessarily
executed at the same point in startup as the existing one-click
filesystem test. Do not force early invocation or interpret
missing ConVars as broken audio until the real callback order
and object lifetime are verified.

Normal production VClient011::Init still fails closed. The
furthest owner-observed real Windows checkpoint is 0x53494E74,
while the read-only soundscape 0x53494E76/77 stages have yet
to be owner-observed. Android and Quest 3 gameplay remain
downstream of completing native Windows game startup.
