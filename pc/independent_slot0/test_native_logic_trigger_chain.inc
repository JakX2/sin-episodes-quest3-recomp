// Included in the map-entity regression main; shares the established fixture.
    {
        alignas(4) unsigned char actor[timer_extent]{},selector[branch_extent]{},target[relay_extent]{};
        put(actor,0,timer_vtable);put(actor,0x378,0x4001u);actor[0x1f0]=1;
        put(selector,0,branch_vtable);put(selector,0x378,0x5002u);selector[0x1f0]=1;
        put(target,0,relay_vtable);put(target,0x378,0x3000u);target[0x1f0]=1;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        struct TriggerChain {ConnectedOutput* queue;ActionPool* pool;void* actor;void* selector;void* target;unsigned scheduled=0;float next=0;} f{&co,&pool,actor,selector,target};
        auto keys=kb;keys.context=&f;keys.native_output=&actions;
        keys.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<TriggerChain*>(p);*bytes=a==f.actor?timer_extent:a==f.selector?branch_extent:0;return *bytes!=0;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"selector"))*token=201;
            else if(!std::strcmp(text,"ToggleTest"))*token=0x224207f0u;
            else if(!std::strcmp(text,"relay"))*token=202;
            else if(!std::strcmp(text,"Disable"))*token=0x223edce8u;
            else if(!std::strcmp(text,"Enable"))*token=0x223edd00u;
            else return 0;return 1;
        };
        assert(api->key_value(actor,"RefireTime","2",&keys,&kr)&&api->key_value(actor,"OnTimer","selector,ToggleTest,,0,-1",&keys,&kr));
        assert(api->key_value(selector,"OnTrue","relay,Disable,,0,-1",&keys,&kr)&&api->key_value(selector,"OnFalse","relay,Enable,,0,-1",&keys,&kr));
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<TriggerChain*>(p);for(unsigned i=0;i<f.pool->used;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        RelayQueueBindings route{};route.context=&f;route.root=co.root;route.root_token=99;route.queue=&co.queue;
        route.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{auto& f=*static_cast<TriggerChain*>(p);if(a==f.actor)*handle=0x4001u;else if(a==f.selector)*handle=0x5002u;else if(a==f.target)*handle=0x3000u;else return 0;return 1;};
        TimerBindings timer_behavior{};timer_behavior.context=&f;timer_behavior.output=&fire;timer_behavior.now=&co.now;timer_behavior.native_queue=&route;
        timer_behavior.schedule=[](void* p,void* a,float time) noexcept ->int{auto& f=*static_cast<TriggerChain*>(p);assert(a==f.actor);++f.scheduled;f.next=time;return 1;};
        BranchBindings branch_behavior{&fire,&route};RelayBindings relay_behavior{};
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.timer=&timer_behavior;input.branch=&branch_behavior;input.relay=&relay_behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            switch(token){
                case 0x22420054u:*text="FireTimer";break;
                case 0x224207f0u:*text="ToggleTest";break;
                case 0x223edce8u:*text="Disable";break;
                case 0x223edd00u:*text="Enable";break;
                default:return 0;
            }
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        sin_independent::EntityHandleEntry entries[3]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(target);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        entries[2].serial=5;entries[2].entity=reinterpret_cast<CBaseEntity*>(selector);
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<TriggerChain*>(p);*bytes=a==f.actor?timer_extent:a==f.selector?branch_extent:a==f.target?relay_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=3;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<TriggerChain*>(p)->queue;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<TriggerChain*>(p)->queue;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** out) noexcept ->int{auto& f=*static_cast<TriggerChain*>(p);assert(f.queue->locked&&a==f.actor&&(name==201||name==202)&&c==(name==201?f.actor:f.selector));*out=prev?nullptr:(name==201?f.selector:f.target);return 1;};
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<TriggerChain*>(p)->queue;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        unsigned empty[5]{0,0,0,0xffffffffu,0};
        for(unsigned i=0;i<2;++i){
            std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=200.f+2.f*i;
            assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,0x22420054u,0x3000,0x3000,empty,sizeof(empty),0.f,20+i,&co.queue,&cancel_qr));
            assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==3&&es.native_inputs==3&&!es.rejected_inputs&&!es.foreign_inputs&&co.released==3&&!word(co.root,0x30));
            assert(selector[0x394]==1-i&&target[0x3ac]==1-i&&f.scheduled==i+1&&f.next==co.now+2.f);
            assert(word(co.rows[1],0x14)==41&&word(co.rows[1],0xc)==0x4001u&&word(co.rows[1],0x10)==0x4001u&&!word(co.rows[1],0x2c));
            assert(word(co.rows[2],0x14)==42+i&&word(co.rows[2],0xc)==0x4001u&&word(co.rows[2],0x10)==0x5002u&&!word(co.rows[2],0x2c));
        }
    }
