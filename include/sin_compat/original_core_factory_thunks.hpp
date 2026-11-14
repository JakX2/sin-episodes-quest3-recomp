#pragma once
// Phase226: original 2006 SiN core CreateInterface singleton-return thunks.
// Original PE32 factories are SIX native bytes: MOV EAX,imm32; RET. The
// imm32 has a HIGHLOW relocation. This host model returns *address tokens*,
// NOT live original Source engine interface pointers or initialized vtables.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace sin_compat::original_core_factory_thunks {

enum class Module { Server, Client };
inline constexpr std::size_t kOriginalThunkBytes=6u;
inline constexpr std::size_t kOriginalPostThunkINT3Bytes=10u;
inline constexpr std::size_t kOriginalTotalCoreFactories=18u;
inline constexpr std::size_t kNewPhase226FactoryBodies=9u;
inline constexpr std::size_t kPreviouslyReviewedPhase178Bodies=9u;
inline constexpr std::size_t kSeparateClientAllocatingMaterialProxies=47u;

struct Binding {
    Module module{};
    const char* interface_name{};
    std::uint32_t factory_va{};
    std::uint32_t singleton_va{};
    // Only read from file-backed original .data. Zero means the
    // singleton is in the unbacked virtual tail of original .data;
    // neither its initial vptr nor runtime initialization is known.
    std::uint32_t original_vptr_va{};
    std::uint32_t original_first_virtual_method_va{};
    bool newly_verified_phase226{};
};

inline constexpr std::array<Binding,kOriginalTotalCoreFactories> kBindings{{
    {Module::Server,"IEffects001",0x22129680u,0x224ECE00u,0x224059B8u,0x22129810u,true},
    {Module::Server,"ServerGameDLL003",0x221E7630u,0x22510650u,0x22442EFCu,0x221E7640u,false},
    {Module::Server,"ServerGameEnts001",0x221E82C0u,0x22510654u,0x22443030u,0x221E9360u,false},
    {Module::Server,"ServerGameClients003",0x221E8320u,0x22510658u,0x22443054u,0x221E63F0u,false},
    {Module::Server,"PluginHelpersCheck001",0x221FC4F0u,0x22511A14u,0x2244688Cu,0x221FC500u,true},
    {Module::Server,"PlayerInfoManager001",0x22257780u,0x2251A048u,0x22452E64u,0x22257860u,true},
    {Module::Server,"PlayerInfoManager002",0x222577A0u,0x2251A048u,0x22452E64u,0x22257860u,true},
    {Module::Server,"BotManager001",0x222577B0u,0x2251A04Cu,0x22452E70u,0x222578A0u,true},
    {Module::Server,"HLTVDirector001",0x2225F8C0u,0x2257AEA0u,0u,0u,true},
    {Module::Server,"GameMovement001",0x222CFFE0u,0x225934B0u,0u,0u,false},
    {Module::Client,"GameClientExports001",0x240FD810u,0x242FA4B0u,0x24296B18u,0x240FE9B0u,false},
    {Module::Client,"VClient011",0x240FD840u,0x242FA4B8u,0x24296C6Cu,0x240FE0E0u,false},
    {Module::Client,"VClientEntityList003",0x241056B0u,0x2434FDDCu,0u,0u,false},
    {Module::Client,"ClientLeafSystem001",0x24108B20u,0x24357F30u,0u,0u,true},
    {Module::Client,"IEffects001",0x24127780u,0x242FB554u,0x24298E54u,0x24127890u,true},
    {Module::Client,"VCENTERPRINT002",0x241EC540u,0x2431DBF0u,0x242ABC4Cu,0x241EC430u,true},
    {Module::Client,"GameMovement001",0x2420FA40u,0x24379920u,0u,0u,false},
    {Module::Client,"VClientPrediction001",0x2422EA30u,0x2437FA00u,0u,0u,false},
}};

inline constexpr std::uint32_t original_image_base(Module module) noexcept {
    return module==Module::Server ? 0x22000000u : 0x24000000u;
}
inline const Binding* registered_core_factory(Module module,
                                               std::string_view exact_name) noexcept {
    // Native CreateInterface compares case-sensitive zero-terminated
    // interface names and returns first exact match; Phase177 already
    // verified the original linked-list dispatcher and registration order.
    for(const auto& entry:kBindings)
        if(entry.module==module && exact_name==entry.interface_name)
            return &entry;
    return nullptr;
}
inline std::uint32_t rebased_address_token(Module module,
       std::uint32_t original_absolute_va,std::uint32_t loaded_image_base) {
    // Only computes the address *expected* after an i386 HIGHLOW
    // relocation. This does not apply relocations to a loaded PE module,
    // check the object vtable, or make the original memory accessible.
    const auto original=original_image_base(module);
    if(original_absolute_va<original)
        throw std::out_of_range("original absolute VA below PE32 image base");
    const auto offset=original_absolute_va-original;
    const auto relocated=static_cast<std::uint64_t>(loaded_image_base)+offset;
    if(relocated>std::numeric_limits<std::uint32_t>::max())
        throw std::overflow_error("host-only 32-bit rebased factory token overflow");
    return static_cast<std::uint32_t>(relocated);
}
inline std::uint32_t singleton_return_token(const Binding& binding,
                                            std::uint32_t loaded_image_base) {
    return rebased_address_token(binding.module,binding.singleton_va,loaded_image_base);
}
inline std::uint32_t file_backed_vptr_token(const Binding& binding,
                                           std::uint32_t loaded_image_base) {
    if(binding.original_vptr_va==0u)
        throw std::logic_error("original singleton has no file-backed vptr evidence");
    return rebased_address_token(binding.module,binding.original_vptr_va,loaded_image_base);
}
inline std::size_t newly_reviewed_count() noexcept {
    std::size_t count=0;
    for(const auto& entry:kBindings)
        if(entry.newly_verified_phase226)++count;
    return count;
}

} // namespace sin_compat::original_core_factory_thunks
