# Source BSP entity-lump reader

**9 November 2026.** This is the first map-file ingestion layer for the
portable SiN reconstruction. It reads the standard little-endian VBSP
header, validates the lump-0 directory entry, and converts the entity text
into ordered key/value records.

## What the reader does

- Accepts Source BSP header versions 19 and 20, with the 64-entry lump
  directory and 1036-byte header.
- Reads only LUMP_ENTITIES (index 0); it does not interpret geometry,
  visibility, collision, lighting, game lumps, or embedded pak content.
- Rejects signed-negative/out-of-file offsets, empty or malformed entity
  lumps, and nonzero lump encoding/compression markers rather than trying
  to decompress an unknown representation.
- Parses quoted entity key/value pairs, // and /* */ comments, UTF-8
  BOM, common quoted-string escapes, and trailing NUL padding.
- Preserves entity order, property order, and duplicate keys. find_first
  is a convenience lookup only; it does not discard later duplicate fields.
- Builds into a temporary document and publishes it only after the whole
  entity lump parses successfully.

## Format references and limits

The header layout, 64-lump directory, entity lump index, and compression
field are documented in Valve's public Source SDK bspfile.h:
https://github.com/ValveSoftware/source-sdk-2013/blob/master/src/public/bspfile.h

The standard Source BSP format overview also identifies VBSP, the
little-endian header, and lump 0 as the entity text:
https://reverseengineering.wiki/index.php/Format%3ABSP_%28Source%29

This is a clean-room reader of the public container format, not copied
Valve source. The reader is not yet validated against a retail SiN Episodes
map in this run. Supporting BSP v19/v20 here does not assert that every SiN
map uses both versions. The original campaign map version and entity-lump
quirks still need a read-only comparison against the user's local maps.

The parser is not a map loader yet: no brush/face/vis/collision construction,
model/material/sound resolution, entity factory instantiation, target
connection scheduling, or save/restore integration is performed. It is a
real input component that can feed those later stages, not a playable map
runtime or a SourceVR game-code module.

## Tests

tests/test_source_bsp_entities.cpp builds small in-memory BSP fixtures to
exercise the real parser's header/range checks, ordered entity extraction,
duplicate preservation, quoting, comments, NUL padding, malformed input,
unsupported encoding, and output atomicity. These are parser unit tests,
not a binary audit and not Quest execution.

No BSP, model, texture, audio, DLL, ELF, APK, or other commercial game asset
is committed.
