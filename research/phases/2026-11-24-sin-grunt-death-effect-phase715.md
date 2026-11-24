# Phase 715: native client grunt death-effect override

Both owner original DLL hashes rechecked. Client method 2421cc30..2421cdc4:
base 240bf660 first (ordinary ragdoll path), return pointer saved.
Skip if null, source byte +d74 zero, or client service vslot99 returns
true. Bodygroup find 240b3520 with heads at 242b745c, count 240b3570,
set 240b3430 with count minus one. Then RandomFloat(0,1) through
24276268 compared with float .1 at 24276f80. Strict greater chooses
first branch, equality second.

Both branches call 2421c080 -> 240545e0: vector of three independent
RandomInt(0,32767) via 241a8b90, multiply by float at 242768f4
(bits 38000100), scale by (180 - -180), add -180. Host samples RNG;
double arithmetic models ordinary x87 intermediate precision.
First branch reads source vslot10 then vslot127, constructs pointer
record with source position/velocity/angular vector, three zero words,
model lookup and call 2419b130 (arguments include 0, -1, source).
Second branch reads vslot127 only, calls 240e4690 with model name,
position, received velocity, angular vector, 4.0f. Both return the
original base ragdoll result, not detached effect object.
Resource 242b7430 models/characters/mobgrunt/decaphead.mdl.
Source +d78 velocity and +d74 flag are the Phase713 fields.

Native API preserves ordered host effects, actual received fields,
random branch/vector, and return identity. No engine ragdoll or model
implementation, no assumption of opaque effect record binary ABI.
Failed callback marks control blocked until explicit host reconciliation.
Paired PE32 fixture supplies flag/velocity via original-layout network
callbacks and exercises null base, flag/suppression gate, exact .1
boundary, both paths, order, range and retained base result.
