# Phase 485: original visual benchmark setup

## Authenticated 2006 retail evidence

Private read-only analysis used the genuine original SiN Episodes
client.dll SHA256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
CPerfVisualBenchmark::Init starts at original VA 0x24182800 and ends
at the ret at VA 0x24182C4C, spanning **1,101 bytes** with
full-method SHA256 b0d4cc7e256fd830307d51db70757bb857c0951ec220976a4e281c796ae89b51.

Its native descriptor-insertion helper 0x24182500 is 141 bytes,
SHA256 1edb9978199fcb2d8eb80dab896085e7093588d130306130ea8b38c0efeb8989.
The original Init calls this helper **22 times** at exact positions from
0x24182838 through 0x24182C3A. The helper grows a vector if needed,
shifts existing elements in 24-byte strides when inserting in its
middle, copies six DWORDs (24 bytes) into the selected position, and
returns with callee cleanup ret 8.

After 22 insertions, Init sets byte this+0x18 to zero and returns true.
The original initializer explicitly assigns **five known DWORDs** in
each temporary descriptor. The copied sixth DWORD is not established
as a deterministic source constant and must NOT be falsely described
as original-initialized zero.

The private audit verifies the two complete method hashes, every
CALL edge, selected original helper opcode witnesses, the ending
state write and return, and 88 original string references.

## Source-derived benchmark descriptors

| # | ConVar | Low | High | Label | Weight |
|---:|---|---|---|---|---:|
| 0 | empty | empty | empty | Default | 1.5 |
| 1 | r_drawdetailprops | 0 | 1 | detail props | 1.5 |
| 2 | r_drawworld | 0 | 1 | world geometry | 1.5 |
| 3 | r_drawentities | 0 | 1 | entities | 1.5 |
| 4 | r_3dsky | 0 | 1 | 3D skybox | 1.5 |
| 5 | r_drawdecals | 0 | 1 | decals | 1.5 |
| 6 | mat_stub | 1 | 0 | material system and below | 1.5 |
| 7 | mat_viewportscale | .2 | 1.0 | fillrate | 1.5 |
| 8 | r_drawstaticprops | 0 | 1 | fillrate | 1.5 |
| 9 | r_drawbrushmodels | 0 | 1 | brush models | 1.5 |
| 10 | r_renderoverlayfragment | 0 | 1 | overlays | 1.5 |
| 11 | r_drawdisp | 0 | 1 | displacements | 1.5 |
| 12 | r_drawviewmodel | 0 | 1 | viewmodel | 1.5 |
| 13 | cl_drawhud | 0 | 1 | hud | 1.5 |
| 14 | r_drawparticles | 0 | 1 | particles | 1.5 |
| 15 | r_drawsprites | 0 | 1 | sprites | 1.5 |
| 16 | mat_bumpmap | 0 | 1 | bump mapping | 10.0 |
| 17 | mat_specular | 0 | 1 | specularity | 10.0 |
| 18 | mat_drawwater | 0 | 1 | water | 1.5 |
| 19 | r_dynamic | 0 | 1 | dynamic lighting | 1.5 |
| 20 | r_shadows | 0 | 1 | shadows | 1.5 |
| 21 | r_drawropes | 0 | 1 | ropes | 1.5 |

**Important original quirks:** r_drawstaticprops reuses the earlier
fillrate label. mat_stub swaps the low/high values. Only
mat_bumpmap and mat_specular use 10.0 as their raw weight.
No speculative corrections were applied.

## Independently authored implementation

client_perf_visual_benchmark_owned.h defines the original-derived
22-item description, a six-DWORD 24-byte native x86 owned record,
and an atomic-batch append interface with a conservative independent
64-item capacity limit. It explicitly sets the sixth independent
record field to zero for predictable initialization; this **differs
from unverified original runtime memory**.

Native Win32 test covers every descriptor's name, values, label,
weight, source order, repeat append, capacity refusal, atomic
rollback, and independent cleanup.

audit_sin_original_perf_visual_benchmark.py runs SHA-authenticated
source metadata checks on a private original client DLL, but its
public --self-test constructs only fabricated source-free method
regions. Original executable bytes and proprietary assets are never
committed to this repository.

## Still incomplete

No original ConVar is changed, no graphics benchmark is run, and
no Source renderer objects, timing callbacks or lifecycle methods
have been reconstructed. This work establishes the complete
source-derived descriptor table and insertion helper semantics,
not full CPerfVisualBenchmark game-system parity.

Keep CPerfVisualBenchmark marked incomplete alongside
CClientShadowMgr, C_StatsSystem and C_SoundscapeSystem.
The real distributed client.dll VClient011::Init still fails closed.
The last actual owner-retail Windows checkpoint remains
0x53494E74. This synthetic Windows test is not game boot.
