// Real object-layout Spawn/Touch and exact event setter order.
    {
        struct Trigger {
            alignas(4) unsigned char storage[dynamic_trigger_extent]{};
            unsigned other=0,event=0;bool gate=false,player=true,available=true;
            std::string order,name;
            int descriptor=-1,owner=0,subject=0,decision=0;
            float values[4]{};
            unsigned floats=0;
        } f;
        auto* a=f.storage;
        CampaignTriggerBindings b{};b.context=&f;
        b.spawn_gate=[](void* p,bool* gate) noexcept ->int{auto& f=*static_cast<Trigger*>(p);*gate=f.gate;f.order+='G';return 1;};
        b.base_spawn=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(a==f.storage);f.order+='B';unsigned token=3;std::memcpy(f.storage+0xc0,&token,4);f.storage[0x120]=8;return 1;
        };
        b.warning=[](void* p,const char* text) noexcept ->int{assert(!std::strcmp(text,"DD Trigger with don't destroy set\n"));static_cast<Trigger*>(p)->order+='W';return 1;};
        b.string_text=[](void* p,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);f.order+='n';*text=token==1?"":token==2?"classname":token==3?"renamed":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        b.is_player=[](void* p,void* a,bool* player) noexcept ->int{auto& f=*static_cast<Trigger*>(p);assert(a==&f.other);*player=f.player;f.order+='P';return 1;};
        b.special=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(a==f.storage);f.order+='X';int decision=-9;std::memcpy(f.storage+0x474,&decision,4);return 1;
        };
        b.create_event=[](void* p,const char* name,bool force,void** event) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);unsigned vt=0;std::memcpy(&vt,f.storage,4);
            assert(!force&&!std::strcmp(name,vt==dynamic_trigger_vtable?"dynamic_difficulty":"game"));
            f.order+='C';*event=f.available?&f.event:nullptr;return 1;
        };
        b.owner_word=[](void* p,int* value) noexcept ->int{auto& f=*static_cast<Trigger*>(p);f.order+='O';*value=0x5002;return 1;};
        b.set_int=[](void* p,void* event,const char* key,int value) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(event==&f.event);
            if(!std::strcmp(key,"descriptor")){f.order+='d';f.descriptor=value;}
            else if(!std::strcmp(key,"owner")){f.order+='o';f.owner=value;}
            else if(!std::strcmp(key,"subject")){f.order+='s';f.subject=value;}
            else if(!std::strcmp(key,"decision")){f.order+='i';f.decision=value;}
            else return 0;return 1;
        };
        b.set_string=[](void* p,void* event,const char* key,const char* value) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(event==&f.event&&!std::strcmp(key,"name"));f.order+='N';f.name=value;return 1;
        };
        b.set_float=[](void* p,void* event,const char* key,float value) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);const char* names[]{"value","x","y","z"};
            assert(event==&f.event&&f.floats<4&&!std::strcmp(key,names[f.floats]));f.order+='v';f.values[f.floats++]=value;return 1;
        };
        b.fire_event=[](void* p,void* event,bool no_broadcast) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(event==&f.event&&!no_broadcast);f.order+='F';return 1;
        };
        b.end_game=[](void* p) noexcept ->int{static_cast<Trigger*>(p)->order+='E';return 1;};
        b.remove=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Trigger*>(p);assert(a==f.storage);f.order+='R';return 1;};
        CampaignTriggerResult r{};
        put(a,0,dynamic_trigger_vtable);put(a,0xb4,1024u);put(a,0xc,2u);
        assert(api->campaign_trigger_spawn(a,dynamic_trigger_extent,&b,&r)&&r.warned&&r.base_spawned&&f.order=="GBW"&&!r.event_attempted);
        f.order.clear();f.gate=true;
        assert(api->campaign_trigger_spawn(a,dynamic_trigger_extent,&b,&r)&&r.spawn_suppressed&&r.removed&&f.order=="GR");
        f.gate=false;f.order.clear();f.player=false;
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.nonplayer&&f.order=="P");
        f.player=true;f.order.clear();put(a,0xc0,1u);put(a,0xb8,0x800u);put(a,0x470,0x41480000u);put(a,0x478,257u);
        put(a,0x2bc,0x3f800000u);put(a,0x2c0,0xc0000000u);put(a,0x2c4,0x40400000u);
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.special_called&&r.event_fired&&r.manager_signaled&&r.removed);
        assert(f.order=="PnnXCOdosNivvvvFER"&&f.name=="classname"&&f.descriptor==0&&f.owner==0x5002&&f.subject==-1&&f.decision==-9&&f.floats==4&&f.values[0]==12.5f&&f.values[1]==1.f&&f.values[2]==-2.f&&f.values[3]==3.f);
        f.order.clear();f.available=false;put(a,0x478,256u);
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.event_unavailable&&!r.manager_signaled&&r.removed&&f.order=="PnnXCR");
        f.order.clear();put(a,0x478,257u);
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.event_unavailable&&r.manager_signaled&&r.removed&&f.order=="PnnXCER");
        f.order.clear();f.available=true;put(a,0,secret_trigger_vtable);put(a,0xc0,1u);
        assert(api->campaign_trigger_spawn(a,secret_trigger_extent,&b,&r)&&r.event_fired&&f.order=="GnnBCdosNF"&&f.descriptor==11&&f.name=="classname"&&f.owner==-1&&f.subject==-1);
        f.order.clear();
        assert(api->campaign_trigger_touch(a,secret_trigger_extent,&f.other,&b,&r)&&r.event_fired&&r.removed&&f.order=="PnCdosNFR"&&f.descriptor==12&&f.name=="renamed"&&!r.special_called&&!r.manager_signaled);
        // Same original base update and networkable layout as logic
        // entities, but no claim of shared derived input/destructor maps.
        void* slots[1]{};DeferredRemovalQueue queue{slots,1};
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(a==f.storage&&handler==0x221446f0u);
            f.order+='U';return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(net==f.storage+0x160);f.order+='D';return 1;
        };
        b.remove=nullptr;b.deferred=&deferred;
        for(unsigned vt:{dynamic_trigger_vtable,secret_trigger_vtable}){
            const unsigned extent=removal_entity_extent(vt);
            assert(extent&&!logic_kill_extent(vt));
            std::memset(a,0,dynamic_trigger_extent);put(a,0,vt);put(a,0x378,0x3000u);put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));
            put(a,0xc0,3u);f.order.clear();f.gate=true;
            assert(api->campaign_trigger_spawn(a,extent,&b,&r)&&r.spawn_suppressed&&r.native_removed&&r.removed&&f.order=="GU"&&queue.count==1&&!word(a,0xc0));
            DeferredRemovalResult dr{};
            assert(api->cleanup_deferred(&deferred,&dr)&&dr.released==1&&f.order=="GUD"&&!queue.count);
            put(a,0xb8,0u);put(a,0xc0,3u);put(a,0x478,257u);f.order.clear();f.gate=false;f.available=false;
            assert(api->campaign_trigger_touch(a,extent,&f.other,&b,&r)&&r.event_unavailable&&r.native_removed&&r.removed&&queue.count==1);
            assert(f.order==(vt==dynamic_trigger_vtable?"PnCEU":"PnCU")&&!word(a,0xc0));
            assert(api->cleanup_deferred(&deferred,&dr)&&dr.released==1&&!queue.count);
        }
    }
