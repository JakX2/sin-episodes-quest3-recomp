# Gameplay Phase582: original Grunt event 0x8D/0x92 numeric signals, timers and angular normalization

**Scope:** 2006 SiN Episodes: Emergence original `server.dll`, following the Grunt-only animation dispatcher from gameplay Phase581. This decompiles two previously unexamined original functions and adds a source-bounded reconstruction of *already-authenticated* Grunt-specific event cases. It does **not** reimplement Half-Life 2 physics/surfaces, model/prop loading/rendering, game boot, Android or VR.

## Exact original binary and complete native method identity

The unchanged owner-private original binaries are SHA256-authenticated **before** reading native code:

- `server.dll`: `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`
- `client.dll`: `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`
- Preferred 32-bit PE image base `0x22000000`; all VAs refer to this original unchanged binary.

| Original VA | Whole extent | Whole-function original SHA256 | Role |
|---|---:|---|---|
| `0x22049C20` | **181 bytes**, through `0x22049CD4` | `1662be502c6a43a12394b61206a00c54178d6e15e29e415defb8fabe6f55f524` | Numeric NPC state/signal request dispatch used by several Grunt event branches |
| `0x22028200` | **60 bytes**, through `0x2202823B` | `0804a4997f4b278c9b17c985396aeb16edcefaead374be03c3d72c0e421842e0` | Angle-domain encode/decode under temporary x87 rounding mode |
| **Total** | **241 new native bytes** | Two independently verified full functions | Phase581's previously authenticated Grunt dispatcher is **not** counted again |

Both whole native method extents are followed by original `INT3` padding and terminate in original `ret 4` and `ret` respectively. The Phase581 enclosing handler `0x22351C20` (1330 bytes, inclusive of embedded jump tables) is independently whole-body SHA-verified again. No raw assembly dumps or original DLLs are committed.

## Original method 0x22049C20: signal routing is not simple assignment

The original method takes one 32-bit signal argument, retains the NPC at `ECX`, and distinguishes several cases:

1. When signal equals **2**, the method immediately returns. This precedes the optional diagnostic logging gate and does **not** update NPC `+0x880`.
2. Otherwise an optional debug path checks a native global (`0x2254D090` member `+0x2C`) and NPC byte `+0xA9 & 0x10`, using native formatting/callback helpers. The independent model represents this as an **opaque diagnostic callback**; the string formatter's C++ identity is not recovered.
3. When signal equals **0**, it invokes owner virtual byte offset **`+0x4DC(0)`** and returns. It does not enter the nonzero state storage branch.
4. For other values, it stores the signal directly to NPC **`+0x880`**.
5. The special nonzero signal **`0x9A`** stops there. Other nonzero values call original **`0x220EB8A0(owner)`**. If the result is nonzero, the original invokes **`0x22049A60`** with the signal and pointers to NPC raw fields **`+0x884`, `+0x888`, `+0x88C`**. The type/meaning and downstream operations are deliberately unresolved.

The direct E8 callsites authenticated inside this method are `0x22049C5C` and `0x22049C66` to `0x22022A90`, `0x22049CA4` to `0x220EB8A0`, and `0x22049CCB` to `0x22049A60`. Previous Phase581 has three independently authenticated incoming Grunt-event calls: `0x22351C75`, `0x22351FC7`, and `0x22352084`. This means the event-`0x91` signal `0x1E` and event-`0x8D` signal `0x19` are **real raw numeric state requests** through the same native entry.

## Original method 0x22028200: quantized angular wrap

The entire 60-byte native method uses x87:

1. Reads a **single-precision** input angle from the caller stack.
2. Multiplies by original float **182.04444885253906** at `0x223D8E0C`.
3. Saves x87 control word, sets both rounding-control bits with `or ah,0x0C`, performs a signed 32-bit integer conversion by `fistp`, and restores the original control word.
4. Masks the converted integer with **`0xFFFF`**, converts back to an x87 real, and multiplies by original float **0.0054931640625** at `0x223D8E08`. The result remains on the x87 stack.

The authored C++17 host approximation rejects nonfinite/out-of-range values and uses `trunc`/16-bit masking. This is faithful to the *supported ordinary finite-domain algorithm*, **not** a claim of exact x87 extended-precision or FPU-exception behavior at boundaries.

This method is reached at the already authenticated original event `0x8D` callsite **`0x22351F5E`**. The parent event code separately converts the returned angle with native float `0.01745329238474369` at `0x223D48EC`, then performs native `fsin`/`fcos` operations with a genuine **220.0f** vector scale at `0x2244579C`. Those are two additional original `.rdata` float witnesses. The exact callback contract of `0x22141A00` is left abstract.

## Two Grunt event cases in the authentic Phase581 dispatcher

### Event `0x92`, branch `0x22351D7F`

It executes the following exact original call order:

1. `0x2214C730` with literal argument `0x400`.
2. `0x22032220` with argument `0`.
3. `0x220303B0` with argument `4`.
4. `0x22030390` with argument `1`.
5. `0x22143610` with caller stack values **`3,0`**.
6. Engine-global random float via native random interface vtable `+0x04` with arguments **5.0f, 10.0f**.
7. Reads original current engine time (global `0x22572190`, member `+0x0C`), adds the random float, writes owner float **`+0xC20`**. No unverified symbolic purpose for the timer is claimed.

The independent host tests preserve all five original callsite addresses, argument values, callback order, and deadline update. They do not invent function names for the five native callees.

### Event `0x8D`, branch `0x22351F3D`

1. Original random integer interface returns a value using signed inputs **`-180` and `180`**.
2. Converts it into float and invokes the 60-byte quantization function `0x22028200`.
3. Invokes random float interface with **`0.1f` and `0.5f`**. Its exact interaction with the subsequent x87 stack is not modeled as a physics/vector API.
4. Uses the quantized angle and genuine original constants `0.01745329238474369` and `220.0f` in native trigonometry; calls original `0x22141A00` with a stack vector-like argument.
5. Calls original numeric signal helper **`0x22049C20(owner,0x19)`**, then clears owner byte **`+0xC3C=0`**.
6. Invokes original random float with **`2.0f` and `5.0f`**, adds current engine time, writes owner float **`+0xC38`**.

The host model intentionally exposes the native directional call as an abstract callback, rather than substituting guessed spatial code or asserting exact x87 vector coordinates. Event `0x8F` also has directional updates, but its independent native calls remain an explicit next research boundary.

## Validation, source model and limitations

The read-only SHA auditor `research/tools/audit_sin_original_grunt_event_signals_phase582.py` authenticates:

- Exact owner-private original DLL pair and PE32 preferred base, **two whole-method SHA256 hashes**, 241 new native code bytes and both original `INT3` boundaries.
- **4 E8 direct calls inside new methods**, **4 previously authenticated incoming Grunt event calls**, **6 other event-branch E8 calls**, and **37 exact instruction/field opcode witnesses**.
- **4 original float constants** and repeated whole-function SHA of the Phase581 1330-byte native handler, with previous handler bytes intentionally **not double-counted**.
- Synthetic wrong-binary rejection and every mutation of the opcode or relative target of all checked direct calls: **PASS**.

Independent C++17 model `include/sin_compat/original_grunt_event_signals_phase582.hpp` and tests `tests/test_original_grunt_event_signals_phase582.cpp`: strict GCC/Clang warnings-as-errors PASS, GCC ASan/UBSan PASS, standalone CTest **2/2 PASS**. The host intentionally rejects exceptional FPU cases and relies on callbacks for the engine RNG, diagnostics, scene/vector calls and scheduler requests. **This is partial original gameplay decompilation, not a working replacement server.dll or Android/VR build.**

Reproduce from original-free project source:

```bash
python3 research/tools/audit_sin_original_grunt_event_signals_phase582.py --self-test
python3 research/tools/audit_sin_original_grunt_event_signals_phase582.py --bin-zip /path/to/private/original/bin.zip
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Next research focus: original scheduling predicate and routing helper `0x220EB8A0`/`0x22049A60`, or the additional Grunt-specific event `0x8F` position update. Keep original game-specific code as priority and avoid remaking known HL2-equivalent physics/surface/model systems.