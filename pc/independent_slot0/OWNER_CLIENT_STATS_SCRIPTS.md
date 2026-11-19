# Original SiN Episodes 2006 stats script grammar

Owner-provided SE1/scripts.zip was inspected locally. The assets are
not committed into this repository.

stats_definitions.txt: SHA256 a18afd2e8fba54653a9bd173c3eacf440fe016b3659b74578f2ecd1f37a156a8.
Nested StatDefinitions root, 62 unique stat objects, each with name,
short_text and long_text; eight have percentage_of references and all resolve.

stats_display_layout.txt: SHA256 850dd06942b459843236dd8c6770b40a5b28e6b6a0fcab7bf4e60dec55322dbe.
Flat 156 quoted name/format pairs plus 66 unpaired divider markers
(26 equals signs). All STAT_ references resolve. Original formats
include tab, title, value, percent, percent_of, time, dynamic_block.

The new C++17 client_stats_script_parser.h handles both distinct
formats, comments, bounded tokens/input, duplicate checks, references,
exact ordering and malformed input without mutating earlier results.
Native CI uses synthetic data only, while optional owner validation
accepts 2 extracted paths:
sin_client_stats_script_parser_test.exe [stats_definitions.txt] [stats_display_layout.txt]

This parser was also compiled locally with GCC 14 C++17 against the two
actual owner scripts and returned definitions=62 percentage_refs=8
display_pairs=156 dividers=66.

This is not the 2006 game's KeyValues/file-manager binary ABI.
No claim of full C_StatsSystem Init or playable Windows game.
The latest retail owner checkpoint remains 0x53494E63.

## Independent on-disk SE1 asset loading

client_stats_asset_loader.h implements an owned standard-C++17
filesystem loader taking the SE1 content directory and reading
scripts/stats/stats_definitions.txt and
scripts/stats/stats_display_layout.txt. Inputs are bounded at 256 KiB,
then both parse into temporary vectors. Only after BOTH parse
successfully does the owner replace its existing parsed state.
Missing, truncated, oversized or malformed files preserve the last
valid definitions/layout and return a concrete error.

Native CTest exercises synthetic temporary directory files, successful
read, invalid layout rollback, successful reload and oversized input.
The optional owner mode runs:
sin_client_stats_asset_loader_test.exe <path-to-SE1>

The code was also compiled with GCC 14 C++17 locally and run against
the two genuine owner Drive stats scripts under an SE1 directory,
reporting exactly 62 definitions, 8 references, 156 layout pairs and
66 dividers. This is real source asset IO, not a replacement for the
proprietary 2006 engine IFileSystem or in-game HUD renderer.

## Optional integration at the seven-stage Stats Init boundary

The existing independent C_StatsSystem Init coordinator now accepts an
optional SE1 content-root pointer. When enabled, its original
LoadDisplayLayout callsite reads both real stats text documents into an
owned, atomic stats_assets::Documents object. Missing/broken files
produce AssetLoadRefused at attempted step 4 (three completed), and
the last valid documents remain readable. The normal callback for
the still-unimplemented original layout helper is REQUIRED afterwards:
parsing a script is not a full 2006 engine layout registration.

Without a content-root pointer, earlier independent CTest and ABI
behavior are unchanged. The new synthetic native integration test
covers success, malformed layout refusal, retained prior state, and
successful retry. This deliberately does not claim that original
stats_definitions resource enumeration runs in exactly this order:
the source files are reloaded together at the independently owned
display-layout stage for host safety. No retail game initialization
or complete original filesystem ABI is claimed.
