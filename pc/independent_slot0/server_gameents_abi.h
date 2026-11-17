#pragma once
// Independent declaration of the 2004 Source IServerGameEnts x86 ABI.
//
// Provenance: the owner's SHA256-pinned original SiN server.dll contains
// exactly seven slots for ServerGameEnts001. Names, pointer types, and their
// order agree with source-sdk-2004-master/src_mod/public/eiface.h.
// This abstract contract is NOT a game entity implementation and MUST NOT
// be returned by CreateInterface until real method behavior is recovered.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error This interface boundary is validated only for native MSVC x86
#endif

struct edict_t;
class CBaseEntity;
struct CCheckTransmitInfo;

namespace sin_independent {

class ServerGameEntsAbi {
public:
    // MSVC x86 emits a one-entry scalar deleting destructor in the vtable.
    virtual ~ServerGameEntsAbi() noexcept = default;

    virtual void __thiscall SetDebugEdictBase(edict_t* base) noexcept = 0;
    virtual void __thiscall MarkEntitiesAsTouching(
        edict_t* first, edict_t* second) noexcept = 0;
    virtual void __thiscall FreeContainingEntity(edict_t* entity) noexcept = 0;
    virtual edict_t* __thiscall BaseEntityToEdict(
        CBaseEntity* entity) noexcept = 0;
    virtual CBaseEntity* __thiscall EdictToBaseEntity(
        edict_t* entity) noexcept = 0;
    virtual void __thiscall CheckTransmit(
        CCheckTransmitInfo* info, const unsigned short* indices,
        int count) noexcept = 0;
};

} // namespace sin_independent
