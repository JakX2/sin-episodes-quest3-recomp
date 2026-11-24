# Phase 672: BSP-to-native map entity host composition

Phase146 SourceMapRuntime and Phase671's loaded native server module
now have a concrete adapter, NativeBehaviorHost. The underlying host
must provide actual engine-owned shell entities, base keyvalues and
base spawning. For env_global and info_challenge, the adapter attaches
the DLL-owned behavior to the host identity. Other classnames are
forwarded to the underlying host, never substituted. Only supported
class-specific properties are consumed; targetname and output
connection strings remain the host's responsibility. Named inputs
route to the same native behavior. The adapter detaches native state
after the host removes its entity; live refused removals remain
retryable. Level teardown must follow runtime unload. Engine-side
unsolicited destruction needs explicit future reconciliation.

The actual loader test now compiles the existing BSP entity reader
and SourceMapRuntime into the separate consumer executable. A
synthetic VBSP v19 lump containing worldspawn, env_global and
info_challenge reaches the DLL through the adapter. It checks ordered
duplicate keyvalues, base key/output preservation, spawn, activation,
challenge output, explicit subsequent host input, persistent global
state, unload retry and rollback on bad properties/unsupported
class. SourceMapRuntime requires entities live after spawn; null-name
env_global RemoveSelf explicitly fails this ordinary loader path and
rolls back shells rather than pretending successful removal.
Global writes already performed before a later spawn failure are
not transactional. A real Source-specific host must handle that.

This is integration against a synthetic engine-shell host, not a
real Source engine entity implementation or complete BSP load.
No geometry, model, physics, or actual engine event queue work.
No original retail assets included. Windows and Linux test paths
exercise the same actual native module plus portable host adapter.
