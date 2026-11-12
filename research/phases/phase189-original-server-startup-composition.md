# Phase 189: original server startup gate composition regression

Date: 2026-11-12. Parent branch: Phase188 (PR #241). **Synthetic host integration only.**

## Verified upstream evidence

- Phase187's original `CServerGameDLL::DLLInit` dispatch requires (1) AutoGameSystem initialization (`0x221F5700`), (2) scene repository initialization (`0x221AD8A0` -> `0x221AD3E0`), and (3) model-sounds repository initialization (`0x22149B60` -> `0x221498C0`), short-circuiting on the first false result. The pending system list is cleared and initialized flag set before system callbacks.
- Phase188's independently reviewed `scene.cache` and `modelsounds.cache` initializers use repository version 3 and 2 respectively; missing cache or incompatible header marks cache invalid/missing and returns success. Invalid cache headers are closed before file invalidation. The initialized flag enables no-I/O subsequent entry. Original per-record helper calls remain opaque.

## New code

- `include/sin_compat/original_server_startup_composition.hpp` binds the *existing* Phase187 three-gate driver to the *existing* Phase188 cache initialization model with distinct scene/model-sounds states and host operations. Compile-time assertions pin both singleton and implementation address associations.
- `tests/test_original_server_startup_composition.cpp` verifies: pointer dedup and callback ordering, scene records processed before model-sounds startup, missing scene cache remaining nonfatal, two incompatible cache headers closing before invalidation, failed system callback preventing both caches while preserving original-known registry state, and no-I/O initialized repository re-entry.
- Root CTest and native Windows x86 MSVC CTest target added. The tests use synthetic host data and fake handles, not original commercial DLL bytes.

## Validation

Local optimized `g++` and `clang++` C++17 test builds passed with `-Wall -Wextra -Wpedantic -Werror -UNDEBUG` using matching copies of the Phase187/188 model headers. Hosted CI and original Windows engine runtime have **not** been verified by this result.

## Limits and next work

**Zero new original native function bodies are claimed.** The reviewed atlas stays at 175 unique methods (83 server, 92 client). The entire `CServerGameDLL::DLLInit` sequence after these gates, concrete Source IFileSystem ABI and virtual method calling conventions, cache record payload parsing, original executable-hosted DLL replacements and gameplay remain unsolved.

Next high-value milestone: pin real filesystem interface factory version, vtable slot conventions, and startup callback signatures against the owner-supplied original DLLs, then execute the Windows PE32 proxy smoke under the original executable with a rollback-safe setup. PC before Android before Quest 3.
