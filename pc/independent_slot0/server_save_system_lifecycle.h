#pragma once
// Independently owned original-guided startup manager. No retail objects,
// original code addresses or placeholder gameplay callbacks are accepted.
// Original five SaveRestore handlers and six explicit system getters are
// identified by their call sites in the authenticated PE32 DLL.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Win32 original interface ABI only
#endif
namespace sin_independent::server_systems {

class SaveBlockHandler {
public:
    virtual const char* __thiscall Name() noexcept=0; // 0
    virtual void __thiscall PreSave(void*) noexcept=0; // 1
    virtual void __thiscall Save(void*) noexcept=0; // 2
    virtual void __thiscall WriteHeaders(void*) noexcept=0; // 3
    virtual void __thiscall PostSave() noexcept=0; // 4
    virtual void __thiscall PreRestore() noexcept=0; // 5
    virtual void __thiscall ReadHeaders(void*) noexcept=0; // 6
    virtual void __thiscall Restore(void*,bool) noexcept=0; // 7
    virtual void __thiscall PostRestore() noexcept=0; // 8
protected:
    ~SaveBlockHandler()=default;
};
class SaveBlockSet:public SaveBlockHandler {
public:
    virtual void __thiscall AddHandler(SaveBlockHandler*) noexcept=0; // 9
    virtual void __thiscall RemoveHandler(SaveBlockHandler*) noexcept=0; // 10
    virtual void __thiscall RestoreHandler(SaveBlockHandler*,int,void*,bool) noexcept=0; // 11
};
// SDK corroborates the five identities; actual original names are
// checked separately by the metadata inspector. Entity block is first.
enum class SaveKind : std::size_t { Entities, Physics, AI, Templates, Response, Count };
using SaveHandlers=std::array<SaveBlockHandler*,static_cast<std::size_t>(SaveKind::Count)>;

class SaveRegistration {
public:
    SaveRegistration()=default;
    SaveRegistration(const SaveRegistration&)=delete;
    SaveRegistration& operator=(const SaveRegistration&)=delete;
    bool Add(SaveBlockSet* set,const SaveHandlers& handlers) noexcept {
        if(!set || busy_ || active_)return false;
        for(std::size_t i=0;i<handlers.size();++i) {
            if(!handlers[i] || handlers[i]==set)return false;
            for(std::size_t j=0;j<i;++j)if(handlers[i]==handlers[j])return false;
        }
        set_=set;handlers_=handlers;busy_=true;
        for(auto* handler:handlers_)set_->AddHandler(handler);
        active_=true;busy_=false;
        return true;
    }
    bool Remove() noexcept {
        if(!active_ || busy_)return false;
        busy_=true;
        // Original DLLShutdown removes in reverse registration order.
        for(std::size_t i=handlers_.size();i>0;--i)
            set_->RemoveHandler(handlers_[i-1]);
        set_=nullptr;handlers_={};active_=false;busy_=false;
        return true;
    }
    bool Active() const noexcept{return active_;}
private:
    SaveBlockSet* set_=nullptr;
    SaveHandlers handlers_{};
    bool busy_=false,active_=false;
};

// Original server IGameSystem table starts with Init (not a destructor).
class GameSystem {
public:
    virtual bool __thiscall Init() noexcept=0; // 0
    virtual void __thiscall Shutdown() noexcept=0; // 1
    virtual void __thiscall LevelInitPreEntity() noexcept=0; // 2
    virtual void __thiscall LevelInitPostEntity() noexcept=0; // 3
    virtual void __thiscall LevelShutdownPreEntity() noexcept=0; // 4
    virtual void __thiscall LevelShutdownPostEntity() noexcept=0; // 5
    virtual void __thiscall OnSave() noexcept=0; // 6
    virtual void __thiscall OnRestore() noexcept=0; // 7
    virtual void __thiscall FrameUpdatePreEntityThink() noexcept=0; // 8
    virtual void __thiscall FrameUpdatePostEntityThink() noexcept=0; // 9
    virtual void __thiscall PreClientUpdate() noexcept=0; // 10
    virtual ~GameSystem()=default; // 11, original deleting destructor
};
class AutoGameSystem:public GameSystem {
public:
    AutoGameSystem* next=nullptr;
};
static_assert(sizeof(GameSystem)==4 && sizeof(AutoGameSystem)==8,
              "original x86 game-system prefix");

// Retail explicitly calls six getter functions in order. The third
// and fourth are kept as original call-site identities until their
// complete functionality is identified; never substitute placeholders.
using ExplicitSystems=std::array<GameSystem*,6>;
enum class SystemState { Pending, Initializing, Ready, Failed, ShuttingDown, Stopped };
enum class InitResult { Ready, RejectedInput, AlreadyAttempted, CallbackFailed };
struct CacheHooks {
    void (*enter)(void*) noexcept=nullptr;
    void (*leave)(void*) noexcept=nullptr;
    void* context=nullptr;
    bool Complete()const noexcept{return enter && leave;}
};

template<std::size_t Capacity>
class GameSystemList {
public:
    static_assert(Capacity>=6,"six original explicit systems");
    GameSystemList()=default;
    GameSystemList(const GameSystemList&)=delete;
    GameSystemList& operator=(const GameSystemList&)=delete;
    SystemState State()const noexcept{return state_;}
    std::size_t Count()const noexcept{return count_;}
    std::size_t InitializedCount()const noexcept{return initialized_;}
    std::size_t DuplicateAutos()const noexcept{return duplicates_;}
    GameSystem* At(std::size_t i)const noexcept{return i<count_?systems_[i]:nullptr;}

    bool AddExplicit(const ExplicitSystems& explicitSystems) noexcept {
        if(state_!=SystemState::Pending || count_)return false;
        for(std::size_t i=0;i<explicitSystems.size();++i) {
            if(!explicitSystems[i])return false;
            for(std::size_t j=0;j<i;++j)
                if(explicitSystems[i]==explicitSystems[j])return false;
        }
        for(auto* p:explicitSystems)systems_[count_++]=p;
        return true;
    }

    // Original late auto-construction appends directly after the
    // initialized flag is set. Allow this during Init; the loop must
    // observe the newly appended entry. Outside Init, do not claim
    // to have initialized a new system automatically.
    bool AppendDuringInit(GameSystem* p) noexcept {
        if(state_!=SystemState::Initializing || !p || count_==Capacity || Contains(p))
            return false;
        systems_[count_++]=p;
        return true;
    }

    InitResult Initialize(AutoGameSystem* autoHead) noexcept {
        if(state_!=SystemState::Pending)return InitResult::AlreadyAttempted;
        if(count_<6)return InitResult::RejectedInput;
        // Validate the linked chain and capacity before consuming it.
        // Retail appends unique autos in head-to-tail order and warns
        // when an existing pointer is encountered.
        auto candidate=systems_;
        std::size_t n=count_,dups=0,walked=0;
        for(auto* p=autoHead;p;p=p->next) {
            if(walked++==Capacity)return InitResult::RejectedInput;
            bool seen=false;
            for(std::size_t i=0;i<n;++i)if(candidate[i]==p){seen=true;break;}
            if(seen){++dups;continue;}
            if(n==Capacity)return InitResult::RejectedInput;
            candidate[n++]=p;
        }
        systems_=candidate;count_=n;duplicates_=dups;
        state_=SystemState::Initializing;
        // Original sets its initialized flag before invoking Init.
        // It returns false on the first failure with no automatic rollback.
        for(std::size_t i=0;i<count_;++i) {
            if(!systems_[i]->Init()) {
                state_=SystemState::Failed;
                return InitResult::CallbackFailed;
            }
            ++initialized_;
        }
        state_=SystemState::Ready;
        return InitResult::Ready;
    }

    // The original forward/reverse dispatch wraps each invocation in
    // engineCache critical-section calls. Restrict cleanup here to a
    // fully initialized owned system list; failed Init needs an owner
    // specific cleanup strategy rather than guessed virtual calls.
    bool Shutdown(const CacheHooks& cache) noexcept {
        if(state_!=SystemState::Ready || !cache.Complete())return false;
        state_=SystemState::ShuttingDown;
        for(std::size_t i=count_;i>0;--i) {
            cache.enter(cache.context);
            systems_[i-1]->Shutdown();
            cache.leave(cache.context);
        }
        state_=SystemState::Stopped;
        return true;
    }

private:
    bool Contains(GameSystem* p)const noexcept {
        for(std::size_t i=0;i<count_;++i)if(systems_[i]==p)return true;
        return false;
    }
    std::array<GameSystem*,Capacity> systems_{};
    std::size_t count_=0,initialized_=0,duplicates_=0;
    SystemState state_=SystemState::Pending;
};
} // namespace sin_independent::server_systems
