# Phase 539: original 2006 SiN client.dll `VClient011::Init` suffix and game-system lifecycle decompilation

## Scope and independent evidence boundary

This phase continues the already authenticated original `client.dll` `VClient011::Init` at **VA `0x240FE0E0`**, 1,220 bytes, SHA-256 `14d81449866220aaef1f5f906b577179ea107df78a424a364c715261adbb35db`. The original companion `server.dll` and `client.dll` **whole-file SHA-256s** are required by the verifier (`server`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`; `client`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`).

**This is a source-level reverse-engineering report, not an implementation of a Source engine DLL.** The pseudocode is independently written and annotates original control flow, including unresolved targets. No original commercial machine-code bytes, disassembly dumps, or extracted assets are committed.

Five additional **complete original executable functions** were individually authenticated by whole-function SHA-256 and their following INT3 padding. Their combined size is **525 original bytes** (the already authenticated 1,220-byte Init is not double-counted):

| Original client VA | Exact length | Role recoverable from original instructions | Whole-function SHA-256 |
|---|---:|---|---|
| `0x24156840` | 23 | Adapter to append a system pointer to growable array | `e48841e7e14a9565ac425416c3fc3230615c8246fa603be65d7149500396292e` |
| `0x24156860` | 277 | Discover/merge registered game systems, call each initialization virtual, return result | `b4f5ca8aefcb1248cdd7a62edf042a3f7fd5a04ac782fde277d13f335067ccb7` |
| `0x24164120` | 54 | Resolve linked-list interface providers; release and fail on unresolved initialized slot | `645a9002d77b476692fe8688d691e87c4d2ebf8a61976be9eda071e41d7d3b7f` |
| `0x24182C60` | 97 | Three additional factory requests and combined setup callback | `46e079ea76e0cfee721efe3a421d68920cf980079aea4d76d7542f6f9b5281f2` |
| `0x240FCDD0` | 74 | Allocate/initialize final 0x4B0-byte client-owned object and publish pointer, or set global null | `8a9a82e9e7a8bbd9eb61c80802270817f00c8053bff3b21ba941e9c46311d289` |

The report calls these *roles* rather than claiming original C++ method names. All addresses are original PE32 VAs, not Android addresses or SourceVR ABI slots.

## Two distinct initialization systems, not one

### A. Linked-list required-interface resolver: `0x24164120`

Called from `Init` at **`0x240FE39B`**, immediately followed by an `al` test and fail-closed JE at `0x240FE3A2` to the common Init failure VA `0x240FE59E`.

Original global `0x24369374` holds the head of a linked list. Each original record has these observed 32-bit fields:

| Offset | Recovered instruction-level role |
|---|---|
| `+0x00` | Address of a destination pointer slot, dereferenced on success |
| `+0x04` | Callback invoked via native `call [record+4]` to produce interface pointer |
| `+0x08` | Callback used by separate original cleanup at `0x241640F0` |
| `+0x0C` | Link to next record |

The resolver walks records in order. On a non-null produced interface, it writes the returned pointer through the destination slot. If the callback returns null and the original destination slot is **also null**, it moves to the next record. If the callback returns null **but the record has a non-null destination slot**, it invokes the native cleanup helper `0x241640F0` and returns false. Otherwise it returns true after the list is exhausted.

```cpp
// C-like decompilation of 0x24164120, not exact original type definitions.
bool ResolveRegisteredInterfaces() {
    for (NativeRecord *p = *(NativeRecord**)0x24369374; p; p = p->next) {
        void *resolved = p->getter(); // original indirect call [p+4]
        if (resolved) {
            *p->destination_slot = resolved;
        } else if (p->destination_slot != nullptr) {
            OriginalCleanup_241640F0();
            return false;
        }
    }
    return true;
}
```

`0x241640F0` and the higher-level meanings of the individual callbacks remain dependencies for further decompilation. The cleanup is **not** claimed to be an ordinary C++ destructor or transaction rollback; this report preserves the actual call target.

### B. Client game-system registration, merge and initialization: `0x24156840` / `0x24156860`

The `Init` suffix calls **`0x24156840` exactly twelve times** in this order, each with one pointer-sized argument. This adapter forwards the address of that argument, the current collection count, and a pointer to the original dynamic-list object at `0x242FBFA8` to native array insertion `0x24156640`. This is a *pointer list*, not a vector of engine-interface version names.

| # | Original append CALL site | Immediately supplied original source |
|---:|---|---|
| 1 | `0x240FE3FC` | Return pointer of `0x2413D240` |
| 2 | `0x240FE407` | Return pointer of `0x241BCAA0` |
| 3 | `0x240FE420` | `0x242FA79C` pointer, adjusted by `+4` if non-null, else null |
| 4 | `0x240FE42B` | Return pointer of `0x24123CB0` |
| 5 | `0x240FE436` | Return pointer of `0x241ED2F0` |
| 6 | `0x240FE441` | Return pointer of `0x24105660` |
| 7 | `0x240FE44D` | Value read from global `0x242FA890` |
| 8 | `0x240FE457` | Address of static client object `0x24378DE8` |
| 9 | `0x240FE461` | Address of static client object `0x24378D18` |
| 10 | `0x240FE46B` | Address of static client object `0x242FA6B8` |
| 11 | `0x240FE476` | Return pointer of `0x240F23A0` |
| 12 | `0x240FE481` | Return pointer of `0x24181FC0` |

The first two arguments are cleaned up before item 3. At `0x240FE48E` the original caller cleans **another 0x28 bytes** of stack arguments. All twelve calls target the same exact native adapter, and there is **no immediate per-registration Boolean failure gate**.

Next, at `0x240FE4B3`, `Init` calls original `0x24156860`. This method reads original linked-list head `0x24366C8C`, traverses next pointers at `+4`, compares each entry to the dynamic array rooted at `0x242FBFA8`, and appends missing entries (or routes duplicate detection to its original diagnostic). It then sets the global linked-list head to null, sets initialization flag byte `0x24366C84 = 1`, and calls the first virtual function on each resulting registered system. A zero byte result from **any** initializer returns false, otherwise it returns true. `Init` itself checks this Boolean and branches to `0x240FE59E` on zero.

The original diagnostic associated with the duplicate path reads `AutoGameSystem already added to game system list!!!` (literal stored at `0x2429EED0`). This provides stronger naming evidence than an otherwise anonymous dynamic pointer array, but does not prove each individual object subclass.

```cpp
// C-like decompilation; original game-system pointer types are not recovered.
void AddClientGameSystem(void *system) {
    OriginalDynamicListInsert_24156640(0x242FBFA8, current_count, &system);
}

bool InitializeClientGameSystems() {
    for (NativeSystemLink *node = global_head_24366C8C;
         node != nullptr; node = node->next) {
        if (contains(original_system_array_242FBFA8, node->system)) {
            OriginalDuplicateSystemDiagnostic();
        } else {
            append(original_system_array_242FBFA8, node->system);
        }
    }
    global_head_24366C8C = nullptr;
    initialized_flag_24366C84 = 1;
    for (void *system : original_system_array_242FBFA8) {
        if (!OriginalGameSystemVirtual0(system)) return false;
    }
    return true;
}
```

This is a control-flow sketch, not a verified portable replacement of native vector growth/capacity corner cases or the duplicate diagnostic's side effects. `0x24156640` and the individual registered system virtuals remain future decompilation targets.

## The suffix has at least five explicit additional fail-closed gates

These five original call/branch pairs are authenticated by exact CALL targets and JE destinations, beyond Phase538's 21 direct engine factory null checks and other derived-interface checks:

| Original direct CALL | Original native target | JE instruction to shared failure `0x240FE59E` | Observed meaning |
|---|---|---|---|
| `0x240FE39B` | `0x24164120` | `0x240FE3A2` | Linked required-interface resolution |
| `0x240FE3B9` | `0x2417F680` | `0x240FE3C0` | Further subsystem initialisation, `0x800` supplied argument |
| `0x240FE3C7` | `0x241EDFB0` | `0x240FE3D1` | Another factory-dependent subsystem init |
| `0x240FE4B3` | `0x24156860` | `0x240FE4BA` | Registered client game-systems init |
| `0x240FE540` | `0x24182C60` | `0x240FE54A` | Secondary factory additional requirements |

The final row uses a **two-byte short JE**, unlike the other four six-byte JE instructions. All return to the *same* original failure block. Passing these gates is necessary but not sufficient to prove the full game can initialize independently.

## Second factory and its exact three interface names

The original `Init` function loads one of its stack parameters at `0x240FE53B`, pushes it, and calls `0x24182C60` at `0x240FE540`. The helper treats this value as another factory pointer and requests three named interfaces in strict order:

| Request | Original name | Native resulting global | Immediate failure if absent |
|---:|---|---|---|
| 1 | `VPhysics030` | `0x24374398` | Yes |
| 2 | `VPhysicsSurfaceProps001` | `0x243743A8` | Yes |
| 3 | `VPhysicsCollision007` | `0x243743A0` | Yes |

All three names come directly from original null-terminated client strings at `0x242A2338`, `0x242A2320`, `0x242A2308` and the helper's native code. It calls `0x241927A0` with two fetched objects after all three requests succeed, then returns `al=1`; every missing interface leads to common `al=0` at `0x24182CBD`.

**Owner priority boundary:** this is a *client startup ABI dependency inventory*, not permission to spend time rewriting Source physics, material/surface properties, or model rendering. The existing HL2-based content port handles those systems; the decompilation record merely preserves original dependency order for future runtime integration.

## Late allocation does not gate the original Init Boolean

At `0x240FE590`, the original function calls `0x240FCDD0`. That method requests **0x4B0 bytes** from `0x240C4FF0`, invokes `0x240CCE30` on a non-null result, writes four vtable pointers (`0x2429632C`, `0x242962C4`, `0x2429628C`, `0x24296274`) at offsets `0`, `+4`, `+8`, `+0xC`, and publishes the pointer in global **`0x2433F3F4`**. If allocation fails, the method writes **null** to this global instead.

Crucially, **`VClient011::Init` does not check the return of `0x240FCDD0`**. It immediately executes its success return block at `0x240FE595` (`eax=1`, `ret 0x0C`). The opposite failure block is `0x240FE59E` (`eax=0`, `ret 0x0C`). Do not accidentally turn this late pointer allocation into a new mandatory error gate in a future reconstruction.

## Readable original-tail execution-order sketch

```cpp
// This is only the post-interface portion of original VClient011::Init.
// Each unresolved Original_* is a genuine native call or virtual dispatch,
// not a guessed Source SDK API. For the 21 initial interface queries, see Phase538.
if (!OriginalResolveRegisteredInterfaces_24164120()) return 0;
if (!OriginalSetup_2417F680(0x800, material_system_ptr)) return 0;
if (!OriginalSetup_241EDFB0(engine_factory)) return 0;

// Third independent factory-derived interface failure is checked by here.
if (!OriginalSurfaceFactoryQuery_MatSystemSurface003()) return 0;

AddClientGameSystem(OriginalGetter_2413D240());
AddClientGameSystem(OriginalGetter_241BCAA0());
AddClientGameSystem(original_242FA79C ? original_242FA79C + 4 : nullptr);
AddClientGameSystem(OriginalGetter_24123CB0());
AddClientGameSystem(OriginalGetter_241ED2F0());
AddClientGameSystem(OriginalGetter_24105660());
AddClientGameSystem(original_242FA890);
AddClientGameSystem(original_at_24378DE8);
AddClientGameSystem(original_at_24378D18);
AddClientGameSystem(original_at_242FA6B8);
AddClientGameSystem(OriginalGetter_240F23A0());
AddClientGameSystem(OriginalGetter_24181FC0());

OriginalSubsystemOperations_240FE486_to_240FE4B3();
if (!InitializeClientGameSystems_24156860()) return 0;
OriginalSubsystemOperations_240FE4C0_to_240FE53B();
if (!SecondFactoryRequirements_24182C60(second_factory)) return 0;
OriginalDispatchThreeFactoryObjects_240FE54C_to_240FE590();
OriginalLateAllocatedObject_240FCDD0(); // result NOT checked
return 1;
```

The sketch preserves the order and key branch decisions. It does **not** claim the method's unknown `0x240FE486..0x240FE53B` virtual dispatches have been fully semantically recovered. Those remain important dependencies to decompile, along with the first `0x2417F680` and `0x241EDFB0` failure-gated helpers.

## Reproduce with owner's exact DLLs

```sh
python3 research/tools/audit_sin_original_client_init_tail.py --self-test
python3 research/tools/audit_sin_original_client_init_tail.py --bin-zip /private/original/bin.zip
```

The original-DLL audit passed and emitted `ORIGINAL_CLIENT_INIT_TAIL_CONTROL_FLOW_AUTHENTICATED`: five complete helper methods (525 bytes), twelve ordered registrations, five additional failure-gated call pairs, three secondary factory interface names, game-system array/link/flag witnesses and late allocation's non-checked return. Offline synthetic regressions mutate each byte of an invented CALL/JE, test the short JE and refuse invalid original hashes.

## Next original-binary decompilation targets

1. **`0x2417F680` and `0x241EDFB0`:** resolve exact checks behind the early post-interface failures.
2. **`0x24156640`:** complete array append and growth semantics, especially null/duplicate edge cases, to turn the game-system sketch into a more faithful C++ source-level expression.
3. **`0x24156860` callback destinations:** identify each of the twelve registered game-system objects by native RTTI where present, then recover real init virtuals rather than pretend that every system shares the same semantic function.
4. **`0x24182C60` from the original caller:** strengthen the exact second factory argument attribution. Do not build physics or surface implementations while the owner prioritizes the SiN-specific DLL decompilation.
5. Continue parallel **server-side `CNPC_MobGrunt` schedule/weapon/AI** function recovery and client HUD/input/message decompilation, retaining per-function SHA witnesses and explicit unknowns.

No actual independently compiled game DLL, playable map, Android APK, or Quest 3 native execution is claimed by this phase.