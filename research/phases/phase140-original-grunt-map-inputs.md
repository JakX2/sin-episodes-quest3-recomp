# Phase 140: Original CNPC_GruntBase map-input behavior

**Evidence:** read-only static analysis of the owner-supplied SiN Episodes (2006) x86 server.dll, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. The preserved audit validates the original instruction bytes, datamap records and entrypoints. No proprietary executable bytes or game assets are included.

## Recovered input registrations

The original CNPC_GruntBase datamap contains these six inherited map inputs. The same datamap is referenced by CNPC_MobGrunt and CNPC_SintekGrunt.

| Map input | Original handler VA | Datamap type | Confirmed behavior |
|---|---:|---|---|
| StartPatrolling | 0x22357E30 | void | writes 1 to m_bShouldPatrol at this+0xC65 |
| StopPatrolling | 0x22357E40 | void | writes 0 to m_bShouldPatrol at this+0xC65 |
| LookOff | 0x2235F260 | void | sets bit 0x10000 in a 32-bit flag word |
| LookOn | 0x2235DDC0 | void | clears bit 0x10000 in the same flag word |
| Assault | 0x22357E50 | string | calls helper 0x220CFC00 with integer argument 3 |
| ThrowGrenades | 0x22357EA0 | boolean | boolean path writes 0.0f to m_flGrenadeHeight at +0xC70, then the input value to m_bGrenadeHappy at +0xC6C |

The Assault helper's meaning is not yet recovered. Its string payload is preserved and passed through the host callback rather than translated into an invented schedule. The target flag-word member/offset for LookOff and LookOn is not established; only the mask and set/clear operations are verified. The alternate non-boolean path inside the ThrowGrenades handler is not reproduced because the registered map input is FIELD_BOOLEAN.

## Independently authored reconstruction

Added OriginalGruntMapInput dispatch to the portable sin_original_grunt_ai library:

- Patrol inputs update the shared m_bShouldPatrol host field, which is the input consumed by the original patrol condition path.
- Look inputs pass the verified 0x10000 mask and set/clear direction to the engine host without assuming a native object offset.
- Assault preserves its FIELD_STRING payload and helper argument 3; the unknown helper stays an explicit host operation.
- ThrowGrenades preserves the observed height-reset-before-boolean write order.

The old x86 addresses and offsets are evidence only. They are not used as ARM64 object layouts or virtual slots.

## Verification

Desktop CTest includes the new map-input regression. Android ARM64 CI compiles the library and regression binary. Android Bionic x86_64 CI executes the regression with a fake host. These are portable-code/build and emulator checks only, not SourceVR, OpenXR or Quest 3 gameplay.

## Remaining integration

The Source host must bind the patrol byte to its native NPC state, resolve the correct look-flag word, implement the original Assault helper, and route the grenade fields into the actual NPC. This phase does not create a SourceVR game module or establish a licensed native game-code host.
