# Phase 490: exact original CClientShadowMgr::Init configuration contract

## Retail original 2006 evidence

The private owner's original \`client.dll\` was authenticated with
whole-file SHA256
\`6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d\`
and inspected read-only. It is not committed or used by public CI.

Original CClientShadowMgr:
- vtable at **0x24297C4C**, \`Init\` slot0 -> **0x24118880**;
- method length **146 bytes** with complete SHA256
  \`32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438\`;
- original material initializer helper **0x24167810**;
- optional helper **0x24118610**;
- original callback symbol address **0x24115E70**.

The original initializer's code, **in exact observed order**:

1. Load string literal \`decals/simpleshadow\` and category
   \`Decal textures\`, clear object byte \`this+0xCD\`, then invoke
   \`0x24167810\` on embedded state at \`this+0x14\`.
2. Prepare three 32-bit floats with exact bit patterns:
   \`0x3DCCCCCD\`, \`0x3DCCCCCD\`, \`0xBF800000\`
   (approximately 0.1, 0.1, -1.0), and call **this-vtable
   slot 24** (byte offset +0x60) with their address.
3. Call **this-vtable slot 27** (+0x6C) with float32
   \`0x42480000\` (50.0).
4. Call **this-vtable slot 28** (+0x70) with float32
   \`0x3BA3D70A\` (approximately 0.005).
5. Write \`this+0xD8 = 8\`.
6. Read the external object pointer at global VA \`0x243581C0\`
   and its \`+0x2C\` value. **Only if nonzero**, call the
   private helper at \`0x24118610\`.
7. Call virtual method at byte offset **0x1D0** (slot 116)
   on external global object pointer \`0x2433F69C\`, passing
   callback address **0x24115E70**.
8. Return \`true\` by setting \`AL=1\` regardless of whether the
   optional helper was invoked.

This preserves exact original float32 bits, callback identity,
material strings, object offsets and source callsite order.
The names/semantics of the original vtable methods, the external
callback registration service, and the conditional helper's side
effects have **not** yet been fully verified. Therefore this
report deliberately does not invent those names or use Source
2013 APIs to fill missing behavior.

## New independent code and native tests

\`client_shadow_original_init_owned.h\` implements a deterministic
*owned* event-contract generator \`MakePlan(external_condition)\`.
It contains event types for all eight or nine actions (depending
on the original external flag) and copies the original raw float
values and method-slot identities into that plan. There are no
real renderer pointers, material objects or registered callbacks.

\`ApplyPlanToOwnedModel\` refuses any altered source sequence,
material string, float bits, method slot, callback VA, action
count or optional branch, leaving the prior independent model
unchanged on refusal. This reproduces the **known observed
configuration/state intent** and does not claim to have executed
the original engine methods.

\`test_client_shadow_original_init_owned.cpp\` validates both
conditional paths, exact ordered source actions, raw float
configuration, object offset writes, callback slot+argument,
return-true and a negative forged-plan matrix.

\`audit_sin_original_client_shadow_init.py\`:
- \`--client\` / \`--archive\`: private read-only exact 2006
  original whole-DLL and 146-byte-method SHA validation, vtable
  slot, material strings and original direct CALL edges;
- \`--self-test\`: public fabricated method region and source-witness
  mutation refusals, no proprietary original method bytes.

## Do NOT count this as normal Init success yet

This phase recovers the precise source original \`Init\` **call
contract**, not an engine-call-capable \`CClientShadowMgr\` with
the complete real renderer interface ABI, material ownership,
callback lifetime, shadow manager storage and level shutdown.
The entry remains \`init_body_reconstructed=false\` in the original
12-AutoGameSystem sequence until these external calls have safe,
tested implementations.

The normal standalone \`VClient011::Init\` still fails closed.
The last genuine owner-retail Windows diagnostic checkpoint remains
\`0x53494E74\`; the prepared read-only soundscape target
\`0x53494E76\` is still unobserved on the owner's retail game.
The Android reference CI build/smoke does not provide playable
Windows, Android or Quest 3 gameplay.
