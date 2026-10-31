# SiN Episodes: Emergence | public reverse-engineering research

This archive preserves research **reports and selected original Python verification scripts** from Phases 44–55, plus **historical summary-only notes** from early enemy AI, dynamic-difficulty, and vehicle investigations. Earlier phases without extant source material are **not** silently recreated.

> **Important evidence distinction.** "Verified" inside an archived report refers to the original research run with its particular x86 Windows DLL builds. Publication here did not reproduce every instruction signature or behavioral assertion. Phases 45 and the separate automated Phase 55 audit worked from **preserved excerpts** rather than freshly reading the full SiN DLLs. Do not silently combine those evidence levels.

## Retained prior research

- [Early enemy AI, grunt hierarchy and schedule names](legacy/phase01-enemy-ai.md) (historical excerpt only).
- [Phase 5: dynamic difficulty and game rules](legacy/phase05-dynamic-difficulty.md) (historical excerpt only).
- [Phase 22: vehicle views and monitor cameras](legacy/phase22-vehicle-cameras.md) (historical excerpt only).

## Backfilled findings for previously missing Phases 23–43

The original Phase 23–43 ZIP packages are **not** restored. However, the following recovered summaries and a **newly written, re-run original-DLL verifier** are now available:

- [Recovered-research index and provenance](recovered/README.md)
- [Vehicle, phone, poison and antigen (23–34)](recovered/phase23-34-gameplay.md)
- [Assault rifle, scattergun and projectile callbacks (35–36)](recovered/phase35-36-weapons.md)
- [Magnum DUAG, iron sights and quick grenades (37–43)](recovered/phase37-43-magnum-ads.md)
- [Backfill verifier for the original 2006 x86 DLLs](verifiers/phase23-43-backfill-verify.py)

The freshly written verifier passed two original DLL hashes, 23 instruction signatures and three firearm ZoomFOV virtual getters during the backfill. Its scope does **not** cover all archived claims. Missing historical reports/scripts remain missing, not fabricated.

## Original phase reports and verifier index

| Phase | Focus | Original report | Independent verifier retained | Source archive SHA-256 |
|---|---|---|---|---|
| phase44 | Zoom input / FOV | [Report](phases/phase44.md) | [Script](verifiers/phase44-verify.py) | `979c1264d6f49e2d…` |
| phase45 | Zoom decision model (excerpt-only) | [Report](phases/phase45.md) | [Script](verifiers/phase45-verify.py) | `dad96291451cf4e1…` |
| phase46 | Input button edges | [Report](phases/phase46.md) | [Script](verifiers/phase46-verify.py) | `5b51228a0bd8cbee…` |
| phase47 | Weapon activity mapping | [Report](phases/phase47.md) | [Script](verifiers/phase47-verify.py) | `95fc11474d4bbb01…` |
| phase48 | Animation deadlines / client viewmodel | [Report](phases/phase48.md) | [Script](verifiers/phase48-verify.py) | `5a1ecb2bfda807aa…` |
| phase49 | Weapon-script origin offset | [Report](phases/phase49.md) | [Script](verifiers/phase49-verify.py) | `39d571c762235278…` |
| phase50 | Script-to-viewmodel transfer | [Report](phases/phase50.md) | [Script](verifiers/phase50-verify.py) | `6e32388d272012ce…` |
| phase51 | Animation/viewmodel lifecycle | [Report](phases/phase51.md) | [Script](verifiers/phase51-verify.py) | `1bae94aed852b1e9…` |
| phase52 | Deployment and holster | [Report](phases/phase52.md) | [Script](verifiers/phase52-verify.py) | `666b5acd04ce81fe…` |
| phase53 | Reload activity and ammo | [Report](phases/phase53.md) | [Script](verifiers/phase53-verify.py) | `acfa2a9cd347c519…` |
| phase54 | Reload interruption scheduling | [Report](phases/phase54.md) | [Script](verifiers/phase54-verify.py) | `eea95acb86b52dbd…` |
| phase55 | Clip config and accounting | [Report](phases/phase55.md) | [Script](verifiers/phase55-verify.py) | `dd8373965c11ef64…` |
| phase55-automation | Preserved-excerpt reload audit / VR bridge | [Report](phases/phase55-automation.md) | [Script](verifiers/phase55-automation-verify.py) | `e8100758aed9e174…` |

The *full original phase packages* (where available) have `README`, selected original x86 disassembly excerpts, C++ reference models, tests and hash manifests. **This public repository intentionally does not copy their `.asm` evidence dumps or game DLLs.** Many archived report relative references (`evidence/`, `src/`, `tests/`) refer to files retained in the original local ZIP packages and may not resolve here. The clean-room reference headers that were incorporated in the buildable prototype live under [`../third_party/research/`](../third_party/research/).

## Provenance and reproduction

- [`archive-index.json`](archive-index.json) preserves the filename, ZIP checksum, archived-report filename and digest, and the exact source of each published report.
- [`binary-provenance.json`](binary-provenance.json) preserves verified file lengths and SHA-256 fingerprints for the original SiN and HL2 comparison DLLs, **without including their contents**.
- [`tools/check_binary_hashes.py`](tools/check_binary_hashes.py) compares user-supplied ZIPs or extracted DLL files to those fingerprints, with no network requests.
- [`verifiers/`](verifiers/) holds selected independent phase verification scripts; consult each script's help/source for original inputs. They are preserved as research tools, not guaranteed one-command automated repo tests. Some originally depended on local scripts, assets or modules outside this public subset.

## Legal and technical boundaries

This is an independent, unofficial project. SiN, Half-Life 2, Source and associated assets remain their owners' property. Researchers must use lawfully obtained game data and comply with relevant rules. No original game executables, DLLs, models, maps or SDK-derived code are committed here. Original findings are research notes, **not** the original Ritual source and **not** proof that a native Quest 3 port runs.

## Still missing

The full original reports or evidence for Phases 2–4, 6–21 and 23–43 remain unavailable in the recovered material used for this publication; newly written summaries and a limited independent verification baseline for 23–43 are linked above. Early grunt/vehicle notes are partial chat excerpts, not full reverse-engineering dossiers. Add recovered *independently authored* notes through pull requests with clear provenance; never invent a missing phase or mark a guess as confirmed.
