# Original SiN Episodes 2006 physics surface-properties resource bootstrap

## Source provenance

User's 2006 original client.dll SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Privately SHA-verified original methods:
- 0x241927A0, 247 bytes: SHA256 7357bdfb9934f175e0d67f4a81236d81087a07bb6145918083c44fd255ca6905. Loads scripts/surfaceproperties_manifest.txt through original KeyValues LoadFromFile mode rb with pathID GAME, iterates file entries.
- 0x24191EC0, 136 bytes: SHA256 f5e1bbb19b7d8305690aa65992653d244ee0e8aaaf105e057fb760a889c218b5. Opens per-file surface scripts through original VFileSystem012 using rb and NULL pathID, then invokes original VPhysicsSurfaceProps001 virtual slot1 to register surface data. This later engine registration is not implemented.

## Original asset provenance

From owner-private scripts.zip:
- scripts/surfaceproperties_manifest.txt: 595 bytes, SHA256 d8bead334f07cd9f7cdfa30d691076b242ee469727aa0f0dcb734f3699d92a11
- scripts/surfaceproperties.txt: 23485 bytes, SHA256 86bb98fc2874d0e3ece18ff606fc4ee668c0a795596a393009e5534f372289a8
- scripts/surfaceproperties_hl2.txt: 2812 bytes, SHA256 125ff956d7d3cb9bf5cdacbd04a0bdde2243c0d31106adf72e71e15be5c34068

The manifest root is surfaceproperties_manifest; it lists exactly two source script files in the order above. Exact original asset lexer counts independently checked: 84+9=93 surface definitions, 62+9=71 direct base keys, no duplicate surface names ignoring case. These counts describe authentic retail data, not every custom Source mod.

## Source-backed, independent implementation

client_original_surfaceprops_manifest_owned.h implements bounded independent parsing, strict retail list/shape, conservative path validation and atomic publication of a complete owned metadata snapshot. It never calls physics surface registration and cannot represent a full Source KeyValues tree.

client_original_surfaceprops_engine_fs_bridge.h calls only previously owner-retail verified VFileSystem012 slots to read the manifest rb/GAME and the two scripts rb/nullptr, then invokes the independent metadata parser. It does not dereference or retain engine physics pointers. Native MSVC x86 original-free tests use a synthetic vtable and verify the exact call pattern, three file handles opened/closed, 93/71 shape and safe rollback.

## Next owner-only runtime check

The owner already achieved the real Windows original-engine 0x53494E76 soundscape checkpoint and both original soundscape ConVars via read-only lookup, recorded in PR #512. Phase 513 independently reconstructed the 97-byte triple physics Factory query and passed 135/135 Win32 tests; this phase adds optional read-only surface properties resources after a same-session physics 3/3 and source soundscape+CVar proof.

After all four Windows/Android CI workflows pass and the latest artifact is checked, extract the standalone Windows package outside the installed game, then in cmd run:

    RUN_SOUNDSCAPE_CATALOG_TEST.bat -SoundscapeCvarLookup -PhysicsFactoryLookup -SurfacePropsRead

The one-click launcher preserves original-SHA authentication, a disposable copy, earlier presence/read/staged 66-Stat/27:28:5 hook/soundscape prerequisites, safe restoration and a text-only results ZIP. The ZIP summary includes separately the original physics Factory triple and the original surfaceprops manifest+script shape result, or NOT_RECORDED if an earlier stage refused. A verified 3/3 or 93/71 would still be a read-only diagnostic, not successful original VClient011 Init.

## Limitations

NO original VPhysicsSurfaceProps001 virtual ParseSurfaceData method is invoked, NO native game-system registry Init or server ServerGameDLL003 DLLInit is completed, and NO physics simulation or game map is running. The original 0x53494E76/77 soundscape exit code remains unchanged by these optional checks. Windows/Android/Quest 3 gameplay has not been demonstrated.
