# November 2026 original DLL and HL2-log research

Independent SiN Episodes: Emergence reverse-engineering notes, begun on **2026-11-04**, with later rounds appended.

- [Security-bot pulse projectile and three jetpack variants: Deep Dive 16](reports/SiN_Original_Jetpack_SecurityBot_Pulse_DeepDive_16.md)
- [Jetpack grunts & security bots: Deep Dive 15](reports/SiN_Original_Jetpack_SecurityBot_DeepDive_15.md)
- [HL2 unknown entity errors: Deep Dive 08](reports/SiN_HL2_Runtime_Entity_Failure_DeepDive_08.md)
- [DLL and archive crosswalk: Deep Dive 09](reports/SiN_Original_Drive_DLL_Asset_Crosswalk_DeepDive_09.md)
- [Elite grunt and take-cover: Deep Dive 12](reports/SiN_Original_Elite_Grunt_Takecover_DeepDive_12.md)
- [Cover/elite animation runtime: Deep Dive 13](reports/SiN_Original_Cover_Behavior_Elite_Event_Runtime_DeepDive_13.md)
- [Chaingun cycle and cover retries: Deep Dive 14](reports/SiN_Original_Cover_Retry_Chaingun_State_DeepDive_14.md)

Additional related reports in [reports/](reports/) cover Jessica/JC, custom map entities, damage/health, and compiled DLL inventory. Where original reports reference local ZIPs, binary excerpts, or locally generated JSON not present in this public snapshot, their reproduction requires **legally obtained private source inputs**, not a network download from this repository.

The [verifiers/](verifiers/) scripts are **read-only local research tools**, not CI or proof of a game port. They expect original SHA-pinned DLLs and/or original assets and user-captured logs outside the repo. See [evidence/](evidence/) for the structured JSON snapshots included in this archive. Some larger evidence snapshots could not be copied through this archive workflow; regenerate them using the associated verifier and own source data.

**Evidence standard:** VAs are original 32-bit PE virtual addresses, not modern engine or Quest addresses. A named datamap field or compiled schedule is not sufficient to prove complete gameplay behavior. Numerical labels for animation events and schedules remain provisional unless traced. Console totals count observed attempts and may include repeated loads.

**Licensing:** Original DLLs, models, maps, textures, sounds, VCDs, game scripts, and extracted assembly dumps are intentionally excluded. No original proprietary code is distributed. The Quest 3 port's implementation files are unchanged by this documentation update.

## Archived reports
- [SiN_HL2_Runtime_Entity_Failure_DeepDive_08.md](reports/SiN_HL2_Runtime_Entity_Failure_DeepDive_08.md)
- [SiN_Original_Drive_DLL_Asset_Crosswalk_DeepDive_09.md](reports/SiN_Original_Drive_DLL_Asset_Crosswalk_DeepDive_09.md)
- [SiN_Original_Elite_Grunt_Takecover_DeepDive_12.md](reports/SiN_Original_Elite_Grunt_Takecover_DeepDive_12.md)
- [SiN_Original_Cover_Behavior_Elite_Event_Runtime_DeepDive_13.md](reports/SiN_Original_Cover_Behavior_Elite_Event_Runtime_DeepDive_13.md)
- [SiN_Original_Cover_Retry_Chaingun_State_DeepDive_14.md](reports/SiN_Original_Cover_Retry_Chaingun_State_DeepDive_14.md)
- [SiN_Original_Jessica_JC_AI_DeepDive_07.md](reports/SiN_Original_Jessica_JC_AI_DeepDive_07.md)
- [SiN_Original_Custom_Entity_DeepDive_01.md](reports/SiN_Original_Custom_Entity_DeepDive_01.md)
- [SiN_Original_Custom_Entity_DeepDive_02.md](reports/SiN_Original_Custom_Entity_DeepDive_02.md)
- [SiN_Original_Antigen_Damage_Flag_DeepDive.md](reports/SiN_Original_Antigen_Damage_Flag_DeepDive.md)
- [SiN_Original_Client_Server_DLL_Inventory.md](reports/SiN_Original_Client_Server_DLL_Inventory.md)
- [SiN_Original_Entity_Damage_Health_Handlers_DeepDive.md](reports/SiN_Original_Entity_Damage_Health_Handlers_DeepDive.md)
- [SiN_Original_Jetpack_SecurityBot_DeepDive_15.md](reports/SiN_Original_Jetpack_SecurityBot_DeepDive_15.md)
