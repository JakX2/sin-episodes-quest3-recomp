// Actual GameFrame slot4: prequeued removal, scheduled one-shot auto,
    // inherited Kill in the event queue, post-event destruction.
    {
        struct FrameRemoval {
            Fixture* frame;
            CampaignFixture* campaign;
            void* pre;
            sin_independent::EntityHandleEntry entries[3]{};
            void* slots[3]{};
            nm::DeferredRemovalQueue queue{slots,3};
            std::string order;
            unsigned passes=0;
        } removal{&f,&camp,af.actor};
        std::memset(af.actor,0,sizeof(af.actor));
        std::memset(camp.automatic,0,sizeof(camp.automatic));
        std::memset(camp.relay,0,sizeof(camp.relay));
        void* actors[]{camp.automatic,camp.relay,af.actor};
        for(unsigned i=0;i<3;++i){
            auto* a=actors[i];put(a,0,i==0?nm::auto_vtable:nm::relay_vtable);
            put(a,0x160,0x22454f00u);put(a,0x168,static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(a)));
            put(a,0x378,((3u+i)<<12)|i);put(a,0x20c,0xffffffffu);put(a,0xc0,77u);
            removal.entries[i].serial=3+i;removal.entries[i].entity=reinterpret_cast<CBaseEntity*>(a);
        }
        put(camp.automatic,0x74,82);put(camp.automatic,0xb4,1u);camp.relay[0x1f0]=1;
        nm::DeferredRemovalBindings deferred{};deferred.context=&removal;deferred.queue=&removal.queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& r=*static_cast<FrameRemoval*>(p);assert(handler==0x221446f0u);
            if(a==r.pre){assert(!r.frame->depth&&!r.passes);r.order+='p';}
            else if(a==r.campaign->automatic){assert(r.frame->depth==1&&r.passes==2);r.order+='a';}
            else {assert(a==r.campaign->relay&&r.campaign->locked&&!r.frame->depth&&r.passes==2);r.order+='r';}
            r.queue.chained_update=true;return 1;
        };
        deferred.pre_cleanup=[](void* p) noexcept ->int{
            auto& r=*static_cast<FrameRemoval*>(p);assert(!r.frame->depth&&!r.campaign->locked&&r.queue.cleanup_active&&!r.queue.handle_access_disabled);++r.passes;r.order+='C';return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& r=*static_cast<FrameRemoval*>(p);assert(!r.frame->depth&&!r.campaign->locked&&r.queue.handle_access_disabled);
            auto* a=static_cast<unsigned char*>(net)-0x160;unsigned i=3;
            if(a==r.campaign->automatic)i=0;else if(a==r.campaign->relay)i=1;else if(a==r.pre)i=2;
            unsigned flags=0,name=0;std::memcpy(&flags,a+0xb8,4);std::memcpy(&name,a+0xc0,4);
            assert(i<3&&r.entries[i].entity==reinterpret_cast<CBaseEntity*>(a)&&(flags&1u)&&!name);
            assert(r.passes==(i==2?1u:3u));
            r.order+=i==0?'A':i==1?'R':'P';
            r.entries[i].entity=nullptr;++r.entries[i].serial;
            if(i==0){r.frame->slots[0]=nullptr;std::memset(a,0xdd,nm::auto_extent);}
            else std::memset(a,0xdd,nm::relay_extent);
            return 1;
        };
        nm::DeferredRemovalResult dr{};
        assert(map->defer_logic_remove(af.actor,sizeof(af.actor),&deferred,&dr)&&dr.queued);
        nm::AutoBindings startup{};startup.context=&camp;startup.load_type=&camp.load;startup.output=&fire;startup.deferred=&deferred;
        startup.low_violence=[](void*,bool* low) noexcept ->int{*low=false;return 1;};
        auto primary=cp;primary.behavior=&startup;primary.entries=removal.entries;primary.entry_count=3;
        nm::EntityKillBindings kill{};kill.deferred=&deferred;
        ServerEntityAcceptInputBindings killer{};killer.storage=&storage;killer.logic_kill=&kill;
        killer.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token!=0x2240b624u)return 0;*text="Kill";*bytes=5;return 1;
        };
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& q=*static_cast<CampaignFixture*>(p);*bytes=a==q.relay?nm::relay_extent:a==q.automatic?nm::auto_extent:0;return *bytes!=0;
        };
        events.entities.entries=removal.entries;events.entities.count=3;events.native_input=&killer;
        camp.event_alloc=camp.event_releases=0;std::memset(camp.root,0,sizeof(camp.root));std::memset(camp.events,0,sizeof(camp.events));
        camp.queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token==0x2240b624u;return 1;};
        b.logic_timer=nullptr;b.logic_auto=&primary;b.deferred=&deferred;b.cleanup=nullptr;
        f.g.curtime=10.25f;f.g.tick=82;f.skip=false;f.transition=false;f.double_step=0;
        f.slots[0]=camp.automatic;f.slots[1]=f.slots[2]=nullptr;
        unsigned empty[5]{0,0,0,0xffffffffu,0};ServerEntityNamedEventQueueResult event{};
        assert(camp.api->enqueue_direct(camp.root,sizeof(camp.root),99,f.g.curtime,0x4001u,0x2240b624u,0x3000u,0x3000u,empty,sizeof(empty),0.f,50,&camp.queue,&event));
        nm::DeferredRemovalQueue wrong_queue{};auto wrong=deferred;wrong.queue=&wrong_queue;startup.deferred=&wrong;
        assert(!boundary(true,&state,&b,&out)&&!out.cleanup_calls&&removal.order=="p"&&removal.queue.count==1&&!removal.passes);
        startup.deferred=&deferred;
        assert(boundary(true,&state,&b,&out)&&out.completed&&out.cleanup_calls==3&&out.native_cleanup_calls==3&&out.native_released==3);
        assert(out.native_auto==1&&out.logic_auto.invoked&&out.logic_auto.behavior.removed&&out.events.native_inputs==1&&out.events.events==1&&camp.event_releases==1);
        assert(removal.order=="pCPCarCAR"&&!removal.queue.count&&!removal.queue.handle_access_disabled&&removal.passes==3);
        assert(!removal.entries[0].entity&&!removal.entries[1].entity&&!removal.entries[2].entity&&!f.slots[0]&&!f.depth&&out.iteration_closed);
    }
