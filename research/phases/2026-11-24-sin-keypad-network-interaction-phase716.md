# Phase 716: original SiN keypad networking and map interaction

Owner DLL hashes matched. DT_VGuiScreen constructors 222906ae / 240f95f5,
7 slots; parent DT_BaseEntity. DTKeypad 222fc07a / 2421bbd6, 3 slots;
DT_CPropKeypad 22301db5 / 242203a3, 3 slots. Both keypad classes inherit
DT_VGuiScreen. Original client class list order by registration:
CVGuiScreen, CKeypad, CPropKeypad (prepend reverses this).
Offsets (server/client), widths/bits:

| Field | Server | Client | Wire metadata |
|---|---|---|---|
| m_flWidth | 39c | 48c | NOSCALE float |
| m_flHeight | 3a0 | 490 | NOSCALE float |
| m_nAttachmentIndex | 3a8 | 4ac | unsigned 5 |
| m_nPanelName | 3a4 | 494 | unsigned 8 |
| m_fScreenFlags | 3b0 | 4b4 | unsigned 3 |
| m_nOverlayMaterial | 3ac | 4b0 | unsigned 10 |
| m_bDisabled (both) | 3c8 | 510 | unsigned byte 1 |
| m_combination | 3cc | 511 | pointer string flags 0x200; recv 255 |
| m_sInput | 3c9 | 511 | inline string flags 0; recv 32 |

Server string pointer wrapper 2227ece0/2227ecc0 returns empty static string
for null pointer. Prop keypad forwards inline string via 2225e580.
Recv 24126ca0 bounded copy plus last-byte NUL, preserves other bytes
after early NUL. The independent callback stages reads before writes.
Separate three-class extension pins the aggregate, no version-1 array
size changes. No VGUI rendering implementation.

Datamap 2252953c: Enable -> 222fc160, Disable -> 222fc110.
Original writes tracked byte only when enabled/disabled state changes.
CKeypad listener 222fbe20 on secondary +394 interface recognizes
keypad_combo_entered and bad_keypad_combo_entered. Fires output
subobject +3d0 or +3e8 respectively, uses self for activator and caller,
zero delay. It has no disabled-state check. Original spawn/restore
register listener for both names. Event registration stays host work.

Client panel command 2421bdf0 resolves associated entity and checks
disabled byte +510, else widget append virtual +26c and read text +238
(capacity 255). Exact string equality emits correct event. Otherwise
entered length >= combination length emits incorrect event; shorter input
continues. For either event, create(force=1), fire if nonnull(local_only=0),
clear text, call base command last. Missing/disabled keypad calls base
only. Port implements decisions with explicit host widget and game
event services. A failed host callback stops the command, no replay.
No claim of implementing CPropKeypad's distinct input handling.
Original CKeypad allocation 0x400 server/0x610 client, CPropKeypad
client 0x538. No retail code, graphics, collision or original asset
bytes committed. Paired regression covers both schemas and original
client-to-server event decision cases, disable, bounded string copies,
and teardown guards.
