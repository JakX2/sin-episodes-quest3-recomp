#pragma once
// Phase250: original SiN Episodes: Emergence (2006) client.dll
// DT_EntityParticleTrailInfo RecvTable and CRT registration host reference.
// Original method whole-byte SHA256 provenance is checked by the
// Phase250 auditor. Not the original Source entity class or gameplay DLL.
#include <array>
#include <cstddef>
#include <cstdint>

namespace sin_compat::original_client_particle_trail_recvtable_crt {

inline constexpr std::uint32_t kNativeRecvtableInitVA=0x241281D0u;
inline constexpr std::uint32_t kNativeCrtLifetimeRegistrationVA=0x24268960u;
inline constexpr std::uint32_t kNativeCrtInitializeVA=0x24268980u;
inline constexpr std::uint32_t kNativeCrtCleanupThunkVA=0x242736B0u;
inline constexpr std::uint32_t kNativePlaceholderFactoryVA=0x24126DF0u;
inline constexpr std::uint32_t kNativeFloatFactoryVA=0x24126D80u;
inline constexpr std::uint32_t kNativeFloatRecvProxyVA=0x24126C40u;
inline constexpr std::uint32_t kNativeTableRegisterVA=0x24126A30u;
inline constexpr std::uint32_t kNativeTableConstructVA=0x24126D30u;
inline constexpr std::uint32_t kNativeTableDestructVA=0x24126A20u;
inline constexpr std::uint32_t kNativeCrtTerminationRegistrationVA=0x24248788u;

inline constexpr std::uint32_t kNativeInitOnceFlagsVA=0x24361040u;
inline constexpr std::uint32_t kNativePlaceholderDescriptorVA=0x24360F50u;
inline constexpr std::uint32_t kNativeFirstNetworkDescriptorVA=0x24360F8Cu;
inline constexpr std::uint32_t kNativeSecondNetworkDescriptorVA=0x24360FC8u;
inline constexpr std::uint32_t kNativeThirdNetworkDescriptorVA=0x24361004u;
inline constexpr std::uint32_t kNativeRecvTableObjectVA=0x24361044u;
inline constexpr std::uint32_t kNativeCrtResultVA=0x24361058u;
inline constexpr std::uint32_t kNativeTableNameVA=0x24298EF8u;
inline constexpr std::uint32_t kNativePlaceholderNameVA=0x242762D8u;
inline constexpr std::uint32_t kNativeLifetimeNameVA=0x24278E80u;
inline constexpr std::uint32_t kNativeStartSizeNameVA=0x24278DE4u;
inline constexpr std::uint32_t kNativeEndSizeNameVA=0x24278DD8u;
inline constexpr std::uint32_t kNativeCleanupPointerVA=0x242736B0u;
inline constexpr std::uint32_t kNativeCrtPointer0VA=0x242F3A38u;
inline constexpr std::uint32_t kNativeCrtPointer1VA=0x242F3A3Cu;

inline constexpr std::size_t kWholeNativeMethods=4u;
inline constexpr std::size_t kWholeNativeCodeBytes=199u;
inline constexpr std::size_t kNativeRecvPropDescriptorStride=0x3Cu;
inline constexpr std::uint32_t kNativeRealRecvPropCount=3u;
inline constexpr std::uint32_t kNativeFloatSizeBytes=4u;
inline constexpr std::uint32_t kOriginalInitializerReturn=1u;

struct FloatRecvProperty {
    const char* property_name;
    std::uint32_t original_name_VA;
    std::uint32_t original_descriptor_VA;
    std::uint32_t original_member_offset;
};
// The original networked fields are three FLOAT props, not four.
// A preceding placeholder occupies a separate descriptor and is NOT
// included in native RecvTable registration's count=3.
inline constexpr std::array<FloatRecvProperty,3> kFloatProperties{{
    {"m_flLifetime",kNativeLifetimeNameVA,kNativeFirstNetworkDescriptorVA,8u},
    {"m_flStartSize",kNativeStartSizeNameVA,kNativeSecondNetworkDescriptorVA,12u},
    {"m_flEndSize",kNativeEndSizeNameVA,kNativeThirdNetworkDescriptorVA,16u},
}};
inline constexpr const char* kNativeRecvTableName="DT_EntityParticleTrailInfo";
inline constexpr const char* kNativePlaceholderName="should_never_see_this";

// These injected callbacks stand for the retail DLL's still-unrecovered
// 0x24126DF0, 0x24126D80, 0x24126A30, CRT and cleanup bodies.
// The host never executes an original 32-bit game-process function pointer.
struct RuntimeCallbacks {
    virtual ~RuntimeCallbacks()=default;
    virtual void construct_placeholder(
        std::uint32_t descriptor_VA,std::uint32_t name_VA,
        std::uint32_t member_offset,std::uint32_t data_bytes)=0;
    virtual void construct_float(
        std::uint32_t descriptor_VA,std::uint32_t name_VA,
        std::uint32_t member_offset,std::uint32_t data_bytes,
        std::uint32_t proxy_VA)=0;
    virtual void register_table(
        std::uint32_t object_VA,std::uint32_t first_descriptor_VA,
        std::uint32_t count,std::uint32_t table_name_VA)=0;
    virtual void construct_table_object(std::uint32_t object_VA)=0;
    virtual void register_termination_handler(std::uint32_t cleanup_VA)=0;
    virtual void destruct_table_object(std::uint32_t object_VA)=0;
};

// A host-owned record for the original flags DWORD at 0x24361040
// and cached CRT initializer result DWORD at 0x24361058.
struct Globals {
    std::uint32_t once_flags{};
    std::uint32_t crt_initialized_result{};
};

// Original 0x241281D0: if flag bit0 is unset, OR bit0, construct
// the internal placeholder plus exactly 3 float RecvProp descriptors.
// Regardless of first-time state, always register the three real props
// via original 0x24126A30 and return EAX=1. Opaque factory details are
// supplied by RuntimeCallbacks, not guessed from generic Source SDKs.
inline std::uint32_t initialize_recv_table(
    Globals& globals,RuntimeCallbacks& callbacks) {
    if((globals.once_flags&1u)==0u) {
        globals.once_flags|=1u;
        callbacks.construct_placeholder(
            kNativePlaceholderDescriptorVA,kNativePlaceholderNameVA,
            0u,kNativeFloatSizeBytes);
        for(const auto& field:kFloatProperties) {
            callbacks.construct_float(
                field.original_descriptor_VA,
                field.original_name_VA,
                field.original_member_offset,
                kNativeFloatSizeBytes,
                kNativeFloatRecvProxyVA);
        }
    }
    callbacks.register_table(
        kNativeRecvTableObjectVA,
        kNativeFirstNetworkDescriptorVA,
        kNativeRealRecvPropCount,
        kNativeTableNameVA);
    return kOriginalInitializerReturn;
}

// Original 0x24268980: PUSH 0, CALL 0x241281D0, store returned EAX
// at original global 0x24361058. It is separate from the constructor
// at 0x24268960 even though their pointers are adjacent in .data.
inline void run_crt_table_initializer(
    Globals& globals,RuntimeCallbacks& callbacks) {
    globals.crt_initialized_result=initialize_recv_table(globals,callbacks);
}

// Original 0x24268960: initialize the table object via 0x24126D30,
// then register 0x242736B0 with the CRT termination helper.
// This does not itself construct the individual float descriptors.
inline void register_crt_table_lifetime(RuntimeCallbacks& callbacks) {
    callbacks.construct_table_object(kNativeRecvTableObjectVA);
    callbacks.register_termination_handler(kNativeCleanupPointerVA);
}

// Original 0x242736B0 adjusts to the registered global RecvTable
// object and tail-JMPs into native destructor 0x24126A20.
inline void cleanup_crt_table(RuntimeCallbacks& callbacks) {
    callbacks.destruct_table_object(kNativeRecvTableObjectVA);
}

} // namespace sin_compat::original_client_particle_trail_recvtable_crt
