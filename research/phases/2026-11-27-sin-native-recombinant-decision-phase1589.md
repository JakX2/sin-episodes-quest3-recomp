# Phase 1589: original SiN mutant decision and condition policies

Original server SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650 verified. Fresh direct disassembly of CNPC_RecombinantMajor/Minor. Major selector +434 22378010: current condition69 returns83,67 returns86,22 clears +c28 and checks serial +c44 (live86, missing39; does NOT modify handle). Flinch helper 22376e30 prioritizes condition17 before16, gate +988&40, original activity/model/sequence checks; returns22 or21. State3 with cond25 and live enemy +8a0 uses float-rounded squared distance and live cvar threshold; >= returns81, original +770 constant true. All remaining branches delegate inherited selector 2204fd80. Minor selector 2237b160 clears condition67->81, clears68 on 68/66->82, otherwise inherited selection. For parent41/42, 2237b060 uses independent [0,1] draws: first <0.6f ->91, else second < actor+ c44 ->41 else42. Helper cannot return zero, so its original dead fallback not synthesized.

Major gather +420 22378fe0 clears condition66 before inherited gather, then serial +c44 determines current67 and20 mutually exclusive. Major +490 223790b0 sets interrupt67 unless ideal86, delegates then clears16/17 if ideal39 or +bf&8. Minor +490 2237b2e0 sets/clears66 by ideal86 and clears17 before inherited call. Major +42c 22376d70 clamps long timer to now+.5 only if now+1<old deadline; delegates. Major +49c 22376da0 uses native current condition and schedule-interrupt check, inherited flinch effect +4a0 as external leaf, then clears flag40 only when now strictly exceeds +9e0. Existing inherited implementation is not recreated. Registered server-only API and loaded regression. No original bytes committed. No full actor scheduler claimed.

Regions VA/bytes/SHA256:
22378010/236/dcc9e07774c541613e8a44be886e75a91cb866e9d0dd8925613d687eede178b4
2237b160/124/4f6bc864355ffe3f0c15d1b3322a5c2a0c5a8183d93c995dbc8f36c90f2e5a32
2237b060/80/9a1febae48e7945a5f755f8a7c9858daa6307e4c5cd4045189d0d59d80769532
22378fe0/102/1ddb279cddacf0ad8780abe572e9ec4f16b9f4e34ac14bc7b2dc7b7cc7a3c3c2
223790b0/76/3cba0242dbe89d7ec7e60a8cf006d743e8bac92f3eda4dc399cf128541d78e20
22376d70/48/98601e8e27ebf23cc372037162edbaed03723eddb1ac2e7f60fe5fe3f9df7a14
22376da0/143/6ca3a8af24b05bcf51acea7f55045030e2967b8abcc52edd410da06fa1eeaed8
22376e30/135/5eac9bd54a6361ce6404467c4fa97ac00994a19e55b599564866ecf9a8891eff
CI pending.
