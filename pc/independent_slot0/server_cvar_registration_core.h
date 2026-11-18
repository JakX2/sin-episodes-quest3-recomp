#pragma once
// Original-guided independent console registration lifecycle.
// These records are NOT retail ConCommandBase/ConVar ABI overlays.
// No engine register call may receive their addresses in retail.
#include <cstddef>
#include <cstdint>
namespace sin_independent {
constexpr std::uint32_t kCvarUnregistered=0x1;
constexpr std::uint32_t kCvarGameDll=0x4;
constexpr std::uint32_t kCvarReplicated=0x2000;

struct ConsoleRecord {
    ConsoleRecord* next=nullptr;
    const char* name=nullptr;
    const char* defaultValue=nullptr;
    std::uint32_t flags=0;
    bool command=false;
    bool registered=false;
};
struct ConsoleHooks {
    void (*registerCommand)(ConsoleRecord&,void*) noexcept=nullptr;
    const char* (*commandLineValue)(const char*,void*) noexcept=nullptr;
    void (*setValue)(ConsoleRecord&,const char*,void*) noexcept=nullptr;
    ConsoleRecord* (*findEngineVar)(const char*,void*) noexcept=nullptr;
    void* context=nullptr;
    bool Complete() const noexcept {
        return registerCommand && commandLineValue && setValue && findEngineVar;
    }
};
enum class ConsoleInitResult {
    Initialized, InvalidInput, InvalidList, AlreadyInitialized, Busy, MissingSvCheats
};
inline unsigned char AsciiFold(unsigned char c) noexcept {
    return c>='A' && c<='Z' ? static_cast<unsigned char>(c-'A'+'a') : c;
}
inline bool EqualConsoleNames(const char* a,const char* b) noexcept {
    if(!a || !b)return false;
    while(*a && *b && AsciiFold(static_cast<unsigned char>(*a))==
                       AsciiFold(static_cast<unsigned char>(*b))) {++a;++b;}
    return AsciiFold(static_cast<unsigned char>(*a))==AsciiFold(static_cast<unsigned char>(*b));
}
inline ConsoleRecord* FindConsoleRecord(ConsoleRecord* head,const char* name,
                                          std::size_t bound) noexcept {
    for(std::size_t i=0;head && i<bound;++i,head=head->next)
        if(EqualConsoleNames(head->name,name))return head;
    return nullptr;
}
// A manager first caches the successor, then invokes registration; the
// accessor detaches the current record and the engine may relink it.
// The hooks must keep every original record and saved default alive
// throughout registration. They own the engine-side links and values.
class ServerConsoleRegistration {
public:
    bool Ready() const noexcept {return initialized_;}
    ConsoleRecord* Developer() const noexcept {return developer_;}
    ConsoleRecord* SvCheats() const noexcept {return cheats_;}

    ConsoleInitResult Initialize(ConsoleRecord* head,std::size_t bound,
                                   const ConsoleHooks& hooks) noexcept {
        if(busy_)return ConsoleInitResult::Busy;
        if(initialized_)return ConsoleInitResult::AlreadyInitialized;
        if(!hooks.Complete() || !bound)return ConsoleInitResult::InvalidInput;
        std::size_t n=0;
        for(auto* p=head;p;p=p->next) {
            // Bounded validation before mutation; a cycle necessarily
            // exceeds the bound. Already registered nodes are skipped.
            if(n++==bound || !p->name || !*p->name)
                return ConsoleInitResult::InvalidList;
        }

        busy_=true;
        for(auto* p=head;p;) {
            auto* next=p->next;
            if(!p->registered) {
                p->flags|=kCvarGameDll;
                const bool replicated=(p->flags&kCvarReplicated)!=0;
                const char* preserved=replicated&&!p->command?p->defaultValue:nullptr;
                p->next=nullptr;
                hooks.registerCommand(*p,hooks.context);
                const char* value=hooks.commandLineValue(p->name,hooks.context);
                if(!p->command) {
                    if(value)hooks.setValue(*p,value,hooks.context);
                    else if(replicated && preserved)hooks.setValue(*p,preserved,hooks.context);
                }
                p->registered=true;
            }
            p=next;
        }
        // Original InitializeCvars then calls ICvar's const FindVar
        // for developer. DLLInit searches its current linked list
        // for sv_cheats, and fails if absent.
        developer_=hooks.findEngineVar("developer",hooks.context);
        cheats_=FindConsoleRecord(head,"sv_cheats",bound);
        // Registration has side effects even if sv_cheats was absent.
        // Do not allow a second registration pass after partial failure.
        initialized_=true;
        busy_=false;
        return cheats_ ? ConsoleInitResult::Initialized : ConsoleInitResult::MissingSvCheats;
    }
private:
    bool busy_=false,initialized_=false;
    ConsoleRecord* developer_=nullptr;
    ConsoleRecord* cheats_=nullptr;
};
} // namespace sin_independent
