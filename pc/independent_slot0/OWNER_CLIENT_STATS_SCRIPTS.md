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
