#pragma once
// Source SDK 2004 public/vgui_controls/Panel.h:
// IPanelAnimationPropertyConverter exposes exactly three void virtual
// methods and NO virtual destructor. The original SiN binary's live
// registration function remains unverified. These signatures model only
// the client-supplied converter object in Win32 x86 MSVC.
#include "client_hud_texture_converter_core.h"
namespace sin_independent::hud_texture {
struct AnimationConverterAbi {
    virtual void GetData(void* panel,void* keyvalues,void* entry) noexcept=0;
    virtual void SetData(void* panel,void* keyvalues,void* entry) noexcept=0;
    virtual void InitFromDefault(void* panel,void* entry) noexcept=0;
};
class NativeConverter final:public AnimationConverterAbi {
public:
    explicit NativeConverter(Hooks hooks) noexcept:core_(hooks){}
    NativeConverter(const NativeConverter&)=delete;
    NativeConverter& operator=(const NativeConverter&)=delete;
    NativeConverter(NativeConverter&&)=delete;
    NativeConverter& operator=(NativeConverter&&)=delete;
    void GetData(void* panel,void* kv,void* entry) noexcept override {
        lastResult_=core_.GetData(panel,kv,entry);
    }
    void SetData(void* panel,void* kv,void* entry) noexcept override {
        lastResult_=core_.SetData(panel,kv,entry);
    }
    void InitFromDefault(void* panel,void* entry) noexcept override {
        lastResult_=core_.InitFromDefault(panel,entry);
    }
    bool lastResult() const noexcept {return lastResult_;}
private:
    Converter core_;
    bool lastResult_=false;
};
} // namespace sin_independent::hud_texture
