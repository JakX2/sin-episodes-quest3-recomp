# Phase 674: loaded native trigger host composition

Fix baseline Phase673 standalone native targets to inherit the existing
third_party/research include root (the root CMake target already uses it).
Phase674 extends Phase672 NativeBehaviorHost using Phase529's recovered
host contract, but delegates to the *loaded DLL's* CampaignTriggerApi,
not a second OriginalTriggerEntityFactory.

The shell factory must create a genuine engine class. Both trigger
classnames require an explicitly supplied API, observed field provider
and native callback table. All native trigger keyvalues are forwarded
in order to the shell, then class-specific behavior observes them.
The observed flags and origin are never inferred from raw BSP keys.
The trigger callback is the only native base Spawn path. Ordinary
SourceMapRuntime refuses suppressed/self-removing spawn instead of
activating a dead entity. A real Source specialized loader must handle
that path and invalidate any queued deletion of a rolled-back shell.

Touch and retry dispatch through the same DLL-owned map registry and
fixed campaign. An explicit notify_deleted hook detaches behavior after
real deferred engine deletion, outside the DLL callback. The ordinary
loader skips dead engine handles; real hosts MUST send this notification
before reusing a slot. No callback or engine pointer is retained by DLL.

Actual loader regression builds a synthetic BSP containing worldspawn,
trigger_dynamic_difficulty and trigger_secret. It checks ordered shell
keys, single base spawn, independently observed flags, event descriptor
11/0/12, nonplayer, missing snapshot, pending removal/retry, native
global campaign completion, and deferred deletion reconciliation.
Missing trigger services and suppressed spawn fail closed. Existing
ordinary env_global/info_challenge paths remain covered.

This is executable host composition, not actual Source class allocation,
datamap registration, collision or game event queue integration.
No retail startup or playable DLL claim.
