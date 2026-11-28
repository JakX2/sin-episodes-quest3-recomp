# Phase 1695: retail engine ClientLeafSystem001 factory boundary

## Owner proof, not synthetic

The owner ran the reversible Phase1693 client-only DLL replacement in their disposable SiN Episodes test copy. The retail Windows engine loaded the independent client.dll, successfully called VClient011.Init, obtained all 21 source app interfaces, invoked native VSoundEmitter001.Connect(true), registered cl_predict, and accepted VClientPrediction001, VClientEntityList003 and VCENTERPRINT002. The first subsequently rejected factory name was ClientLeafSystem001. Native Toolhelp observed the module (78/78 samples), where managed enumeration was a known false negative. Original client.dll was restored and hash verified, server.dll was unchanged, and the final restoration verdict was VERIFIED_COMPLETE. Exit code 0 is not a playable boot, menu, map or frame claim.

## Original module provenance and source ABI

Original private owner client.dll SHA256: 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.
Original private owner engine.dll SHA256: f3add5b41300e6d709fd0b492bfcdd37b9b3d20d9744f874c0b860bf021cec81.

The original engine's 100-byte post-prediction factory window was already authenticated in Phase1689: VClientEntityList003 at 0x20023D24, VCENTERPRINT002 at 0x20023D4A, and ClientLeafSystem001 at 0x20023D70. Engine result global 0x203E4020 is a nonnull pointer when the interface is obtained; a null result takes the original diagnostic path. This factory order was now observed independently at genuine runtime.

The six-byte original client factory thunk at VA 0x24108B20 returns primary CClientLeafSystem singleton 0x24357F30 and was ALREADY recovered in Phase226. It is not counted again as new complete method bytes. Original constructor vptr writes at 0x2410C7A0 (+0), 0x2410C7A6 (+4), 0x2410C7AD (+8) and RTTI descriptor 0x242FA830 document three distinct subobjects:

| Source object location | Original vtable | Slots | Vtable SHA256 |
| --- | --- | ---: | --- |
| primary +0 | 0x2429756C | 22 | 16fe7aeba8e5d611a640ba3d4bd3419d5a05c5b9c69d19df68244f5cf3417b8b |
| AutoGameSystem +4 | 0x24297534 | 12 | f2bd0c8c39b9831d8e2bbc2a817aa48b026945857cc66427a7cade33241ba0fe |
| auxiliary +8 | 0x2429752C | 1 | 89db44486f34dddb679b8fff0c6fd8062111505a1c5b86a9b51ed1159ef6b179 |

All 35 original virtual entry pointers and original C++ class name CClientLeafSystem are authenticated against the SHA-pinned private client image. Original engine references at 0x20140083, 0x2014107C and 0x2014163E use virtual offsets +4, +0 and +0xC after acquisition. These are rendering/leaf management callbacks, intentionally outside the current reconstruction priority. Source evidence authenticates interface negotiation and call boundaries, NOT virtual behavior.

## Independent guarded runtime and tests

The new owner opt-in environment flag SIN_INDEPENDENT_CLIENT_LEAF_FACTORY_PROBE is available only after successful native sound Connect, prediction, entity list and cold centerprint gates. Default behavior continues to reject ClientLeafSystem001. With the new flag, factory returns a stable independent primary object whose first 12 bytes expose method-table pointers at original subobject offsets +0/+4/+8. All 22+12+1 virtual methods are non-returning guards invoking the existing unsupported-callback diagnostic. They NEVER pretend to execute original leaf visibility, model/prop/rendering logic or return invented results. Original thiscall argument/stack ABI for unknown operations is not claimed.

A source-only Python auditor validates entire original vtables, RTTI, caller requests, relocation witnesses, constructor stores and later engine calls. Its public self-test uses invented pointer vectors. The dedicated native MSVC x86 CTest loads the ACTUAL independent PE32 client.dll, validates the 21 factory requests and source-dependent native callbacks, then checks the 22/12/1 factory layout, shutdown revocation and intentional terminal virtual slot0 exit in a child process. The owner PowerShell wrapper additionally requires explicit disposable-copy confirmation, manifest SHA256 integrity and separate -EnableClientLeafFactoryProbe opt-in; its default mode still runs the prior baseline.

## Owner next test, after the new exact-head Windows CI is green

Extract the NEW Phase1695 owner diagnostic artifact outside the game, then run in its extracted folder:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\Run-OwnerClientStartupBoundary.ps1 -GameRoot "D:\SiN-DisposableTest" -DisposableTestCopyConfirmed -EnableClientLeafFactoryProbe

Save the console transcript and current session-specific independent trace. A new accepted ClientLeafSystem001 factory is a milestone; a subsequent unsupported-callback log identifies first genuine engine virtual use. Do not equate either with completed client Init or game boot. Verify OWNER_INDEPENDENT_RESTORATION: VERIFIED_COMPLETE before another run. This does not reopen HL2-equivalent physics, surfaces, props/models or renderer work.
