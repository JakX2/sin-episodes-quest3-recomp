#pragma once
// Client-specific original-guided native console registration.
// Unlike the server accessor this does not query a command-line
// override or restore a replicated default. Owned fixtures only.
#include "server_native_cvar_registration.h"
#include <array>
namespace sin_independent::console_abi {
constexpr int kClientDllFlag=0x8;
constexpr int kUnregisteredFlag=0x1;
enum class ClientRegisterResult { Completed, InvalidInput, AlreadyAttempted, InvalidList };
template<std::size_t Capacity>
class ClientConsoleRegistration {
public:
    static_assert(Capacity>0);
    ClientConsoleRegistration()=default;
    ClientConsoleRegistration(const ClientConsoleRegistration&)=delete;
    ClientConsoleRegistration& operator=(const ClientConsoleRegistration&)=delete;
    bool Add(CommandBase& p) noexcept {
        if(attempted_ || p.next || !p.name || !*p.name || p.registered) return false;
        for(std::size_t i=0;i<count_;++i)if(nodes_[i]==&p)return false;
        // Original CreateBase excludes FCVAR_UNREGISTERED from the
        // pending linked list. Such a record remains caller-owned.
        if(p.flags&kUnregisteredFlag)return true;
        if(count_==Capacity)return false;
        p.next=head_;head_=&p;nodes_[count_++]=&p;
        return true;
    }
    CommandBase* Head()const noexcept{return head_;}
    std::size_t Count()const noexcept{return count_;}
    bool Attempted()const noexcept{return attempted_;}
    ClientRegisterResult Initialize(EngineCvar* engine) noexcept {
        if(attempted_)return ClientRegisterResult::AlreadyAttempted;
        if(!engine)return ClientRegisterResult::InvalidInput;
        // Ownership and list order must still match original prepend
        // order. Duplicate names are valid original metadata; distinct
        // object addresses are required.
        auto* p=head_;
        for(std::size_t i=count_;i>0;--i) {
            if(!p || p!=nodes_[i-1] || !p->GetName() || !*p->GetName())
                return ClientRegisterResult::InvalidList;
            p=p->next;
        }
        if(p)return ClientRegisterResult::InvalidList;
        attempted_=true;
        AccessorScope scope(&Access,engine);
        // Original manager caches successor before virtual Init.
        // Engine may relink the current object or its ConVar parent.
        for(auto* current=head_;current;) {
            auto* next=current->next;
            current->Init();
            current=next;
        }
        return ClientRegisterResult::Completed;
    }
private:
    static bool __cdecl Access(CommandBase* object,void* context) noexcept {
        object->AddFlags(kClientDllFlag);
        object->next=nullptr;
        static_cast<EngineCvar*>(context)->RegisterConCommandBase(object);
        return true;
    }
    std::array<CommandBase*,Capacity> nodes_{};
    CommandBase* head_=nullptr;
    std::size_t count_=0;
    bool attempted_=false;
};
} // namespace sin_independent::console_abi
