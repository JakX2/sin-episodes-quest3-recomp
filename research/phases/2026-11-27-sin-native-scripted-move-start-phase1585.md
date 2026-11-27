# Phase 1585: recover original shared scripted move start

Retail server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 verified from Drive bin.zip. Start9/10/11 dispatch 22050e36 to 2204ff10. Goal handle actor+8a8; stale/missing fails code1 then clears arrival state and calls TaskComplete (condition21 prevents completion). Dirty bit +b8&800 invokes transform refresh; goal +2bc vector minus actor +360, float-rounded squared distance, strictly <32 logs and calls TaskComplete twice with reset between. Walk activity6, run10, custom 2204fa20: name +3a0 -> activity lookup -> sequence lookup -> 15 or6; absent token uses actor+980 ->15 or6. Non15 calls native 22049870 translation and tests -1 without replacing requested activity in descriptor. Original 2204bc20 constructor produces 0x30-byte type1 goal, FLT_MAX positions, -1 sentinel fields, owner live word +2c. Script state4 uses +974 activity or +978 sequence lookup. Navigator +14 mode4; failure code11; success refresh/re-read target angles +354, owner 22073fd0; common arrival reset +974=-1,+978=0, TaskComplete. Explicit leaves remain transform refresh, name lookup, navigation goal/angles; no generic pathfinding, physics or model code. Run9/10/11 map to default 22054a70; normal success completes during Start. Standalone loaded test includes strict boundary, missing goal, activity/sequence fallback, rejected goal and native lifecycle. Full outer integration pending.

Region hashes (VA / bytes / SHA256):
- 2204ff10 / 565 / 9a076e6d15e2e6349ca44f9f498cbf3aeb0223ff3a51fd3fa3e643d1f430012a
- 2204fa20 / 250 / 5e8fa13cf1d86eacec26d1f2e83d7f8a659493136a8e5e821f922b5af140c73b
- 22050e36 / 21 / 7e9f13322d4b52ee16eaab8a3d866264937c0b6ee5e2374382e19149f11e077d
- 2204bc20 / 87 / 5a38e33eed1af6cd610762bfc0d3c21ba0a1d20cbac7907b7f5d9d34a10505c3
- 22073fd0 / 44 / f543180b015560cf8181f70dd3b8718e018395f2d7a6def5f91720fb727c6685
- 2202f0b0 / 29 / 47d8d0979bbf82ddc11077e168e38470c51499d26f96898a4801ab399bd69724
- 223d8fdc / 4 / 825ac1bb838d399fb1ba55a6247e2c8c7a0c3ec25898f3c94dbfb51fa6e73951
- 223ddf04 / 26 / aa7c5c9c0a3495174761603b4d9871c1945842671030f123712ffe5a7a2d3eb6

No original binary bytes committed. CI pending.
