# Phases 23–34: gameplay systems recovery notes

**Status:** recovered notes, **not** the original report ZIPs or fully reconstructed gameplay modules. Verified historic details below were reported from the owner's original 2006 SiN `server.dll`, but are **not all revalidated by the new baseline verifier**.

## Phases 23–25: SiN vehicle

The `prop_vehicle_sin` / `CPropSinVehicle` implementation uses **100-byte view records**. Recorded fields include `view_allow_weapons` and `view_damage_mod`, and prior disassembly linked these to player-state writes and a damage path. Separate inputs exist for vehicle view selection, in-car monitor camera selection and monitor on/off. Phase 25 produced an independent mock-Source compatibility prototype, with 25 input handlers and 59 standalone assertions. It was **not** a working SourceVR or Quest 3 build.

Important restriction for VR: driver's head tracking, the selected vehicle view and the in-car monitor-camera link are separate concepts. Never flatten all three into a single camera toggle.

## Phase 26: phone and other entities

Investigations inventoried SiN-specific phone/keypad metadata, inputs and class names. The partial phone model was experimental; the full scripted interaction, UI and map event wiring remained unresolved.

## Phases 27–29: poison exposure, delayed injury and HUD

The original code indicated exposure changing a poison meter at approximately **0.15 units/second**, a timer starting at **1.0**, and a delayed damage path with candidate damage amount **10** and approximately **0.6-second shared cooldown**. The distinctive gameplay sequence reported by the player is **slow motion first**, with health damage only after prolonged exposure. These are separate mechanics, not immediate damage on entering the cloud.

Prior work identified network-facing poison state and a client HUD. Actual health-damage application details, class integration, save/load and replayable game tests remain open. Do **not** turn the candidate numbers into authoritative gameplay parameters without rechecking the calling paths.

## Phase 31: antigen dispenser

The original `item_antigen_dispenser` / `CAntigenDispenser` implementation had a **100-unit resource** on spawn and dispensing behavior reported at **7 units/second**. Research recorded an approximately **80-unit use radius** and a **0.1-second per-use cap**, a percent display and resource callbacks. An HL2-style charging interaction is insufficient to reproduce the SiN-specific canister handling.

## Phases 32–33: removable antigen canister

The player's firsthand account established that canisters can be removed, shot, found loose and plugged back into dispensers. The dispenser `OnTouch` checks container flag **0x8** before inserting the canister. Ejection spawns `prop_antigen_explosive_barrel` and transfers remaining resource. Outputs include `OnCanisterInserted` and `OnCanisterEjected`.

Both `prop_antigen_canister` and `prop_antigen_explosive_barrel` share the same `CEntityFactory<CAntigenCanister>`. `CAntigenCanister` inherits `CPropContainer`, including leak behavior. The explosive path invokes shared explosion mechanics and canister break models, plus `OnExplode`.

## Phase 34: container damage flags and explosion

The inherited `CPropContainer` damage callback at original VA **`0x22300F60`** filters damage bits `& 0xC6` (bullet, slash, blast and club in the studied branch). It records attacker information. With container flags bit **0x1**, the callback invokes an explosion virtual immediately; otherwise it can start a leak (flag **0x100**) and use a later hit with flag **0x2**, when the elapsed leak interval exceeds **0.25 seconds**, to explode.

A critical correction from the historical tests: although the generic container defaults to flags **0x3**, the actual `CAntigenCanister` factory passes **0x9**, allowing dispenser insertion (bit 0x8) alongside the immediate-explosion behavior (bit 0x1). Do **not** instantiate antigen canisters with generic `CPropContainer` default flags.

## Engine integration gaps

These remain **standalone reference findings**, not complete game entities. A SourceVR port must recreate the actual map-facing class names, callbacks, events, save-game data and physics integration. Validate against legally obtained original level data; the project must not publish copyrighted maps, models or DLLs.
