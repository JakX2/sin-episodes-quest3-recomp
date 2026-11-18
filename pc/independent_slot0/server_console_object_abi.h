#pragma once
// Independently authored original-guided MSVC x86 console ABI fixture.
// Not exported by the game DLL. Registration, list lifetime and memory
// ownership must be integrated before any retail engine use.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original native x86 layout only
#endif
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <cstdint>

namespace sin_independent::console_abi {
class CommandBase;
class Variable;
using RegisterAccessor=bool (__cdecl *)(CommandBase*,void*) noexcept;
using ChangeCallback=void (__cdecl *)(Variable*,const char*);
using CommandCallback=void (__cdecl *)();
using CompletionCallback=int (__cdecl *)(const char*,char (*)[64]);

// The original accessor is module-global. The independent fixture
// requires a scoped explicit owner; this is never an engine singleton.
struct AccessorScope {
    inline static thread_local RegisterAccessor current=nullptr;
    inline static thread_local void* context=nullptr;
    RegisterAccessor old;
    void* oldContext;
    AccessorScope(RegisterAccessor fn,void* c) noexcept:
        old(current),oldContext(context) {current=fn;context=c;}
    ~AccessorScope(){current=old;context=oldContext;}
    AccessorScope(const AccessorScope&)=delete;
    AccessorScope& operator=(const AccessorScope&)=delete;
};
class CommandBase {
public:
    CommandBase()=default;
    CommandBase(const CommandBase&)=delete;
    CommandBase& operator=(const CommandBase&)=delete;
    virtual ~CommandBase()=default; // 0
    virtual bool __thiscall IsCommand() const noexcept {return true;} // 1
    virtual bool __thiscall IsBitSet(int mask) const noexcept {return (flags&mask)!=0;} // 2
    virtual void __thiscall AddFlags(int mask) noexcept {flags|=mask;} // 3
    virtual const char* __thiscall GetName() const noexcept {return name;} // 4
    virtual const char* __thiscall GetHelpText() const noexcept {return help;} // 5
    virtual bool __thiscall IsRegistered() const noexcept {return registered;} // 6
    virtual void __thiscall CreateBase(const char* n,const char* h,int f) noexcept { // 7
        if(!n || registered)return;
        name=n;help=h?h:"";flags=f;next=nullptr;
    }
    virtual void __thiscall Init() noexcept { // 8
        if(!registered && AccessorScope::current) {
            // The original ignores the accessor bool and sets the byte.
            AccessorScope::current(this,AccessorScope::context);
            registered=true;
        }
    }

    CommandBase* next=nullptr;
    bool registered=false;
    const char* name=nullptr;
    const char* help="";
    int flags=0;
};
static_assert(sizeof(CommandBase)==0x18,"original console base stride");

class Variable final:public CommandBase {
public:
    Variable()=default;
    ~Variable() override {delete[] string;}
    bool __thiscall IsCommand() const noexcept override{return false;}
    bool __thiscall IsBitSet(int mask) const noexcept override{return (parent->flags&mask)!=0;}
    void __thiscall AddFlags(int mask) noexcept override{parent->flags|=mask;}
    const char* __thiscall GetName() const noexcept override{return parent->name;}
    const char* __thiscall GetHelpText() const noexcept override{return parent->help;}
    bool __thiscall IsRegistered() const noexcept override{return parent->registered;}

    // Explicit distinct names preserve the original MSVC overloaded
    // setter ordering: int, float, string, then internal string/float/int.
    virtual void __thiscall SetInt(int value) {parent->InternalInt(value);} // 9
    virtual void __thiscall SetFloat(float value) {parent->InternalFloat(value);} // 10
    virtual void __thiscall SetString(const char* value) {parent->InternalString(value);} // 11
    virtual void __thiscall InternalString(const char* value) { // 12
        if(!value || !string)return;
        auto number=static_cast<float>(std::strtod(value,nullptr));
        if(!std::isfinite(number))return;
        char normalized[64]{};
        if(Clamp(number)) {
            std::snprintf(normalized,sizeof(normalized),"%f",number);
            value=normalized;
        }
        Commit(number,ToInt(number),value);
    }
    virtual void __thiscall InternalFloat(float value) { // 13
        if(!string || !std::isfinite(value) || real==value)return;
        Clamp(value);
        char formatted[64]{};
        std::snprintf(formatted,sizeof(formatted),"%f",value);
        Commit(value,ToInt(value),formatted);
    }
    virtual void __thiscall InternalInt(int value) { // 14
        if(!string || integer==value)return;
        float number=static_cast<float>(value);
        if(Clamp(number))value=ToInt(number);
        char formatted[64]{};
        std::snprintf(formatted,sizeof(formatted),"%d",value);
        Commit(number,value,formatted);
    }
    virtual bool __thiscall Clamp(float& value) noexcept { // 15
        if(hasMin && value<min) {value=min;return true;}
        if(hasMax && value>max) {value=max;return true;}
        return false;
    }
    virtual void __thiscall ChangeString(const char* value) { // 16
        if(!value || !string)return;
        ReplaceValue(real,integer,value);
    }
    virtual void __thiscall CreateVariable(const char* n,const char* def,int f,const char* h,
            bool minEnabled,float minimum,bool maxEnabled,float maximum,
            ChangeCallback onChange) { // 17
        if(string || !n || !*n ||
           (minEnabled&&!std::isfinite(minimum)) ||
           (maxEnabled&&!std::isfinite(maximum)) ||
           (minEnabled&&maxEnabled&&minimum>maximum))return;
        def=def?def:"";
        const auto length=std::strlen(def)+1;
        const auto number=static_cast<float>(std::strtod(def,nullptr));
        if(length>kMaxString || !std::isfinite(number))return;
        auto* initial=new(std::nothrow) char[length];
        if(!initial)return;
        std::memcpy(initial,def,length);
        parent=this;defaultValue=def;string=initial;capacity=static_cast<int>(length);
        real=number;integer=ToInt(number);
        hasMin=minEnabled;min=minimum;hasMax=maxEnabled;max=maximum;
        callback=onChange;
        CreateBase(n,h,f);
        if(AccessorScope::current)Init();
    }

    const char* String() const noexcept {
        return flags&0x1000?"FCVAR_NEVER_AS_STRING":parent->string?parent->string:"";
    }
    // All roots must outlive aliases. This matches the shape of the
    // original one-hop parent pointer; the engine owns actual relinking.
    Variable* parent=this;
    const char* defaultValue="";
    char* string=nullptr;
    int capacity=0;
    float real=0;
    int integer=0;
    bool hasMin=false;
    float min=0;
    bool hasMax=false;
    float max=0;
    ChangeCallback callback=nullptr;
private:
    static constexpr std::size_t kMaxString=65536;
    static int ToInt(float f) noexcept {
        const double v=f;
        return v<(std::numeric_limits<int>::min)() ||
            v>(std::numeric_limits<int>::max)()?
            (std::numeric_limits<int>::min)():static_cast<int>(v);
    }
    void Commit(float n,int i,const char* text) {
        if(flags&0x1000) {real=n;integer=i;return;}
        ReplaceValue(n,i,text);
    }
    void ReplaceValue(float number,int integerValue,const char* value) {
        // Retain old storage until after copying, including self-aliased
        // input. Old callback text must survive reentrant setter calls.
        const auto n=std::strlen(value)+1;
        if(n>kMaxString)return;
        std::unique_ptr<char[]> old(new(std::nothrow) char[std::strlen(string)+1]);
        if(!old)return;
        std::strcpy(old.get(),string);
        if(n>static_cast<std::size_t>(capacity)) {
            std::unique_ptr<char[]> fresh(new(std::nothrow) char[n]);
            if(!fresh)return;
            std::memcpy(fresh.get(),value,n);
            delete[] string;string=fresh.release();capacity=static_cast<int>(n);
        } else {
            std::memmove(string,value,n);
        }
        real=number;integer=integerValue;
        if(callback)callback(this,old.get());
    }
};

class Command final:public CommandBase {
public:
    static int __cdecl NoCompletion(const char*,char (*)[64]) {return 0;}
    virtual int __thiscall Complete(const char* partial,char (*out)[64]) { // 9
        return completion&&partial&&out?completion(partial,out):0;
    }
    virtual bool __thiscall CanComplete() noexcept {return hasCompletion;} // 10
    virtual void __thiscall Dispatch() {if(execute)execute();} // 11
    virtual void __thiscall CreateCommand(const char* n,CommandCallback fn,const char* h,
                                       int f,CompletionCallback complete) { // 12
        if(!n || !*n || !fn || registered)return;
        execute=fn;completion=complete?complete:&NoCompletion;hasCompletion=complete!=nullptr;
        CreateBase(n,h,f);
        if(AccessorScope::current)Init();
    }
    CommandCallback execute=nullptr;
    CompletionCallback completion=nullptr;
    bool hasCompletion=false;
};
static_assert(sizeof(Variable)==0x44,"original ConVar stride");
static_assert(sizeof(Command)==0x24,"original ConCommand stride");
} // namespace sin_independent::console_abi
