# Independent C_StatsSystem whole-prefix coordinator

The original 2006 client `C_StatsSystem::Init` at 0x241FCAA0 has
seven SHA-verified calls (see `research/tools/audit_sin_client_stats_system_init.py`):
static-stat storage, dynamic-stat storage, owner catalog setup,
display layout, then StaticStat, DynamicStat and ResetHUD registry calls.

The earlier phases recovered three **partial** helper behaviors:
- 55 x 8-byte static entry records, with a 16-case definition type factory;
- 11 x 20-byte dynamically zeroed records;
- 66 x 12-byte owner catalog entries initialized to {0,0,-1}.

`client_stats_coordinator.h` is the first independent wrapper that
**owns all three collections at once**, runs them in authentic native
callsite order, and requires injected original-resource and interface
implementations for every still-unrecovered action. It does not invent
the original resource enumerator, CSV/layout parser or networking
callback registration. The attempted/completed counters distinguish
the exact refusal boundary. Earlier completed allocations remain
owned and retried from zero, with no fabricated rollback.

Native MSVC PE32 fixture `test_client_stats_coordinator.cpp` checks
full successful callback order, exact contexts and 7 callsites,
single-slot allocation failures across all three collections,
missing resource provider, provider refusal, later callback refusal,
retry/reset and the outer AutoGameSystem failure gate.

**Important:** in the original DLL, the outer Stats::Init returns
true after calling its helpers without checking each helper's result.
The new coordinator's fail-closed stop is a deliberate safe design
difference. It has not executed retail C_StatsSystem code and has not
made `VClient011::Init` return successfully. The real owner-verified
Windows checkpoint remains 0x53494E63.
