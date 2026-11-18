# Phase 342: client screen and Gaussian stream startup state

The Phase 340 owner trace now proves 24/24 original variable registrations
and final registry identity verification through the retail engine,
exit 0x53494E4D. Both original DLLs restored/retained and verified.
The exact observed sequence is preserved in the offline classifier test.
This is still a bounded batch experiment, not a full console registry.

Phase 341 passed all four CI workflows, including 37/37 native tests.
The next owner test remains the client material probe, not this new
standalone state fixture.

## Original client evidence

Direct SHA-authenticated owner-original client.dll inspection
(SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d)
establishes the immediate post-material acquisition sequence:

- 0xFE377 calls helper 0xFFAB0. The helper invokes VEngineClient012
  vtable slot 5 with pointers to width global RVA 0x33FC10 and height
  global RVA 0x33FC14. Both globals start zero-filled.
- 0xFE37C..0xFE38D passes the acquired VEngineRandom001 global at
  RVA 0x33F6CC to the imported CGaussianRandomStream::AttachToStream
  via IAT RVA 0x276244, receiver RVA 0x33F700.
- The original static initializer at 0x2681F0 constructs the same
  Gaussian receiver with null through IAT RVA 0x276240.
- The owner SDK 2004 public/cdll_int.h identifies slot 5 as
  GetScreenSize(int&,int&). public/vstdlib/random.h corroborates the
  Gaussian declaration, pointer/bool/float storage and constructor
  and Attach signatures. The original import names match.
- Immediately afterward, the original registers the client console
  declarations, initializes its object registry, particle manager and
  VGui; none of those steps are claimed implemented here.

## Independent state

client_screen_random_core.h implements an independently owned
screen query with exact output reference order and preservation of
actual integer output (including zero), and a typed owned 12-byte
SDK-compatible Gaussian storage adapter with a constructor/attach
callback pair. It does not sample random values or assert exact parity
with a particular vstdlib binary's internal random algorithm.
The native test checks slot 5, output values, missing slots, repeat
refusal and the constructor/attach ABI against deliberately owned
synthetic native callbacks. The private PE inspector validates the
original call sites. **The imported Gaussian callbacks are not enabled
in the distributed DLL**; the real provider's layout and behavior
require separate owner validation. No original memory or original
commercial implementation bodies are committed.
