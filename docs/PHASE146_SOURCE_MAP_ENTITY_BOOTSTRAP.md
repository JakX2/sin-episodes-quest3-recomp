# Phase 146: Source BSP entity instantiation boundary

**9 November 2026.** Phase 145 reads the BSP entity-text lump. This phase
turns those parsed records into an explicit, generation-safe native-host
lifecycle contract instead of stopping at a parser-only static library.

## Implemented

- SourceMapRuntime::load_bsp runs the Phase 145 reader and passes the
  ordered entity document into the runtime.
- Validates one non-empty classname per entity and enforces a configurable
  entity-count limit before any host-side creation.
- Uses the host's real entity factory callback. It does not alias unknown
  SiN classes to stock HL2 classes or fabricate placeholder entities.
- Creates each ordinary entity and forwards every non-classname keyvalue in
  original property order (including duplicate keys), then runs separate
  spawn and activation passes.
- Uses generation-checked EntityRef values, rejects duplicate live entity
  indices, catches host callback failures, blocks reentrant map loads, and
  removes partially-created entities in reverse order.
- If rollback cannot remove an entity, retains its handle so the host can
  retry unload() instead of silently losing track of a live object.
- Adds desktop CTest, Android NDK ARM64 build checks and Android Bionic
  x86_64 execution of the actual parser-to-host runtime with a mock host.

## Deliberate boundary

This is the ordinary entity subset only. It does not implement BSP geometry,
brush models, collision, visibility, lighting, embedded pak files, materials,
models, sounds, Source networking, save/restore, target resolution, output
connection parsing, or campaign transitions.

The public Source SDK 2013 MapEntity_ParseAllEntities path has special
handling for worldspawn, transient nodes/lights, point templates, parent
hierarchy and spawn priority. This generic adapter does not pretend to
reproduce those special paths. The native engine host must handle those
classes through the engine's own map loader or a source-version-compatible
specialized path. The callback order here is an explicit integration seam,
not a claim that the original SiN 2006 DLL used this exact internal loader.

create_entity must route through a real host-registered classname factory.
The independent SiN rifle/grenade behavior factory in PR #154 is not
automatically wired by this component; the selected native host must connect
that factory after it has allocated the engine-owned entity handle. Unsupported
classnames remain the host's responsibility and must fail or be handled by
an independently implemented, source-backed class.

## Evidence and verification limits

- The input format is the Phase 145 clean-room VBSP entity reader.
- The entity lifecycle contract is new project-owned C++17 code.
- Unit fixtures are synthetic. No retail SiN BSP was available to compare in
  this run, and no original 2006 DLL bytes or SourceVR engine binary were
  freshly disassembled here.
- CI can prove desktop behavior, ARM64 compilation and Android Bionic
  execution with the mock host. It cannot prove SourceVR loading or Quest 3
  gameplay.

No original DLL, APK, ELF, BSP, model, texture, sound, script, or other
commercial game asset is included. Keep this branch stacked on draft PR #197.
