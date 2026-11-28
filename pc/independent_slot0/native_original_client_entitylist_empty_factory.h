// Phase1689: original-guided, production PE32 client.dll entity-list
// EMPTY-state factory, exposing precisely the original interface's
// nine native secondary slots and separate three native parent slots.
//
// This is not a fabricated live entity registry. The original source
// registers entities through CClientEntityList PRIMARY virtual slot0
// (RET8) and unregisters through parent slot1 (RET8), each delegating
// to still-incomplete entity/handle helpers. Both are FATAL guards
// here. No entity is ever inserted; all original-shaped slots are
// empty and all reads observe that truthful empty state.
//
// Original owner: client.dll VA 0x2433FDC8. Factory thunk
// 0x241056B0 returns parent+0x10014 = 0x2434FDDC.
// Parent 3-method vtable 0x242973B8; embedded 9-method
// VClientEntityList003 vtable 0x24297390, whole-method original
// evidence Phases229–231, and RTTI offset Phase1656.
// Parent+4 has 4096 16-byte raw slots; secondary+0x14 has
// 4096 8-byte mirror slots. Signed live count guard is 2048,
// mirror storage still spans all 4096 slots (Phase1657).
//
// Source-authenticated complete 9-slot ABI:
// 0 GetClientNetworkable(int) RET4
// 1 GetClientNetworkableFromHandle(uint32) RET4
// 2 GetClientUnknownFromHandle(uint32) RET4
// 3 GetClientEntity(int) RET4
// 4 GetClientEntityFromHandle(uint32) RET4
// 5 NumberOfEntities(bool) RET4
// 6 GetHighestEntityIndex() RET0
// 7 SetMaxEntities(int) RET4
// 8 GetMaxEntities() RET0
//
// All unimplemented entity OWNERSHIP requests (parent slots0/1,
// unexpected nonempty records), bad engine index limits, and any
// post-revocation calls terminate through the production refusal
// callback. The factory is never enabled in an ordinary run.
//
// No original commercial DLL code, binary data, or asset is copied.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1689 source PE32 native entitylist ABI requires MSVC x86
#endif

namespace sin_independent::native_original_client_entitylist_factory {
constexpr char kInterfaceName[]="VClientEntityList003";
constexpr unsigned kSecondaryVirtualCount=9u;
constexpr unsigned kParentVirtualCount=3u;
constexpr unsigned kRawCount=4096u;
constexpr unsigned kMirrorCount=4096u;
constexpr unsigned kNetworkedLiveLimit=2048u;
constexpr unsigned kOriginalSecondaryOffset=0x10014u;
constexpr unsigned kOriginalRawOffset=4u;
constexpr unsigned kOriginalRawStride=16u;
constexpr unsigned kOriginalMirrorOffsetFromSecondary=0x14u;
constexpr unsigned kOriginalMirrorStride=8u;
constexpr unsigned kOriginalSecondaryNetworkedCount=0x04u;
constexpr unsigned kOriginalSecondaryMaxEntities=0x08u;
constexpr unsigned kOriginalSecondaryFlaggedCount=0x0Cu;
constexpr unsigned kOriginalSecondaryHighestIndex=0x10u;
constexpr std::uint32_t kInvalidEhandle=0xFFFFFFFFu;
constexpr std::uint32_t kOriginalEhandleIndexMask=0xFFFu;
constexpr unsigned kOriginalEhandleSerialShift=12u;
using RefuseCallback=void (*)(const char*) noexcept;

struct RawRecord {
    void* unknown=nullptr;
    std::uint32_t serial=0u;
    RawRecord* intrusive_next=nullptr; // original constructor [slot+8]
    RawRecord* intrusive_prev=nullptr; // original constructor [slot+12]
};
struct MirrorRecord {
    void* networkable=nullptr;
    std::uint32_t opaque_aux=0u;
};
static_assert(sizeof(void*)==4u && sizeof(RawRecord)==16u &&
              sizeof(MirrorRecord)==8u,
              "PE32 source raw/mirror table strides changed");

class Secondary final {
public:
    explicit Secondary(RawRecord* raw,RefuseCallback refuse) noexcept:
        raw_(raw),refuse_(refuse) {}

    // Original slot0 reads mirror[index].networkable at +0x14
    // WITHOUT source bounds checking; deliberately reject unsafe
    // indices. This empty-only owner has no insertion path.
    virtual void* __thiscall GetClientNetworkable(int index) noexcept {
        MustBeActive();
        if(index<0 || static_cast<unsigned>(index)>=kMirrorCount)
            return nullptr;
        const void* const value=mirrors_[static_cast<unsigned>(index)].networkable;
        if(value)Refuse("VClientEntityList003.nonempty-networkable-unmodeled");
        return nullptr;
    }
    virtual void* __thiscall GetClientNetworkableFromHandle(
            std::uint32_t handle) noexcept {
        return GetClientUnknownFromHandle(handle)
            ? RefusePointer("VClientEntityList003.unknown-networkable-unmodeled")
            : nullptr;
    }
    virtual void* __thiscall GetClientUnknownFromHandle(
            std::uint32_t handle) noexcept {
        MustBeActive();
        if(handle==kInvalidEhandle)return nullptr;
        const auto index=handle & kOriginalEhandleIndexMask;
        if(!raw_)Refuse("VClientEntityList003.raw-table-unbound");
        const auto& record=raw_[index];
        if(record.serial!=(handle>>kOriginalEhandleSerialShift))
            return nullptr;
        if(record.unknown)
            Refuse("VClientEntityList003.nonempty-raw-owner-unmodeled");
        return nullptr;
    }
    virtual void* __thiscall GetClientEntity(int index) noexcept {
        MustBeActive();
        if(index<0 || static_cast<unsigned>(index)>=kRawCount)
            return nullptr;
        if(raw_[static_cast<unsigned>(index)].unknown)
            Refuse("VClientEntityList003.nonempty-raw-owner-unmodeled");
        return nullptr;
    }
    virtual void* __thiscall GetClientEntityFromHandle(
            std::uint32_t handle) noexcept {
        return GetClientUnknownFromHandle(handle)
            ? RefusePointer("VClientEntityList003.unknown-entity-unmodeled")
            : nullptr;
    }
    virtual int __thiscall NumberOfEntities(
            bool include_secondary) noexcept {
        MustBeActive();
        if(networked_!=0 || flagged_!=0)
            Refuse("VClientEntityList003.nonempty-count-unmodeled");
        // Source special case for include_secondary==1; both
        // terms remain zero because registration is unimplemented.
        return networked_+(include_secondary?flagged_:0);
    }
    virtual int __thiscall GetHighestEntityIndex() noexcept {
        MustBeActive();
        if(highest_!=-1)
            Refuse("VClientEntityList003.nonempty-highwater-unmodeled");
        return highest_;
    }
    virtual void __thiscall SetMaxEntities(int value) noexcept {
        MustBeActive();
        // Source directly stores DWORD, including values that
        // would exceed the 4096-entry data arrays. Fail closed
        // on those rather than introduce later out-of-bounds.
        if(value<0 || value>static_cast<int>(kRawCount))
            Refuse("VClientEntityList003.unsafe-max-entities");
        max_entities_=value;
    }
    virtual int __thiscall GetMaxEntities() noexcept {
        MustBeActive();
        return max_entities_;
    }
    void BindEmpty() noexcept {
        mirrors_.fill(MirrorRecord{});
        networked_=flagged_=0;
        highest_=-1;
        max_entities_=0;
        active_=true;
    }
    void Revoke() noexcept {active_=false;}
    bool Active() const noexcept {return active_;}

private:
    [[noreturn]] void Refuse(const char* label) noexcept {
        if(refuse_)refuse_(label);
        std::abort();
    }
    void* RefusePointer(const char* label) noexcept {
        Refuse(label);
    }
    void MustBeActive() noexcept {
        if(!active_)Refuse("VClientEntityList003.unbound-or-revoked");
    }
    // Exact original source offsets are enforced by the native
    // LoadLibrary integration test against the actual MSVC vptr.
    std::int32_t networked_=0;
    std::int32_t max_entities_=0;
    std::int32_t flagged_=0;
    std::int32_t highest_=-1;
    std::array<MirrorRecord,kMirrorCount> mirrors_{};
    // Host-only metadata follows the entire source-visible
    // mirror table, never interrupting original data offsets.
    RawRecord* raw_=nullptr;
    RefuseCallback refuse_=nullptr;
    bool active_=false;
};

// Actual owner PRIMARY 3 virtuals, with original ABI signatures.
// Original parent slots0/1 are the ONLY insertion/removal path.
// These are deliberately fatal until source entity ownership,
// serial generations and callback graph can be integrated.
// The primary virtual deleting destructor is also fatal because
// the owner is retained for the whole engine diagnostic session.
class Owner final {
public:
    explicit Owner(RefuseCallback refuse) noexcept:
        secondary_(raw_.data(),refuse),refuse_(refuse) {}

    virtual void __thiscall RegisterEntity(
        void*,std::uint32_t) noexcept {
        Refuse("CClientEntityList.primary-register-unsupported");
    }
    virtual void __thiscall UnregisterEntity(
        void*,std::uint32_t) noexcept {
        Refuse("CClientEntityList.primary-unregister-unsupported");
    }
    virtual ~Owner() noexcept {
        Refuse("CClientEntityList.primary-destructor-unsupported");
    }

    Secondary* BindEmpty() noexcept {
        // Original base ctor at 0x24127E80 loops all 4096
        // raw entries, writing each record's own address into
        // its +0x08/+0x0C intrusive links before publishing
        // the owner. Its serial generation calls original
        // vstdlib RandomInt then masks 0x7FFF. We intentionally
        // use deterministic zero serials and do NOT claim the
        // original linked free-list heads, random state, or
        // gameplay entity allocation are reconstructed.
        for(auto& slot:raw_) {
            slot.unknown=nullptr;
            slot.serial=0u; // host-only deterministic value
            slot.intrusive_next=&slot;
            slot.intrusive_prev=&slot;
        }
        secondary_.BindEmpty();
        return &secondary_;
    }
    void Revoke() noexcept {secondary_.Revoke();}
    Secondary* Interface() noexcept {return &secondary_;}
    bool Active() const noexcept {return secondary_.Active();}
    void* RawAddress() noexcept {return raw_.data();}

private:
    [[noreturn]] void Refuse(const char* label) noexcept {
        if(refuse_)refuse_(label);
        std::abort();
    }
    // MSVC native primary vfptr at owner+0, 4096x16-byte
    // base record entries start at owner+4.
    std::array<RawRecord,kRawCount> raw_{};
    // Exactly 16 source-owned bytes between end of raw slots
    // (owner+0x10004) and embedded secondary vfptr at +0x10014.
    std::array<std::uint32_t,4> original_gap_{};
    Secondary secondary_;
    RefuseCallback refuse_=nullptr;
};
} // namespace sin_independent::native_original_client_entitylist_factory
