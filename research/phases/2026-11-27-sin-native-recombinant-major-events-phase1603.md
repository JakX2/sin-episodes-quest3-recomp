# Phase 1603: Major private melee animation events

Original server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Seven ordered private events registered 2237a66c..2237a72b. Handler +2c0 223787d0. Six single-arm variants derive owner forward/right from actor+36c via 22269bc0, add float-rounded forward*200 and right*(+100 right/-100 left), angular vector (-3,-5/-or+5,-2), side1 right/0 left. Medium and ordinary use live 2259d210+28 normal damage, heavy uses 2259d258+28; truncate toward zero. Both-arm event draws twice [-2,2] in order; angular (5,second,first), forward-only helper 22269b30, impulse forward*200, damage trunc(2*normal), side2. Reach helper +76c 22376f40 resolves player predicate +120 on current enemy; 85 for player, 65 otherwise. Final +774 22379fd0 remains owner boundary (includes tracked physics object throw and hit processing). Parent 2204a3b0 for unknown event. No generic angle or hit-trace implementation. Register and dispatch through native event dictionary, reject stale generation. Test covers all seven routes, side, damage, exact vector terms, random order, enemy/null and parent. Also correct inaccurate comment in Minor angular vector code; behavior unchanged.

VA/bytes/SHA256:
223787d0/1800/4fbf05cec9114775fa06411c010d2d3f94338bf10eff68232ec9f083eff15dcc
2237a66c/191/6451ca7fd74280da370328aae4ba891afabd369c22908995a481bc76515da880
22376f40/80/b2fde83d46dd4633e2897541eab31345dbc9223543bf5c5c92108a06fc2ff284

CI pending. No original executable bytes committed.
