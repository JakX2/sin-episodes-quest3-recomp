# SiN Episodes: Emergence (2006) | Custom Entity Deep Dive 01

**Focus:** The original `server.dll` and `client.dll` phone, keypad, and health-station functionality. Read-only static reconstruction. No Quest 3/SourceVR code changed.

## Source identity and verification

| Original binary | SHA-256 |
|---|---|
| `server.dll` | `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650` |
| `client.dll` | `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` |

Companion script: `sin_custom_entities_audit.py`. **118 address-anchored checks passed** across the two original DLLs. Companion machine-readable evidence: `sin_custom_entities_evidence.json`. The audit verifies class/factory RTTI links, entity registry immediate operands, key names and field offsets, input handler pointers, phone control-flow instruction anchors, and client receive-table registration. The script does not implement or run the game.

## 1. Verified entity factory registrations

A C++ RTTI name is not enough to prove an entity can spawn. Here, each row has three independent static anchors: the exact map classname used in a registration function, the address of its factory instance passed to registration, and the factory's actual MSVC RTTI class.

| Map classname | C++ class from factory RTTI | Name pushed at | Factory object |
|---|---|---|---|
| `prop_phone` | `CPropPhone` | `0x223C47A7` | `0x2252A7D0` |
| `prop_keypad` | `CPropKeypad` | `0x223C4787` | `0x2252A58C` |
| `func_healthcharger` | `CWallHealth` | `0x223C4317` | `0x22528C10` |
| `item_healthcharger` | `CNewWallHealth` | `0x223C4347` | `0x22528C14` |

The factory object for each row points to a vtable whose RTTI is `CEntityFactory<that exact class>`. This is stronger evidence than a string-only scan. These factory registrations are present in the **original** server, not proof of correct behavior in an HL2-based replacement runtime.

### Class ancestry differences

- `CPropPhone` → `CBaseAnimating` → `CBaseEntity`.
- `CPropKeypad` → `CVGuiScreen` → `CBaseEntity`. The keypad is a VGUI screen entity, not an ordinary physics prop.
- `CWallHealth` → `CBaseToggle` → `CBaseEntity`.
- `CNewWallHealth` → `CBaseAnimating` → `CBaseEntity`.
- `CAntigenDispenser` → `CBaseAnimating` → `CBaseEntity`. This is distinct from both wall health classes.

## 2. Phone: server behavior actually compiled into the DLL

### A. Original configuration and persisted state

`CPropPhone` has a typed-description / datamap containing these literal field names and offsets:

| Field | Object offset | External key / meaning suggested by name |
|---|---:|---|
| `m_bDisabled` | `+0x494` | `Disabled` |
| `m_sFailSound` | `+0x498` | `FailSound` |
| `m_sPhoneBook` | `+0x49C` | `PhoneBook` |
| `m_nDigitsMatched` | `+0x4A4` | Counts the prior matched/processed input length |
| `m_hKeypad` | `+0x4A8` | Entity handle to linked keypad |
| `m_sKeypadName` | `+0x4AC` | `KeypadName` |

A nearby `m_fLastUse` field at `+0x4A0` appears in the same datamap, not yet deeply traced. The phone caches a loaded phone-book object pointer at `+0x4B0` (observed store at `0x22302854`).

The datamap also registers an external **`DialNumber`** input with original function pointer **`0x22302D80`** (`InputDialNumber`). It accepts an input variant and routes it to the linked keypad/input machinery. The exact full input-variant type should be checked before producing a source-level signature.

### B. Spawn / startup

| Address | Verified behavior |
|---|---|
| `0x223027C0` | Phone initialization/spawn implementation; includes linked-keypad name reference |
| `0x223027EE` | Default phone model pointer set to `models/props_general/payphone_01.mdl` when no model is supplied |
| `0x22302844` | Allocates/initializes a KeyValues-like object rooted at `PhoneBook` |
| `0x22302854` | Stores the loaded-book object pointer into phone `+0x4B0` |
| `0x22302875` | Calls file-load helper using configured phonebook name |
| `0x2230287E` | References diagnostic `Failed to load phone book!` |
| `0x22302F18` | Registers periodic callback `0x22302AC0` |

The **content and location of the actual phone-book configuration file** are not determined by these DLLs alone. Map keys and game resources are a separate evidence source.

### C. Timed `PhoneThink` state machine

At **`0x22302AC0`**, the phone's periodic callback:

1. Checks the `Disabled` state (`+0x494`) and the presence of its phone-book data (`+0x4B0`).
2. Resolves its linked keypad entity handle (`+0x4A8`).
3. Reads the keypad's input buffer (keypad's `+0x3C9`).
4. Tests the number's length against **4 characters minimum** at `0x22302C6E`.
5. Compares the input length against the previously processed count `+0x4A4` at `0x22302C77`.
6. Performs a lookup in the loaded phone book; one code path checks whether the returned value starts with **`*`** (`0x22302CB1`), and treats that as a special sound value.
7. Uses the sound emission helper for a matching or fallback value; a failure path uses the configured `FailSound`. One branch updates `+0x4A4`, while another resets the keypad and counter.
8. Schedules its next periodic update.

This is reconstructed **control flow**. It does **not** establish the full phone-book format, every number's final effect, or how every campaign map configures calls. Those need the original map scripts / resources / runtime observations.

### D. Keypad system

`CPropKeypad` is a `CVGuiScreen` subtype, and the original server has separate `CKeypad` code. The keypad registers **`m_sInput`** at object offset **`+0x3C9`** as a **32-character-sized input field** in its metadata (the underlying string implementation and terminator rules remain to be verified). It also exposes `OnInput`, `InputSetInput`, and `InputClear` names. The server uses literal keypad sound entries **`telephone.0` through `telephone.9`**, and the `prop_keypad_input` control identifier.

Notable anchors: `0x22301D7F` (size `0x20`), `0x22301D81` (field offset `0x3C9`), `0x22301B10–0x22301B6F` (digit sound registration), `0x22301EE0` (update input helper), `0x22301F40` (clear input helper).

## 3. Client responsibilities: genuine SiN-specific VGUI classes

Original `client.dll` RTTI confirms:

- `C_PropPhone` → `C_BaseAnimating`.
- `C_PropKeypad` → `C_VGuiScreen`.
- `C_Keypad` → `C_VGuiScreen`.
- `CPropKeypadPanel` → `CVGuiScreenPanel` → `vgui::EditablePanel`.

The client registers **`DT_CPropPhone`** at `0x24220838`, in a registration function beginning `0x242207F0`. The keypad-panel string **`prop_keypad_panel`** also occurs in the original client.

**Implication for analysis:** a replacement HL2 runtime that knows only generic server-side entities may still lack the SiN-specific client panel and receive-class registration. These missing pieces must be investigated separately. This analysis **does not yet resolve the full send/receive field lists**, so do not assume the phone directly networks every datamap field.

## 4. Health stations: two registered entity types with different ancestry

The original DLL has **both**:

- `func_healthcharger` / `CWallHealth` / `CBaseToggle` (brush/toggle-style ancestry).
- `item_healthcharger` / `CNewWallHealth` / `CBaseAnimating` (animated model entity).

They are **not two classnames for the same factory**. This is proven by both registry object addresses and distinct RTTI family names.

The station code uses these exact sound names:

- `WallHealth.Recharge`
- `WallHealth.LoopingContinueCharge`
- `WallHealth.Start`
- `WallHealth.Deny`

The class implementation also references model `models/props_combine/health_charger001.mdl`, and data-description strings `m_iOn`, `m_iJuice`, `m_iReactivate`, `m_flNextCharge`, `OutRemainingHealth`, `m_OutRemainingHealth`, `Recharge`, `m_flJuice`, and `emptyclick`.

The new wall-health use path at **`0x222FAB20`** examines the interacting entity; its assembly at `0x222FABD6` loads the recipient's health `+0x9C` and compares it to max health `+0x98` at `0x222FABDC`. The same routine branches on the station's remaining charge and emits start, loop, deny or empty sound behavior. This establishes a real interaction routine; the exact consumption and recharge timing formula remains unverified.

**Separately**, the original server registers `item_antigen_dispenser` for `CAntigenDispenser`, whose gameplay code was investigated previously and routes antigen healing via a type flag. Do not conflate an antigen dispenser with either wall charger.

## 5. Why this matters for understanding the HL2 loading failures

The compiled evidence supports **multiple distinct failure categories**, which must not be collapsed into one diagnosis:

1. **Missing server entity factory:** map classname cannot create the correct C++ class at all.
2. **Missing datamap / input handlers:** entity creates but `DialNumber`, `InputSetInput`, `Recharge`, etc. do not work.
3. **Missing SiN-specific client entities / VGUI panels:** in-world keypad interface or prediction/representation is absent.
4. **Missing content:** external phone-book file, models, VGUI resources, sounds, or map-specific configuration unavailable.
5. **Save/restore or networking differences:** the state or client-side class survives poorly across transitions or snapshots.

These are **reasoned implications**, not proof of which single condition caused the user's earlier runtime failure. Neither DLL has been executed in the original or replacement engine during this static investigation.

## 6. Next read-only targets

- Recover the full client `CPropKeypadPanel` input dispatch and link to `CPropKeypad`/`CPropPhone`.
- Recover `PhoneBook` KeyValues layout and cross-reference original game files for defined numbers and sound commands.
- Trace the precise `CNewWallHealth` use/charge/recharge equation and outputs.
- Map SiN's event handlers, phone-station map inputs, and eventually server SendTables versus client RecvTables.
- Expand inventory to `CPropSinVehicle` and weapons using the same registry/RTTI/data-description technique.

No binary code was patched or executed. No proprietary DLL bytes are included in the report or verification script. Original artifacts remain untouched.