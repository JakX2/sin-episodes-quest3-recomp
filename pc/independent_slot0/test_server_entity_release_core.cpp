// Synthetic native x86 test for original FreeContainingEntity's early-return,
// detach and cleanup-call order. No original DLL is loaded or called.
#include "server_entity_release_core.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

class CBaseHandle { public: unsigned int value = 0; };
class CBaseEntity { public: unsigned int marker = 0x51A0BEEF; };
struct edict_t {
    std::uint32_t stateFlags = 0;
    std::uint32_t serialNumber = 0;
    IServerNetworkable* networkable = nullptr;
    sin_independent::ServerUnknownAbi* unknown = nullptr;
};
static_assert(offsetof(edict_t,unknown) == 0x0c,
              "Original edict unknown pointer at +0x0c");
static_assert(sizeof(edict_t) == 16, "Original x86 edict prefix size");

class SyntheticUnknown final : public sin_independent::ServerUnknownAbi {
public:
    explicit SyntheticUnknown(CBaseEntity* entity) : entity_(entity) {}
    void __thiscall SetRefEHandle(const CBaseHandle& handle) noexcept override {
        handle_ = handle;
    }
    const CBaseHandle& __thiscall GetRefEHandle() const noexcept override {
        return handle_;
    }
    ICollideable* __thiscall GetCollideable() noexcept override {
        return nullptr;
    }
    IServerNetworkable* __thiscall GetNetworkable() noexcept override {
        return nullptr;
    }
    CBaseEntity* __thiscall GetBaseEntity() noexcept override {
        ++calls_;
        return entity_;
    }
    int Calls() const noexcept { return calls_; }

private:
    CBaseEntity* entity_;
    CBaseHandle handle_;
    int calls_ = 0;
};

struct CleanupTrace {
    edict_t* edict = nullptr;
    CBaseEntity* expected = nullptr;
    IServerNetworkable* expectedNetworkable = nullptr;
    int order[3] = {};
    int count = 0;
    bool allDetachedBeforeHooks = true;
    bool sameEntity = true;
};

void __cdecl RemoveTouchedList(CBaseEntity* entity, void* context) noexcept;
void __cdecl RemoveGroundList(CBaseEntity* entity, void* context) noexcept;
void __cdecl RemoveImmediate(CBaseEntity* entity, void* context) noexcept;

static void RecordStage(int stage, CBaseEntity* entity, void* context) noexcept {
    auto* trace = static_cast<CleanupTrace*>(context);
    if (trace->count >= 3) {
        trace->sameEntity = false;
        return;
    }
    trace->order[trace->count++] = stage;
    if (entity != trace->expected)
        trace->sameEntity = false;
    if (!trace->edict || trace->edict->unknown != nullptr ||
        trace->edict->stateFlags != 0 ||
        trace->edict->networkable != trace->expectedNetworkable)
        trace->allDetachedBeforeHooks = false;
}
void __cdecl RemoveTouchedList(CBaseEntity* e, void* ctx) noexcept { RecordStage(1,e,ctx); }
void __cdecl RemoveGroundList(CBaseEntity* e, void* ctx) noexcept { RecordStage(2,e,ctx); }
void __cdecl RemoveImmediate(CBaseEntity* e, void* ctx) noexcept { RecordStage(3,e,ctx); }

static int fail(int code, const char* message) {
    std::fprintf(stderr, "FAIL original FreeContainingEntity %d: %s\n",
                 code,message);
    return code;
}

int main() {
    using namespace sin_independent;
    ServerGameEntsConversionCore conversions;
    CBaseEntity entity;
    SyntheticUnknown unknown(&entity);
    SyntheticUnknown empty(nullptr);
    edict_t edict{};
    edict.stateFlags = 0xCAFEBABE;
    edict.serialNumber = 91;
    auto* networkable = reinterpret_cast<IServerNetworkable*>(&entity);
    edict.networkable = networkable;
    edict.unknown = &unknown;
    CleanupTrace trace{&edict, &entity, networkable};
    EntityReleaseHooks hooks{&RemoveTouchedList,&RemoveGroundList,&RemoveImmediate,&trace};

    // Null edict is a strict no-op: no callbacks and no unknown dispatch.
    if (TryReleaseContainingEntity(conversions,nullptr,hooks)
        != EntityReleaseStatus::NoAssociatedEntity || trace.count != 0 ||
        unknown.Calls() != 0)
        return fail(1,"null edict");

    // Missing any required cleanup stage refuses before mutating state or
    // reading an unknown pointer, avoiding an unrecoverable partial release.
    EntityReleaseHooks missing1{nullptr,&RemoveGroundList,&RemoveImmediate,&trace};
    EntityReleaseHooks missing2{&RemoveTouchedList,nullptr,&RemoveImmediate,&trace};
    EntityReleaseHooks missing3{&RemoveTouchedList,&RemoveGroundList,nullptr,&trace};
    for (auto* missing : {&missing1,&missing2,&missing3}) {
        if (TryReleaseContainingEntity(conversions,&edict,*missing)
            != EntityReleaseStatus::MissingCleanupHooks ||
            edict.stateFlags != 0xCAFEBABE ||
            edict.unknown != &unknown || unknown.Calls() != 0 ||
            trace.count != 0)
            return fail(2,"missing hooks must be non-mutating");
    }

    // Original method ignores edicts lacking unknown or base-entity owner.
    edict.unknown = nullptr;
    if (TryReleaseContainingEntity(conversions,&edict,hooks)
        != EntityReleaseStatus::NoAssociatedEntity ||
        edict.stateFlags != 0xCAFEBABE || trace.count != 0)
        return fail(3,"missing edict unknown");
    edict.unknown = &empty;
    if (TryReleaseContainingEntity(conversions,&edict,hooks)
        != EntityReleaseStatus::NoAssociatedEntity ||
        edict.stateFlags != 0xCAFEBABE || trace.count != 0 ||
        empty.Calls() != 1)
        return fail(4,"missing base entity");

    // Original game/SDK ordering: remove touched-list links, ground-list
    // links, then remove entity immediately. These are injected probes,
    // not implementation of those actual game subsystems.
    // First successful release clears unknown + flags before any callback;
    // all stages receive the SAME base pointer, exactly in A/B/C order.
    edict.unknown = &unknown;
    if (TryReleaseContainingEntity(conversions,&edict,hooks)
        != EntityReleaseStatus::DetachedAndHooksInvoked)
        return fail(5,"actual detach");
    if (unknown.Calls() != 1 || trace.count != 3 ||
        trace.order[0] != 1 || trace.order[1] != 2 ||
        trace.order[2] != 3 || !trace.sameEntity ||
        !trace.allDetachedBeforeHooks ||
        edict.unknown != nullptr || edict.stateFlags != 0 ||
        edict.serialNumber != 91 || edict.networkable != networkable)
        return fail(6,"original detach and cleanup-call order");

    // Following the original null guard, repeated release cannot call the
    // helper chain twice once unknown has been cleared.
    if (TryReleaseContainingEntity(conversions,&edict,hooks)
        != EntityReleaseStatus::NoAssociatedEntity ||
        trace.count != 3 || unknown.Calls() != 1)
        return fail(7,"idempotent empty-edict guard");

    std::puts("PASS isolated Win32 x86 FreeContainingEntity: no-op guards, strict callbacks, detach before stages, three-stage order");
    return 0;
}
