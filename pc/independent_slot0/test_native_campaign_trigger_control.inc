// Authenticated inherited inputs traverse the derived map then CBaseTrigger.
    {
        struct Control {
            alignas(4) unsigned char actor[dynamic_trigger_extent]{};
            unsigned calls=0,refresh=0,expected=0,disabled=0;
        } f;
        TriggerControlBindings control{};control.context=&f;
        control.solid_flags=[](void* p,void* member,unsigned flags) noexcept ->int{
            auto& f=*static_cast<Control*>(p);assert(member==f.actor+0xfc&&flags==f.expected&&f.actor[0x408]==f.disabled);
            unsigned short v=static_cast<unsigned short>(flags);std::memcpy(f.actor+0x120,&v,2);++f.calls;return 1;
        };
        control.refresh=[](void* p,void* a,int arg) noexcept ->int{
            auto& f=*static_cast<Control*>(p);assert(a==f.actor&&!arg&&f.calls==f.refresh+1);
            unsigned short flags=0;std::memcpy(&flags,f.actor+0x120,2);assert(flags==f.expected);++f.refresh;return 1;
        };
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.campaign_trigger_control=&control;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==0x223edd00u?"Enable":token==0x223edce8u?"Disable":token==0x223d4914u?"Toggle":token==91?"tOgGlE":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        unsigned empty[5]{0,0,0,0xffffffffu,0};nw::ServerEntityAcceptInputResult result{};
        for(unsigned vt:{dynamic_trigger_vtable,secret_trigger_vtable}){
            const auto n=removal_entity_extent(vt);auto* a=f.actor;std::memset(a,0,sizeof(f.actor));
            put(a,0,vt);put(a,0x120,0xa5a50120u);a[0x408]=1;a[0x1f0]=1;f.calls=f.refresh=0;
            f.expected=0x128;f.disabled=0;
            assert(timer_accept->run(a,n,vt,0x223edd00u,nullptr,nullptr,empty,sizeof(empty),1,&input,&result)&&result.accepted&&result.native_handler&&result.native_maps==2&&result.row==3&&result.handler==0x22060810u&&result.dirty);
            assert(word(a,0x120)==0xa5a50128u&&!a[0x408]&&a[0x200]&&f.calls==1&&f.refresh==1);
            assert(timer_accept->run(a,n,vt,0x223edd00u,nullptr,nullptr,empty,sizeof(empty),2,&input,&result)&&result.accepted&&f.calls==1);
            f.expected=0x120;f.disabled=0;
            assert(timer_accept->run(a,n,vt,91,nullptr,nullptr,empty,sizeof(empty),3,&input,&result)&&result.accepted&&result.row==5&&result.handler==0x22060830u&&f.calls==2&&!a[0x408]);
            // Disable repairs the disabled byte even when active bit is already clear.
            assert(timer_accept->run(a,n,vt,0x223edce8u,nullptr,nullptr,empty,sizeof(empty),4,&input,&result)&&result.accepted&&result.row==4&&a[0x408]&&f.calls==2);
            f.expected=0x128;f.disabled=1;
            assert(timer_accept->run(a,n,vt,0x223d4914u,nullptr,nullptr,empty,sizeof(empty),5,&input,&result)&&result.accepted&&a[0x408]&&f.calls==3);
            f.expected=0x120;f.disabled=1;
            assert(timer_accept->run(a,n,vt,0x223edce8u,nullptr,nullptr,empty,sizeof(empty),6,&input,&result)&&result.accepted&&word(a,0x120)==0xa5a50120u&&f.calls==4&&f.refresh==4);
            TriggerControlResult direct{};
            auto bad=control;bad.refresh=nullptr;
            assert(!api->trigger_control(a,n,"Enable",&bad,&direct)&&a[0x408]&&f.calls==4);
            assert(!api->trigger_control(a,n-1,"Enable",&control,&direct)&&f.calls==4);
        }
    }

// Named event from a one-shot relay -> trigger Disable -> inherited Kill.
    {
        struct CampaignInput {
            alignas(4) unsigned char relay[relay_extent]{},trigger[dynamic_trigger_extent]{},attachment[4]{};
            sin_independent::EntityHandleEntry entries[3]{};
            ActionPool pool{};
            ConnectedOutput* co;
            void* slots[2]{};
            DeferredRemovalQueue queue{slots,2};
            std::string order;
            unsigned notices=0;
        } f{{},{},{},{},{},&co};
        auto* source=f.relay;auto* target=f.trigger;
        put(source,0,relay_vtable);put(source,0x378,0x3000u);put(source,0xb4,1u);source[0x1f0]=1;
        put(target,0,dynamic_trigger_vtable);put(target,0x378,0x4001u);put(target,0x120,8u);target[0x1f0]=1;
        put(target,0x20c,0x5002u);put(target,0xc0,201u);
        for(auto* a:{source,target}){put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));}
        void* actors[]{source,target,f.attachment};
        for(unsigned i=0;i<3;++i){f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=3+i;}
        OutputActionBindings actions=output;actions.context=&f.pool;actions.next_id=&f.pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=relay_extent;return 1;};
        keys.intern=[](void*,const char* text,unsigned* token) noexcept ->int{
            if(!std::strcmp(text,"checkpoint"))*token=201;
            else if(!std::strcmp(text,"Disable"))*token=0x223edce8u;
            else if(!std::strcmp(text,"Kill"))*token=0x2240b624u;
            else return 0;return 1;
        };
        // Prepend means Disable fires first at the same deadline.
        assert(api->key_value(source,"OnTrigger","checkpoint,Kill,,0,1",&keys,&kr));
        assert(api->key_value(source,"OnTrigger","checkpoint,Disable,,0,1",&keys,&kr));
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);for(unsigned i=0;i<2;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool.rows[i]))){*row=f.pool.rows[i];*bytes=0x1c;return 1;}return 0;
        };
        fire.release=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(f.co->locked&&f.pool.released<2&&row==f.pool.rows[1-f.pool.released]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));++f.pool.released;f.order+='O';return 1;
        };
        RelayQueueBindings route{};route.context=&f;route.root=co.root;route.root_bytes=sizeof(co.root);route.root_token=99;route.queue=&co.queue;route.entries=f.entries;route.entry_count=3;route.now=&co.now;
        route.handle=[](void* p,void* a,unsigned* handle) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);if(a==f.relay)*handle=0x3000u;else if(a==f.attachment)*handle=0x5002u;else return 0;return 1;
        };
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&f.queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(f.co->locked&&handler==0x221446f0u);
            if(a==f.relay){assert(f.order=="OO");f.order+='R';}
            else{assert(a==f.trigger&&f.order=="OORDFAC");f.order+='T';}return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(!f.co->locked&&f.queue.handle_access_disabled);
            const unsigned i=net==f.relay+0x160?0u:1u;assert(net==f.relay+0x160||net==f.trigger+0x160);
            f.entries[i].entity=nullptr;++f.entries[i].serial;f.order+=i?'Y':'X';return 1;
        };
        RelayBindings relay{};relay.output=&fire;relay.native_queue=&route;relay.native_fire=true;relay.deferred=&deferred;
        TriggerControlBindings control{};control.context=&f;
        control.solid_flags=[](void* p,void* member,unsigned flags) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(f.order=="OOR"&&f.co->locked&&member==f.trigger+0xfc&&!flags&&f.trigger[0x408]);
            unsigned short value=0;std::memcpy(f.trigger+0x120,&value,2);f.order+='D';return 1;
        };
        control.refresh=[](void* p,void* a,int arg) noexcept ->int{auto& f=*static_cast<CampaignInput*>(p);assert(a==f.trigger&&!arg&&f.order=="OORD");f.order+='F';return 1;};
        EntityKillBindings kill{};kill.context=&f;kill.deferred=&deferred;
        kill.attachment_notice=[](void* p,void* attached,void* a) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(attached==f.attachment&&a==f.trigger&&f.order=="OORDF");f.order+='A';return 1;
        };
        kill.clear_attachment=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(a==f.trigger&&f.order=="OORDFA");unsigned invalid=0xffffffffu;std::memcpy(f.trigger+0x20c,&invalid,4);f.order+='C';return 1;
        };
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.relay=&relay;input.campaign_trigger_control=&control;input.campaign_trigger_kill=&kill;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==0x22422b7cu?"Trigger":token==0x223edce8u?"Disable":token==0x2240b624u?"Kill":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntitySearchBindings search{};search.context=&f;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<CampaignInput*>(p);*bytes=a==f.relay?relay_extent:a==f.trigger?dynamic_trigger_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&f;delivery.entities.entries=f.entries;delivery.entities.count=3;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<CampaignInput*>(p)->co;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<CampaignInput*>(p)->co;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.find_named=[](void* p,void* prev,unsigned name,void* a,void* c,void** out) noexcept ->int{
            auto& f=*static_cast<CampaignInput*>(p);assert(f.co->locked&&name==201&&a==f.attachment&&c==f.relay);
            *out=prev?nullptr:f.trigger;return 1;
        };
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<CampaignInput*>(p)->co;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=280.f;
        co.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token==201||token==0x223edce8u||token==0x22422b7cu||token==0x2240b624u;return 1;};
        unsigned empty[5]{0,0,0,0xffffffffu,0};
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x3000u,0x22422b7cu,0x5002u,0x5002u,empty,sizeof(empty),0.f,61,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==3&&es.native_inputs==3&&!es.foreign_inputs&&!es.rejected_inputs&&co.released==3);
        assert(f.order=="OORDFACT"&&f.queue.count==2&&f.slots[0]==source+0x160&&f.slots[1]==target+0x160&&target[0x408]&&!word(target,0x120)&&word(target,0x20c)==0xffffffffu&&!word(target,0xc0)&&target[0x200]);
        EntityKillResult kr2{};assert(!api->logic_kill(target,dynamic_trigger_extent,dynamic_trigger_vtable,f.entries,3,&kill,&kr2));
        DeferredRemovalResult dr{};
        assert(api->cleanup_deferred(&deferred,&dr)&&dr.released==2&&f.order=="OORDFACTXY"&&!f.entries[0].entity&&!f.entries[1].entity);
    }
