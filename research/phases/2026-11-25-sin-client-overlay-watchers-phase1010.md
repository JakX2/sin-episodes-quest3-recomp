# Phase 1010: client overlay interpolation-vector lifecycle

Retail client SHA-256 `6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d` (user Drive bin.zip). Disassembled PE32 with objdump.

* 240c3670 registers DT_OverlayVars at +7a4, stride 0x20, max 15, reserve callback 240c0780 and length callback 240c3500.
* 240c0780/240c0270 reserve minimum capacity and zero dormant rows [count,target), retaining logical count.
* 240c3500 unregisters existing rows via 240c04e0 (first matching +4 pointer in +788 mapping vector of 12-byte records); changes logical layer vector then watcher vector at +7b8 (stride 0x44).
* 240c07d0 zeroes new layer fields except +0c (order remains untouched by that constructor; the receive reserve has already zeroed dormant rows). 240c06e0 destroys removed watcher rows in reverse order.
* 240c35f0 appends {1,layer,watcher} to mapping vector via 24061220, then watcher virtual slot +0 Setup(layer,1), +14 GetType, entity 240c88e0 interpolation amount, watcher +4 SetAmount.
* Reconstruction uses a preallocated host mapping/watcher buffer and raw layer allocation callback. It rejects live-row reserve relocation, insufficient capacity, malformed counts and missing services before mutation. Original arbitrary allocator growth and watcher constructor/destructor/virtual implementations are still outside this boundary. New layer rows are initialized after reserve; direct resize preserves order field as original constructor does. No claim of rendered interpolation or retail execution.
* Loaded Windows regression exercises existing actual RecvProp graph plus native reserve/resize, growth, equal count, shrink, zero count, preserved unrelated mapping and capacity refusal. CI pending.
