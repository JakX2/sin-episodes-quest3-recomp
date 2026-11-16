# Phase 254: independent factory boundary smoke (synthetic only)

This phase tests a separately built C++17 shared library exporting `CreateInterface(const char*, int*)` without linking to or forwarding into original SiN gameplay DLLs. The fixture returns two owned static objects for deliberately synthetic interface names, reports 0 for success and 1 for unknown/null names, and never pretends to implement a retail engine interface.

Run locally: `python3 -m unittest discover -s research/independent_factory -p 'test_*.py' -v`.

**Evidence tiers:** 403 previously SHA-reviewed original functions (256 client, 147 server); 64,586 PE-scanner candidate addresses (not verified); 0 original methods independently load-tested. This synthetic factory is an independently load-tested *fixture*, not a reconstructed retail client/server DLL. No new original native bytes or functions are claimed. Retail CreateInterface export addresses previously documented: client 0x24164180 and server 0x221FB5F0, but their complete bodies and interface layouts are not authenticated here.

**Next executable milestone:** run MSVC Win32 x86 build/load smoke, capture exact requested retail interface versions and vtable contracts from authenticated Ghidra metadata, implement minimal independent engine-facing interfaces, then verify LoadLibrary/GetProcAddress and engine initialization without original gameplay DLLs. Windows PC precedes Android ARM64 and Quest 3.
