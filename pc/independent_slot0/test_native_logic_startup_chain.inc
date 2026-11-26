// Included in map-entity test main; uses existing real queue APIs.
    {
        alignas(4) unsigned char starter[auto_extent]{},menu[menu_extent]{},relay[relay_extent]{};
        put(starter,0,auto_vtable);put(starter,0x378,0x4001u);put(starter,0xb4,1u);
        put(menu,0,menu_vtable);put(menu,0x394,menu_listener_vtable);put(menu,0x378,0x5002u);
        put(relay,0,relay_vtable);put(relay,0x378,0x3000u);relay[0x1f0]=1;
        ActionPool pool{};OutputActionBindings actions=output;actions.context=&pool;actions.next_id=&pool.id;
        struct Startup {ConnectedOutput* q;ActionPool* pool;void* starter;void* menu;void* relay;unsigned removed=0;int load=0;bool low=true;} f{&co,&pool,starter,menu,relay};
        auto keys=kb;keys.context=&f;keys.native_output=&actions;
        keys.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<Startup*>(p);*bytes=a==f.starter?auto_extent:a==f.menu?menu_extent:0;return *bytes!=0;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"relay"))*token=201;
            else if(!std::strcmp(text,"Enable"))*token=0x223edd00u;
            else if(!std::strcmp(text,"Disable"))*token=0x223edce8u;
            else return 0;return 1;
        };
        assert(api->key_value(starter,"OnNewGame","relay,Enable,,0,-1",&keys,&kr));
        assert(api->key_value(starter,"OnMapSpawn","relay,Disable,,0,-1",&keys,&kr));
        assert(api->key_value(starter,"OnViolenceLow","relay,Enable,,0,-1",&keys,&kr));
        assert(api->key_value(menu,"quitMousedOver","relay,Disable,,0,1",&keys,&kr));
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Startup*>(p);for(unsigned i=0;i<f.pool->used;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool->rows[i]))){*row=f.pool->rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& f=*static_cast<Startup*>(p);assert(row==f.pool->rows[3]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));++f.pool->released;return 1;};
        sin_independent::EntityHandleEntry entries[3]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(relay);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(starter);
        entries[2].serial=5;entries[2].entity=reinterpret_cast<CBaseEntity*>(menu);
        RelayQueueBindings route{};route.context=&f;route.root=co.root;route.root_bytes=sizeof(co.root);route.root_token=99;route.queue=&co.queue;route.entries=entries;route.entry_count=3;route.now=&co.now;
        route.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{auto& f=*static_cast<Startup*>(p);if(a==f.starter)*handle=0x4001u;else if(a==f.menu)*handle=0x5002u;else return 0;return 1;};
        AutoBindings startup{};startup.context=&f;startup.output=&fire;startup.now=&co.now;startup.load_type=&f.load;startup.native_queue=&route;
        startup.low_violence=[](void* p,bool* low) noexcept ->int{auto& f=*static_cast<Startup*>(p);assert(f.q->alloc==2);*low=f.low;return 1;};
        startup.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Startup*>(p);assert(a==f.starter&&f.q->alloc==3);++f.removed;return 1;};
        MenuBindings listener{};listener.context=&f;listener.output=&fire;listener.native_queue=&route;
        listener.event_name=[](void* p,void* event,const char** name,unsigned* bytes) noexcept ->int{assert(p==event);*name="gameui_highlighted";*bytes=static_cast<unsigned>(std::strlen(*name)+1);return 1;};
        listener.event_string=[](void* p,void* event,const char* key,const char* fallback,const char** value,unsigned* bytes) noexcept ->int{assert(p==event&&!std::strcmp(key,"buttonname")&&!*fallback);*value="quit";*bytes=5;return 1;};
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=210.f;
        AutoResult result{};MenuResult mr{};
        entries[1].serial=9;
        assert(!api->auto_think(starter,sizeof(starter),&startup,&result)&&!co.alloc&&!f.removed);
        entries[1].serial=4;
        assert(api->auto_think(starter,sizeof(starter),&startup,&result)&&result.completed&&result.outputs==3&&result.native_output_queued==3&&result.removed&&f.removed==1);
        assert(api->menu_event(menu,sizeof(menu),&f,&listener,&mr)&&mr.highlighted&&mr.native_output_queued==1&&mr.button_reads==8&&pool.released==1&&!word(menu,0x514));
        assert(co.alloc==4&&word(co.root,0x30)==1);
        for(unsigned i=0;i<4;++i){
            assert(word(co.rows[i],0x14)==41+i&&!word(co.rows[i],0x2c));
            assert(word(co.rows[i],0xc)==(i==3?0x5002u:0xffffffffu)&&word(co.rows[i],0x10)==(i==3?0x5002u:0x4001u));
        }
        RelayBindings relay_behavior{};nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.relay=&relay_behavior;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{if(token==0x223edd00u)*text="Enable";else if(token==0x223edce8u)*text="Disable";else return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;};
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<Startup*>(p);*bytes=a==f.relay?relay_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=entries;delivery.entities.count=3;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<Startup*>(p)->q;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<Startup*>(p)->q;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** out) noexcept ->int{auto& f=*static_cast<Startup*>(p);assert(f.q->locked&&name==201&&((!a&&c==f.starter)||(a==f.menu&&c==f.menu)));*out=prev?nullptr:f.relay;return 1;};
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<Startup*>(p)->q;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==4&&es.native_inputs==4&&!es.foreign_inputs&&!es.rejected_inputs&&co.released==4&&!word(co.root,0x30)&&relay[0x3ac]==1);
    }
