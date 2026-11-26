// Actual native map parser and original-shaped trigger allocations.
    {
        struct Triggers {
            alignas(4) unsigned char storage[2][dynamic_trigger_extent]{};
            unsigned allocated=0,base=0,initialized=0,foreign=0,accepted=0,released=0,cleared=0;
            sin_independent::EntityHandleEntry entries[2]{};
            RemovalRegistry reg{entries,2,{&entries[0],&entries[1]},{},2,0,false};
            RegistryRemovalBindings registry{};
            RegistryDestroyBindings base_destroy{};
            LogicDestroyBindings shared{};
            TriggerDestroyBindings destroy{};
            ActionPool pool{};
            const sin_port::native_map::Api* api;
            const TriggerFactoryBindings* factory=nullptr;
            void* slots[2]{};
            DeferredRemovalQueue queue{slots,2};
            alignas(4) unsigned char manager[0x268]{};
            unsigned events=0,spawned=0,warnings=0,notices=0,updates=0,freed=0,scopes=0,listener_count=1,listener=0,player=0;
            void* listeners[1]{&listener};
            int descriptor=-1,owner=0,subject=0;
            std::string name;
            Triggers(const sin_port::native_map::Api* a):api(a){}
            unsigned index(void* a) noexcept{if(a==storage[0])return 0;if(a==storage[1])return 1;assert(false);return 2;}
        } f(api);
        f.entries[0].next=&f.entries[1];f.entries[1].previous=&f.entries[0];
        put(f.manager,0,0x2245cdacu);
        FactoryDictionary d{};assert(api->register_factories(&d)&&api->register_campaign_triggers(&d)&&d.count==14);
        assert(!api->register_campaign_triggers(&d)&&d.count==14);
        const auto* dynamic=api->find_factory(&d,"TRIGGER_DYNAMIC_DIFFICULTY");
        const auto* secret=api->find_factory(&d,"trigger_secret");
        assert(dynamic&&dynamic->extent==0x47c&&dynamic->original_object==0x2250b404u&&dynamic->original_vtable==0x22437fccu);
        assert(secret&&secret->extent==0x470&&secret->original_object==0x2250b408u&&secret->original_vtable==0x22437fd8u);
        TriggerFactoryBindings trigger{};trigger.context=&f;
        trigger.allocate=[](void* p,unsigned n,void** a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(f.allocated<2&&n==(f.allocated?secret_trigger_extent:dynamic_trigger_extent));
            *a=f.storage[f.allocated++];*bytes=n;return 1;
        };
        trigger.base_construct=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(a==f.storage[f.base]&&f.allocated==f.base+1);
            const auto i=f.base++;auto* s=f.storage[i];
            unsigned vt=0x22454f00u,back=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(a)),handle=((3u+i)<<12)|i,invalid=0xffffffffu;
            std::memcpy(s+0x160,&vt,4);std::memcpy(s+0x168,&back,4);std::memcpy(s+0x378,&handle,4);
            std::memcpy(s+0x410,&invalid,4);
            for(unsigned off:{0x420u,0x438u,0x450u})std::memcpy(s+off,&invalid,4);
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(a);f.entries[i].serial=3+i;return 1;
        };
        trigger.initialize=[](void* p,void* a,const char* name) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);unsigned vt=0;std::memcpy(&vt,a,4);
            assert(a==f.storage[f.initialized]&&vt==(f.initialized?secret_trigger_vtable:dynamic_trigger_vtable)&&!std::strcmp(name,f.initialized?"trigger_secret":"trigger_dynamic_difficulty"));++f.initialized;return 1;
        };
        trigger.release_networkable=[](void* p,void* n) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(f.queue.handle_access_disabled&&f.released<2&&n==f.storage[f.released]+0x160);
            const auto i=f.released;LogicDestroyResult r{};
            if(!f.api->destroy_trigger_members(f.storage[i],i?secret_trigger_extent:dynamic_trigger_extent,1,&f.destroy,&r)||!r.completed||r.released!=1||!r.registry_unlinked||!r.trigger_vector_purged||!r.freed)return 0;
            ++f.released;return 1;
        };
        f.factory=&trigger;
        FactoryBindings factories{};factories.dictionary=&d;factories.campaign_triggers=&trigger;
        OutputActionBindings actions=output;actions.context=&f.pool;actions.next_id=&f.pool.id;
        KeyBindings keys{};keys.context=&f;keys.native_output=&actions;
        keys.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);*bytes=a==f.storage[0]?dynamic_trigger_extent:a==f.storage[1]?secret_trigger_extent:0;return *bytes!=0;
        };
        keys.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
            *token=!std::strcmp(value,"trigger_dynamic_difficulty")?71u:!std::strcmp(value,"trigger_secret")?72u:!std::strcmp(value,"secret01")?73u:!std::strcmp(value,"relay")?201u:!std::strcmp(value,"Trigger")?202u:74u;return 1;
        };
        keys.foreign=[](void* p,void*,const char* key,const char*,bool* handled) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(!std::strcmp(key,"model"));++f.foreign;*handled=true;return 1;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<Triggers*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char*,unsigned,const EntitySlice*) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(a==f.storage[f.accepted]);++f.accepted;return 1;
        };
        const char source[]="{ classname trigger_dynamic_difficulty EstimatedTime 12.5 EstimatedDifficulty -3 EndOfGame 257 spawnflags 1024 filtername filter01 StartDisabled 0 OnStartTouch \"relay,Trigger,,0,-1\" model *1 } { classname trigger_secret targetname secret01 OnEndTouchAll \"relay,Trigger,,0,-1\" model *2 }";
        sin_port::native_map::Result result{};
        assert(api->parse(source,sizeof(source),&parse,&result)&&result.completed&&result.native_creates==2&&f.accepted==2&&f.base==2&&f.initialized==2&&f.foreign==2&&f.cleared==2);
        assert(word(f.storage[0],0x470)==0x41480000u&&word(f.storage[0],0x474)==0xfffffffdu&&word(f.storage[0],0x478)==257&&word(f.storage[0],0xb4)==1024&&word(f.storage[1],0xc0)==73);
        assert(word(f.storage[0],0x40c)==74&&!f.storage[0][0x408]&&word(f.storage[0],0x428)==ptrword(f.pool.rows[0])&&word(f.storage[1],0x458)==ptrword(f.pool.rows[1]));
        assert(f.pool.used==2);
        // Verify the middle trigger output as well, without leaving a
        // third action allocated in this two-entity fixture.
        auto* secret_actor=f.storage[1];
        KeyResult key_result{};
        assert(api->key_value(secret_actor,"StartDisabled#1","1",&keys,&key_result)&&key_result.base_field&&secret_actor[0x408]==1);
        assert(api->key_value(secret_actor,"StartDisabled","0",&keys,&key_result)&&!secret_actor[0x408]);
        assert(!api->destroy_campaign_trigger(f.storage[0],dynamic_trigger_extent-1,f.storage[0]+0x160,&trigger)&&!f.released);
        f.registry.registry=&f.reg;f.base_destroy.registry=&f.registry;f.base_destroy.context=&f;f.base_destroy.native_outputs=true;
        f.base_destroy.prefix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);const auto i=f.index(a);assert(f.queue.handle_access_disabled&&f.entries[i].entity&&f.pool.released==i+1);return 1;
        };
        f.base_destroy.suffix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);const auto i=f.index(a);assert(!f.entries[i].entity&&f.entries[i].serial==4+i);return 1;
        };
        f.shared.context=&f;f.shared.native_base=&f.base_destroy;
        f.shared.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool.rows[i]))){*row=f.pool.rows[i];*bytes=0x1c;return 1;}return 0;
        };
        f.shared.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);const auto i=f.pool.released;assert(i<2&&f.entries[i].entity&&f.queue.handle_access_disabled&&row==f.pool.rows[i]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));++f.pool.released;return 1;
        };
        f.shared.free_actor=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);const auto i=f.index(a);assert(i==f.freed&&!f.entries[i].entity&&f.queue.handle_access_disabled);++f.freed;std::memset(a,0xdd,i?secret_trigger_extent:dynamic_trigger_extent);return 1;
        };
        f.destroy.shared=&f.shared;
        DeletionListenerBindings listeners{};listeners.context=&f;listeners.entries=f.entries;listeners.entry_count=2;
        listeners.slots=f.listeners;listeners.count=&f.listener_count;listeners.capacity=1;
        listeners.deleted=[](void* p,void* listener,void* a) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);const auto i=f.index(a);assert(listener==&f.listener&&i==f.notices&&f.entries[i].entity&&!f.freed);
            if(!i)assert(f.manager[0x11]==1&&f.events==2);else assert(f.events==3);
            ++f.notices;return 1;
        };
        NativeRemovalUpdateBindings update{};update.context=&f;update.listeners=&listeners;
        update.tail=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Triggers*>(p);assert(f.index(a)==f.updates&&f.notices==f.updates+1);++f.updates;return 1;};
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&f.queue;deferred.native_update=&update;
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);auto* a=static_cast<unsigned char*>(net)-0x160;const auto i=f.index(a);
            return f.api->destroy_campaign_trigger(a,i?secret_trigger_extent:dynamic_trigger_extent,net,f.factory);
        };
        CampaignTriggerBindings behavior{};behavior.context=&f;behavior.deferred=&deferred;
        behavior.spawn_gate=[](void*,bool* gate) noexcept ->int{*gate=false;return 1;};
        behavior.base_spawn=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);const auto i=f.index(a);assert(i==f.spawned);if(i)f.storage[i][0x120]=8;++f.spawned;return 1;
        };
        behavior.warning=[](void* p,const char* text) noexcept ->int{auto& f=*static_cast<Triggers*>(p);assert(!std::strcmp(text,"DD Trigger with don't destroy set\n"));++f.warnings;return 1;};
        behavior.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==71?"trigger_dynamic_difficulty":token==72?"trigger_secret":token==73?"secret01":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        behavior.is_player=[](void* p,void* a,bool* yes) noexcept ->int{auto& f=*static_cast<Triggers*>(p);*yes=a==&f.player;return 1;};
        behavior.create_event=[](void* p,const char* name,bool force,void** ev) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(!force&&!std::strcmp(name,f.events==1?"dynamic_difficulty":"game"));*ev=&f;return 1;
        };
        behavior.set_int=[](void* p,void* ev,const char* key,int value) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(ev==p);
            if(!std::strcmp(key,"descriptor"))f.descriptor=value;else if(!std::strcmp(key,"owner"))f.owner=value;
            else if(!std::strcmp(key,"subject"))f.subject=value;else if(!std::strcmp(key,"decision"))assert(value==-3);
            else return 0;return 1;
        };
        behavior.set_string=[](void* p,void* ev,const char* key,const char* value) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(ev==p&&!std::strcmp(key,"name"));f.name=value;return 1;
        };
        behavior.set_float=[](void*,void*,const char* key,float value) noexcept ->int{if(!std::strcmp(key,"value"))assert(value==12.5f);return 1;};
        behavior.owner_word=[](void*,int* value) noexcept ->int{*value=0x5002;return 1;};
        behavior.fire_event=[](void* p,void* ev,bool no_broadcast) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(ev==p&&!no_broadcast&&f.events<3&&f.descriptor==(f.events==0?11:f.events==1?0:12)&&f.subject==-1&&f.owner==(f.events==1?0x5002:-1));
            assert(f.name==(f.events==1?"trigger_dynamic_difficulty":"secret01"));++f.events;return 1;
        };
        behavior.difficulty_manager=[](void* p,void** out,unsigned* bytes) noexcept ->int{auto& f=*static_cast<Triggers*>(p);assert(f.events==2&&!f.manager[0x11]);*out=f.manager;*bytes=sizeof(f.manager);return 1;};
        unsigned spawn_listeners=0;const char* map_name="se1_intro01";
        DispatchBindings dispatch{};dispatch.context=&f;dispatch.entries=f.entries;dispatch.entry_count=2;dispatch.current_map=&map_name;
        dispatch.listener_count=&spawn_listeners;
        dispatch.scope=[](void* p,bool) noexcept ->int{++static_cast<Triggers*>(p)->scopes;return 1;};
        SpawnBindings spawn{};spawn.context=&f;spawn.extent=keys.extent;spawn.native_dispatch=&dispatch;spawn.campaign_triggers=&behavior;
        DispatchResult spawned{};
        assert(api->dispatch_spawn(f.storage[0],&spawn,&spawned)&&spawned.native_spawn&&!spawned.marked);
        assert(api->dispatch_spawn(f.storage[1],&spawn,&spawned)&&spawned.native_spawn&&f.spawned==2&&f.scopes==4&&f.warnings==1&&f.events==1);
        CampaignTriggerResult touched{};
        assert(api->campaign_trigger_touch(f.storage[0],dynamic_trigger_extent,&f.player,&behavior,&touched)&&touched.native_manager&&touched.native_removed&&f.events==2&&f.queue.count==1);
        assert(api->campaign_trigger_touch(f.storage[1],secret_trigger_extent,&f.player,&behavior,&touched)&&touched.native_removed&&f.events==3&&f.queue.count==2);
        assert(f.notices==2&&f.updates==2&&!f.released&&!f.freed&&!f.pool.released&&f.reg.entity_count==2);
        assert(sin_independent::EntityHandleView(f.entries,2).Resolve(0x3000u)&&sin_independent::EntityHandleView(f.entries,2).Resolve(0x4001u));
        DeferredRemovalResult removed{};
        assert(api->cleanup_deferred(&deferred,&removed)&&removed.released==2&&!f.queue.count&&f.released==2&&f.freed==2&&f.pool.released==2&&!f.reg.entity_count);
        assert(!sin_independent::EntityHandleView(f.entries,2).Resolve(0x3000u)&&!sin_independent::EntityHandleView(f.entries,2).Resolve(0x4001u)&&f.manager[0x11]==1);
    }
