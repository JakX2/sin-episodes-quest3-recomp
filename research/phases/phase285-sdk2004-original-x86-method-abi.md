# Phase 285: Original SiN x86 stack-cleanup evidence for Source SDK 2004 startup ABI

The focus is the **original Windows x86 game DLL handshake**, not more synthetic
function counts. We compared the original SHA256-authenticated **2006 SiN**
`client.dll` and `server.dll` first-method vtable targets with the archived
[Source SDK 2004](https://github.com/Source-SDK-Archives/source-sdk-2004/tree/962ba835716d3e6d778054ab3fb4534bf3b35e1a/src_mod)
interface declarations.

Unlike a version-string match, a `__thiscall` / MSVC x86 callee cleanup
value is directly checkable in the **original file** using a disassembler.
Each selected return site below was independently located in the original
PE32 executable and checked on a full-file authenticated binary.

| Original module | Original interface slot | SDK2004 candidate | Confirmed original `ret imm16` cleanup | Independently inspected return sites |
| --- | --- | --- | ---: | ---: |
| `client.dll` | `VClient011[0]` | `IBaseClientDLL::Init` | 12 bytes (3 x86 stack args) | 2 |
| `client.dll` | `GameClientExports001[0]` | inherited virtual deleting destructor | 4 bytes (1 x86 stack arg) | 1 |
| `server.dll` | `ServerGameDLL003[0]` | `IServerGameDLL::DLLInit` | 16 bytes (4 x86 stack args) | 3 |
| `server.dll` | `ServerGameClients003[0]` | `IServerGameClients::GetPlayerLimits` | 12 bytes (3 x86 stack args) | 1 |
| `server.dll` | `ServerGameEnts001[0]` | virtual deleting destructor | 4 bytes (1 x86 stack arg) | 1 |
| **Total** | **5 candidate slots** | | | **8 original return instructions** |

The original client's `Init` and server's `DLLInit` also contain multiple
calls through an x86 cdecl engine factory function pointer and fail-fast
paths, consistent with the SDK architecture. This confirms the **stack
cleanup and argument count shape** of the earliest engine-facing methods;
it does **not** prove a particular argument's type, that all original
paths were discovered, or that full method bodies are SDK-identical.

The implementation `research/tools/verify_original_sdk2004_init_abi.py`:
1. Authenticates *both complete original DLL files* using the original
   SHA256 and size pins from Phase 280.
2. Revalidates all five singleton vptr and MSVC32 RTTI/vtable prefixes.
3. Bounds each explicitly reviewed return-site address to its slot-zero
   code entry and checks PE32 executable section membership.
4. Confirms each reviewed x86 `ret imm16` cleanup matches the SDK2004
   virtual signature's parameter byte count.
5. Outputs **metadata only**, never machine instructions or C pseudocode.
6. Adds synthetic PE32 tests for SHA mutations, tampered returns, incorrect
   cleanup, duplicate sites, missing vtables and non-executable addresses.

The original owner-only Ghidra workflow will emit
`source_sdk_2004_original_x86_abi_stack_shapes.json` after both DLLs
are authenticated, **once the research integration is actually present
on the default branch**. Pull-request CI runs only original-free synthetic
tests. The verified local original-byte evidence above is from the pinned
owner archive, not a hosted original-DLL PR job.

## Windows implementation decision

The right first signatures to use in our independently compiled Win32
virtual boundary, subject to source license review, are:

- `VClient011[0]`: `int __thiscall Init(this, CreateInterfaceFn,
  CreateInterfaceFn, CGlobalVarsBase*)`
- `ServerGameDLL003[0]`: `bool __thiscall DLLInit(this, CreateInterfaceFn,
  CreateInterfaceFn, CreateInterfaceFn, CGlobalVars*)`
- `ServerGameClients003[0]`: `void __thiscall GetPlayerLimits(this,
  int&, int&, int&)`

The `this` pointer is in ECX, not part of the immediate return cleanup.
The return sites for the two deleting destructors accept a hidden 32-bit
delete flag and are **not** evidence of actual SDK gameplay behavior.

**Next direct milestone:** implement and test the earliest two initialization
dispatch signatures against verified owner engine factory callbacks, and
run the existing Phase 284 owner startup tracer on a backed-up Windows
game copy to identify the exact first unmet engine dependency. No genuine
retail startup, independently reconstructed method body, or real SDK-2004
gameplay port is claimed by Phase 285. Do not distribute Valve SDK game
source in the project's MIT tree without its license obligations.
