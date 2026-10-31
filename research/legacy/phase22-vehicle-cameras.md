# Phase 22: SiN vehicle cameras (historical conversation excerpt)

**Evidence level:** Preserved earlier research summary, not full original Phase 22 disassembly or verified runtime footage. All addresses and offsets are build-specific and are **not re-verified in this publication**.

| Function / subsystem | Address | Historical observed behavior |
|---|---|---|
| `InputSetView` | `0x2232FA70` | Validate requested vehicle view index against available view count; dispatch view logic |
| `InputSetCamera` | `0x2232FB20` | Update monitor-camera name, unlink previous camera |
| Camera-link cleanup | `0x2232F690` | Resolve old camera handle, cleanup and invalidate stored handle |
| `InputMonitorOn` | `0x22330A60` | Send monitor `open` when transitioning on |
| `InputMonitorOff` | `0x22330A90` | Send `close`, disable monitor, unlink camera |
| Camera resolution call | `0x22332D26` | Earlier reported update call site to camera-resolution routine |

Original diagnostic reported in the DLL: `Failed to link up vehicle to camera %s`. The camera was reported to require a compatible `CPointCamera` entity; linking failure could clear the saved camera name.

The earlier client network-table comparison reported:

| Client class | Table | Headlight property offset |
|---|---|---:|
| `C_PropJeep` | `DT_PropJeep` | `0x888` |
| `C_PropSinVehicle` | `DT_PropSinVehicle` | `0x854` |

**VR implication (proposal, not recovered behavior):** keep HMD position, vehicle viewpoint and dashboard monitor camera independent; avoid merging them into a single toggle.

## Open work

Revisit the reported **100-byte vehicle view records**, full driving/physics/input behavior and monitor rendering pipeline. Investigate valid seat transformations and VR collision/comfort using original game assets and an identified SourceVR host.
