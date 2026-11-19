#pragma once
// Original 2006 converter registration behavior, without assuming native
// CUtlSymbolTable / CUtlRBTree ABI. Caller supplies name-index callbacks.
// The original 0x24009340 warns and leaves the old converter on duplicate.
#include "client_hud_texture_converter_abi.h"
namespace sin_independent::hud_texture {
struct Registry {
    void* context=nullptr;
    AnimationConverterAbi* (*find)(void*,const char*) noexcept=nullptr;
    bool (*insert)(void*,const char*,AnimationConverterAbi*) noexcept=nullptr;
    void (*warn_duplicate)(void*,const char*) noexcept=nullptr;
};
enum class AddResult { Invalid, Inserted, DuplicateIgnored, InsertRejected };
inline const char* AddName(AddResult result) noexcept {
    switch(result) {
        case AddResult::Invalid:return "invalid";
        case AddResult::Inserted:return "inserted";
        case AddResult::DuplicateIgnored:return "duplicate-ignored";
        case AddResult::InsertRejected:return "insert-rejected";
    }
    return "unexpected";
}
inline AddResult RegisterInto(Registry host,const char* type,
                              AnimationConverterAbi* converter) noexcept {
    if(!type || !type[0] || !converter || !host.find || !host.insert)
        return AddResult::Invalid;
    // The original intern table owns its case/key policy. Do not use
    // strcmp/ASCII folding here to invent symbol semantics.
    if(host.find(host.context,type)) {
        if(host.warn_duplicate)host.warn_duplicate(host.context,type);
        return AddResult::DuplicateIgnored;
    }
    return host.insert(host.context,type,converter) ?
        AddResult::Inserted : AddResult::InsertRejected;
}
// Native VGui_Startup sets the one-time bit *before* AddPropertyConverter.
// A registered interface pointer must never be invalidated by copies/moves.
// A concrete process host must make this owner static for its lifetime.
class NativeRegistrationOwner final {
public:
    explicit NativeRegistrationOwner(Hooks hooks) noexcept:converter_(hooks){}
    NativeRegistrationOwner(const NativeRegistrationOwner&)=delete;
    NativeRegistrationOwner& operator=(const NativeRegistrationOwner&)=delete;
    NativeRegistrationOwner(NativeRegistrationOwner&&)=delete;
    NativeRegistrationOwner& operator=(NativeRegistrationOwner&&)=delete;
    AddResult Start(Registry host) noexcept {
        if(started_)return result_;
        if(!host.find || !host.insert)return AddResult::Invalid;
        started_=true;  // matches original flag-before-registration order
        result_=RegisterInto(host,kTypeName,&converter_);
        return result_;
    }
    bool started() const noexcept {return started_;}
    AddResult result() const noexcept {return result_;}
    AnimationConverterAbi* interface() noexcept {return &converter_;}
private:
    NativeConverter converter_;
    bool started_=false;
    AddResult result_=AddResult::Invalid;
};
} // namespace sin_independent::hud_texture
