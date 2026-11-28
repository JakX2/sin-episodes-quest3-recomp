# Phase 1715: original Beam speed RecvProxy, x87 exact default rounding

## Source identity and exact 2006 PE32 evidence

Unchanged original client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d

Original property: DT_Beam.m_fSpeed (static table 128, slot 14).
Native type = 1 (float); native RecvProp member offset +44 = 0x508.
Only one original static constructor references callback 0x2409ED70.
This 44-byte original function has complete SHA256:
77c9e9200d8c3d401d77f13e979c06d0ce35e4b446a65849a92de10f5e01d8d3.
Original RET at 0x2409ED9B precedes four INT3 bytes and the next
independent function at 0x2409EDA0.

The source code reads CRecvProxyData+4 as a binary32 value and
executes an original x87 FMUL QWORD [0x2427B050]. The original
8-byte factor is binary64 0x3FB999999999999A, the nearest binary64
representation to decimal 0.1. It is not a float32 factor: using
0.1f would give different results for some native inputs.

The result remains at x87 extended precision when the method
duplicates it, loads the old owner float32 at +0x508, and uses
FUCOMPP; FNSTSW AX; TEST AH,0x44; JNP. The equality branch
performs no store. Otherwise the source method FSTPs the new
product to object+0x508. Original +0 and -0 compare equal and
must preserve the old signed zero representation on that branch.

Source constructor PUSH callback at 0x2409EFDF = 0x2409ED70.
Source PUSH offset at 0x2409EFE8 = 0x508.
Source PUSH network-name at 0x2409EFED = 0x2427B9F0, m_fSpeed.
The type1 factory CALL at 0x2409EFF7 targets 0x24126D80.

## Bit-exact portable arithmetic under default x87 rounding

Original factor 0x3FB999999999999A can be expressed exactly as
integer significand 0x1999999999999A times 2^-56. A float32
significand is at most 24 bits, giving a 77-bit exact product.
The new independent C++17 implementation uses two unsigned
64-bit integers with checked carries. It rounds that product to
the x87 64-bit significand, then to the original FSTP float32
result, both ties-to-even. No nonportable unsigned __int128,
host long double, x87 process control word, original PE32
pointer, or executable retail callback is required.

It handles signed zeros, normal/subnormal boundaries, smallest
subnormals, infinities, finite underflow, and full native
float32 exponent normalization. A nonzero source whose extended
product later underflows to signed float32 zero still takes
the store path. Only an original incoming signed zero compared
against a previous zero suppresses the write, preserving
the owner's existing sign bit.

Original non-default x87 precision/rounding-control states and
NaN signaling, payload and exception effects are not proven;
the independent host model deliberately refuses incoming NaNs
without modifying any owned bytes.

## Verification

The portable core was compared locally with numpy.longdouble
(x86-64 80-bit extended) using the exact original binary64
factor and an FSTP-equivalent numpy.float32 conversion, for
59,775 finite/infinite original float32 bit patterns. All
matched in GCC, Clang and Clang ASan/UBSan builds. These were
reference vectors, not owner-game execution.

Committed tests/test_original_client_custom_beam_speed_proxy.cpp
contains 52 fixed x87 golden input/output DWORD pairs, exact
source slot and offset checks, positive/negative-zero no-store,
finite-to-zero underflow store, neighboring byte preservation,
short buffer refusal, NaN refusal, invalid source tuple refusal,
and unchanged data-table output token. The existing all-1110
source callback router test requires 1,051 applied routes,
31 unimplemented custom and 28 special constructor paths.

The optional original PE32 private-source auditor is
research/tools/audit_sin_original_client_custom_beam_speed_proxy.py.
Its public self-test validates source classification and
fabricated-bytes refusal; private --client mode authenticates
whole original DLL SHA, complete 44-byte function, binary64
constant, exact x87 opcode witnesses and source constructor
arguments. No proprietary executable bytes are committed.

This is HOST-OWNED reference behavior. Retail VClient011.slot5
remains deliberately blocked and CanPublishRetailProxyPointers()
returns false. There is still no actual native receiver ABI,
network packets, 2006 entity creator objects, Windows game boot,
Android/Quest 3 gameplay, physics, models or renderer.
