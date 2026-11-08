# Original SiN Episodes 2006 CGrenadeAR touch gate, now in C++17

**8 November 2026. Original server.dll touched privately, no original bytes committed.**

## Exact binary provenance

Owner's original SiN Episodes: Emergence server.dll
SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

Original game function: CGrenadeAR touch callback at x86 VA
0x223142F0 through return at 0x22314353 (100 bytes).
Entire original touch function SHA256:
945fb670960ad71aba1a1e4f5e183122c35697fdef8f1021d9d99dae56197b91

Direct inspection via llvm-objdump and a PE32 RVA parser proves:

1. At 0x223142F7, the callback examines the contacted
   object's raw dword +0x11C. A zero value immediately branches
   to the no-detonation return at 0x22314350.
2. At 0x22314300 it tests raw contacted-object flags
   +0x120 against 0x04. When that bit is set it returns
   immediately. Neither raw field is renamed to a guessed
   high-level Source collision class.
3. At 0x22314310 it invokes a virtual function at
   +0xFC on the contacted object. When the return is
   nonnull, it compares that reference against the result
   of projectile-side helper 0x222B2730 at 0x2231431C.
4. Only on a nonnull *different* reference does the original
   write byte 1 through setter 0x222B2B40 at 0x22314337,
   targeted at the projectile's original raw +0x795 byte.
   A null or equal reference bypasses that write.
5. At 0x2231433D the code tests the raw projectile +0x795
   flag. If zero it returns without detonating.
   Otherwise its virtual call at 0x2231434A uses offset
   +0x3E8, which on i386 is method index 250, already
   independently proven to be CGrenadeAR::Detonate using
   named grenade-subclass method exports.
6. The actual original grenade spawn routine checks and
   clears its raw +0x795 byte (0x22314C4D/0x22314C73),
   so an original just-spawned grenade does not necessarily
   call Detonate on every contact.

The helper at 0x222B2B40 is a byte-field mutation helper
with network-change side effects. The exact engine-side
replication type and ownership of the +0xFC returned reference
are not yet fully identified. Thus their values must be
supplied by a verified native host rather than invented.

## Reconstructed C++ components

- include/sin_compat/original_ar_grenade_touch.hpp
- src/original_ar_grenade_touch.cpp

The pure evaluate() function reproduces the original **branch
gates** and reports whether original +0x795 would be set.
The dispatch() helper sends only accepted contacts to the
existing PR #151 portable one-shot CGrenadeAR::Runtime,
which handles the independently proven blast, AI stimuli and
removal ordering.

**Important semantic distinction**: Runtime::armed() is a
modern one-shot *not-yet-detonated* guard, not the actual original
networked flag +0x795. The host supplies the original flag
independently; otherwise the old spawn reset would be lost.

The adapter does not call SourceVR engine object vtables, derive
the contacted object's true physics class from raw offsets,
reimplement the native network setter, or invent a collision
trigger, trace or decal. Those remain real engine integration
tasks. The host must perform necessary original field-setter
side effects itself before calling dispatch().

## Verification and regressions

- research/tools/audit_sin_original_ar_touch_gate.py:
  original SHA256-gated, read-only PE32 check of the entire
  100-byte original touch function fingerprint and individual
  field/branch/vtable/callsite evidence, plus original
  spawn-time +0x795 reset. Emits metadata only.
- research/tests/test_audit_sin_original_ar_touch_gate.py:
  fifteen **fabricated** machine-code window tests. Real DLL
  bytes are not present in the GitHub runner.
- tests/test_original_ar_grenade_touch.cpp:
  actual C++ playback against an independent mock engine host,
  including blocked field/flag contacts, equal/nonnull virtual
  references, flag assignment request, first detonation, repeated
  contact, pre-existing +0x795 and originally-cleared +0x795.
- New CMake libsin_original_ar_grenade_touch and test target.
  Android API26 AArch64 + API29 x86_64 Bionic targets also
  build and CI runs the fixture on an Android emulator.

Run your PRIVATE game binary audit:

    python3 research/tools/audit_sin_original_ar_touch_gate.py --server /private/server.dll

Run source-only tests:

    python3 -m unittest discover -s research/tests -p test_audit_sin_original_ar_touch_gate.py -v
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target sin_original_ar_grenade_touch_tests
    ctest --test-dir build -R '^sin_original_ar_grenade_touch_tests$' --output-on-failure

## Still necessary for standalone Quest 3

Real native entity factory and owner/weapon binding; contact
trace collision eligibility and related-object +0xFC semantics;
native +0x795 byte setter and network change tracking; precise
touch/think detonation interplay; scene effects, actual damage
engine callbacks, and final Quest OpenXR gameplay startup.

The exact original byte checks are evidence, NOT a claim that
our new code can be loaded by native SourceVR IServerGameDLL.
No executable SiN game, on-device VR runtime or Quest APK has
been produced by this touch-gate work.
