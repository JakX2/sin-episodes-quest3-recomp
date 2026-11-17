#pragma once
// Owner-original guided, independently authored x86 entity-conversion core.
//
// These are the first three ServerGameEnts001 behaviors which can be
// described and tested without bootstrapping SiN's complete entity manager.
// They are NOT exported through server.dll's CreateInterface yet.
//
// Evidence: original SHA256-pinned SiN server.dll + Source SDK 2004 public
// interfaces. The offsets below are specific to the authenticated x86 game
// and MUST NOT be reused for an Android/ARM64 runtime.
#include "server_gameents_abi.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original server entity conversion core is currently MSVC x86 only
#endif

class CBaseHandle;
struct ICollideable;
struct IServerNetworkable;

namespace sin_independent {

// This is an independent ABI-level declaration of the first six virtual
// methods of the Source IServerUnknown inheritance chain. The original
// EdictToBaseEntity dispatches virtual slot 5 through the edict's unknown
// pointer. No member functions beyond this contract are assumed here.
class ServerUnknownAbi {
public:
    virtual ~ServerUnknownAbi() noexcept = default;
    virtual void __thiscall SetRefEHandle(const CBaseHandle&) noexcept = 0;
    virtual const CBaseHandle& __thiscall GetRefEHandle() const noexcept = 0;
    virtual ICollideable* __thiscall GetCollideable() noexcept = 0;
    virtual IServerNetworkable* __thiscall GetNetworkable() noexcept = 0;
    virtual CBaseEntity* __thiscall GetBaseEntity() noexcept = 0;
};

constexpr std::size_t kOriginalEntityEdictOffset = 0x16c;
constexpr std::size_t kOriginalEdictUnknownOffset = 0x0c;

// The original SetDebugEdictBase writes a pointer to global game state.
// This core keeps its pointer in independent state so synthetic tests can
// verify the exact behavior without changing the original engine process.
class ServerGameEntsConversionCore final {
public:
    void SetDebugEdictBase(edict_t* base) noexcept {
        debugEdictBase_ = base;
    }

    edict_t* DebugEdictBase() const noexcept {
        return debugEdictBase_;
    }

    edict_t* BaseEntityToEdict(CBaseEntity* entity) const noexcept {
        if (!entity) return nullptr;
        // Authenticated original method reads the edict pointer at +0x16c.
        // The complete CBaseEntity object layout is not reconstructed yet.
        // Call only with a deliberately provisioned x86 fixture.
        edict_t* result = nullptr;
        const auto* source = reinterpret_cast<const unsigned char*>(entity);
        std::memcpy(&result, source + kOriginalEntityEdictOffset,
                    sizeof(result));
        return result;
    }

    CBaseEntity* EdictToBaseEntity(edict_t* edict) const noexcept {
        if (!edict) return nullptr;
        // edict_t's unknown interface pointer lives at +0x0c in the
        // original x86 class. Its GetBaseEntity virtual call is slot 5.
        ServerUnknownAbi* unknown = nullptr;
        const auto* source = reinterpret_cast<const unsigned char*>(edict);
        std::memcpy(&unknown, source + kOriginalEdictUnknownOffset,
                    sizeof(unknown));
        return unknown ? unknown->GetBaseEntity() : nullptr;
    }

private:
    edict_t* debugEdictBase_ = nullptr;
};

static_assert(sizeof(void*) == 4, "Original server entity ABI is PE32");
static_assert(kOriginalEntityEdictOffset == 0x16c, "Owner-original entity offset");
static_assert(kOriginalEdictUnknownOffset == 0x0c, "Owner-original edict offset");

} // namespace sin_independent
