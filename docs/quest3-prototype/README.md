# Quest 3 WebXR encounter prototype (not a native APK)

This independent **engine-agnostic** tech slice is meant to validate controller interaction and a single encounter quickly while the original SiN 2006 / SourceVR native port remains under research.

**What it does:** original-geometry industrial dock environment; Quest Browser immersive VR via WebXR; Touch controller trigger to fire, grip to activate control console, left-stick locomotion, right-stick snap turn, A/X reload; a moving humanoid target and security bot, abstract pulse effects, health/ammo pickup, floodgate unlock and exit win state. Desktop mode uses WASD, mouse click, R, E, Space. A desktop preview is **not** headset validation.

**Not included:** SiN BSP, original models/textures/sounds, original AI, the original 2006 DLLs, game files, Android APK, native engine client/server, actual SourceVR integration, saved campaign progression. The prototype's health/damage, animations, encounter timing and 3D geometry are **synthetic** and are not claimed to reproduce compiled SiN gameplay.

## Test on Meta Quest 3

1. Host the repository `docs/` folder on GitHub Pages over HTTPS, or deploy just this folder to any HTTPS static host. GitHub repository **Settings → Pages → Deploy from branch `main`, directory `/docs`** is one option after merging this branch.
2. Navigate to `https://jakx2.github.io/sin-episodes-quest3-recomp/quest3-prototype/` in the **Quest Browser** (only after Pages deployment is enabled and confirmed).
3. Choose **ENTER VR** and use the Touch controllers. Select trigger once to start. Aim at and shoot the red grunt and hovering security bot. Get near the cyan terminal by the left wall and press grip while pointing at it. Pass through the opened gate to complete the test.
4. Record whether you get stereo VR, correct eye height, left-stick movement, snap turn, trigger aiming, grip interaction, reload, and clear win state.

**Requirements:** Meta Quest Browser supporting `immersive-vr`; HTTPS secure origin; online access to the pinned `three@0.162.0` jsDelivr modules. The native GitHub code browser and local `file://` URLs won't start immersive VR. WebXR is feature-detected by Three.js' VRButton.

## Local desktop preview

Serve the repo root with `python -m http.server 8000` and open `http://localhost:8000/docs/quest3-prototype/`. Immersive VR from a headset requires a separately trusted **HTTPS** origin (or hosted site). The desktop preview permits quick iterative tests without a headset.

## Prototype acceptance criteria

- [ ] VR session starts in the headset (must be observed on device)
- [ ] Both eyes render correctly (must be observed on device)
- [ ] Left-stick movement and right-stick snap turn feel usable (must be observed on device)
- [ ] Controller aim, shot selection, terminal grip, health/reload, and exit work (must be observed on device)
- [x] Self-contained source exists and syntax has been checked
- [ ] Quest 3 in-headset performance/comfort measured

A later Android-native SourceVR implementation must preserve original-game behavior separately. Do not treat WebXR success as proof that SiN's original Windows 32-bit code has been recompiled for Quest 3.