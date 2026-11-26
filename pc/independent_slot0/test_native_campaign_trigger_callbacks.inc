// Real object-layout Spawn/Touch and exact event setter order.
    {
        struct Trigger {
            alignas(4) unsigned char storage[dynamic_trigger_extent]{};
            alignas(4) unsigned char manager[0x268]{};
            unsigned other=0,event=0;bool gate=false,player=true,available=true,raw_check=false;
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
            if(f.raw_check)assert(f.manager[0x11]==1&&f.order=="PnCM");
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
        // Native singleton leaf: the owner acquires an initialized
        // manager; only the recovered +11 byte is changed.
        auto reset=[&](unsigned end){
            std::memset(a,0,dynamic_trigger_extent);put(a,0,dynamic_trigger_vtable);put(a,0x378,0x3000u);
            put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));put(a,0xc0,3u);put(a,0x470,0x41480000u);
            put(a,0x474,7u);put(a,0x478,end);f.order.clear();
        };
        b.end_game=nullptr;
        b.difficulty_manager=[](void* p,void** out,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Trigger*>(p);assert(f.order=="PnC"&&!f.manager[0x11]);f.order+='M';*out=f.manager;*bytes=sizeof(f.manager);return 1;
        };
        std::memset(f.manager,0x5a,sizeof(f.manager));f.manager[0x11]=0;put(f.manager,0,0x2245cdacu);
        reset(257);f.raw_check=true;
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.native_manager&&r.manager_signaled&&r.native_removed&&f.order=="PnCMU");
        assert(f.manager[0x10]==0x5a&&f.manager[0x11]==1&&f.manager[0x12]==0x5a);
        DeferredRemovalResult dr{};assert(api->cleanup_deferred(&deferred,&dr)&&dr.released==1);
        reset(256);f.raw_check=false;f.manager[0x11]=0;
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&!r.native_manager&&!r.manager_signaled&&f.order=="PnCU"&&!f.manager[0x11]);
        assert(api->cleanup_deferred(&deferred,&dr));
        reset(257);put(f.manager,0,0u);
        assert(!api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.event_unavailable&&!r.manager_signaled&&!r.removed&&f.order=="PnCM"&&!queue.count);
        // Existing DLL-owned fixed policy, not a second accounting store.
        using FixedApi=sin_port::fixed_service::Api;
        const auto get_fixed=reinterpret_cast<sin_port::fixed_service::GetApi>(GetProcAddress(server,sin_port::fixed_service::export_name));assert(get_fixed);
        const FixedApi* fixed=get_fixed(sin_port::fixed_service::abi_version,sizeof(FixedApi));assert(fixed&&fixed->begin_campaign(2));
        sin_port::fixed_service::State initial{},observed{};
        assert(fixed->get_state(&initial)&&!initial.trigger_count&&!initial.completed);
        float metric=0,after=0;assert(fixed->get_metric(0,&metric));
        b.difficulty_manager=nullptr;b.fixed_campaign=true;
        reset(256);
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.fixed_observed&&!r.manager_signaled&&r.native_removed&&f.order=="PnCU");
        assert(fixed->get_state(&observed)&&observed.trigger_count==1&&!observed.completed&&observed.revision==initial.revision);
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.already_removed&&!r.fixed_observed&&f.order=="PnCUP");
        assert(fixed->get_state(&observed)&&observed.trigger_count==1);
        assert(api->cleanup_deferred(&deferred,&dr));
        reset(257);
        assert(api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&r.fixed_observed&&r.manager_signaled&&r.native_removed);
        assert(fixed->get_state(&observed)&&observed.trigger_count==2&&observed.completed&&observed.revision==initial.revision&&fixed->get_metric(0,&after)&&after==metric);
        assert(api->cleanup_deferred(&deferred,&dr)&&fixed->end_campaign());
        reset(257);
        assert(!api->campaign_trigger_touch(a,dynamic_trigger_extent,&f.other,&b,&r)&&f.order=="P"&&!queue.count);
    }

// Direct real-slot route from original-layout trigger storage, with no
// hosted Create/Set/Fire callbacks. Synthetic manager owns its event.
    {
        namespace fs=sin_port::fixed_service;
        struct Event {
            virtual ~Event()=default;
            virtual void s1(){} virtual void s2(){} virtual void s3(){} virtual void s4(){}
            virtual void s5(){} virtual void s6(){} virtual void s7(){} virtual void s8(){} virtual void s9(){}
            virtual void integer(const char* key,int value){
                order+=key;order+=';';if(!std::strcmp(key,"owner"))owner=value;
                if(!std::strcmp(key,"descriptor"))descriptor=value;
                if(!std::strcmp(key,"subject"))assert(value==-1);
                if(fail)throw 1;
            }
            virtual void real(const char* key,float value){
                order+=key;order+=';';if(!std::strcmp(key,"x"))x=value;
            }
            virtual void string(const char* key,const char* value){order+=key;order+=';';name=value;}
            std::string order,name;int owner=0,descriptor=0;float x=0;bool fail=false;
        };
        struct Manager {
            virtual ~Manager()=default;
            virtual void s1(){} virtual void s2(){} virtual void s3(){} virtual void s4(){} virtual void s5(){}
            virtual void* create(const char* name,bool force){
                assert(!force&&!live);++creates;kind=name;
                if(unavailable)return nullptr;
                live=true;event.order.clear();return &event;
            }
            virtual bool fire(void* object,bool no_broadcast){
                assert(live&&object==&event&&!no_broadcast);live=false;++fires;
                if(uncertain)throw 2;return fire_ok;
            }
            virtual void s8(){} virtual void s9(){}
            virtual void release(void* object){assert(live&&object==&event);live=false;++frees;}
            Event event;std::string kind;unsigned creates=0,fires=0,frees=0;
            bool live=false,unavailable=false,fire_ok=true,uncertain=false;
        };
        struct Fixture {
            alignas(4) unsigned char storage[dynamic_trigger_extent]{};
            Manager manager;unsigned player=0,owners=0,removes=0,ends=0;bool owner_ok=true;
        } f;
        const auto spawn=reinterpret_cast<EngineTriggerSpawn>(GetProcAddress(server,engine_trigger_spawn_export));
        const auto touch=reinterpret_cast<EngineTriggerTouch>(GetProcAddress(server,engine_trigger_touch_export));
        assert(spawn&&touch&&!GetProcAddress(client,engine_trigger_spawn_export)&&!GetProcAddress(client,engine_trigger_touch_export));
        CampaignTriggerBindings b{};b.context=&f;
        b.spawn_gate=[](void*,bool* gate) noexcept ->int{*gate=false;return 1;};
        b.base_spawn=[](void*,void* a) noexcept ->int{static_cast<unsigned char*>(a)[0x120]=8;return 1;};
        b.string_text=[](void*,unsigned,const char** name,unsigned* bytes) noexcept ->int{*name="checkpoint";*bytes=11;return 1;};
        b.is_player=[](void* p,void* other,bool* yes) noexcept ->int{*yes=other==&static_cast<Fixture*>(p)->player;return 1;};
        b.owner_word=[](void* p,int* owner) noexcept ->int{
            auto& f=*static_cast<Fixture*>(p);assert(f.manager.live&&f.manager.event.order.empty());++f.owners;
            *owner=0x5002;float x=7.f;std::memcpy(f.storage+0x2bc,&x,4);return f.owner_ok;
        };
        b.end_game=[](void* p) noexcept ->int{++static_cast<Fixture*>(p)->ends;return 1;};
        b.remove=[](void* p,void*) noexcept ->int{++static_cast<Fixture*>(p)->removes;return 1;};
        CampaignTriggerResult result{};int publication=99;auto* a=f.storage;
        put(a,0,dynamic_trigger_vtable);put(a,0xc0,1u);put(a,0x478,1u);
        assert(!touch(a,dynamic_trigger_extent,&f.player,&b,nullptr,&result,&publication)&&publication==engine_event_not_attempted);
        assert(touch(a,dynamic_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventFired);
        assert(result.event_fired&&result.manager_signaled&&result.removed&&f.owners==1&&f.removes==1&&f.ends==1);
        assert(f.manager.kind=="dynamic_difficulty"&&f.manager.event.owner==0x5002&&f.manager.event.x==7.f);
        assert(f.manager.event.order=="descriptor;owner;subject;name;decision;value;x;y;z;"&&f.manager.event.name=="checkpoint");
        f.manager.unavailable=true;
        assert(touch(a,dynamic_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventUnavailable&&result.event_unavailable&&result.removed);
        assert(f.owners==1&&f.removes==2&&f.ends==2&&!f.manager.frees);
        f.manager.unavailable=false;f.owner_ok=false;
        assert(!touch(a,dynamic_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventDiscarded);
        assert(f.owners==2&&f.manager.frees==1&&f.removes==2&&!f.manager.live);
        f.owner_ok=true;f.manager.event.fail=true;
        assert(!touch(a,dynamic_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventDiscarded&&f.manager.frees==2);
        f.manager.event.fail=false;f.manager.fire_ok=false;
        assert(touch(a,dynamic_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventFireReturnedFalse);
        assert(!result.event_fired&&result.removed&&f.removes==3&&f.ends==3&&f.manager.frees==2);
        f.manager.uncertain=true;
        assert(!touch(a,dynamic_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventFireUncertain&&f.removes==3&&f.manager.frees==2);
        f.manager.uncertain=false;f.manager.fire_ok=true;put(a,0,secret_trigger_vtable);
        assert(spawn(a,secret_trigger_extent,&b,&f.manager,&result,&publication)&&publication==fs::EventFired&&f.manager.event.descriptor==11);
        assert(f.manager.kind=="game"&&f.manager.event.owner==-1&&f.manager.event.order=="descriptor;owner;subject;name;");
        assert(touch(a,secret_trigger_extent,&f.player,&b,&f.manager,&result,&publication)&&publication==fs::EventFired&&f.manager.event.descriptor==12&&f.removes==4);
        const unsigned creates=f.manager.creates;
        assert(touch(a,secret_trigger_extent,a,&b,&f.manager,&result,&publication)&&result.nonplayer&&publication==engine_event_not_attempted&&f.manager.creates==creates);
    }
