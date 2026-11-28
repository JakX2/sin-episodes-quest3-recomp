# Phase 1711: three source-authenticated movetype, movecollide and default-FOV RecvProxy callbacks

## Source identity and exact native x86 method boundaries

The user's unmodified retail SiN Episodes (2006) PE32 `client.dll`
is SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

All three callbacks are distinct original 2006 methods and each is
used by exactly ONE of the 1,082 canonical source RecvProp constructors:

| Source callback | Original 2006 function VA | Executable size | Original source property |
| --- | --- | ---: | --- |
| Move collision | `0x240C61E0` | 18 bytes | `DT_BaseEntity.movecollide`, ordinal 120 slot 18 |
| Move type | `0x240C7330` | 28 bytes | `DT_BaseEntity.movetype`, ordinal 120 slot 17 |
| Default FOV | `0x240D2590` | 27 bytes | `DT_Local.m_iDefaultFOV`, ordinal 202 slot 4 |

Original complete x86 method digests, in order above:

- `f496a564e61e8a5b3fac73fab8e2f9a0c45afd6fdfa36c38b0eed0d857208c00` (18B)
- `4929e26544167a91cc8298181e40544cb9831cdcb426ebd73b0c1a450b0d730a` (28B)
- `a127871117094cba6b47fa51952629dfcd2bb7a447c82e3957719a20c98531dc` (27B)

All three terminate in their original `RET`, followed by exactly
14, 4 and 5 `INT3` padding bytes respectively. Original source
constructor `PUSH callbackVA` operands are verified at
`0x240C756C` (collision source), `0x240C7552` (move type source)
and `0x240D33A0` (default FOV source), before the matching source
property name and descriptor arguments. Native source addresses are
read-only **original x86 labels** and are never callable portable
pointers.

## Exact effects, all with decoded native DWORD at CRecvProxyData+4

The 18-byte collision callback stores its received DWORD to
**object +0x148**. The 28-byte movement-type callback stores its
DWORD to **object +0x144**, then unconditionally sets object
**+0x148 to zero**. In the original descriptor constructor, both
properties have `RecvProp +0x2C = 0`, so a generic
`object[memberOffset]` copy would be wrong.

The 27-byte `m_iDefaultFOV` callback compares the incoming
DWORD to the previous **object +0x38**. If equal, it performs **no
write at all**. If changed, it resets **object +0x2C to zero**
and then writes the received DWORD to **object +0x38**. The source
RecvProp +0x2C member offset happens to equal `0x38` here,
but the conditional side-effect at object +0x2C must also be
preserved. These are native32 DWORDs, not packet offsets.

These are deliberately bounded host-only reference effects. The
implementation in
`include/sin_compat/original_client_custom_movement_fov_proxies.hpp`
checks exact original source `(table,slot,callback)` identity and all
relevant object buffer endpoints **before any modification**.
It does not call engine-side methods, allocate entity objects or
change engine global state.

## Dispatch and regression checks

The owned-image source receiver router now recognizes these three
specific custom native source identities. Its other 36 non-reconstructed
custom callbacks and all 28 specialized constructor callbacks stay
unsupported. Host-side dispatch classification is now:

| Source callback group | Host-owned reference implementation |
| --- | ---: |
| Standard Source proxy identities | 999 |
| Original handle and moveparent callbacks | 35 |
| Color byte reorder | 4 |
| Decrement integer DWORD | 5 |
| Movement/collision/default FOV | **3** |
| Still-unimplemented custom | **36** |
| Special constructor paths | **28** |
| Total original static source routes | **1,110** |

Thus **1,046/1,110** static properties have a narrowly reconstructed
host-side callback path, a coverage metric only. The underlying
`VClient011.slot5` and every pointer-publication guard remain
**closed**. No real network packet, native ClientClass creation,
original entity state, Windows owner game boot, Android title startup
or VR frame is claimed.

The strict C++17 regression
`tests/test_original_client_custom_movement_fov_proxies.cpp`
covers original offsets, sentinel boundary values, same-FOV no-op,
changed-FOV cache invalidation, never-modify-on-bounds-error
and rejection of wrong table/slot/callback tuples. Existing
`tests/test_original_client_standard_recvproxy_router.cpp`
also verifies all **1,110 source property routes** and exact
1,046 / 36 / 28 classification. Linux ASan/UBSan, Windows
MSVC Win32 x86, Android CMake and native CTest targets use the
same authored portable code.

Private original-byte authentication:

```sh
python research/tools/audit_sin_original_client_custom_movement_fov_proxies.py --self-test
python research/tools/audit_sin_original_client_custom_movement_fov_proxies.py --client /private/client.dll
```

The first command requires no proprietary byte content. The second
verifies the entire unchanged original DLL hash, complete three
function SHA digests, exact native `RET`/padding boundaries,
sparse x86 storage instructions, and all three original source
constructor callback operands.

### Additional audit hardening

The prior Phase1710 private callback audit had a latent
**five-element tuple unpacked into four variables** in
`verify_original()`. Public `--self-test` did not enter that
branch. Phase1711 repairs the tuple unpacking and restores
executable-byte and padding verification in private mode. This was
an audit tooling defect, not evidence that the game DLL was modified.

## Next real engine boundary

Recover the *side-effectful* custom RecvProxy callbacks only after
their external calls and owner lifetimes are source-audited. Then
finish original special arrays/overlay object lifetimes, actual
executable native 2006 callback wrappers, 191 ClientClass creator
functions and engine registry ownership. Continue to deliberately
refuse `VClient011.slot5` until the native PE32 ABI is genuinely
complete. Physics/surfaces/models/renderer remain deferred.
