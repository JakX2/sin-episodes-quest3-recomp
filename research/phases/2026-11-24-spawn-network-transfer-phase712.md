# Phase 712: paired network callbacks on class-created client entities

Phase711 created the first class-record-to-client-registry spawn path.
This regression extends that path using the existing actual server
and client DLL aggregate APIs, rather than standalone client buffers.

For each of five created entities, follow its class record to matching
server/client tables and execute native scalar and nested callbacks
against the actual client allocation returned by the creator. Player:
accuracy, scope, poison, breath, flash time/alpha, health, all 32 ammo
values through owner-local nested tables. Hand: inherited team field.
Each of three weapons: descend class inheritance to BaseCombatWeapon;
owner-gated local and active tables, two clip values (including -1),
ammo type, next attack, state and live owner handle. Mutate server owner
serial to prove local recipient refusal and invalid public owner handle.
Check creator-assigned index and registry handle remain unchanged after
transfers, then perform handle-matched session release.

Owner, engine, collision and constructor services remain explicit native
fixtures. Weapon owner is a server fixture; no simultaneous live client
player/weapon relationship claimed. These are raw property proxy
transfers with recipient callback checks, not an engine bitstream or
retail gameplay test. This phase changes tests only. No physics/model
implementation or original binary bytes.
