# Phase 1605: original Minor leap and spit attack conditions

Direct original disassembly SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Minor virtual +6c4 2237b7e0 checks enemy then current condition10, distance<0 returns36, distance>350 returns37, now>=+c3c then absolute origin Z difference <=256 returns21 (otherwise zero). Minor +6c0 2237d050 checks now>=+60c, grounded +bc&1, same distance conditions, then 2237b6d0 returns20 on true. Helper enemy query, dirty origin refresh +b8&800, enemy +1c4 target vertical distance <=150, cast witness, hull mins/maxs from token+610, second conditional origin refresh, target helper 2237af80 (two enemy queries, +f4 special -> target point else center+10 Z, +120 witness). Hull trace mask0x200400b group0, ignored actor; accept if hit==requeried enemy, otherwise held-object predicate 22256b00 (false on null), otherwise cast success and null hit. No generic collision/physics reconstructed. Source owner supplies trace, hull dimensions, cast, target and origin leaves. Tests cover strict thresholds, equality, cooldown, mask, trace hit and held branches, callback order.

VA/bytes/SHA256:
2237b7e0/176/2887c000f0b63814c368521bf0bfe72ba82f747a771ad24bb1d821fd73618fd0
2237d050/100/6a606bf25fc303e282f70e079b6fe1db83f514577afb15d2ac9f0efea090a225
2237b6d0/272/ddebd9344bb37ceb54961026801ffa4f06af801b8e1bb11bd9ca340f699d7398

Loaded fixture only; no original bytes committed. CI pending.
