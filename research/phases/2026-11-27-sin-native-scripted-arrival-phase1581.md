# Phase 1581: scripted arrival and facing start tasks

Direct objdump of retail server.dll from user Drive bin.zip, SHA-256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. StartTask 22050150 indexes 22052d14 by task-1. Entries 111/112 point to 22052883/220528b5. RunTask 220538d0 indexes 22054b44 by task-2, task112 maps to 2205391e (already recovered facing alias: motor +48(-1), 22031530 yaw error <= .006, then completion). Start111 resolves actor+8a8 with exact serial comparison, fetches absolute origin via 220092d0, calls 22143b60, then TaskComplete even for stale/null goal. Start112 resolves the same goal, quantizes its +370 yaw with 22028200 into actor motor+14, skips virtual +71c only for script state 6, then stops the navigator with true. No TaskComplete in Start112. Dedicated server-only export, owner origin/turn/path leaves, no existing ABI changes. Tests live/stale/missing goal, state6, yaw wrap and short storage. Integration into outer scripted FACE/WALK remains future work; CI pending.

Authenticated region digests (VA, byte length, SHA-256):
- 22052883, 50, e9e507d315145fa2ebddb1c31bf2670ab62a3e104cae9da80eb4ee38547edbe9
- 220528b5, 95, aabb549990b91c1c2dffdfa863de0fb01483650f374ed51a5b16c92bc4891960
- 2205391e, 25, 4a061721318b1cddf59a9a41e9f994bb0ee2ae684de3c27d569f80a60c9b8beb
- 22031530, 32, cca50711f6186d364953ce1978a5bc13e65f1132524060465857f263961e698c
- 22028200, 64, 64ece42a52c9b02e7e0f68d4663240b921cb28eda8d336f250fa987d335e0827
- 220295f0, 112, 8111af626a795cb306d06f4b06e90b89665a579beb3e606bafa190498d651102
- 22052ecc, 8, e3760fc324f0072b5482a1aca7d2a05a394e847def04e24a7c4197e213ddba01
- 22054bb2, 1, 9d1e0e2d9459d06523ad13e28a4093c2316baafe7aec5b25f30eba2e113599c4

No original binary bytes committed. No generic physics implementation.
