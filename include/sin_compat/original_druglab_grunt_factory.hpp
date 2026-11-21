#pragma once
// Phase531: independent CONTROL-FLOW reference for the original 2006
// Druglab Grunt factory, not an engine ABI or an actual Source CBaseNPC.
// Three registered classnames share factory vtable at original server VA
// 0x2249A9F8: creator 0x2236A320, delegated release 0x22369E30.
// The verified wrapper calls constructor 0x2236A340, allocates 0xFD8
// bytes, sets three vptr fields and returns native object+0x160.
//
// Never treat the original PE32 vtable VAs as live pointers on Windows
// independent DLLs or ARM64. Real engine host must substitute its OWN
// verified native object layouts, constructors, send tables and AI.
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sin_compat::original_druglab_grunt {
inline constexpr std::size_t kOriginalAllocationBytes=0xFD8u;
inline constexpr std::size_t kFactoryReturnedSubobjectOffset=0x160u;
inline constexpr std::uint32_t kPrimaryVptrOffset=0;
inline constexpr std::uint32_t kSecondVptrOffset=0x790u;
inline constexpr std::uint32_t kThirdVptrOffset=0xC14u;
inline constexpr std::uint32_t kOriginalFactoryVtableVA=0x2249A9F8u;
inline constexpr std::uint32_t kOriginalCreatorVA=0x2236A320u;
inline constexpr std::uint32_t kOriginalDelegateVA=0x22369E30u;
inline constexpr std::uint32_t kOriginalConstructorVA=0x2236A340u;

enum class Variant : std::uint8_t {
    Pistol, Scattergun, AssaultRifle, Unsupported
};
enum class VptrRole : std::uint8_t {
    Primary, Secondary790, TertiaryC14
};
enum class Code : std::uint8_t {
    Created, UnsupportedClass, Reentrant, AllocationFailed,
    HostRejected, PointerOverflow, RollbackFailed
};
struct Result {
    Code code=Code::UnsupportedClass;
    Variant variant=Variant::Unsupported;
    std::uintptr_t allocation_base=0;
    std::uintptr_t factory_return_view=0;
    bool ok() const noexcept {return code==Code::Created;}
};
class IHost {
public:
    virtual ~IHost()=default;
    // Host-owned storage, actual original class allocator must be adapted.
    virtual std::uintptr_t allocate_native_object(std::size_t bytes)=0;
    virtual bool invoke_verified_base_constructor(std::uintptr_t object)=0;
    // The three original vptr writes at +0,+0x790,+0xC14.
    // This does NOT pass original PE32 vtable VA as a live pointer.
    virtual bool establish_native_vtable(
        std::uintptr_t object,std::uint32_t offset,VptrRole role)=0;
    // Source original virtual byte +0x70 accepts an opaque factory arg.
    virtual bool invoke_native_virtual_70(
        std::uintptr_t object,std::uintptr_t forwarded_arg)=0;
    // Independent safe rollback, not the 2006 game's exact failure path.
    virtual bool reclaim_unpublished_object(std::uintptr_t object)=0;
    // Original second factory method calls its input's vtable +0x18
    // when input is non-null. Host must resolve ownership itself.
    virtual bool invoke_delegate_virtual_18(std::uintptr_t entity)=0;
};

class GruntFactoryReference final {
public:
    explicit GruntFactoryReference(IHost& host) noexcept:host_(host){}
    GruntFactoryReference(const GruntFactoryReference&)=delete;
    GruntFactoryReference& operator=(const GruntFactoryReference&)=delete;
    static Variant classify(std::string_view name) noexcept;
    Result create(std::string_view classname,
                  std::uintptr_t opaque_factory_arg) noexcept;
    bool delegate_release(std::uintptr_t entity) noexcept;
private:
    class Guard {
    public:
        explicit Guard(bool& b) noexcept:busy_(b),entered_(!b){
            if(entered_)busy_=true;
        }
        ~Guard() noexcept {if(entered_)busy_=false;}
        explicit operator bool() const noexcept{return entered_;}
    private:
        bool& busy_;
        bool entered_;
    };
    IHost& host_;
    bool busy_=false;
};
} // namespace sin_compat::original_druglab_grunt
