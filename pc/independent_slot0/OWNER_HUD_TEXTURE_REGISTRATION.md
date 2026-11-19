# Independent duplicate-ignoring VGUI property converter registry adapter

Owner-authenticated original SiN client `VGui_Startup` sets its once flag
before invoking `0x24009340` for `CHudTextureHandle`; the native
registrar interns the name, warns and ignores duplicates, otherwise
inserts the converter pointer into its own symbol map. Full original
registrar and three converter virtual bodies are SHA-verified in
`research/tools/audit_sin_client_hud_converter_native.py`.

`client_hud_texture_registration.h` provides a callback-driven
**independent** registration algorithm: a host-supplied key lookup,
insertion callback, and optional duplicate warning, preserving the
native duplicate-ignore behavior while *deliberately* not assuming
the original CUtlSymbolTable case rules or binary layout. The owner
sets its once bit before calling into the host (reentrancy safe)
and is non-copyable/non-movable, keeping published ABI pointers stable.
Refusing a malformed host before setting the flag and propagating
insertion failure are deliberate safety affordances. The original
0x24009340 is void; its exact failure behavior beyond duplicates is
not claimed. A normal Windows host must give the owner process
lifetime if the VGUI registry can retain its pointer.

Native MSVC x86 fixtures exercise duplicate-ignore preservation,
callback counts, once-guard, reentrancy, invalid input, rejected
insertion and non-movable ownership. This does NOT call into the
retail engine's original symbol map or Panel::AddPropertyConverter.
It is a prerequisite for a later real GUI root/HUD component,
not a new owner-engine checkpoint.
