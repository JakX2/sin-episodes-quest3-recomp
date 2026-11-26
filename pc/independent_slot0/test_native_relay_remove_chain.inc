// Direct input -> relay's native OnTrigger -> deferred self-removal ->
    // downstream native input. No hosted enqueue, refire or remove.
    {
        alignas(4) unsigned char source[relay_extent]{},target[relay_extent]{},activator[4]{},save[autosave_extent]{};
        put(source,0,relay_vtable);put(source,0x378,0x3000u);put(source,0x160,0x22454f00u);put(source,0x168,ptrword(source));
        put(source,0xb4,1u);put(source,0xc0,55u);source[0x1f0]=1;
        put(target,0,relay_vtable);put(target,0x378,0x4001u);target[0x1f0]=1;
        put(save,0,autosave_vtable);put(save,0x378,0x6003u);put(save,0x160,0x22454f00u);put(save,0x168,ptrword(save));put(save,0x20c,0xffffffffu);
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=relay_extent;return 1;};
        assert(api->key_value(source,"OnTrigger","relay,Use,,0,1",&keys,&kr));
        sin_independent::EntityHandleEntry entries[4]{};
        void* actors[]{source,target,activator,save};
        for(unsigned i=0;i<4;++i){entries[i].serial=3+i;entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);}
        struct Chain {
            ConnectedOutput* co;ActionPool* pool;void* source;void* target;void* activator;void* save;
            sin_independent::EntityHandleEntry* entries;void* slots[2]{};
            DeferredRemovalQueue queue{slots,2};
            std::string order;
        } f{&co,&pool,source,target,activator,save,entries};
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);if(token!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[0])))return 0;*row=f.pool->rows[0];*bytes=0x1c;return 1;
        };
        fire.release=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(f.co->locked&&f.co->alloc==2&&row==f.pool->rows[0]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));f.order+='O';++f.pool->released;return 1;
        };
        RelayQueueBindings route{};route.context=&f;route.root=co.root;route.root_bytes=sizeof(co.root);route.root_token=99;route.queue=&co.queue;route.entries=entries;route.entry_count=4;route.now=&co.now;
        route.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{auto& f=*static_cast<Chain*>(p);if(a==f.source)*handle=0x3000u;else if(a==f.activator)*handle=0x5002u;else return 0;return 1;};
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&f.queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(handler==0x221446f0u&&f.co->locked);
            if(a==f.source){assert(f.order=="O"&&f.co->alloc==2);f.order+='U';}
            else{assert(a==f.save&&f.order=="OUF");f.order+='S';}return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(!f.co->locked&&f.queue.handle_access_disabled);
            const auto* a=static_cast<unsigned char*>(net)-0x160;
            const unsigned i=a==f.source?0u:3u;assert(a==f.source||a==f.save);
            f.entries[i].entity=nullptr;++f.entries[i].serial;f.order+=i==0?'F':'D';return 1;
        };
        RelayBindings relay{};relay.output=&fire;relay.native_queue=&route;relay.native_fire=true;relay.deferred=&deferred;
        AutosaveBindings autosave{};autosave.deferred=&deferred;
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.relay=&relay;input.logic_autosave=&autosave;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token==0x22422b7cu)*text="Trigger";else if(token==203)*text="Disable";else if(token==0x2240b624u)*text="Kill";else return 0;
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<Chain*>(p);*bytes=a==f.source||a==f.target?relay_extent:a==f.save?autosave_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=4;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<Chain*>(p)->co;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<Chain*>(p)->co;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** out) noexcept ->int{
            auto& f=*static_cast<Chain*>(p);assert(f.co->locked&&name==201&&a==f.activator&&c==f.source&&f.order=="OU"&&f.queue.count==1);*out=prev?nullptr:f.target;return 1;
        };
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<Chain*>(p)->co;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=240.f;
        co.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token==201||token==203||token==0x22422b7cu||token==0x2240b624u;return 1;};
        unsigned empty[5]{0,0,0,0xffffffffu,0};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x3000u,0x22422b7cu,0x5002u,0x5002u,empty,sizeof(empty),0.f,51,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==2&&es.native_inputs==2&&!es.foreign_inputs&&!es.rejected_inputs&&co.released==2);
        assert(f.order=="OU"&&f.queue.count==1&&target[0x3ac]==1&&pool.released==1&&!word(source,0x3a8)&&!word(source,0xc0)&&!source[0x3ad]&&source[0x200]);
        DeferredRemovalResult r{};
        assert(api->cleanup_deferred(&deferred,&r)&&r.released==1&&f.order=="OUF"&&!entries[0].entity);
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x6003u,0x2240b624u,0x5002u,0x5002u,empty,sizeof(empty),0.f,52,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==1&&es.native_inputs==1&&f.order=="OUFS"&&f.queue.count==1);
        assert(api->cleanup_deferred(&deferred,&r)&&r.released==1&&f.order=="OUFSD"&&!entries[3].entity);
    }
