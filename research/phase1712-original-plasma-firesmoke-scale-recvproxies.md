# Phase 1712: original Plasma/FireSmoke x87 scale receive callbacks

## Authenticated retail source

Private user-owned unchanged 2006 PE32 `client.dll` SHA256:
`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d`.

Two original type1 float property callbacks are short x87 methods that
alter an owner's transition-state fields without needing a genuine
engine virtual function or unresolved global singleton.

| Source property | Original executable function | Size | Full original SHA256 |
| --- | --- | ---: | --- |
| `DT_Plasma.m_flScale` (table94, slot2) | `0x240EAA20` | 56 bytes | `5d57913b243190d93e89a433b6cd6168eb08047f1e4db82d171cbc9db251a530` |
| `DT_FireSmoke.m_flScale` (table111, slot2) | `0x240DA3E0` | 70 bytes | `99b2ce915c93019fa10726580b4b971ab35cbdc28403bc171b8309ec4be053b6` |

Both complete native functions terminate at original `RET`,
followed by original `INT3` padding (8 and 10 bytes respectively).
The actual original float constructor callsites
`0x240EAB87` and `0x240DA557` push those callback addresses
14 bytes earlier in each original x86 argument sequence.
Both RecvProp descriptors have type1 and **native +44=0x484**,
source-backed independently.

## Correct original x87 comparisons, not a byte-copy guess

Both original functions execute `FUCOMPP; FNSTSW AX;
TEST AH,0x44; JNP`. The masked status contains x87
C3=1 for ordered equality and C2=1 for unordered comparison.
The x86 `JNP` branch is taken for **exactly one asserted bit**
(odd parity), hence **ordered equality**. Greater-than, less-than,
or NaN-unordered comparisons do not take that equality branch.
This subtle condition was checked against the original instructions
instead of mechanically treating `JNP` as a traditional greater/less
jump. Signed zero (`+0` and `-0`) compares equal.

For **Plasma** the source behavior is:

- Compare decoded float DWORD `CRecvProxyData+4` against owner
  `+0x484`. On ordered equality, **do nothing**.
- Otherwise write the incoming float bits to owner
  `+0x4A4` and owner `+0x484`, and copy the prior
  owner `+0x49C` DWORD to owner `+0x4A0`.

For **FireSmoke** the source behavior is:

- Compare incoming float against owner `+0x484`.
- If different, compare against owner `+0x49C`.
  Only if both comparisons differ, copy owner `+0x494`
  to owner `+0x498` and write new float to owner `+0x49C`.
- **Always** write the incoming float to owner `+0x484`
  on the final x87 `FSTP`. Unlike Plasma, even ordered-equal
  `+0/-0` updates the stored sign bit.

The portable implementation deliberately uses **integer-bit IEEE754
binary32 ordered equality**, with the special signed-zero equivalence.
This avoids host CPU flush-to-zero and floating-point environment
differences for all non-NaN values. Original x87 NaN payload
quieting on `FLD/FSTP` is not fully proven: if the incoming float
or a compared owner field is NaN, the independent implementation
**refuses without any write**, rather than fabricating a compatible
x87 NaN payload rule.

## Code, tests, coverage and scope

`include/sin_compat/original_client_custom_scale_transition_proxies.hpp`
retains original callback identities and exact table/slot/type/offset
source assertions. It performs all required owner-memory bounds checks
before ANY writes, including `+0x4A8` exclusive for Plasma and
`+0x4A0` exclusive for FireSmoke. No commercial original code,
actual Source objects or x86 function pointers are executed.

The original-source callback router recognizes only those two source
identities and reports NaN refusal as an **unsupported source
value**, retaining complete object immutability. Full static
1110-property source callback classification after this phase:

| Original source callbacks | Host-side bounded behavior |
| --- | ---: |
| Standard 2006 proxy families | 999 |
| Original handles / moveparent | 35 |
| Color byte reorder | 4 |
| Decrement integer | 5 |
| Movement/collision/default FOV | 3 |
| Plasma/FireSmoke x87 scale | **2** |
| Remaining custom | **34** |
| Specialized array/overlay constructor routes | **28** |
| Total original source properties | **1,110** |

Total portable HOST reference coverage **1,048 / 1,110**, *not*
a claim of executable native network callback compatibility or of
Android game boot.

`tests/test_original_client_custom_scale_transition_proxies.cpp`
checks equality/no-change, changed animation transition writes,
signed-zero differences, secondary target equality,
infinities, full bounded refusal and NaN fail-closed paths.
`tests/test_original_client_standard_recvproxy_router.cpp`
still enumerates all authentic 1,110 source slots and requires the
1,048/34/28 classification. Focused strict C++17, Linux
AddressSanitizer/UndefinedBehaviorSanitizer, MSVC Win32 and Android
CMake targets are wired.

Source audits:

```sh
python research/tools/audit_sin_original_client_custom_scale_transition_proxies.py --self-test
python research/tools/audit_sin_original_client_custom_scale_transition_proxies.py --client /private/client.dll
```

The optional private mode verifies original DLL SHA256, two whole
function SHA digests, exact x87 conditional/opcode witnesses,
`RET`/padding boundaries and original 2006 source PUSH callback
immediates. The public mode never needs proprietary original bytes.

**Retail Windows `VClient011.slot5` remains deliberately guarded**,
all exposure helpers remain false, no original DLL is replaced,
and this is not complete packet decoding, native proxy ownership,
ClientClass entity factory recovery, Android/Quest 3 gameplay, physics,
surface parameter, prop/model or rendering work.
