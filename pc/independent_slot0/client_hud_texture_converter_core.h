#pragma once
// Reconstructed 2004 client CHudTextureHandle converter semantics, not
// the original game's binary-compatible vgui::Panel registration ABI.
// Input Panel/Entry/KeyValues and HUD lookup are deliberately callbacks.
// This allows testing the original algorithm without making up a GUI ABI.
#include <cstddef>
#include <cstring>
namespace sin_independent::hud_texture {
inline constexpr const char kTypeName[]="CHudTextureHandle";
struct Handle {
    void* icon=nullptr;
    void Set(void* texture) noexcept {icon=texture;}
    void* Get() const noexcept {return icon;}
};
struct Hooks {
    void* context=nullptr;
    Handle* (*lookup)(void*,void*,void*) noexcept=nullptr;
    const char* (*entry_name)(void*,void*) noexcept=nullptr;
    const char* (*entry_default)(void*,void*) noexcept=nullptr;
    const char* (*read_string)(void*,void*,const char*) noexcept=nullptr;
    void (*write_string)(void*,void*,const char*,const char*) noexcept=nullptr;
    void* (*find_icon)(void*,const char*) noexcept=nullptr;
    const char* (*short_name)(void*,void*) noexcept=nullptr;
};
class Converter {
public:
    explicit Converter(Hooks callbacks) noexcept:hooks_(callbacks){}
    bool GetData(void* panel,void* keyvalues,void* entry) const noexcept {
        if(!hooks_.lookup || !hooks_.entry_name || !hooks_.write_string)return false;
        Handle* handle=hooks_.lookup(hooks_.context,panel,entry);
        const char* name=hooks_.entry_name(hooks_.context,entry);
        if(!handle || !name)return false;
        const char* textureName=nullptr;
        if(handle->Get() && hooks_.short_name)
            textureName=hooks_.short_name(hooks_.context,handle->Get());
        hooks_.write_string(hooks_.context,keyvalues,name,textureName?textureName:"");
        return true;
    }
    bool SetData(void* panel,void* keyvalues,void* entry) const noexcept {
        if(!hooks_.lookup || !hooks_.entry_name || !hooks_.read_string)return false;
        Handle* handle=hooks_.lookup(hooks_.context,panel,entry);
        const char* name=hooks_.entry_name(hooks_.context,entry);
        if(!handle || !name)return false;
        const char* textureName=hooks_.read_string(hooks_.context,keyvalues,name);
        return Assign(*handle,textureName);
    }
    bool InitFromDefault(void* panel,void* entry) const noexcept {
        if(!hooks_.lookup || !hooks_.entry_default)return false;
        Handle* handle=hooks_.lookup(hooks_.context,panel,entry);
        if(!handle)return false;
        return Assign(*handle,hooks_.entry_default(hooks_.context,entry));
    }
private:
    bool Assign(Handle& handle,const char* text) const noexcept {
        if(!text || !text[0]) {
            handle.Set(nullptr);
            return true;
        }
        if(!hooks_.find_icon)return false;
        // Unknown icon names are stored as a null handle, matching the
        // original converter's CHud::GetIcon result assignment.
        handle.Set(hooks_.find_icon(hooks_.context,text));
        return true;
    }
    Hooks hooks_;
};
using RegisterFn=void (*)(void*,const char*,Converter*) noexcept;
// Original VGui_OneTimeInit sets its initialized flag BEFORE invoking
// Panel::AddPropertyConverter. The converter and guard must be held by
// a process-lifetime owner if the engine can keep its pointer.
class OneTimeOwner {
public:
    explicit OneTimeOwner(Hooks callbacks) noexcept:converter_(callbacks){}
    bool Register(void* ctx,RegisterFn registrar) noexcept {
        if(initialized_ || !registrar)return false;
        initialized_=true;  // reentrant calls do not double-register
        registrar(ctx,kTypeName,&converter_);
        return true;
    }
    bool initialized() const noexcept {return initialized_;}
    Converter* converter() noexcept {return &converter_;}
private:
    Converter converter_;
    bool initialized_=false;
};
} // namespace sin_independent::hud_texture
