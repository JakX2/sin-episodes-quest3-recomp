# Phase 1001: owner-only retail VCD event inventory

Run Python 3.11+ with `python research/tools/audit_retail_vcd_events.py scenes.zip`. Authenticates the original owner-supplied archive by SHA256, refuses duplicate normalized names or oversized members, reports aggregate declarations only, never extracts or publishes private scene content. Executed locally on the Drive archive: 261 VCDs, 2,994 event declarations.

| Type | Events | VCDs |
|---|---:|---:|
| gesture | 1083 | 116 |
| flexanimation | 572 | 119 |
| speak | 392 | 227 |
| lookat | 345 | 182 |
| sequence | 230 | 77 |
| face | 153 | 114 |
| firetrigger | 113 | 55 |
| stoppoint | 34 | 34 |
| loop | 31 | 31 |
| moveto | 31 | 15 |
| section | 7 | 3 |
| generic | 2 | 1 |
| expression | 1 | 1 |
| subscene | 0 | 0 |

240 scenes have one actor declaration, 21 have multiple. Caption type occurs in 201, absolute tags in 115, event ramp in 140. Prioritize engine-backed gesture/flex, sound/captions, look-at, sequence, facing and trigger leaves over speculative nested-scene corrections. This is a line-level declaration inventory, not full grammar validation, runtime hit counts or proof that every scene plays in the campaign.
