# Phase 802 fixture alignment correction

Loaded service workflow 45429442712 failed at the first state.resume assertion (test_native_npc_scene_playback.h line 502). The synthetic child array had stride 0x329, so only the first event began at a 4-byte boundary; hologram_writable rejects the second object. Pad each fixture row to 0x32c while retaining 0x329 minimum extent and all assertions. No production change. Retest current Phase803 head. Earlier immutable Phase802 CI remains failed.
