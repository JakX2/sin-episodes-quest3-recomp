#include "sin_compat/original_druglab_grunt_factory.hpp"
#include <limits>

namespace sin_compat::original_druglab_grunt {
Variant GruntFactoryReference::classify(std::string_view name) noexcept {
    if(name=="npc_druglab_grunt_pistol")return Variant::Pistol;
    if(name=="npc_druglab_grunt_scattergun")return Variant::Scattergun;
    if(name=="npc_druglab_grunt_assault_rifle")return Variant::AssaultRifle;
    return Variant::Unsupported;
}
Result GruntFactoryReference::create(
    std::string_view classname,std::uintptr_t opaque_factory_arg) noexcept {
    const Variant v=classify(classname);
    if(v==Variant::Unsupported)return {Code::UnsupportedClass,v};
    Guard guard(busy_);
    if(!guard)return {Code::Reentrant,v};
    std::uintptr_t object=0;
    try {
        object=host_.allocate_native_object(kOriginalAllocationBytes);
        if(!object)return {Code::AllocationFailed,v};
        if(object>std::numeric_limits<std::uintptr_t>::max()-
                  kFactoryReturnedSubobjectOffset) {
            const bool released=host_.reclaim_unpublished_object(object);
            return {released?Code::PointerOverflow:Code::RollbackFailed,v};
        }
        if(host_.invoke_verified_base_constructor(object) &&
           host_.establish_native_vtable(object,kPrimaryVptrOffset,
                                         VptrRole::Primary) &&
           host_.establish_native_vtable(object,kSecondVptrOffset,
                                         VptrRole::Secondary790) &&
           host_.establish_native_vtable(object,kThirdVptrOffset,
                                         VptrRole::TertiaryC14) &&
           host_.invoke_native_virtual_70(object,opaque_factory_arg)) {
            return {Code::Created,v,object,
                    object+kFactoryReturnedSubobjectOffset};
        }
    } catch (...) {}
    // Original 2006 bytecode assumes successful allocation/base
    // initialization and does not provide this safe modern rollback.
    // Refuse to publish a half-constructed Source entity.
    try {
        if(object!=0 && !host_.reclaim_unpublished_object(object))
            return {Code::RollbackFailed,v};
    } catch (...) {return {Code::RollbackFailed,v};}
    return {Code::HostRejected,v};
}
bool GruntFactoryReference::delegate_release(
    std::uintptr_t entity) noexcept {
    if(entity==0)return true;
    Guard guard(busy_);
    if(!guard)return false;
    try {return host_.invoke_delegate_virtual_18(entity);}
    catch (...) {return false;}
}
} // namespace sin_compat::original_druglab_grunt
