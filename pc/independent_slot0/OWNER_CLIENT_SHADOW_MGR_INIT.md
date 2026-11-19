# Original CClientShadowMgr::Init: independent sequence, NOT full game boot

Original 2006 SiN Episodes client.dll file SHA256:
6535f13e3c5d8f0205b5d198a3124d3b99971404de30c3fdacaf0f8728e1841d.

Original MSVC x86 virtual slot-zero CClientShadowMgr::Init at
0x24118880 is 146 bytes; whole method digest:
32dbbce9d4da9dce01522425c0531510640a5841a662c01abcec443df3d0d438.

The original function performs the following sequence:
1. Sets byte this+0xCD=0, registers "decals/simpleshadow" under
   "Decal textures" using a texture-related helper 0x24167810 on
   this+0x14.
2. Calls the object's own vtable slot 24 (byte offset +0x60) with
   float[3]={0.1,0.1,-1.0}.
3. Calls slot 27 (+0x6C) with float 50.0.
4. Calls slot 28 (+0x70) with float 0.005.
5. Sets DWORD this+0xD8 to 8.
6. If DWORD at [global 0x243581C0]+0x2C is nonzero, invokes an
   instance helper 0x24118610.
7. Calls the virtual method at slot 116 (+0x1D0) on the interface
   held at original global 0x2433F69C, passing callback address
   0x24115E70. The original method returns true.

Important: the identity of the 3 virtual configuration methods and
the full original meaning of the helper callbacks are not proven by
this trace. The native game assumes its singleton pointers exist.
Our callback-driven model deliberately refuses missing host pointers
and small object memory rather than dereferencing arbitrary globals.

The independent code in client_shadow_mgr_init.h models exact visible
state writes, ordered arguments, and branch-dependent calls while
leaving texture setup, vtable methods and engine callback registration
as injected host operations. It never calls 2006 function addresses.
The MSVC Win32 test exercises strict ordering, scalar constants,
byte preservation, conditional refresh, repeated registration and the
surrounding AutoGameSystem Init loop.

This is *not* the complete original ShadowMgr behavior, not a
finished one-of-twelve game-system Init replacement, and not a new
owner-game runtime checkpoint. Last owner runtime checkpoint is
0x53494E63, real sound-system Init returned true.

## Source SDK 2004 method identity corroboration

The user-supplied `source-sdk-2004-master.zip` contains
`src_mod/cl_dll/clientshadowmgr.cpp`, with
`CClientShadowMgr::Init` and its Source-era class method declarations.
The SDK `Init` logic matches the original 2006 146-byte native
method's order, constants, conditional and callback arguments:

- `m_bRenderTargetNeedsClear = false` corresponds to `this+0xCD`.
- `m_SimpleShadow.Init("decals/simpleshadow", TEXTURE_GROUP_DECAL)`
  corresponds to resource call at original 0x2411889A on
  `this+0x14`; the original category literal is "Decal textures".
- Own virtual slots 24, 27 and 28 correspond to
  `SetShadowDirection(Vector(0.1,0.1,-1))`,
  `SetShadowDistance(50)` and
  `SetShadowBlobbyCutoffArea(0.005)`.
- `m_nMaxModelsRendered = 8` corresponds to `this+0xD8`.
- `if (r_shadowrendertotexture.GetBool()) InitRenderToTextureShadows()`
  corresponds to the original provider at global
  `0x243581C0`, flag at offset `+0x2C`, and helper 0x24118610.
- `materials->AddRestoreFunc(ShadowRestoreFunc)` corresponds to
  the engine interface at original global `0x2433F69C`,
  virtual slot 116 (byte offset 0x1D0), with
  callback function pointer `0x24115E70`.

The original 2006 binary and Source SDK 2004 together establish
the *intent* of each step and exact original ABI positions. This
supports porting the **named dependency methods** next instead of
inventing VGUI/gameplay APIs. It still does not imply our independent
module has registered the callback, initialized the shadow texture
or reconstructed the internal render-to-texture graph.
