// Global DEAD transition and non-physics metadata after delete listeners.
    {
        struct Metadata {
            alignas(4) unsigned char actor[relay_extent]{},edict[8]{};
            sin_independent::EntityHandleEntry entry{};
            void* listener[1]{};
            unsigned listener_count=1,id=7;
            void* slots[1]{};
            DeferredRemovalQueue queue{slots,1};
            const sin_port::fixed_service::GlobalStateApi* global;
            std::string order;
            int expected=1,index=0;
            Metadata(const sin_port::fixed_service::GlobalStateApi* g):global(g){}
        } f(globals);
        assert(globals->clear());f.index=globals->add("removal_gate","se1_intro01",1);assert(f.index>=0);
        f.listener[0]=&f.id;
        DeletionListenerBindings listeners{};listeners.context=&f;listeners.entries=&f.entry;listeners.entry_count=1;
        listeners.slots=f.listener;listeners.count=&f.listener_count;listeners.capacity=1;
        listeners.deleted=[](void* p,void* listener,void* a) noexcept ->int{
            auto& f=*static_cast<Metadata*>(p);assert(a==f.actor&&listener==&f.id);
            unsigned flags=0;std::memcpy(&flags,f.actor+0xbc,4);assert(flags==0x20u);
            char name[64]{},level[64]{};int state=0;assert(f.global->read(f.index,name,64,level,64,&state)&&state==1);
            f.order+='N';return 1;
        };
        RemovalMetadataBindings metadata{};metadata.context=&f;
        metadata.string_text=[](void* p,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Metadata*>(p);assert(f.order=="N");f.order+='G';
            *text=token==71?"removal_gate":token==72?"missing_gate":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        metadata.cleanup_224=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Metadata*>(p);assert(a==f.actor&&f.order=="NG");
            char name[64]{},level[64]{};int state=0;assert(f.global->read(f.index,name,64,level,64,&state)&&state==f.expected);
            unsigned flags=0;std::memcpy(&flags,f.actor+0x78,4);assert(flags==0x41u);
            std::memset(f.actor+0x218,0,4);f.order+='C';return 1;
        };
        metadata.edict=[](void* p,unsigned token,void** edict,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Metadata*>(p);assert(f.order=="NGC"&&token==77);f.order+='E';*edict=f.edict;*bytes=8;return 1;
        };
        NativeRemovalUpdateBindings update{};update.context=&f;update.listeners=&listeners;update.metadata=&metadata;
        update.tail=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Metadata*>(p);assert(a==f.actor);unsigned flags=0;std::memcpy(&flags,f.actor+0x78,4);assert(flags==0x40u);
            f.order+='T';return 1;
        };
        DeferredRemovalBindings deferred{};deferred.queue=&f.queue;deferred.native_update=&update;
        auto init=[&](unsigned token,bool local,bool edict){
            std::memset(f.actor,0,sizeof(f.actor));std::memset(f.edict,0,sizeof(f.edict));f.order.clear();
            put(f.actor,0,relay_vtable);put(f.actor,0x160,0x22454f00u);put(f.actor,0x168,ptrword(f.actor));put(f.actor,0x378,0x3000u);
            put(f.actor,0x10,token);put(f.actor,0xbc,0x20u);put(f.actor,0x78,0x41u);put(f.actor,0x218,1u);put(f.actor,0x16c,edict?77u:0u);
            f.actor[0x1f0]=local?1:0;f.entry.serial=3;f.entry.entity=reinterpret_cast<CBaseEntity*>(f.actor);
        };
        DeferredRemovalResult dr{};
        init(71,false,true);f.expected=2;
        assert(api->defer_logic_remove(f.actor,sizeof(f.actor),&deferred,&dr)&&dr.metadata&&dr.edict_flag_added&&dr.global_dead&&dr.cleanup_224&&dr.flags_cleared&&dr.dirty_marked&&dr.queued);
        assert(f.order=="NGCET"&&word(f.actor,0xbc)==0x04000020u&&word(f.edict,0)==1&&!word(f.actor,0x218)&&!f.actor[0x200]);
        assert(api->discard_deferred(&f.queue,&dr)&&globals->set_state(f.index,1));
        assert(globals->set_updates_disabled(1));
        init(71,true,true);f.expected=1;
        assert(api->defer_logic_remove(f.actor,sizeof(f.actor),&deferred,&dr)&&dr.global_noop&&!dr.global_dead&&dr.dirty_marked);
        assert(f.order=="NGCT"&&f.actor[0x200]&&!word(f.edict,0));
        assert(api->discard_deferred(&f.queue,&dr)&&globals->set_updates_disabled(0));
        init(72,false,false);
        assert(api->defer_logic_remove(f.actor,sizeof(f.actor),&deferred,&dr)&&dr.global_noop&&!dr.edict_flag_added&&!dr.dirty_marked);
        assert(f.order=="NGCT"&&word(f.actor,0xbc)==0x20u&&globals->find("missing_gate")==-1);
        assert(api->discard_deferred(&f.queue,&dr)&&globals->clear());
    }
