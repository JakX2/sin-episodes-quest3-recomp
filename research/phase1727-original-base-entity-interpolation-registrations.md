# Phase 1727: original base entity interpolation registrations

The original 573-byte C_BaseEntity constructor identified in Phase 1726 initializes an embedded registration list at owner+0x10. The two calls to the 121-byte list insertion helper at 0x24061220 append two 12-byte records, in order. Their source first DWORD is 2 in both cases; its semantic meaning is not claimed. Their second and third DWORDs are original owner-relative pointers:

| registration | value | interpolator |
|---|---|---|
| Vector | +0x220 | +0x22C |
| QAngle | +0x25C | +0x268 |

RTTI identifies the final interpolator types as CInterpolatedVar<Vector> and CInterpolatedVar<QAngle>. The 115-byte growth helper at 0x24060870 initially reserves three entries on the source zero-grow-size path, then doubles capacity. The original insertion shifts following 12-byte records before copying the input record. The portable list uses owner-relative offsets instead of physical pointers, and implements bounded insertion, the logical zero-grow-size capacity policy and the exact two initial records. Its std::vector storage is not the original CUtlVector or engine allocator ABI; invalid-index rejection is a host safety behavior, not a claim about the original invalid-input behavior. No interpolation math is claimed.

The constructor also embeds a CCollisionProperty subobject at +0x16C, subsequently installs a NetworkVar_m_Collision vtable, and invokes a final 414-byte initialization helper containing virtual and engine-global dependencies. Those effects remain unimplemented; retail factory publication is still blocked.

Private audit: python research/tools/audit_sin_original_client_base_interpolation_registrations.py --client /private/client.dll

Local private audit, GCC strict C++17 and Clang ASan/UBSan passed. Source bytes and game DLLs were not committed. VClient011.slot5 and native callbacks remain guarded. This is portable behavioral reconstruction, not proof of actual game startup.
