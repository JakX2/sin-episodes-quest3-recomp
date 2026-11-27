# Phase 1613: original Minor +0x398 damage reaction gate

Authenticated retail server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Vtable Minor+0x398 ->2237e8c0. Inherited Source vtable+0x398 ->22044730 returns false unconditionally, so this is a SiN-specific override. The Minor queries the original global engine service at 22572198 virtual+0x170; when true the override returns false without touching its argument. Otherwise reads 32-bit float at damage-info offset+0x2c, and FCOMP with exact static10.0 at 223d8f14. Original AH&0x41 branch accepts only strictly above10; equality, below and unordered values return false. Distinguishes valid rejected gate from invalid input in native API, fails closed for truncated buffers. No shared damage handling or engine game-mode behavior duplicated. Regression tests exact threshold equality, null source while mode suppressed, positive/negative, NaN, wrong actor class and owner callback failure.

Original region VA/bytes/SHA256: 2237e8c0/56/e08a5b91b9f7e3411976303123a218fb28e6d3ff2df0690a1e8df6525b714ff7.
No original binary code committed. CI pending.
