#pragma once
#include "native_npc_scene_playback.h"

namespace sin_port::map_host {
namespace history_npc=sin_port::native_weapon_tables;
// Persistent owner adapter for a script actor's history. The per-call map
// binding may be a transient copy, so keep only the stable retirement
// configuration; no reference to the map-owned registry is retained.
class ScriptResponseHistoryOwner {
public:
    ScriptResponseHistoryOwner(const history_npc::ScriptActorSelectionApi& selection,
        const history_npc::ResponseRecordApi& record,const history_npc::ScriptActorSelectionBindings& ownership,
        const history_npc::ResponseHistoryBindings& history) noexcept:
        selection_(selection),record_(record){
        retirement_.actor.selection.criteria=ownership.actor.selection.criteria;
        retirement_.actor.selection.host.context=ownership.actor.selection.host.context;
        retirement_.actor.selection.host.destroy=ownership.actor.selection.host.destroy;
        retirement_.owned=ownership.owned;
        history_.history=history;history_.context=this;history_.destroy=&drop;
    }
    ScriptResponseHistoryOwner(const ScriptResponseHistoryOwner&)=delete;
    ScriptResponseHistoryOwner& operator=(const ScriptResponseHistoryOwner&)=delete;
    bool ready()const noexcept{
        return selection_.version==history_npc::version&&selection_.size==sizeof(selection_)&&selection_.retire&&
            record_.version==history_npc::version&&record_.size==sizeof(record_)&&record_.record&&
            retirement_.actor.selection.host.destroy&&retirement_.owned.release&&history_.history.lookup&&
            history_.history.insert&&history_.history.entry&&history_.history.clock&&history_.history.notify;
    }
    // Caller must supply a newly selected, initialized, uniquely owned
    // response and a live expeditor. Native record -1 may have transferred
    // ownership to history (e.g. notification failure); never drop again.
    // Native 0 is preflight rejection and leaves ownership with us.
    int record(void* exp,const char* concept,void* response) noexcept{
        if(!ready())return 0; // configuration must be checked before speech
        const int status=record_.record(exp,0x7c,&history_,concept,response,0x124,1);
        if(status==0)(void)drop(this,response);
        return status==1;
    }
    // Use in ExpeditorLifecycleBindings.destroy_response after validating
    // that the history row belongs to this actor and allocator family.
    int retire_word(std::uint32_t word) noexcept{
        if(!word||!ready())return 0;
        return drop(this,reinterpret_cast<void*>(static_cast<std::uintptr_t>(word)));
    }
    static int __cdecl record_callback(void* p,void* exp,const char* concept,void* response) noexcept{
        return p?static_cast<ScriptResponseHistoryOwner*>(p)->record(exp,concept,response):0;
    }
    static int __cdecl retire_callback(void* p,std::uint32_t word) noexcept{
        return p?static_cast<ScriptResponseHistoryOwner*>(p)->retire_word(word):0;
    }
private:
    static int __cdecl drop(void* p,void* response) noexcept{
        auto& self=*static_cast<ScriptResponseHistoryOwner*>(p);
        return self.selection_.retire(&self.retirement_,response);
    }
    const history_npc::ScriptActorSelectionApi& selection_;
    const history_npc::ResponseRecordApi& record_;
    history_npc::ScriptActorSelectionBindings retirement_{};
    history_npc::ResponseRecordBindings history_{};
};
}
