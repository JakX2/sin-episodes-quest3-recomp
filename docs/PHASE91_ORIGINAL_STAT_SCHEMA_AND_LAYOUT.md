# Phase 91: Original SiN stats schema and display/graph cross-reference audit

**Evidence:** Owner-supplied original \`game stuff.zip\`, specifically three original text files in \`SE1/scripts/stats\`. This is an independent portable schema implementation, not a recovered Ritual engine, stats data collector, or adaptive-difficulty algorithm. **No original scripts or commercial game data committed.**

## Provenance (files remain with owner)

| File | Original SHA-256 |
|---|---|
| scripts/stats/stats_definitions.txt | a18afd2e8fba54653a9bd173c3eacf440fe016b3659b74578f2ecd1f37a156a8 |
| scripts/stats/stats_display_layout.txt | 850dd06942b459843236dd8c6770b40a5b28e6b6a0fcab7bf4e60dec55322dbe |
| scripts/stats/stats_graphs_layout.txt | 8e5bdc958138eccef57554f48d6c74dfcf5d7956dee324bc95e722fa17bf2032 |

## What the original files actually establish

The **StatDefinitions** file contains **62 repeated \`"stat"\` blocks**, each with \`name\`, \`short_text\`, and \`long_text\`. Eight records additionally declare a \`percentage_of\` relation to another existing stat. These are not duplicate invalid keys: repeated \`stat\` blocks are how this particular schema is organized. In contrast, the Phase90 combat-spread loader rejects repeated keys and is **not appropriate for these definitions**.

The source comments explicitly describe a ratio display: 10 minutes in combat divided by 20 minutes total should display 50%. This does not establish how either raw time stat is collected or what the game does when the denominator is absent/zero.

The separate **stats_display_layout.txt** is line-oriented, with 123 stat-reference rows (58 distinct defined stats), **23 dynamic_block headings**, and **3 percent_of rows**. Every referenced stat exists; all three percent_of rows have a valid link in the definitions.

The nested **stats_graphs_layout.txt** has **five graph pages**, with **ten stat-reference rows** (ten distinct graph-series stat names) and four stat-based graph maxima plus one numeric maximum. All graph-series and stat-maximum identifiers resolve to definitions. Repeated \`name\` fields inside a graph's \`stats\` block are intentional.

There are definitions named \`STAT_CHALLENGE_RATING\`, \`STAT_INSTANT_CHALLENGE_RATING\`, \`STAT_GAME_PLAY_VAR_AVERAGES\`, \`STAT_EXPECTED_TIME\`, \`STAT_TOTAL_DAMAGE_TAKEN\` and numerous combat/health accuracy measurements. **These names are evidence of UI/stat metadata, not evidence of which values feed the original adaptive-difficulty formula.**

## Native C++17 implementation

\`StatSchema\` in \`include/sin_compat/stat_schema.hpp\` / \`src/stat_schema.cpp\`:

- Parses the original schema's quoted, comment-aware nested KeyValues subset and preserves the order of repeated \`stat\` objects.
- Verifies required fields, unique stat names, optional \`percentage_of\` references (including forward references), bounded document size, token length and nesting.
- Rejects malformed, unknown or duplicate fields and fails closed: unsuccessful reload clears the existing schema.
- \`find(name)\` returns the definition metadata.
- \`percentage(name, values)\` computes 100 * numerator / denominator only when both runtime numbers were supplied, are finite, and the denominator is nonzero. Separate typed statuses represent missing definitions, non-percentage stats, missing values, invalid values and zero denominator.
- Does **not** implement data collection, save/load, dynamic weapon/level stat values, UI rendering, game rules, or adaptive challenge.

Compiled into the existing native/Android ARM64 static library without changing any exported host C ABI symbol.

## Verification

- \`tests/test_stat_schema.cpp\`: original-free synthetic repeated-block parser tests, ratio calculation, missing/zero values, invalid/duplicate schema and forward reference behavior.
- \`tests/check_private_stat_schema.cpp\`: optional owner-only binary that loads the original file from a supplied local path and checks 62 definitions, eight links, known stat names and one 50% example. It is built but not run in CI.
- \`research/tools/audit_original_stats_layout.py\`: SHA-pinned original-file-only metadata auditor across definitions, display rows and graphs. Outputs aggregates and Boolean fields, not original content.
- \`research/tests/test_audit_original_stats_layout.py\`: seven synthetic Python cross-file tests including intentionally repeated graph name keys, malformed formats, unresolved stats and duplicate definitions.

Reproduce:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    python3 -m unittest discover -s research/tests -p 'test_*.py'
    ./build/sin_stat_schema_private_audit /private/scripts/stats/stats_definitions.txt
    python3 research/tools/audit_original_stats_layout.py '/private/game stuff.zip'

The private original source passed the native schema checker locally. The synthetic native C++ test passed GCC C++17 strict warnings and AddressSanitizer/UBSan; seven Python tests and the SHA-pinned three-file original layout audit passed. GitHub CI independently runs all synthetic tests and cross-builds on the official Android NDK, without private source data.

**Next task:** trace actual 2006 server.dll writers/readers of \`STAT_CHALLENGE_RATING\`, \`STAT_INSTANT_CHALLENGE_RATING\` and relevant combat averages, and connect only verified effects to the Phase84 checkpoint system. Do not invent challenge formulas based on names.

No game BSP, SourceVR-native game module, full original AI engine or Quest headset tested. R1 issue #12 and campaign issue #20 remain open.
