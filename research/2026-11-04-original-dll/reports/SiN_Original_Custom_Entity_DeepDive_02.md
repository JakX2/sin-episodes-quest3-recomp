# SiN Episodes: Emergence (2006) — Original custom entities, deep dive 02

**Scope:** Original x86 `client.dll` and `server.dll`, read-only static disassembly. Focus on client→server in-world keypad input, plus both health-charger implementations. No Quest 3 code changes, no runtime tests.

## Binary provenance

| Binary | Original SHA-256 |
|---|---|
| `client.dll` | `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` |
| `server.dll` | `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` |

Companion verifier: `sin_custom_entities_round02_audit.py`. It passed **88 static, address-anchored checks** using SHA-verified original DLLs, including original literals, vtable RTTI, exact call-site bytes, VGUI message keys, and floating-point timing constants. Companion evidence: `sin_custom_entities_round02_evidence.json`.

## 1. Keypad: a client/server KeyValues-style message handshake

The keypad does **not** work merely because the original `prop_keypad` factory creates a server entity. Ritual built coordinated VGUI, client entity, server input, and phone polling logic.

### Client-side class and UI discoveries

| Static anchor | Evidence |
|---|---|
| `CPropKeypadPanel` RTTI/vtable `0x242B89F4` | A dedicated SiN keypad VGUI panel, not generic HL2 HUD |
| `C_PropKeypad` RTTI/vtable `0x242B87BC` | Distinct client representation for `prop_keypad` |
| `C_Keypad` RTTI/vtable `0x242B6664` | Separate client keypad class with event processing |
| `0x24220570` and call-site `0x2422057A`, `0x2422057F` | Panel initialization obtains `KeypadScheme` from `resource/KeypadScheme.res` |
| `0x2426F5AC` | Registers panel identifier `prop_keypad_panel` |
| `0x2421BEC1` | Client event handler references `keypad_combo_entered` |
| `0x2421BEF1` | Same handler references `bad_keypad_combo_entered` |

The `C_Keypad` handler at approximately `0x2421BDF0` compares a candidate input string against the client's keypad state. It can issue either the positive or negative named VGUI event. The exact downstream UI/audio effects of these named events are not determined here.

In `C_PropKeypad`'s client logic, a resolution path at `0x24220601` obtains a typed `C_PropKeypad` entity. Code checks a byte at `+0x510`, and accesses the keypad input string at `+0x511` (e.g. `0x2422062D`). These are **client-side object offsets**, not the server's `+0x3C9` input buffer. Their representation is consistent with networked keypad state, though the exact send/receive field declarations need fuller recovery.

### Client constructs the server-directed message

At **`0x242206F0`**, the `C_PropKeypad` client handler:

1. Resolves the entity handle to the associated `C_PropKeypad` object (`0x2422072B` provides the class's RTTI descriptor).
2. Tests its disabled state (`+0x510`).
3. Retrieves a KeyValues/message object called **`prop_keypad_input`** (`0x2422075C`).
4. Adds an **`entindex`** field (`0x2422077D`) containing the relevant entity index.
5. Adds a **`Command`** field (`0x2422078A`) from the incoming client input.
6. Hands the populated message to a virtual interface method (`0x2422079F`). The complete message transport API is not yet named from the original ABI.

**Crucial distinction:** `prop_keypad_input` here is a *message/control identifier*, not evidence of an independently spawnable map entity. The actual server-side factory verified in the previous round is `prop_keypad` / `CPropKeypad`.

### Server recognizes exactly that message

The server routine at **`0x22302640`** receives a message-like argument and checks:

- its name is `prop_keypad_input` (`0x22302653` with a length-18 inclusive-null comparison);
- its **`entindex`** field (`0x2230267B`);
- its **`Command`** field (`0x22302690`).

It performs an entity-index comparison (`0x22302685`), then forwards the command to `0x22302560` at call site **`0x223026A1`**. The higher-level actor authorization and engine transport contract remain partially unverified; these checks must not be mistaken for comprehensive security analysis.

At **`0x22302560`**, server keypad input processing accesses the input buffer at entity offset **`+0x3C9`**, copies/limits incoming input with a **32-character argument**, and calls `0x22301DE0` through `0x223025B4` to propagate the change to the keypad's output/event mechanism. It also constructs a sound name using the literal **`telephone.`** (`0x223025BB`), consistent with the previously verified digit sound family (`telephone.0` through `telephone.9`). The event path is more complex than a plain string setter.

A second helper at **`0x22301EE0`** uses a copy limit of **31** (at `0x22301F0D`), and `0x22301F40` sets the input to an empty string. These are separate entry paths to the keypad buffer.

### Reconstructed execution sketch (not original source)

```text
CPropKeypadPanel / C_Keypad VGUI interactions
    -> C_PropKeypad client entity and input handler (0x242206F0)
    -> message name: prop_keypad_input
       fields: entindex, Command
    -> engine/interface message forwarding (exact ABI unresolved)
    -> server receives prop_keypad_input (0x22302640)
    -> validates name / compares entity index / reads Command
    -> server keypad processor (0x22302560)
    -> server input string +0x3C9, OnInput and telephone.* sound
    -> CPropPhone::PhoneThink reads linked keypad and looks up PhoneBook
```

The final phone step was established by the previous report; this round adds a previously missing, verified **client-to-server message correspondence**.

## 2. Health charger code: two classes, shared healing pattern, different layouts

The earlier report established separate factories:

| Spawn classname | Runtime class | Verified use-method vtable slot `87` | Juice field |
|---|---|---|---|
| `func_healthcharger` | `CWallHealth` | `0x222F9CD0` | `+0x410` |
| `item_healthcharger` | `CNewWallHealth` | `0x222FAB20` | `+0x49C` |

These are distinct types with different base classes. The methods above were located by walking original RTTI-linked vtables, not guessing from nearby function order.

### Verified healing sequence, *both* variants

1. Confirm use actor and relevant interaction eligibility via virtual method (`+0x120`).
2. Check available charger juice and the recipient's current health (`+0x9C`) versus maximum health (`+0x98`).
3. Apply a health request through recipient virtual method **`+0xE4`**, pushing float **`1.0`** and integer **`0`**.
4. Test the returned integer: **only if nonzero**, decrement stored charger juice by one.
5. Divide the remaining juice by a configured health-charger capacity (`sk_healthcharger` current numeric value) and pass the ratio to a per-charger output/event helper.
6. Record a subsequent use-rate time using the original constant **`0.1` seconds**. The time store occurs after the request even if the return value is zero, provided this portion of the routine is reached.

The direct x86 call sites are:

| Operation | `CWallHealth` | `CNewWallHealth` |
|---|---|---|
| Health request `1.0` | `0x222F9E9D` | `0x222FAD20` |
| Virtual `+0xE4` | `0x222F9EA4` | `0x222FAD27` |
| Check return | `0x222F9EAA` | `0x222FAD2D` |
| Juice decrement | `0x222F9EAE` | `0x222FAD31` |
| Divide juice/capacity | `0x222F9EC2` | `0x222FAD46` |
| Add `0.1`-second constant | `0x222F9EDD` | `0x222FAD61` |

### Capacity and initialization

`sk_healthcharger` is registered in the original executable at `0x223C41F0–0x223C4204`. The constructor receives:

- name: **`sk_healthcharger`** at `0x2246C918`;
- compiled default string: **`"50"`** at `0x22408DEC`;
- ConVar object: `0x22596670`;
- references to current-value storage through pointer `0x22596688` and `+0x28` in charging/recharge code.

The animated charger initializes `+0x49C` using this configured value at **`0x222F9F95`**. Runtime difficulty, scripts, or configuration may override the compiled default, so **50 should not be hardcoded as an immutable capacity**.

### `CNewWallHealth` refill and visual/display state

At **`0x222F9B60`**, the animated charger's refill callback:

- emits `WallHealth.Recharge` (`0x222F9B66`);
- reads the capacity ConVar through `0x22596688`;
- converts the capacity to an integer and restores juice at `+0x49C` (`0x222F9B8E`);
- updates its floating-point display/animation amount at `+0x4E0` (`0x222F9B94`);
- clears particular charging/empty state fields and updates animation/think state.

The class metadata also contains `m_flJuice`, `m_OutRemainingHealth`, `OutRemainingHealth`, and a named input `Recharge`. The current analysis establishes these original names and the nearby runtime fields, but a complete timing/recharge state machine still needs additional analysis.

### Other verified constants (do not overinterpret)

| Original data VA | Type/value | Verified use neighborhood |
|---|---|---|
| `0x223D8FE8` | double `0.1` | Health-use pacing |
| `0x223D99D0` | float `0.25` | Scheduled use processing |
| `0x2246B950` | double `0.56` | Transition to looping charging sound |
| `0x2246AEF8` | double `0.62` | Empty/deny timing path |
| `0x2246C378` | float ~`1/7` | Animated charger's visual/juice transition |

These are code constants, not a measured in-game timing profile. Other rules and frame scheduling can affect observed behavior.

### Pseudocode for the shared heal operation (simplified)

```cpp
// Derived from verified disassembly; not Ritual's source code.
void HealthChargerUse(Entity* user) {
    if (!eligible(user)) return;
    if (juice <= 0) { HandleEmptyOrRechargeState(); return; }
    if (user->health >= user->maxHealth) { HandleFullHealthDeny(); return; }
    if (!canChargeAtCurrentTime()) return;

    StartOrContinueSoundState();
    int accepted = user->GiveHealthLikeMethod(1.0f, 0); // virtual offset +0xE4
    if (accepted != 0) --juice;
    UpdateRemainingOutput(float(juice) / currentSkHealthcharger());
    nextChargeTime = currentTime + 0.1;
}
```

**Limitations:** The sketch is intentionally abstract. It omits first-use/loop sounds, the precise offsets of timers and intermediate states, method arguments beyond those seen at the callsite, and any engine/player restrictions. No source-level method prototype is claimed from the virtual offset alone.

## 3. Implications for the eventual port (research only)

The original binaries now provide much better fault categories for HL2-based runtime incompatibilities:

- A correctly spawned **server `prop_keypad`** is insufficient without its `C_PropKeypad` client state, `CPropKeypadPanel` resource, and input-message relay.
- The original **`prop_keypad_input`** is a named VGUI/KeyValues message and must be routed to the server receiver. It should not be implemented as an additional map entity without evidence.
- A generic charger that blindly gives health may miss the **per-use return-value check**, `sk_healthcharger` capacity, **remaining-juice output**, and the **recharge audiovisual states**.
- `func_healthcharger` and `item_healthcharger` cannot blindly share one entity memory layout even if a reusable algorithm exists; their original field offsets and ancestry differ.
- The compiled DLL evidence alone does not establish why any particular map phone or charger failed in an experimental runtime; runtime ABI, content resources and map I/O still need to be checked separately.

## 4. Remaining work

1. Trace `C_PropKeypad` **RecvTable** to the server `CPropKeypad` **SendTable** and determine exact disabled/input field encoding.
2. Confirm message transport interface and actor authorization for `prop_keypad_input` end-to-end (potential bug or compatibility mismatch).
3. Reverse the complete `CNewWallHealth` charge/deny/recharge think-scheduling state machine, including `Recharge` map input.
4. Locate `resource/KeypadScheme.res` and actual `PhoneBook` resources to connect game UI to campaign content.
5. Extend the same factory, datamap and cross-DLL tracing to script events, SinTEK vehicles, and SiN weapons.

**Method statement:** Everything above is recovered from original binary code and data. Any interpretations such as "health-gain virtual method" and "networked keypad state" are grounded in instruction use and known calls, but original debug symbols and a full runnable game trace remain unavailable. No proprietary DLL sections are embedded in the report or scripts.