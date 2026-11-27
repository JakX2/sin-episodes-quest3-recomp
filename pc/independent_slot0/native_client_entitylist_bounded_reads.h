// Phase 1657: original-free bounded nine-method IClientEntityList diagnostic.
//
// Original SiN 2006 PE32 evidence: original secondary interface vtable VA
// 0x24297390 has nine entries (Phases 229, 230, 1656). This independent
// host-owned model deliberately does NOT register VClientEntityList003 in
// replacement client.dll, make engine calls, or borrow retail entity memory.
// All inputs are caller-owned synthetic values. It cannot prove gameplay boot.
//
// Method order and reads from original client.dll:
//  [0] 241056F0: [secondary_this + 0x14 + index*8] mirror pointer, RET 4
//  [1] 24105BB0: slot2(handle) -> unknown vcall +0x10, RET 4
//  [2] 241074D0: handle serial/index raw slot table, RET 4
//  [3] 24105AF0: index raw slot table -> unknown vcall +0x18, RET 4
//  [4] 24105BE0: slot2(handle) -> unknown vcall +0x18, RET 4
//  [5] 24105700: live count [+0x4] + optional [+0xC], RET 4
//  [6] 24105740: highest index [+0x10], plain RET
//  [7] 24105720: max entity setter [+0x8], RET 4
//  [8] 24105730: max entity getter [+0x8], plain RET
// The secondary_this owner offset is +0x10014, but this model does not
// fabricate or expose the original large CClientEntityList owner layout.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1657 diagnostic contract requires MSVC native Windows x86
#endif

namespace sin_independent {
namespace client_entitylist_bounded {

constexpr unsigned kNativeRawSlots=4096u;
constexpr unsigned kNativeMirrorSlots=4096u;
constexpr unsigned kNetworkedLiveLimit=2048u;
constexpr std::uint32_t kEhandleIndexMask=0xFFFu;
constexpr unsigned kEhandleSerialShift=12u;
// Original Phase248 base-slot constructor selects serials in 0..0x7FFF;
// the EHandle method nevertheless compares all upper 20 handle bits.
constexpr std::uint32_t kOriginalSerialGenerationMask=0x7FFFu;
constexpr std::uint32_t kInvalidHandle=0xFFFFFFFFu;

// Never retain engine-owned pointers here. The callbacks accept only
// owner-provided synthetic pointer tokens. A null adapter rejects the
// conversion; no original IClientUnknown method is ever invoked.
struct UnknownCallbacks {
    void* context=nullptr;
    void* (*networkable)(void*,void*) noexcept=nullptr;
    void* (*client_entity)(void*,void*) noexcept=nullptr;
};
struct RawSlot {
    void* unknown=nullptr;
    std::uint32_t serial=0u;
};
struct MirrorSlot {
    void* networkable=nullptr;
    std::uint32_t auxiliary=0u;
};
static_assert(sizeof(RawSlot)==8u,"synthetic PE32 raw slot width");
static_assert(sizeof(MirrorSlot)==8u,"synthetic PE32 mirror slot width");

// The real original raw table uses 16-byte slots, whose 0/4 DWORDs
// are (unknown,serial); the independent view intentionally carries
// these two fields only, never synthesizes the remaining fields.
struct State {
    std::array<RawSlot,kNativeRawSlots> raw{};
    std::array<MirrorSlot,kNativeMirrorSlots> mirrors{};
    std::int32_t networked_count=0;
    std::int32_t secondary_flagged_count=0;
    std::int32_t highest_live_index=-1;
    std::int32_t max_entities=0;
    UnknownCallbacks callbacks{};
};

// No virtual destructor: original secondary vtable slot 0 is an
// indexed one-argument lookup, NOT a deleting destructor.
class NineMethodInterface {
public:
    virtual void* __thiscall GetClientNetworkable(int) noexcept=0;
    virtual void* __thiscall GetClientNetworkableFromHandle(
        std::uint32_t) noexcept=0;
    virtual void* __thiscall GetClientUnknownFromHandle(
        std::uint32_t) noexcept=0;
    virtual void* __thiscall GetClientEntity(int) noexcept=0;
    virtual void* __thiscall GetClientEntityFromHandle(
        std::uint32_t) noexcept=0;
    virtual int __thiscall NumberOfEntities(bool) noexcept=0;
    virtual int __thiscall GetHighestEntityIndex() noexcept=0;
    virtual void __thiscall SetMaxEntities(int) noexcept=0;
    virtual int __thiscall GetMaxEntities() noexcept=0;
protected:
    ~NineMethodInterface() = default;
};

// State and callbacks must outlive this owned diagnostic reader. It is
// purposely not a CClientEntityList and cannot be returned as game ABI.
class BoundedReader final : public NineMethodInterface {
public:
    explicit BoundedReader(State& state) noexcept : state_(state) {}

    void* __thiscall GetClientNetworkable(int index) noexcept override {
        if(index<0 || static_cast<unsigned>(index)>=kNativeMirrorSlots)
            return nullptr; // original does NOT bounds-check: safe deviation
        return state_.mirrors[static_cast<unsigned>(index)].networkable;
    }
    void* __thiscall GetClientNetworkableFromHandle(
        std::uint32_t handle) noexcept override {
        void* unknown=GetClientUnknownFromHandle(handle);
        return (unknown && state_.callbacks.networkable)
            ? state_.callbacks.networkable(state_.callbacks.context,unknown)
            : nullptr;
    }
    void* __thiscall GetClientUnknownFromHandle(
        std::uint32_t handle) noexcept override {
        if(handle==kInvalidHandle)return nullptr;
        const auto index=handle & kEhandleIndexMask;
        // Exact unsigned 20-bit serial comparison as original 241074D0;
        // bounded array membership makes the raw pointer math safe.
        const auto& slot=state_.raw[index];
        return slot.serial==(handle>>kEhandleSerialShift)
            ? slot.unknown : nullptr;
    }
    void* __thiscall GetClientEntity(int index) noexcept override {
        if(index<0 || static_cast<unsigned>(index)>=kNativeRawSlots)
            return nullptr; // do not reproduce original unchecked indexing
        void* unknown=state_.raw[static_cast<unsigned>(index)].unknown;
        return (unknown && state_.callbacks.client_entity)
            ? state_.callbacks.client_entity(state_.callbacks.context,unknown)
            : nullptr;
    }
    void* __thiscall GetClientEntityFromHandle(
        std::uint32_t handle) noexcept override {
        void* unknown=GetClientUnknownFromHandle(handle);
        return (unknown && state_.callbacks.client_entity)
            ? state_.callbacks.client_entity(state_.callbacks.context,unknown)
            : nullptr;
    }
    int __thiscall NumberOfEntities(bool include_secondary) noexcept override {
        // Host safety: count metadata is trusted only if within the
        // synthetic host's capacities. Original just adds two DWORDs.
        if(state_.networked_count<0 ||
           state_.networked_count>static_cast<int>(kNetworkedLiveLimit) ||
           state_.secondary_flagged_count<0 ||
           state_.secondary_flagged_count>static_cast<int>(kNativeRawSlots))
            return 0;
        return state_.networked_count+
               (include_secondary?state_.secondary_flagged_count:0);
    }
    int __thiscall GetHighestEntityIndex() noexcept override {
        // Original returns a DWORD. The host can represent -1 for empty.
        const int hi=state_.highest_live_index;
        return hi>=-1 && hi<static_cast<int>(kNetworkedLiveLimit) ? hi:-1;
    }
    void __thiscall SetMaxEntities(int maximum) noexcept override {
        // Host safety: unlike the original raw DWORD write, refuse
        // invalid limits rather than letting them corrupt downstream data.
        if(maximum>=0 && maximum<=static_cast<int>(kNativeRawSlots))
            state_.max_entities=maximum;
    }
    int __thiscall GetMaxEntities() noexcept override {
        return state_.max_entities;
    }

private:
    State& state_;
};
static_assert(sizeof(void*)==4u,"original 2006 PE32 pointer width");
static_assert(!std::has_virtual_destructor<NineMethodInterface>::value,
              "no deleting-destructor in secondary nine-slot vtable");
static_assert(!std::has_virtual_destructor<BoundedReader>::value,
              "do not insert an unexpected deleting destructor");

} // namespace client_entitylist_bounded
} // namespace sin_independent
