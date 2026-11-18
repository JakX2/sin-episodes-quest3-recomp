#pragma once
// Original-guided registration adapter for independently owned native
// console objects. Only the synthetic engine owner exercises it at
// present. No original game object is passed through these C++ classes.
#include "server_console_object_abi.h"

namespace sin_independent::console_abi {
// The original VEngineCvar002 has no virtual destructor ahead of these
// slots. Verified from the original server registration accessor and
// owner's SDK 2004 icvar.h.
class EngineCvar {
public:
    virtual void __thiscall RegisterConCommandBase(CommandBase*) noexcept=0; // 0
    virtual const char* __thiscall GetCommandLineValue(const char*) noexcept=0; // 1
    virtual Variable* __thiscall FindMutableVar(const char*) noexcept=0; // 2
    virtual const Variable* __thiscall FindConstVar(const char*) const noexcept=0; // 3
    virtual CommandBase* __thiscall GetCommands() noexcept=0; // 4
protected:
    ~EngineCvar()=default;
};

enum class RegisterResult {
    Registered, MissingSvCheats, InvalidList, InvalidEngine, AlreadyAttempted, Busy
};
inline bool SameName(const char* a,const char* b) noexcept {
    if(!a || !b)return false;
    auto fold=[](unsigned char c) noexcept {
        return c>='A'&&c<='Z'?c+('a'-'A'):c;
    };
    while(*a && *b && fold(*a)==fold(*b)){++a;++b;}
    return fold(*a)==fold(*b);
}

class NativeConsoleRegistration final {
public:
    NativeConsoleRegistration()=default;
    NativeConsoleRegistration(const NativeConsoleRegistration&)=delete;
    NativeConsoleRegistration& operator=(const NativeConsoleRegistration&)=delete;

    RegisterResult Initialize(CommandBase* localHead,std::size_t bound,EngineCvar* engine) noexcept {
        if(busy_)return RegisterResult::Busy;
        if(attempted_)return RegisterResult::AlreadyAttempted;
        if(!engine)return RegisterResult::InvalidEngine;
        if(!bound)return RegisterResult::InvalidList;
        std::size_t n=0;
        for(auto* p=localHead;p;p=p->next) {
            if(n++==bound || !p->GetName() || !*p->GetName())
                return RegisterResult::InvalidList;
        }
        busy_=true;
        engine_=engine;
        {
            AccessorScope scope(&Access,this);
            // Traverse the original pending list, saving next before
            // virtual Init. Registration is allowed to relink current.
            for(auto* p=localHead;p;) {
                auto* next=p->next;
                p->Init();
                p=next;
            }
        }
        developer_=static_cast<const EngineCvar*>(engine)->FindConstVar("developer");
        // Original DLLInit searches from its local-head pointer, NOT
        // ICvar::GetCommands or FindVar. Relinking may expose engine
        // objects on that chain. Refuse a malformed/cyclic chain.
        std::size_t walked=0;
        for(auto* p=localHead;p && walked<bound;p=p->next,++walked) {
            if(SameName(p->GetName(),"sv_cheats")){cheats_=p;break;}
        }
        // External registration effects cannot be rolled back by
        // retrying. Keep all passed objects alive until owner teardown.
        attempted_=true;busy_=false;
        return cheats_?RegisterResult::Registered:RegisterResult::MissingSvCheats;
    }

    bool Attempted() const noexcept {return attempted_;}
    bool Ready() const noexcept {return attempted_ && cheats_;}
    const Variable* Developer() const noexcept {return developer_;}
    CommandBase* SvCheats() const noexcept {return cheats_;}
private:
    static bool __cdecl Access(CommandBase* p,void* context) noexcept {
        auto& self=*static_cast<NativeConsoleRegistration*>(context);
        p->AddFlags(0x4);
        const bool replicated=p->IsBitSet(0x2000);
        const char* preserved=nullptr;
        if(replicated && !p->IsCommand()) {
            auto* v=static_cast<Variable*>(p);
            preserved=v->parent->defaultValue;
        }
        p->next=nullptr;
        self.engine_->RegisterConCommandBase(p);
        const char* overrideValue=self.engine_->GetCommandLineValue(p->GetName());
        if(!p->IsCommand()) {
            const char* value=overrideValue?overrideValue:preserved;
            if(value)static_cast<Variable*>(p)->SetString(value);
        }
        return true;
    }
    EngineCvar* engine_=nullptr;
    const Variable* developer_=nullptr;
    CommandBase* cheats_=nullptr;
    bool busy_=false,attempted_=false;
};
} // namespace sin_independent::console_abi
