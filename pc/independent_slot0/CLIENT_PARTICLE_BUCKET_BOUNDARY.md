# Client particle bucket ownership boundary (manual follow-on to Phase 348)

Phase 348 commit 1d77de3f81e3ab37a6c6642465a634fa5936e2dc passed all four CI workflows. The real engine has not yet run the Phase 348 three-variable diagnostic.

The earlier SHA-pinned subsystem audit `research/tools/audit_sin_subsystem_startup.py` and `research/phases/phase180-original-client-app-physics-postinit.md` are distinct from the particle-trail RecvTable work. The original startup evidence identifies the following:
- After the deferred object initializer succeeds, the client particle startup is called before VGUI.
- The original manager resets before allocating a 48-byte bucket object.
- The bucket constructor is passed the numeric argument 96 and label CParticleMgr::m_pParticleBucket; the resulting pointer is stored at manager offset +0x84.
- The constructor argument alone does not establish the backing pool capacity or every particle field. The reported 2048 record capacity is not independently verified in this change.

This change models the known order using an independently owned backend boundary. It tests success, allocation failure, construction failure, repeated starts, explicit teardown, destructor teardown and callback reentry. The cleanup, retries and callback contracts are independent safety design choices, not authenticated original teardown semantics. The backend intentionally does not call the original binary or engine addresses. It is not attached to the distributed DLL and makes no new retail startup or rendering claim.

To integrate it, first establish the allocator/constructor ownership contract, the manager reset behavior, the backing allocation and the VGUI prerequisites. Run the Phase 348 real-game diagnostic first. No new original function coverage is counted here.
