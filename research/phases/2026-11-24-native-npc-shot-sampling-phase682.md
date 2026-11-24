# Phase 682: original in-game NPC shot direction consumer

Authenticated original server.dll SHA256
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650.
Direct PE32 disassembly of 0x22033280..0x220334e7 (616 bytes),
SHA256 6bb5018731565f4ecf9e0e98ad798cfe96b1a8c6441fb896dcad99d10e732c9e,
matches the independently inspected owner SDK2004 CShotManipulator
method shape. Original calls at 0x22036b61 and 0x220371dc follow
slot244 bias and slot243 spread (Phase681).

Original ConVar constructor 0x223bb030 names ai_shot_bias_min
(default "-1.0"), object 0x22559638; constructor 0x223bb060 names
ai_shot_bias_max (default "1.0"), object 0x22559680. Parent globals
0x22559650/0x22559698 read +0x28 floats. Native method clamps the
input bias to [0,1], interpolates between those values, takes half
the magnitude as flatness, and consumes four RandomFloat(-1,1)
calls per attempt via VEngineRandom001 slot1. Even zero-weight terms
consume RNG. For negative effective bias it inverts each coordinate
about its signed endpoint. Reject when squared radius >1, not >=1.
Constants verified as 1,0,0.5,-1. The output is forward + weighted
right + weighted up; spread.z is not used and no normalization occurs.

Native module exposes a portable game-only sampler and Win32
composition of Phase681 aiming, min/max ConVar reads and actual
verified random vslot. Caller supplies basis vectors (original
CShotManipulator stores them at +0,+0xc,+0x18); the result is a
direction descriptor, not a trace, projectile, hit or firing action.
The implementation follows explicit original float stores, uses
double intermediates as a portable approximation to x87 extended
precision, and rejects invalid values. Original rejection loop is
unbounded; port limit is 256 attempts, with reported attempted RNG
calls so a failed stream is not silently replayed. No engine pointers
retained and no reentrant campaign mutation. No original raw bytes
or assets committed. Loaded-module tests cover retry order, flat
distribution zero-weight draws, negative bias, inclusive boundary,
failure/exception and bounded attempts, and Win32 native cvar/RNG
order. Actual retail NPC vtable and engine firing remain unbound.
