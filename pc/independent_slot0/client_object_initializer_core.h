#pragma once
// Original-guided client Initializer list at RVAs 0x1640C0, 0x1640F0,
// 0x164120. All records and destinations must belong to the caller.
// Never invoke a callback address taken from the original binary.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Native Win32 ABI only
#endif
#include <cstddef>
#include <array>
namespace sin_independent {
using CreateClientObject=void* (__cdecl *)();
using DestroyClientObject=void (__cdecl *)(void*);
struct ClientInitializer {
    void** destination=nullptr;
    CreateClientObject create=nullptr;
    DestroyClientObject destroy=nullptr;
    ClientInitializer* next=nullptr;
};
static_assert(sizeof(ClientInitializer)==16 &&
    offsetof(ClientInitializer,destination)==0 &&
    offsetof(ClientInitializer,create)==4 &&
    offsetof(ClientInitializer,destroy)==8 &&
    offsetof(ClientInitializer,next)==12,"original x86 initializer record");
enum class InitializerState { Pending, Initializing, Ready, Freeing, Failed, Freed };
enum class InitializerResult { Initialized, InvalidList, AlreadyAttempted, RequiredCreationFailed, InvalidOptionalResult };
template<std::size_t Capacity>
class ClientInitializers {
public:
    static_assert(Capacity>0);
    ClientInitializers()=default;
    ClientInitializers(const ClientInitializers&)=delete;
    ClientInitializers& operator=(const ClientInitializers&)=delete;
    InitializerState State() const noexcept{return state_;}
    ClientInitializer* Head() const noexcept{return head_;}
    bool Add(ClientInitializer& record) noexcept {
        if(state_!=InitializerState::Pending || count_==Capacity ||
           !record.create || (record.destination&&!record.destroy) || record.next)
            return false;
        std::size_t walked=0;
        for(auto* p=head_;p;p=p->next) {
            if(walked++==count_ || p==&record ||
               (p->destination && p->destination==record.destination))
                return false;
        }
        if(walked!=count_)return false;
        record.next=head_;head_=&record;++count_;
        return true;
    }
    InitializerResult Initialize() noexcept {
        if(state_!=InitializerState::Pending)return InitializerResult::AlreadyAttempted;
        // Snapshot the owned list before any callbacks, rejecting an
        // externally damaged chain or nonempty destination. Original
        // required globals start null. Caller must not modify records
        // or destination ownership during the callbacks.
        std::size_t n=0;
        for(auto* p=head_;p;p=p->next) {
            if(n==Capacity || !p->create ||
               (p->destination && (!p->destroy || *p->destination)))
                return InitializerResult::InvalidList;
            for(std::size_t i=0;i<n;++i)
                if(nodes_[i]==p || (p->destination && nodes_[i]->destination==p->destination))
                    return InitializerResult::InvalidList;
            nodes_[n++]=p;
        }
        if(n!=count_)return InitializerResult::InvalidList;
        state_=InitializerState::Initializing;
        for(std::size_t i=0;i<n;++i) {
            auto* p=nodes_[i];
            void* created=p->create();
            if(created && p->destination) {
                *p->destination=created;
            } else if(!created && !p->destination) {
                // Original function-only initializer returns null.
                continue;
            } else {
                // A non-null optional result is invalid rather than
                // dereferencing a null destination as the original would.
                if(created && p->destroy)p->destroy(created);
                const auto result=created?InitializerResult::InvalidOptionalResult:
                    InitializerResult::RequiredCreationFailed;
                Cleanup(InitializerState::Failed);
                return result;
            }
        }
        state_=InitializerState::Ready;
        return InitializerResult::Initialized;
    }
    bool Free() noexcept {
        if(state_!=InitializerState::Ready)return false;
        Cleanup(InitializerState::Freed);
        return true;
    }
private:
    void Cleanup(InitializerState next) noexcept {
        state_=InitializerState::Freeing;
        // Original frees from list head forward, not reverse order,
        // and invokes each required deleter even for null destinations'
        // values after a partial creation failure.
        for(std::size_t i=0;i<count_;++i) {
            auto* p=nodes_[i];
            if(p->destination) {
                p->destroy(*p->destination);
                *p->destination=nullptr;
            }
        }
        state_=next;
    }
    std::array<ClientInitializer*,Capacity> nodes_{};
    ClientInitializer* head_=nullptr;
    std::size_t count_=0;
    InitializerState state_=InitializerState::Pending;
};
} // namespace sin_independent
