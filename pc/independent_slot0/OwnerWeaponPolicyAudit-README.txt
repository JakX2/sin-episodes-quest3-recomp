SiN Episodes weapon-policy differential audit (Win32, diagnostic only)

Extract this artifact into its own folder. Never copy the bundled server.dll
into your game installation. Drag your original SE1\bin\server.dll onto
Run-OwnerWeaponPolicyAudit.cmd, or double-click the CMD and enter the path.

The selected original DLL must match SHA256:
568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650

The launcher runs both standalone audits and writes weapon-policy-audit.log.
Expected owner-mode totals: 7776 ammo cases, 74844 original range-leaf
comparisons, and 8749 melee cases. Exit code 0 means both audits passed.
A preferred-address reservation failure means the test cannot map its isolated
code slices; it is not a behavioral mismatch. A nonzero result should be
reported with the log. The launcher does not modify the game or load the
original DLL as a Windows module. Original functions are replayed from
authenticated code slices against synthetic services and fixtures.
This is not proof of complete live-engine behavior or a playable replacement DLL.

To run native-only regression tests without the original DLL:
powershell -NoProfile -File Run-OwnerWeaponPolicyAudit.ps1 -SelfTest

The built executables and reconstructed DLL require the normal Microsoft
Visual C++ x86 runtime. No original game bytes are included in the artifact.
