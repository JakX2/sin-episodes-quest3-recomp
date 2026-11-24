# Phase 731: owner SiN Episodes SDK audit and generic screen dispatch

User requested explicit SDK review. Reopened owner Drive archive
SinEpisodesSDK.zip (44,068,406 bytes), SHA256
5a934e6c3f99151f2ba1a90fd080c7d6cd3f80a2b2b8118e5ef9cb2f3bb3d20b.
801 ZIP entries including directories. No .c/.cpp/.h/.hpp/.pdb/.lib/.sln/.vcproj,
no game client.dll/server.dll. Editor tools, two FGD files, scripts,
resource definitions and content. No SDK assets copied into repository.

## SDK references and differences

bin/SinEpisodes.fgd SHA256
4e803f2e23e09ee2dd275064ee71be133536549d34321ff2c7237320ac8d60f9.
172 concrete class declarations, 16 base declarations; includes base.fgd.
These counts are editor declarations, not runtime class/factory counts.
- item_megahealth is the actual editor classname (earlier synthetic
  pickup tests used item_healthmega; tests' opaque classname argument
  is not an asserted factory name). SDK describes +100, vial +10.
  Skill values remain runtime convars rather than hardcoded.
- env_poison_gas editor presets active 1, damage 20, radius 300,
  duration 30, spawnrate 48; gas labels U4, ANTIGEN, RADIATION,
  INERT. Do NOT replace Phase728's binary constructor defaults with
  editor values. SDK declares TurnOn, TurnOff, SetRadius,
  SetDuration, SetSpawnRate, SoundOn, SetStartColor, SetEndColor;
  remaining input paths need original verification.
- npc_hologram documents HologramOnDone / HOlogramOffDone outputs;
  not yet verified runtime dispatch. item_antigen_dispenser documents
  both canister outputs, disabled and antigen. SDK item_dna_scanner
  model differs from observed runtime model; editor preview not authority.
- Deprecated prop_propane_tank, prop_oxygen_tank, prop_u4_barrel
  point to renamed oxygen/mutagen classes; verify actual factory aliases.
- npc_jessica specifies ThrowItemAtPlayer, both ammo-count inputs,
  HolsterWeapon, and invulnerability spawn flag 65536.
- rules_combat.txt SHA256
  0d4c47cfa543435deb7098b4218e62707a02971351957260959ffd6016667f1e:
  hierarchical class -> weapon -> target spread mappings, 15 degree
  player-target default for NPCs, 10/13-degree specialized rules,
  5-degree generic target, zero player baseline. Reuse actual script;
  native lookup/fallback semantics must still be confirmed.
- scripts/HudAnimations.txt confirms PoisonTimeFadeIn/Out are
  0.5-second linear PoisonTimerColor alpha animations, not audio cues.
  Earlier Phase730 callback called sound is a semantic animation
  dispatch boundary and must be wired to HUD animation controller.
- stats definitions/display/graphs and talker response rules are
  useful original data for gameplay and NPC research. Weapon scripts
  provide ZoomFOV, kick ramp, viewmodel offset, ammo and HUD icons.

## Generic screen correction and implementation

SE1/scripts/vgui_screens.txt SHA256
be5f7c978b0b2fa0d54530eeb0b36c3397bda8bdc127e1bc311f623a82c3aaa5.
vgui_dispenser_panel type is vgui_screen_panel, 70x20 pixels,
resfile scripts/screens/vgui_dispenser_panel.res SHA256
7323a3bc84bf25b568b792094a23e635d7d513607edeff89642815410103626e.
Background MaterialImage and Eject MaterialButton with command eject.
No bespoke client dispenser panel necessary. prop_keypad is separate
prop_keypad_panel (200x310). SDK also contains actual generic screen
resources for docks controls, highrise elevators, pit tunnel door,
U4Lab_HologramControl and lab doors. Some registrations have missing
resource files in this archive, so do not assume full SDK completeness.

Fresh SHA-matching original client.dll: CVGuiScreenPanel primary
vtable 24295b54; generic command 240fad20..240fadaf. Creates event
button_press (24295df4) force=1 through global event manager slot6.
If created, resolves packed panel +d8 using 12-bit index/serial
against client entity table 242fa734; calls resolved screen+8
networkable slot9 for index; event slot10 sets entindex
(24295de8), slot12 sets command (242d3c5c); fires via manager
slot7 local_only=0, then base command 240070c0. Null event still
calls base. No disabled check in generic method. Original null
resolved screen would dereference; native refuses, discards event,
and blocks pending explicit reconciliation. Host consumes event
on Fire attempt, even if it reports failure. Original control
factory 240f9c30 recognizes MaterialImage and MaterialButton,
otherwise delegates to base; no custom dispenser widget needed.
Actual generic resource loading/control construction, input and
event transport remain host work.

Loaded-pair regression directly delivers client event through
Phase726 native server button listener; checks exact eject event
and scheduling, unrelated SDK elevator command, different entity,
null event, stale/invalid handle, setup and fire failure ownership.
No physics, model loading, renderer or commercial asset changes.
