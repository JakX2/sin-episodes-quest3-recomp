# SiN Episodes: Emergence (2006): Jessica's original ballistic throw, November 8

**Direct original x86 evidence plus independently authored C++17 behavior.**
Primary input: unchanged private original 2006 `server.dll`, SHA-256
`568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`.
This file is NOT present in GitHub. This server-side pass does not claim to
decompile new client HUD logic or execute a native Quest 3 game.

## What the binary proves

- Jessica's animation handler is at `0x22362420`. After the original
  `anim_attachment_RH` lookup and creation at `0x22144DA0`, its final
  trajectory block is `0x223624E7..0x22362537`.
- The original `sv_gravity` ConVar registration begins `0x223BF9E0`.
  Its object at `0x22572DC0` has an internal pointer at `+0x18`,
  which Jessica dereferences at `0x223624ED` to read a float `+0x28`.
  The original registered default is **600**, but the game uses the live
  float value rather than a permanently hard-coded 600.
- At `0x223624FD`, Jessica pushes `0x41700000`, i.e. **15.0f**, and
  then calls player virtual method byte offset `+0x214`. The caller
  obtains a pointer to the target position from this virtual method;
  this analysis does not conclusively name the original Source method.
- The caller passes an output vector, right-hand attachment origin,
  player target pointer, gravity and 15.0 to the helper at
  **`0x2204C850`** (callsite `0x2236251A`). The initial velocity
  is then sent to spawned-entity method **`0x2214BA20`** (callsite
  `0x22362529`). That setter handles different entity physics states.
- The helper zeroes the horizontal direction's Z component, normalizes
  XY, retains `dz`, performs two x87 square roots and divides the
  horizontal motion by the resulting flight time. The fifth argument
  sets **extra height above max(source.z,target.z)**, not throw speed.

## Reconstructed equation (not original Ritual C++)

For attachment source `s`, target `d`, `g=sv_gravity`,
`dz=d.z-s.z`, and `extra=15` for Jessica:

```text
apex_above_source = max(dz, 0) + extra
rise_speed = sqrt(2 * g * apex_above_source)
fall_speed = sqrt(2 * g * (apex_above_source - dz))
flight_seconds = (rise_speed + fall_speed) / g
initial_velocity = ((d.x-s.x)/flight_seconds,
                    (d.y-s.y)/flight_seconds,
                    rise_speed)
```

Example with horizontal distance 100 and g=600:

| Target height difference | Initial horizontal speed | Initial vertical speed | Flight time |
| ---: | ---: | ---: | ---: |
| -30 | 163.692 | 134.164 | 0.610905 sec |
| 0 | 223.607 | 134.164 | 0.447214 sec |
| +30 | 163.692 | 232.379 | 0.610905 sec |

The values above are calculations from the equation, **not gameplay
measurements**. Source x87 uses extended intermediate precision and may
differ in the least significant bits from the portable double arithmetic.
The original helper at `0x2204C850` is also used by other engine code,
so it is not a uniquely Jessica-only physics primitive.

## Implementation in this branch

- `include/sin_compat/original_jessica_toss.hpp` and
  `src/original_jessica_toss.cpp`: portable solver with explicit
  validation of finite input and positive gravity. These guards protect
  future hosts; the original DLL itself is not proven to check them.
- Extends `ThrowRuntime` with optional `IOriginalTossProvider`. A native
  host supplies the captured attachment/player geometry, the **live**
  ConVar value and the genuine entity initial-velocity setter. Missing
  or invalid geometry fails closed, without falling back to an invented
  throw velocity. The legacy Phase 103 callback remains for existing
  original-free tests only.
- `tests/test_original_jessica_toss.cpp`: flat, uphill, downhill and
  vertical trajectories, analytic destination/apex checks, degenerate
  and nonfinite input.
- `tests/test_original_jessica_toss_host.cpp`: full fake-host animation
  -> spawn -> ammo -> geometry -> applied-velocity sequence and invalid
  gravity blocking.
- `research/tools/audit_sin_original_jessica_toss.py`: private
  SHA-pinned, read-only PE32 verifier. Checks **four exact original
  function SHA-256 witnesses, five original relative call targets,
  the ConVar registration, the 15.0 float and the x87 math shape**.
- `research/tests/test_audit_sin_original_jessica_toss.py`: artificial
  PE32 mutation tests, without original files.

Private verification:

```sh
python3 research/tools/audit_sin_original_jessica_toss.py \
  --server /owner/private/SiN/server.dll
```

Desktop tests (once the PR's branch is checked out):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Important remaining gaps

This is **not** a native engine `CNPC_Jessica` instance, a working
SourceVR game module, a launched SiN campaign or a Quest 3 APK. The
specific player virtual getter name, attachment-failure semantics,
complete velocity-setter ABI, map scripting, original model and animation
behavior still require validation. Native entity ownership and the
correct SourceVR interfaces must be proven before the portable physics
can make a real item fly in-game.

No proprietary SiN/Valve binary, assets, decompiled raw source or
original gameplay data has been committed.
