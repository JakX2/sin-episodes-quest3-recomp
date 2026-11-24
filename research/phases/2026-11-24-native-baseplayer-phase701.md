# Phase 701: native BasePlayer and PlayerState graph composition

Fresh SHA-matching owner originals: BasePlayer server constructor 0x22249fbb, client 0x240d21f1, both 15 direct slots. Parent pointers 0x2251f2a0 -> 0x22590f6c and 0x242f7d48 -> 0x2433768c, both DT_BaseCombatCharacter. Localdata table is last on server, second on client. DT_PlayerState one property, server 0x2224b76f / client 0x240d1fd1, nested at original offset 0x904 / 0xefc.

| Field | Server slot / offset | Client slot / offset | Original server encoding |
|---|---|---|---|
| baseclass | 0/0 | 0/0 | direct |
| pl | 1/904 | 2/efc | DT_PlayerState |
| m_hVehicle | 2/9ac | 3/f7c | handle helper |
| m_nVehicleView | 3/9b0 | 4/f84 | signed 32 |
| m_MoveType | 4/204 | 5/144 | unsigned 4 |
| m_iHealth | 5/9c | 6/fc0 | signed 10 |
| m_lifeState | 6/8c | 7/94 | unsigned 3 |
| m_flMaxspeed | 7/1234 | 8/fbc | 12 bits ROUNDDOWN 0..2048 |
| m_fFlags | 8/bc | 9/2e0 | unsigned 9 flags 0x401 |
| m_iObserverMode | 9/9dc | 10/f68 | unsigned 3 |
| m_hObserverTarget | 10/9e4 | 11/f6c | handle helper |
| m_hObserverViewModel | 11/bd4 | 12/1060 | handle element, client name [0] |
| m_hObserverViewModel | 12/0 | 13/0 | unlinked DPT_Array, 2 stride4 |
| m_szLastPlaceName | 13/12e8 | 14/1188 | DPT_String |
| localdata | 14/0 | 1/0 | original recipient callback |
| deadflag (nested) | 0/4 | 0/4 | unsigned byte 1 |

Hex offsets. Original maxspeed multiplier 2, upper endpoint 2047.5. Original flags send proxy 0x22245b80 masks to 0x1ff before wire encoding. Server localdata proxy 0x22258250 clears eight words and sets object-id-minus-one bit (reuse existing range-guarded proxy). String send 0x2225e580 forwards borrowed char pointer, factory takes length16 but does not store it in the SendProp. Client 0x24126ca0 copies through first NUL or string_size=18 and terminates last byte; port stages bounded source before writing. Test covers short-string tail preservation and long-string truncation. Native string pointer must remain valid for synchronous decode; no wire packet is constructed. Host provides writable entity storage. Observer array remains pre-initialization unlinked, like Phase700 viewmodels.

Paired actual native module regression constructs the entire BaseAnimating / OverlayVars / BaseFlex / BaseCombatCharacter / LocalPlayerExclusive / BasePlayer hierarchy, with only DT_BaseEntity and DT_Local supplied as fixture placeholders and host callbacks. Checks recipient masks, server/client order, handle packing, health/flags/state/string raw proxy dispatch, array metadata and reverse-order destruction. No retail entity construction, engine registration, model/prop/physics port or bitstream encoding claimed. No original bytes committed.
