#pragma once
// Phase169: native 2006 SiN server.dll entity angle update at 0x221439C0
// and three original x87 vector helper routines. Host neutral, not MSVC ABI.
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace sin_compat::original_server_entity_angles {
struct Angle3 { float xyz[3]{}; };
static_assert(sizeof(Angle3)==12, "original three-DWORD vector");
static_assert(std::is_trivially_copyable<Angle3>::value, "raw angle copy");

inline void copy_native_angles(Angle3& out,const Angle3& in) noexcept {
    std::memcpy(&out,&in,sizeof(Angle3));
}
// Original 0x220E2E70, 60 bytes: x87 equal/NaN/+0 vs -0.
inline bool original_angles_equal(const Angle3& a,const Angle3& b) noexcept {
    return a.xyz[0]==b.xyz[0] && a.xyz[1]==b.xyz[1] && a.xyz[2]==b.xyz[2];
}
// Original 0x2213A360, 91 bytes: compare-only, returns 1 if different.
// Original reads the proposed vector into its own 12-byte stack snapshot.
inline bool original_angles_different(const Angle3& a,const Angle3& b) noexcept {
    return !original_angles_equal(a,b);
}
struct ChangeNotification {
    std::uint8_t mode_byte_1f0=0;
    std::uint8_t dirty_byte_200=0;
    std::uint32_t* external_flags_16c=nullptr;
};
// Original 0x22140040, 99 bytes, ECX=vector at entity+0x36C.
// Mode from vector-0x17C, dirty byte at vector-0x16C,
// event pointer at vector-0x200. Changed-only raw three-DWORD store.
inline bool original_assign_tracked_angles(Angle3& current,
                                          const Angle3& proposed,
                                          ChangeNotification& notify) noexcept {
    if(original_angles_equal(current,proposed))return false;
    if(notify.mode_byte_1f0!=0)notify.dirty_byte_200=1;
    else if(notify.external_flags_16c!=nullptr)
        *notify.external_flags_16c|=1u;
    copy_native_angles(current,proposed);
    return true;
}
struct ParentAngles {
    Angle3 absolute_angles_354{};
    std::uint32_t flags_b8=0;
};
struct EntityAngles {
    Angle3 absolute_angles_354{};
    Angle3 local_angles_36c{};
    std::uint8_t flags_b9=0;
    std::uint32_t parent_handle_f0=0xffffffffu;
    ChangeNotification notify{};
};
inline constexpr std::uint32_t kInvalidParentHandle=0xffffffffu;
inline constexpr std::uint32_t kParentRecomputeAnglesFlag=0x800u;
inline constexpr std::uint32_t kOriginalEntityHandleRegistryVA=0x224F5620u;
struct HandleParts { std::uint32_t index,serial; };
inline HandleParts decode_original_parent_handle(std::uint32_t raw) noexcept {
    return {raw&0xfffu,raw>>12u};
}
enum class Outcome : std::uint8_t {
    Unchanged,ChangedNoValidParent,ChangedParentAnglesMatched,
    ChangedParentTransformed,
};
// 0x221439C0, 412 original x86 bytes. Host provides original engine
// cache, transforms/matrices, handle-registry and clocked notification effects.
// The complete MSVC x86 entity/virtual ABI is not implemented.
template<class Host>
Outcome original_set_absolute_angles(EntityAngles& self,
                                     const Angle3& proposed,Host& host) {
    host.refresh_absolute_angles(self); // native 0x2213FA60, even on no change
    if(original_angles_equal(self.absolute_angles_354,proposed))
        return Outcome::Unchanged;

    host.notify_transform_change(self,2u); // native 0x2214B620
    self.flags_b9=static_cast<std::uint8_t>(self.flags_b9&~0x08u);
    copy_native_angles(self.absolute_angles_354,proposed); // +0x354..0x35C
    host.rebuild_absolute_angles_matrix(self,proposed); // 0x22269F80
    host.rebuild_auxiliary_angle_transform(self,3u); // 0x22269000

    ParentAngles* parent=nullptr;
    if(self.parent_handle_f0!=kInvalidParentHandle) {
        const HandleParts h=decode_original_parent_handle(self.parent_handle_f0);
        parent=host.resolve_parent(h.index,h.serial); // registry 0x224F5620
    }

    Angle3 local{};
    Outcome outcome=Outcome::ChangedNoValidParent;
    if(parent==nullptr) {
        copy_native_angles(local,proposed);
    } else {
        if((parent->flags_b8&kParentRecomputeAnglesFlag)!=0)
            host.refresh_parent_absolute_angles(*parent); // 0x2213FA60
        if(original_angles_equal(self.absolute_angles_354,
                                 parent->absolute_angles_354)) {
            local=Angle3{}; // native three raw zero DWORDs
            outcome=Outcome::ChangedParentAnglesMatched;
        } else {
            // Original checks parent bit11 AGAIN after the equality callback.
            if((parent->flags_b8&kParentRecomputeAnglesFlag)!=0)
                host.refresh_parent_absolute_angles(*parent);
            host.invert_parent_rotation(*parent); // 0x22268F10
            host.compose_parent_relative_rotation(self,*parent); // 0x22269520
            local=host.extract_relative_angles(self,*parent); // 0x22268A60
            outcome=Outcome::ChangedParentTransformed;
        }
    }
    // 0x2213A360 is comparison ONLY. 0x22140040 changes the tracked
    // vector if needed, after which 0x22042D70 receives the game clock.
    if(original_angles_different(self.local_angles_36c,local)) {
        original_assign_tracked_angles(self.local_angles_36c,local,self.notify);
        const float game_time=host.read_game_time();
        host.on_local_angle_change(self,game_time);
    }
    return outcome;
}
} // namespace sin_compat::original_server_entity_angles
