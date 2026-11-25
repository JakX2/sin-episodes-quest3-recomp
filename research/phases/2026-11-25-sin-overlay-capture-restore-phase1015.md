# Phase 1015: client overlay network capture and restore

Original client disassembly authenticated against SHA-256 6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d:
- 240c21c0 watcher virtual +8 forwards stored delay +3c to 240c21e0. 240c21e0 records the bound row with replacement enabled, copies all 32 bytes to watcher+8, stores global 24369394 at watcher+28, then calls 240c2230 with float(current time from engine globals+0c minus delay minus 2.0f). The caller must supply these distinct clock values; no invented clock source.
- 240c2230 visits the chain with zero-based index, leaves index 0..2 alone, frees later samples with timestamp <= cutoff (equality included). This differs from 240c3230 interpolation-time retention.
- 240c1250 watcher virtual +c: clear chain, record current bound row three times at engine current time with replacement disabled, copy row to snapshot. Does not change watcher+28.
- 240c11d0 watcher virtual +18 restores all 32 snapshot bytes into bound row.
New client-only capture export preserves Phase1014 API ABI and composes existing native bounded recording/pruning. Loaded regression drives capture -> interpolation -> restore, reset duplicates, exact trim cutoff, snapshot preservation, finite clocks and bound-row validation. Storage ownership and original allocator caveats from Phase1014 remain. Windows CI pending; no retail game or full original capture execution claimed.
