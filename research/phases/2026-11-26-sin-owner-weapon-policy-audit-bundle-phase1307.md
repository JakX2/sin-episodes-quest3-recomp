# Phase 1307: Owner weapon-policy audit bundle

Package the native server DLL, both standalone Win32 audit executables and a hash-gated PowerShell/CMD launcher in the independent PE32 workflow. Use a separate diagnostic folder, never touch the installed game. CI runs the bundled native-only launcher before artifact upload; owner can drag original SE1/bin/server.dll onto CMD and return the local log. Distinguish owner original comparisons from native-only boundary witnesses in the range audit output. No original bytes included. Artifact availability depends on pending Windows CI.
