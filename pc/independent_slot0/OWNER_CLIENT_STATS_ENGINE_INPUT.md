# Independent stats coordinator: engine-backed documents

This change is stacked on PR #450, the independent Source SDK 2004
`VFileSystem012` read-only Win32 x86 ABI experiment. Its original-base
anchor is the SHA-reviewed 2006 `C_StatsSystem::Init` at 0x241FCAA0.

## Preserved authentic operation order

1. Initialize 55 independent static statistics records
2. Initialize 11 dynamic statistics records
3. Initialize 66 catalog pointer slots and request the unresolved
   original resource enumeration continuation
4. Load `scripts/stats/stats_display_layout.txt` and source definitions
   through a single independent parsed-document owner
5. Register the `StaticStat` callback
6. Register the `DynamicStat` callback
7. Register the `ResetHUD` callback

The original function unconditionally ends by returning true, but the
independent host must fail closed on unimplemented or refused callbacks.
This is not evidence that original engine structs, registry or helper
calls have been reimplemented.

## New read callback

`stats_coordinator::Hooks::read_engine_documents` is an optional
`noexcept` function pointer:

```cpp
bool (*)(void* context, std::string& definitions,
         std::string& layout, std::string& error) noexcept;
```

`engine_fs::ReadForStatsCoordinator` implements that signature using
the previously reconstructed SDK virtual calls. The reader receives the
separate non-null Hooks::asset_reader_context engine interface pointer,
while catalog and event callbacks continue to receive Hooks::context.
A missing engine context fails before the first owned storage reset as
MissingAssetReaderContext. Both files must read
successfully before it swaps either caller-visible string. The stats
coordinator then validates the actual SiN grammars via
`stats_assets::Documents::LoadTexts`, atomically publishing both parsed
vectors only when every reference resolves. Earlier valid parsed
documents survive an I/O failure, malformed source, or allocation
exception. An exception produces a bounded `AssetLoadRefused` outcome
from the `noexcept` coordinator, rather than unwinding into the engine.

Using `game_content_root` and `read_engine_documents` together yields
`ConflictingAssetSources` before resetting the three owned vectors.
This is a host invariant. It is not a claim about retail Source API.

**The engine ABI is still not owner retail verified.** The original
stats catalog resource enumeration and `C_StatsSystem::Init` event
registration methods remain injected, opaque, and unimplemented.
That is why this integration is currently only a synthetic native
test and is not enabled in the replacement game's live initialization.
Do not confuse test-injected placeholder resource/event hooks with
original 2006 callbacks.

## Tests

- New independent Win32 x86 test verifies the seven-step host order,
  missing-resource refusal, engine-backed content injection, parser
  refusal, no later registrations, intact prior documents and retry.
- Existing `VFileSystem012` synthetic MSVC test now runs the complete
  host fixture using the actual engine ABI adapter as its reader.
  It includes short-read refusal, retained scripts, and retry.
- CTest still uses synthetic source-free fixtures. No original assets
  or binaries are committed or published.

Windows PC startup and original VClient011 completion are still
unverified beyond the owner-proven sound-system checkpoint
`0x53494E63`. Android and Quest 3 remain follow-up targets after
independent Windows engine startup is proven.
