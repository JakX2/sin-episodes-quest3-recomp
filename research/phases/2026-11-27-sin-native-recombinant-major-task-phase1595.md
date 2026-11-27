# Phase 1595: Recombinant Major StartTask override

Authenticated original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Primary Major vtable +444 ->22377410, +44c ->22376fa0. Byte dispatch 223775a8 and dword 22377590 identify exactly 54,55,69,121,139. All other StartTask calls delegate 22050150; RunTask is literally a five-byte JMP to inherited 220538d0. Do not recover inherited implementation again.

54/55 OR flag40 at +988, call +728 with (0/1,0), assign returned activity through 22049c20, draw [1,1.5] and write now+draw to +9e0. No direct completion. 69 checks ideal schedule39, draws [0,1] and compares strict < +c30: set current69 and reset .2 on success, otherwise add original .1f; then inherited Start. 121 clears +c28 before inherited Start. 139 constructs 48-byte goal via 2202eb00 (kind4, position+c48, activity10, all five unspecified numeric fields -1, flags0, live owner global 225372b0), submits navigator +14 mode0. On accept re-read navigator, acquire enemy +154, call 22074000 then native lifecycle completion; on reject invalidate +c44, resolve original failure string and call native lifecycle fail. No physics or navigation internals recovered. Test exact equality, callback ordering, navigator re-read, native lifecycle failure and all dispatch IDs; generic work stays delegated.

VA/bytes/SHA256:
22377410/494/f5d4f44725ef8ed29f3b11041c3e09e83d7d24fd1a27453f6a3f76929be351a6
22376fa0/16/0669edddb0912b84c8d93dde0793aa89a2e8b33de1ece014a273ae71bc052875
Original owner token not hardcoded in implementation. CI pending.
