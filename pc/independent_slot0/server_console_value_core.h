#pragma once
// Owned, original-guided console variable value state. This is deliberately
// NOT a ConVar ABI overlay and must never be registered with retail ICvar.
// The original root pointer is at +0x18; original internal setters operate
// on the root. Here the link is explicit and the owner retains both objects.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>

namespace sin_independent {
constexpr std::uint32_t kConsoleNeverAsString=0x1000;
class ConsoleValue final {
public:
    using Changed=void (*)(ConsoleValue&,const char*,void*) noexcept;
    struct Bounds {bool hasMin=false;float min=0;bool hasMax=false;float max=0;};
    ConsoleValue()=default;
    ConsoleValue(const ConsoleValue&)=delete;
    ConsoleValue& operator=(const ConsoleValue&)=delete;

    bool Initialize(const char* initial,std::uint32_t flags,Bounds bounds) {
        if(initialized_ || !initial ||
           (bounds.hasMin && !std::isfinite(bounds.min)) ||
           (bounds.hasMax && !std::isfinite(bounds.max)) ||
           (bounds.hasMin && bounds.hasMax && bounds.min>bounds.max))return false;
        const auto parsed=Parse(initial);
        if(!std::isfinite(parsed))return false;
        default_=initial;
        string_=initial;
        flags_=flags;
        bounds_=bounds;
        number_=parsed;
        integer_=Integer(parsed);
        initialized_=true; // original Create does not clamp the default
        return true;
    }
    bool LinkTo(ConsoleValue& root) noexcept {
        if(!initialized_ || !root.initialized_ || root.parent_ ||
           parent_ || hasAliases_ || this==&root)return false;
        parent_=&root;
        root.hasAliases_=true;
        return true;
    }
    const char* Default() const noexcept {return Root().default_.c_str();}
    const char* String() const noexcept {
        return (flags_ & kConsoleNeverAsString) ? "FCVAR_NEVER_AS_STRING" : Root().string_.c_str();
    }
    float Float() const noexcept {return Root().number_;}
    int Int() const noexcept {return Root().integer_;}
    bool InstallChanged(Changed callback,void* context) noexcept {
        auto& root=Root();
        if(!root.initialized_ || root.callback_ ||
           (root.flags_&kConsoleNeverAsString) || !callback)return false;
        root.callback_=callback;
        root.context_=context;
        callback(root,root.string_.c_str(),context);
        return true;
    }
    bool SetString(const char* value) {
        auto& root=Root();
        if(!root.initialized_ || !value)return false;
        float number=Parse(value);
        if(!std::isfinite(number))return false;
        const bool clamped=root.Clamp(number);
        char formatted[64]={};
        if(clamped)std::snprintf(formatted,sizeof(formatted),"%f",number);
        return root.Commit(number,Integer(number),clamped?formatted:value);
    }
    bool SetFloat(float value) {
        auto& root=Root();
        if(!root.initialized_ || !std::isfinite(value))return false;
        if(value==root.number_)return true;
        root.Clamp(value);
        char formatted[64]={};
        std::snprintf(formatted,sizeof(formatted),"%f",value);
        return root.Commit(value,Integer(value),formatted);
    }
    bool SetInt(int value) {
        auto& root=Root();
        if(!root.initialized_)return false;
        if(value==root.integer_)return true;
        float number=static_cast<float>(value);
        if(root.Clamp(number))value=Integer(number);
        char formatted[32]={};
        std::snprintf(formatted,sizeof(formatted),"%d",value);
        return root.Commit(number,value,formatted);
    }
    bool Revert() {return SetString(Default());}

private:
    ConsoleValue& Root() noexcept {return parent_?*parent_:*this;}
    const ConsoleValue& Root() const noexcept {return parent_?*parent_:*this;}
    static float Parse(const char* value) noexcept {
        return static_cast<float>(std::strtod(value,nullptr));
    }
    static int Integer(float value) noexcept {
        // The original x87 conversion truncates and returns integer
        // indefinite for out-of-range finite input. Avoid C++ UB.
        const double v=value;
        if(v<std::numeric_limits<int>::min() ||
           v>static_cast<double>(std::numeric_limits<int>::max()))
            return std::numeric_limits<int>::min();
        return static_cast<int>(v);
    }
    bool Clamp(float& value) const noexcept {
        if(bounds_.hasMin && value<bounds_.min) {value=bounds_.min;return true;}
        if(bounds_.hasMax && value>bounds_.max) {value=bounds_.max;return true;}
        return false;
    }
    bool Commit(float number,int integer,const char* text) {
        if(flags_&kConsoleNeverAsString) {
            number_=number;integer_=integer;
            return true;
        }
        // Copy both strings before mutation, including when the supplied
        // value aliases current storage. The callback sees the old string
        // and the new state. No engine allocator or original object used.
        std::string next(text);
        std::string old(string_);
        string_=std::move(next);
        number_=number;integer_=integer;
        if(callback_)callback_(*this,old.c_str(),context_);
        return true;
    }
    ConsoleValue* parent_=nullptr;
    std::string default_,string_;
    Bounds bounds_{};
    std::uint32_t flags_=0;
    float number_=0;
    int integer_=0;
    bool initialized_=false,hasAliases_=false;
    Changed callback_=nullptr;
    void* context_=nullptr;
};
} // namespace sin_independent
