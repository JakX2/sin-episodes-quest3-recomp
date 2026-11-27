# Phase 1596: Recombinant Minor task overrides, first native dispatch

Direct inspection authenticated server.dll SHA256 568a0086fde607345adf31d7cfb2a5bf4e4efe9a86f416faa97076ddf9d97650. Start +444 2237cb20 byte table 2237ce0c, dword 2237cdec yields 13,67,131,140,141,142,143. Run +44c 2237ce90 byte table 2237d000, dword 2237cfe8 yields 67,95,121,140,142. Native routing implements six Start and five Run overrides; Start140 is explicitly unrecovered, never falsely delegated. Start13 checks enemy twice, faces center through motor with 0 then parent; 67 invokes shared 220325c0(5,0); 131 sets movement(3,0) then parent; 141 resolves target +8a8, checks +620, obtains origin and constructs goal via 2204c660, invokes +6f8 goal adjustment, submits navigator mode2, completes or fails11 (diagnostic only after submit rejection); 142 assigns live private activity; 143 sets path arrival activity then completes. Run95 resets aim then parent; 121 checks ideal85, checks/reacquires enemy, faces origin via motor(-1) then parent; 140 completes on ground else faces origin+velocity, refreshes dirty velocity; 142 completes if +298 or grounded or live ground handle; 67 completes on grounded/live ground, waits until actor+9dc, then support probe 2237b550 determines completion or shared leap retry. Source motor/nav/collision/leap internals remain owner-bound. No generic physics implementation. Uses native lifecycle for condition33 and completion. Exact task route tests and explicit unsupported branch test.

Original VA/bytes/SHA256:
2237cb20/879/8413384e7ca28eabe049a494c8eada32210431fcafc4ffe43755a29d5ab6ca14
2237ce90/444/2be5ab8e342ee2dd3356355e6a1e7c6da36fc48ce2deefea1066b96771dca98e
2237c340/24/ef4a34ef68ecef2b1afee3995aa085cd988226330a2b5f47177fe382f73c1bd7
2204c660/80/c7d0fb8f10085edd43342659c5433a35c4c651c9e71272f6246d68a38ce80ffc

Start140 needs deeper trace of 2237c8c0 and stack vector construction; no invented fallback. No original executable bytes committed. CI pending.
