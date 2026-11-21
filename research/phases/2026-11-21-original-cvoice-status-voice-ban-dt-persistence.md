# Phase 553: original `CVoiceStatus` voice-ban persistence, `voice_ban.dt` V1

## Provenance and evidence boundary

This report is a bounded, independently authored source-level interpretation of **the user's original 2006 Windows x86 SiN Episodes: Emergence DLLs**, not purported original Ritual C++. Full-file SHA256 identities are `client.dll` `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` and `server.dll` `568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650`. Neither DLL is redistributed. Original PE image base `0x24000000`.

Phase 551 had independently authenticated the 256-bucket, 32-byte opaque identity-key table and its native insert/reset routines. Phase 552 had authenticated `CVoiceStatus` RTTI, initialization and destructor, and demonstrated calls to *previously unresolved* helpers `0x241E4F90` and `0x241E4D00`. This phase identifies those original helpers as persistence **load** and **save** respectively.

### Three new full original method digests

| VA | Length | Complete SHA256 | Verified role |
| --- | ---: | --- | --- |
| `0x241E4D00` | 130 | `a2e7e39084e17a42aea5aecc7dd5ff5a94dc6e0be414f7d8caad2f842a619629` | Binary write of v1 header and 32-byte identities |
| `0x241E4F80` | 5 | `0873af1117bd7228ee29e0b904b306310d0b12e225a6dd1bb9836b1a2e9ddb6c` | Direct tail jump to previously recovered hash-table reset |
| `0x241E4F90` | 187 | `0cdc0f2ce180fb3eb921a0311b65735fa8b46d275b4220cac5f7d32909f0d471` | Clear-first v1 file read, version check, 32-byte key reconstruction |

**322 newly authenticated and mutually disjoint original instruction bytes.** Each method ends immediately before `INT3` padding. The reference auditor verifies their full hashes, 40 exact instruction/data witnesses, four direct-call edges (two are *previously recovered callers*), one tail jump, and three exact source strings. It does not count Phase 551/552 method bodies again.

## Original filename, native file calls, and binary version

The original `client.dll` data global `0x2431D238` initially contains pointer `0x242A9EE4` to the null-terminated native literal **`voice_ban.dt`**. Both helpers read this global rather than embedding an arbitrary new file path. The writer uses the exact null-terminated literal **`wb`** at `0x242A03AC`; the reader uses **`rb`** at `0x24289C0C`.

The original file-service object pointer is loaded from client global `0x2433F6BC`. This original pointer's source interface and runtime validity are not independently reconstructed here. Based on identical indirect virtual slots and call signatures, the relevant observed operations are:

| Native virtual byte offset | Observed use | Source-backed arguments |
| --- | --- | --- |
| `+0x08` | Open binary file | filename pointer, literal `wb` or `rb`, option 0 |
| `+0x04` | Write bytes | native handle, 4-byte version or 32-byte record |
| `+0x00` | Read bytes | destination buffer, 4-byte header or 32-byte record, handle |
| `+0x0C` | Close handle | handle |
| `+0x10` | Position handle | handle, offset and original numeric mode |
| `+0x14` | Query current file position | handle |

**Do not substitute a modern filesystem vtable.** Native seek arguments are evidenced as `(offset=0, mode=2)` and then `(offset=4, mode=1)`, but the original provider's enumeration semantics remain unresolved. The file format itself is derived from actual writes/reads and is independent of naming those enum values.

The writer sets a native dword to **1** at `0x241E4D33` and invokes the provider writer for **4** bytes at `0x241E4D3E`. As the original x86 client is little-endian, the on-disk header is `01 00 00 00` for successful writes. The reader performs a 4-byte read and explicitly requires the dword to equal 1 (`0x241E4FD1`). There are no additional confirmed per-entry tags or length headers.

### Binary disk layout (normal successful native save)

```text
0x0000 .. 0x0003  uint32 LE format_version = 1
0x0004 .. 0x0023  key[0] = 32 opaque raw bytes (if any)
0x0024 .. 0x0043  key[1] = 32 opaque raw bytes (if any)
...              zero or more further 32-byte keys
```

Keys are **not** known to be Steam IDs, names, plaintext strings or GUIDs. Do not invent a specific user identity format.

## Save path: `0x241E4D00`

The destructor authenticated in Phase552 calls `0x241E4D00` at native **`0x241E50B1`**, passing the embedded hash object at owner+`0x28`, conditional on an engine-provided nonnull pointer and on byte owner+`0x282C` being nonzero. The save method itself:

1. Uses the original file-service pointer and original filename pointer to open with mode `wb`. If opening fails, it returns after closing no handle.
2. Writes one dword containing version 1 using virtual byte slot `+0x04`, with count `4`.
3. Runs a fixed outer loop of **256** buckets. For each native bucket sentinel, starts at its `next` pointer `+0x24`, traverses until the sentinel is reached, and writes the first **32 bytes** of each stored key through the same provider write slot.
4. Advances each bucket by the original native **40-byte** stride (`0x28`).
5. Closes the handle using provider virtual `+0x0C` and returns with `ret 4`.

Bounded C-like reconstruction (method signatures and provider typing are descriptive):

```cpp
void save_voice_bans_like(HashTable256& blocked) {
    FileHandle h = provider->open(filename_global /* voice_ban.dt */, "wb", 0);
    if (!h) return;
    const std::uint32_t disk_version = 1;
    provider->write(&disk_version, 4, h);
    for (std::size_t bucket = 0; bucket < 256; ++bucket) {
        Sentinel& sentinel = blocked.bucket[bucket];
        for (Record* p = sentinel.next; p != &sentinel; p = p->next)
            provider->write(p->opaque_key, 32, h);
    }
    provider->close(h);
}
```

Original code does **not** check every write's returned byte count before continuing. The original return register should not be interpreted as a verified file-write success result.

## Load path: `0x241E4F90`

The Phase552 `CVoiceStatus` initializer calls this method at native **`0x241E5C79`** after obtaining an engine-provided pointer, and writes flag owner+`0x282C` to 1 afterwards. The entire load method:

1. **Clears all existing entries first** via complete native Phase551 method `0x241E4EC0` (`call` at `0x241E4F97`).
2. Opens `voice_ban.dt` with original `rb`. If open fails, skips to its final `al=1` return. Clearing already occurred.
3. Reads exactly a requested **4 bytes** into its local header storage, and checks it equals 1. If not 1, closes the file and returns `al=1` with an empty native hash.
4. If the version matches, executes the native file-provider positioning virtual `+0x10` with `(0,2)`, calls position-query `+0x14`, subtracts **4** and shifts the unsigned result right **5** (`>> 5`) to derive a record count.
5. Issues the second position virtual call with `(4,1)` before reading the keys. For each counted record, requests a **32-byte read** into a stack buffer and directly calls previously authenticated hash insertion method `0x241E4E30` with that buffer.
6. Closes the native file handle and returns with `al=1` and `ret 4`. This return is not a reliable indication of loaded-record count.

Bounded C-like *intent* (the numeric native seek semantics are deliberately not invented):

```cpp
bool load_voice_bans_like(HashTable256& blocked) {
    blocked.clear();
    FileHandle h = provider->open(filename_global /* voice_ban.dt */, "rb", 0);
    if (!h) return true; // original return path
    std::uint32_t version; // original 4-byte stack target
    provider->read(&version, 4, h); // native ignores short-read status here
    if (version == 1) {
        provider->position(h, 0, /*native enum*/2);
        std::uint32_t record_count = (provider->tell(h) - 4u) >> 5;
        provider->position(h, 4, /*native enum*/1);
        while (record_count--) {
            unsigned char key[32];
            provider->read(key, 32, h); // original short-read status not checked
            blocked.insert_opaque_key(key);
        }
    }
    provider->close(h);
    return true;
}
```

**Failure/unsafe path warning:** unlike the independent C++17 reference, the retail x86 loader neither rejects every truncated file nor validates that the file has a multiple of 32 bytes following the header. Its `length-4` unsigned arithmetic can underflow. The code does not locally verify each read or insertion success. Do not use this decompiled sketch as a hardened file parser and do not assert the native implementation safely handles corrupt `voice_ban.dt` data.

## Independently authored source reference and verification

`include/sin_compat/original_voice_ban_persistence.hpp` and `tests/test_original_voice_ban_persistence.cpp` supply an **original-free** C++17 format/reference model, matching the authenticated 256-bucket ordering, original 32-byte identities and little-endian version 1. It enforces extra safeguards on malformed/truncated files, caps record count and deduplicates repeated identities. Those defensive checks are *intentional differences* from the original x86 loader and are explained in source.

```sh
python3 research/tools/audit_sin_original_voice_ban_persistence.py --self-test
python3 research/tools/audit_sin_original_voice_ban_persistence.py \
  --bin-zip /path/to/private/original/bin.zip
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The local strict GCC and Clang C++17 tests passed. The private SHA-pinned auditor returned `ORIGINAL_VOICE_BAN_PERSISTENCE_AUTHENTICATED`: **3 original complete methods, 322 new native bytes, 4 original call edges (including two verified previous callers), 1 direct jump, 40 opcode witnesses, and 3 original data strings**.

**Still unresolved:** original file-service virtual interface name and seek enum; exact player identity representation; guaranteed persistence on real Windows installations; actual voice network policy; any parallel save/load interactions. This is source-level original game decompilation, **not a Windows/Quest 3 game boot**.