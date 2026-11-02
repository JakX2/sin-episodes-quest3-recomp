# Engineering roadmap

| Milestone | Deliverable | Status |
|---|---|---|
| R0 | Portable C++17 project and mock-host weapon tests | Implemented as prototype |
| R1 | SourceVR host adapter compiled against selected engine headers | Not implemented; engine ABI and loader integration unverified |
| R2 | Real weapon scripts/animation sequences validated against test level | Partial: original SiN weapon script parser and local validation, real-engine model/animation pending |
| R3 | First original level launches with Magnum, assault rifle, Scattergun | Not started |
| R4 | SinTEK grunt combat AI integrated and scripted encounters | Not started |
| R5 | Dynamic difficulty, special NPCs, vehicle and scripted campaign features | Not started |
| R6 | Android ARM64 Quest 3 native VR runtime, performance and input | Not started |

Avoid declaring milestones complete based on synthetic regression assertions. Every milestone needs an executable demonstration using its real target environment.

See [Local weapon script integration](LOCAL_WEAPON_SCRIPTS.md).
