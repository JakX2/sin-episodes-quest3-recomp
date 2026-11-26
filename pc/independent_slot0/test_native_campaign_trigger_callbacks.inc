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

// Inherited EndTouch resolves identity twice, removes a single duplicate,
// observes output before stale cleanup, and fires EndTouchAll last.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct EndFixture {
            alignas(4) unsigned char actor[dynamic_trigger_extent]{},others[3][0x37c]{};
            unsigned vector[8]{},actions[2][7]{};
            sin_independent::EntityHandleEntry entries[3]{};
            void* expected=nullptr;
            std::string order;
            unsigned expected_count=0;bool insert_stale=false,fail=false;
        } f;
        const auto end=reinterpret_cast<EndCampaignTriggerOutput>(GetProcAddress(server,end_touch_output_export));
        assert(end&&!GetProcAddress(client,end_touch_output_export));
        for(unsigned i=0;i<3;++i){
            put(f.others[i],0,0x224aec3cu);put(f.others[i],0x378,((4+i)<<12)|i);
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(f.others[i]);f.entries[i].serial=4+i;
        }
        for(unsigned i=0;i<2;++i){f.actions[i][0]=201+i;f.actions[i][1]=202;f.actions[i][4]=0xffffffffu;f.actions[i][5]=91+i;}
        CampaignEndTouchBindings b{};b.context=&f;b.entries=f.entries;b.entry_count=3;
        b.resolve_vector=[](void* p,unsigned token,void** storage,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<EndFixture*>(p);assert(token==8);*storage=f.vector;*bytes=sizeof(f.vector);return 1;
        };
        nw::ServerEntityVoidOutputBindings ob{};ob.context=&f;
        ob.resolve=[](void* p,unsigned token,void** action,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<EndFixture*>(p);assert(token==91||token==92);*action=f.actions[token-91];*bytes=0x1c;return 1;
        };
        ob.enqueue=[](void* p,void* activator,void* caller,const nw::ServerEntityVoidOutputAction* a) noexcept ->int{
            auto& f=*static_cast<EndFixture*>(p);assert(activator==f.expected&&caller==f.actor);
            unsigned count=0;std::memcpy(&count,f.actor+0x468,4);
            if(a->target==201){
                assert(count==f.expected_count);f.order+='E';
                if(f.insert_stale){
                    f.vector[count]=0xffffffffu;const unsigned next=count+1;
                    std::memcpy(f.actor+0x468,&next,4);f.insert_stale=false;
                }
                return !f.fail;
            }
            assert(a->target==202&&count==0);f.order+='A';return 1;
        };
        ob.release=[](void*,unsigned,void*) noexcept ->int{assert(false);return 0;};b.output=&ob;
        CampaignEndTouchResult r{};
        for(unsigned vt:{dynamic_trigger_vtable,secret_trigger_vtable}){
            const unsigned extent=vt==dynamic_trigger_vtable?dynamic_trigger_extent:secret_trigger_extent;
            std::memset(f.actor,0,sizeof(f.actor));put(f.actor,0,vt);put(f.actor,0x45c,8u);put(f.actor,0x460,8u);
            put(f.actor,0x468,5u);put(f.actor,0x46c,8u);put(f.actor,0x440,91u);put(f.actor,0x458,92u);
            const unsigned initial[]{0x4000,0x5001,0x4000,0x7002,0xffffffffu};
            std::memcpy(f.vector,initial,sizeof(initial));f.order.clear();
            f.expected=f.others[2];
            assert(end(f.actor,extent,f.expected,0x37c,&b,&r)&&r.completed&&!r.matched&&r.handle_reads==1&&r.count==5&&f.order.empty());
            f.expected=f.others[0];f.expected_count=4;f.insert_stale=true;
            assert(end(f.actor,extent,f.expected,0x37c,&b,&r)&&r.matched&&r.handle_reads==2&&r.removed&&r.stale_removed==3&&r.live_remaining==2&&r.count==2&&r.end_output&&!r.all_output&&f.order=="E");
            assert(f.vector[0]==0x5001&&f.vector[1]==0x4000);
            f.expected_count=1;
            assert(end(f.actor,extent,f.expected,0x37c,&b,&r)&&r.count==1&&r.live_remaining==1&&f.order=="EE");
            f.expected=f.others[1];f.expected_count=0;
            assert(end(f.actor,extent,f.expected,0x37c,&b,&r)&&r.count==0&&r.end_output&&r.all_output&&r.queued==2&&f.order=="EEEA");
            assert(end(f.actor,extent,f.expected,0x37c,&b,&r)&&!r.matched&&f.order=="EEEA");
            // Null identity matches the first stale row; remaining stale
            // rows are pruned only after OnEndTouch.
            put(f.actor,0x468,2u);f.vector[0]=0x7002;f.vector[1]=0xffffffffu;
            f.expected=nullptr;f.expected_count=1;
            assert(end(f.actor,extent,nullptr,0,&b,&r)&&r.removed&&r.stale_removed==1&&r.all_output&&f.order=="EEEAEA");
            // Refused output is a partial effect: row already removed,
            // no automatic retry or stale cleanup.
            put(f.actor,0x468,2u);f.vector[0]=0x4000;f.vector[1]=0xffffffffu;
            f.expected=f.others[0];f.expected_count=1;f.fail=true;
            assert(!end(f.actor,extent,f.expected,0x37c,&b,&r)&&r.removed&&!r.end_output&&!r.stale_removed&&r.count==1);
            f.fail=false;
        }
    }

// Original ordered +298 logical filter, including exact raw masks.
    {
        struct FilterFixture {
            alignas(4) unsigned char trigger[dynamic_trigger_extent]{},other[0x9b0]{},custom[4]{};
            sin_independent::EntityHandleEntry entries[2]{};
            const char* classname="FUNC_PUSHABLE";unsigned names=0,players=0,npcs=0,vehicles=0,customs=0;
            bool player=false,npc=false,vehicle=false,accept=true;
        } f;
        const auto filter=reinterpret_cast<TestCampaignTriggerFilter>(GetProcAddress(server,campaign_filter_export));
        const auto start=reinterpret_cast<FilteredStartCampaignOutput>(GetProcAddress(server,filtered_start_touch_export));
        assert(filter&&start&&!GetProcAddress(client,campaign_filter_export)&&!GetProcAddress(client,filtered_start_touch_export));
        put(f.trigger,0,dynamic_trigger_vtable);put(f.trigger,0x410,0xffffffffu);
        put(f.other,0,0x224aec3cu);put(f.other,0x9ac,0xffffffffu);
        CampaignFilterBindings b{};b.context=&f;b.entries=f.entries;b.entry_count=2;
        b.classname=[](void* p,void*,const char** s,unsigned* n) noexcept ->int{
            auto& f=*static_cast<FilterFixture*>(p);++f.names;*s=f.classname;*n=*s?static_cast<unsigned>(std::strlen(*s)+1):0;return 1;
        };
        b.is_player=[](void* p,void*,bool* yes) noexcept ->int{auto& f=*static_cast<FilterFixture*>(p);++f.players;*yes=f.player;return 1;};
        b.npc_condition=[](void* p,void*,bool* yes) noexcept ->int{auto& f=*static_cast<FilterFixture*>(p);++f.npcs;*yes=f.npc;return 1;};
        b.player_vehicle=[](void* p,void*,bool* yes) noexcept ->int{auto& f=*static_cast<FilterFixture*>(p);++f.vehicles;*yes=f.vehicle;return 1;};
        b.custom_filter=[](void* p,void* filter,void* other,bool* yes) noexcept ->int{
            auto& f=*static_cast<FilterFixture*>(p);assert(filter==f.custom&&other==f.other);++f.customs;*yes=f.accept;return 1;
        };
        CampaignFilterResult r{};
        auto run=[&](unsigned flags){put(f.trigger,0xb4,flags);return filter(f.trigger,sizeof(f.trigger),f.other,sizeof(f.other),&b,&r);};
        assert(run(0)&&!r.accepted&&!r.player_read);
        assert(run(64)&&r.accepted&&r.eligibility==64&&r.player&&!f.players);
        assert(run(1)&&!r.accepted);f.other[0xbc]=0x80;
        assert(run(1|2|4|8)&&r.accepted&&r.eligibility==1&&!f.names);
        f.other[0xbc]=0;f.other[0xbd]=0x20;
        assert(run(1|2|4|8)&&r.accepted&&r.eligibility==2&&!f.names);
        f.other[0xbd]=0;
        assert(run(4|8)&&r.accepted&&r.eligibility==4&&f.names==1);
        f.classname="func_other";put(f.other,0x204,6);
        assert(run(4|8)&&r.accepted&&r.eligibility==8&&f.names==2);
        assert(run(64|32)&&!r.accepted&&r.vehicle_reads==1);
        f.entries[0].entity=reinterpret_cast<CBaseEntity*>(f.custom);f.entries[0].serial=3;
        put(f.other,0x9ac,0x3000u);
        assert(run(64|32)&&r.accepted&&r.vehicle_reads==1);
        assert(run(64|512)&&!r.accepted&&r.vehicle_reads==1);
        assert(run(64|32|512)&&!r.accepted&&r.vehicle_reads==2);
        put(f.other,0x9ac,0x4000u);assert(run(64|512)&&r.accepted);
        put(f.trigger,0x410,0x3000u);
        assert(run(64)&&r.accepted&&r.custom_called&&f.customs==1);
        f.accept=false;assert(run(64)&&!r.accepted&&f.customs==2);
        put(f.trigger,0x410,0x4000u);assert(run(64)&&r.accepted&&!r.custom_called);
        put(f.other,0,relay_vtable);
        assert(run(64|16)&&!r.accepted&&r.npc_read&&!f.npcs);
        put(f.other,0,0u);
        assert(run(64|16)&&!r.accepted&&f.players==1&&f.npcs==1);
        f.npc=true;assert(run(64|16)&&r.accepted&&f.npcs==2);
        f.player=true;f.vehicle=true;assert(run(64|32)&&r.accepted&&f.vehicles==1);
        put(f.other,0,0x224aec3cu);
        put(f.trigger,0xb4,64|32);
        assert(!filter(f.trigger,sizeof(f.trigger),f.other,4,&b,&r));
        put(f.trigger,0xb4,0);
        CampaignStartTouchBindings st{};CampaignStartTouchResult sr{};
        assert(start(f.trigger,sizeof(f.trigger),f.other,sizeof(f.other),&st,&sr,&b,&r)&&sr.completed&&sr.filter_called&&!sr.accepted&&!sr.appended);
    }

// Actual native campaign Start/EndTouch -> named queue -> native lookup
// -> actual logic_relay inputs. No hosted enqueue, target search or input.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct QueueFixture {
            alignas(4) unsigned char trigger[dynamic_trigger_extent]{},player[0x37c]{},relay[relay_extent]{};
            unsigned contacts[2]{},actions[3][7]{};
            alignas(4) unsigned char root[0x38]{},rows[4][0x38]{};
            sin_independent::EntityHandleEntry entries[3]{};
            float now=10.f;unsigned allocated=0,released=0;bool locked=false;
        } f;
        auto start=reinterpret_cast<FilteredStartCampaignOutput>(GetProcAddress(server,filtered_start_touch_export));
        auto end=reinterpret_cast<EndCampaignTriggerOutput>(GetProcAddress(server,end_touch_output_export));
        auto get_service=reinterpret_cast<nw::GetServerEntityEventServiceApi>(GetProcAddress(server,nw::server_entity_event_service_export));
        assert(start&&end&&get_service);const auto* service=get_service(1,sizeof(nw::ServerEntityEventServiceApi));assert(service);
        nw::ServerEntityNamedEventQueueBindings queue{};queue.context=&f;
        queue.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<QueueFixture*>(p);if(!token||token>f.allocated)return 0;*row=f.rows[token-1];*bytes=0x38;return 1;
        };
        queue.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<QueueFixture*>(p);if(f.allocated==4)return 0;*row=f.rows[f.allocated++];*token=f.allocated;*bytes=0x38;return 1;
        };
        queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=204;return 1;};
        nw::ServerEntityVoidOutputBindings ob{};ob.context=&f;
        ob.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<QueueFixture*>(p);if(token<11||token>13)return 0;*row=f.actions[token-11];*bytes=0x1c;return 1;
        };
        ob.release=[](void*,unsigned,void*) noexcept ->int{assert(false);return 0;};
        RelayQueueBindings route{};route.context=&f;route.root=f.root;route.root_bytes=sizeof(f.root);route.root_token=99;
        route.queue=&queue;route.entries=f.entries;route.entry_count=3;route.now=&f.now;
        route.handle=[](void* p,void* actor,unsigned* handle) noexcept ->int{
            auto& f=*static_cast<QueueFixture*>(p);if(actor==f.trigger)*handle=0x3000;else if(actor==f.player)*handle=0x5001;else if(actor==f.relay)*handle=0x7002;else return 0;return 1;
        };
        auto resolve_vector=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<QueueFixture*>(p);if(token!=8)return 0;*row=f.contacts;*bytes=sizeof(f.contacts);return 1;
        };
        CampaignStartTouchBindings st{};st.context=&f;st.resolve_vector=resolve_vector;st.output=&ob;st.native_queue=&route;
        CampaignEndTouchBindings et{};et.context=&f;et.resolve_vector=resolve_vector;et.entries=f.entries;et.entry_count=3;et.output=&ob;et.native_queue=&route;
        CampaignFilterBindings fb{};fb.entries=f.entries;fb.entry_count=3;
        f.entries[0].entity=reinterpret_cast<CBaseEntity*>(f.trigger);f.entries[0].serial=3;f.entries[0].next=&f.entries[1];
        f.entries[1].entity=reinterpret_cast<CBaseEntity*>(f.player);f.entries[1].serial=5;f.entries[1].previous=&f.entries[0];f.entries[1].next=&f.entries[2];
        f.entries[2].entity=reinterpret_cast<CBaseEntity*>(f.relay);f.entries[2].serial=7;f.entries[2].previous=&f.entries[1];
        put(f.trigger,0,dynamic_trigger_vtable);put(f.trigger,0x378,0x3000);put(f.trigger,0xb4,64);put(f.trigger,0x410,0xffffffffu);
        put(f.trigger,0x45c,8);put(f.trigger,0x460,2);put(f.trigger,0x46c,8);
        put(f.player,0,0x224aec3cu);put(f.player,0x378,0x5001);
        put(f.relay,0,relay_vtable);put(f.relay,0x378,0x7002);put(f.relay,0xc0,201);f.relay[0x3ac]=1;
        const unsigned output_heads[]{0x428,0x440,0x458};
        const float delays[]{.25f,.5f,1.f};
        for(unsigned i=0;i<3;++i){
            put(f.trigger,output_heads[i],11+i);
            f.actions[i][0]=201;f.actions[i][1]=202+i;std::memcpy(&f.actions[i][3],&delays[i],4);
            f.actions[i][4]=0xffffffffu;f.actions[i][5]=80+i;
        }
        nw::ServerEntitySearchBindings search{};search.context=&f;search.head=f.entries;
        search.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<QueueFixture*>(p);*bytes=actor==f.trigger?sizeof(f.trigger):actor==f.player?sizeof(f.player):actor==f.relay?sizeof(f.relay):0;return *bytes!=0;
        };
        auto text=[](void*,unsigned token,const char** value,unsigned* bytes) noexcept ->int{
            constexpr const char* names[]{"station_relay","Enable","Disable","Toggle"};
            if(token<201||token>204)return 0;*value=names[token-201];*bytes=static_cast<unsigned>(std::strlen(*value)+1);return 1;
        };
        search.string_text=text;
        nw::ServerNpcActivePostAnimationBindings storage{};
        RelayBindings relay{};nw::ServerEntityAcceptInputBindings input{};input.storage=&storage;input.relay=&relay;input.string_text=text;
        nw::ServerEntityEventServiceBindings sb{};sb.context=&f;sb.entities.entries=f.entries;sb.entities.count=3;sb.resolve=queue.resolve;
        sb.native_search=&search;sb.native_input=&input;sb.live_now=&f.now;
        sb.lock=[](void* p,bool enter) noexcept ->int{auto& f=*static_cast<QueueFixture*>(p);assert(f.locked!=enter);f.locked=enter;return 1;};
        sb.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& f=*static_cast<QueueFixture*>(p);assert(f.locked&&token&&token<=f.allocated&&row==f.rows[token-1]);++f.released;return 1;};
        CampaignStartTouchResult sr{};CampaignFilterResult fr{};CampaignEndTouchResult er{};
        assert(start(f.trigger,sizeof(f.trigger),f.player,sizeof(f.player),&st,&sr,&fb,&fr)&&fr.accepted&&sr.native_queued==1&&sr.count==1);
        assert(end(f.trigger,sizeof(f.trigger),f.player,sizeof(f.player),&et,&er)&&er.end_output&&er.all_output&&er.native_queued==2&&!er.count);
        assert(f.allocated==3&&word(f.root,0x30)==1&&word(f.rows[0],0x30)==2&&word(f.rows[1],0x30)==3);
        for(unsigned i=0;i<3;++i){
            assert(word(f.rows[i],4)==201&&word(f.rows[i],8)==202+i&&word(f.rows[i],0xc)==0x5001&&word(f.rows[i],0x10)==0x3000);
            float deadline=0;std::memcpy(&deadline,f.rows[i],4);assert(deadline==10.f+delays[i]);
        }
        nw::ServerEntityEventServiceResult result{};int steps=0;
        auto tick=[&](float now){f.now=now;return service->service(f.root,sizeof(f.root),99,now,false,&steps,&sb,&result);};
        assert(tick(10.f)&&result.future_stopped&&!result.events&&f.relay[0x3ac]);
        // Separate success and postconditions so the first refusal is
        // distinguishable from an accepted input or continuation error.
        const bool first=tick(10.25f);
        if(!first)std::fprintf(stderr,"campaign queue refusal: events=%u deliveries=%u native_inputs=%u named_searches=%u rejected=%u removed=%u relay_disabled=%u released=%u\n",
            result.events,result.deliveries,result.native_inputs,result.native_named_searches,result.rejected_inputs,result.removed,unsigned(f.relay[0x3ac]),f.released);
        assert(first);
        assert(result.events==1&&result.native_inputs==1&&!result.rejected_inputs&&result.native_named_searches==2&&!f.relay[0x3ac]&&f.released==1);
        assert(tick(10.5f)&&result.events==1&&result.native_inputs==1&&!result.rejected_inputs&&f.relay[0x3ac]&&f.released==2);
        assert(tick(11.f)&&result.events==1&&result.native_inputs==1&&!result.rejected_inputs&&!f.relay[0x3ac]&&f.released==3&&!word(f.root,0x30)&&!f.locked);
    }

// Retail custom filter wrappers: name/class including wildcard and
// !player special, child negation and short-circuiting multi-filter.
    {
        struct CustomFixture {
            alignas(4) unsigned char trigger[dynamic_trigger_extent]{},player[0x9b0]{},filters[4][0x3f4]{};
            sin_independent::EntityHandleEntry entries[5]{};
            unsigned foreign_calls=0;
        } f;
        const auto evaluate=reinterpret_cast<TestCampaignFilterEntity>(GetProcAddress(server,custom_filter_export));
        const auto callback=reinterpret_cast<CampaignFilterEntityCallback>(GetProcAddress(server,custom_filter_callback_export));
        const auto filter=reinterpret_cast<TestCampaignTriggerFilter>(GetProcAddress(server,campaign_filter_export));
        assert(evaluate&&callback&&filter&&!GetProcAddress(client,custom_filter_export)&&!GetProcAddress(client,custom_filter_callback_export));
        CampaignCustomFilterBindings b{};b.context=&f;b.entries=f.entries;b.entry_count=5;
        b.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<CustomFixture*>(p);
            if(a==f.player){*bytes=sizeof(f.player);return 1;}
            for(auto& row:f.filters)if(a==row){*bytes=sizeof(row);return 1;}
            return 0;
        };
        b.string_text=[](void*,unsigned token,const char** value,unsigned* bytes) noexcept ->int{
            constexpr const char* names[]{"hero","HE*ignored","player","npc_*","!PLAYER","different"};
            if(token<1||token>6)return 0;*value=names[token-1];*bytes=static_cast<unsigned>(std::strlen(*value)+1);return 1;
        };
        b.foreign_filter=[](void* p,void*,void*,bool* yes) noexcept ->int{++static_cast<CustomFixture*>(p)->foreign_calls;*yes=true;return 1;};
        put(f.player,0,0x224aec3cu);put(f.player,0xc0,1);put(f.player,0xc,3);put(f.player,0x9ac,0xffffffffu);
        for(unsigned i=0;i<4;++i){
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(f.filters[i]);f.entries[i].serial=3+i;
        }
        put(f.filters[0],0,base_filter_vtable);put(f.filters[1],0,name_filter_vtable);
        put(f.filters[2],0,class_filter_vtable);put(f.filters[3],0,multi_filter_vtable);
        for(unsigned i=0;i<5;++i)put(f.filters[3],0x3e0+4*i,0xffffffffu);
        CampaignCustomFilterResult r{};
        auto run=[&](unsigned i){return evaluate(f.filters[i],f.player,&b,&r);};
        assert(run(0)&&r.accepted&&r.native_filters==1);
        f.filters[0][0x394]=1;assert(run(0)&&!r.accepted&&r.negated==1);f.filters[0][0x394]=0;
        put(f.filters[1],0x3c8,1);assert(run(1)&&r.accepted);
        put(f.filters[1],0x3c8,2);assert(run(1)&&r.accepted);
        put(f.filters[1],0x3c8,6);assert(run(1)&&!r.accepted);
        put(f.filters[1],0x3c8,5);assert(run(1)&&r.accepted&&r.player_reads==2&&r.vehicle_reads==1);
        put(f.player,0x9ac,0x3000u);assert(run(1)&&!r.accepted&&r.player_reads==2);
        put(f.player,0x9ac,0x4000u);assert(run(1)&&r.accepted);
        put(f.player,0,relay_vtable);assert(run(1)&&!r.accepted&&r.player_reads==1&&!r.vehicle_reads);
        put(f.player,0,0x224aec3cu);
        put(f.filters[2],0x3c8,3);assert(run(2)&&r.accepted);
        put(f.filters[2],0x3c8,4);assert(run(2)&&!r.accepted);
        f.filters[2][0x394]=1;assert(run(2)&&r.accepted&&r.negated==1);f.filters[2][0x394]=0;
        assert(run(3)&&r.accepted&&r.stale==5);
        put(f.filters[3],0x3c8,1);assert(run(3)&&!r.accepted&&r.stale==5);
        put(f.filters[3],0x3e0,0x4001);put(f.filters[3],0x3e4,0x5002);
        put(f.filters[3],0x3c8,0);assert(run(3)&&!r.accepted&&r.visited==3&&r.short_circuits==1);
        put(f.filters[3],0x3c8,1);assert(run(3)&&r.accepted&&r.visited==2&&r.short_circuits==1);
        f.filters[3][0x394]=1;assert(run(3)&&!r.accepted&&r.negated==1);f.filters[3][0x394]=0;
        put(f.filters[3],0x3c8,0);put(f.filters[3],0x3e0,0x6003);
        assert(!run(3)&&!r.completed); // cyclic owner filter refused
        put(f.filters[3],0x3e0,0x7003);assert(run(3)&&!r.accepted&&r.stale==1); // stale serial skipped
        put(f.filters[0],0,0x1234);put(f.filters[3],0x3e0,0x3000);assert(run(3)&&!r.accepted&&r.foreign_filters==1&&f.foreign_calls==1);
        put(f.filters[0],0,base_filter_vtable);
        // Actual trigger +410 resolves custom filter and calls the native
        // ABI-compatible adapter (not an owner-defined predicate).
        put(f.trigger,0,dynamic_trigger_vtable);put(f.trigger,0xb4,64);put(f.trigger,0x410,0x4001);
        CampaignFilterBindings outer{};outer.context=&b;outer.entries=f.entries;outer.entry_count=5;outer.custom_filter=callback;
        CampaignFilterResult result{};
        assert(filter(f.trigger,sizeof(f.trigger),f.player,sizeof(f.player),&outer,&result)&&result.accepted&&result.custom_called);
        f.filters[1][0x394]=1;
        assert(filter(f.trigger,sizeof(f.trigger),f.player,sizeof(f.player),&outer,&result)&&!result.accepted&&result.custom_called);
    }

// Original Activate ordering, real native name lookup, first-match cast,
// full serial handle, five independent child rows and untouched empty names.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct ActivationFixture {
            alignas(4) unsigned char trigger[dynamic_trigger_extent]{},filters[3][0x3f4]{},relay[relay_extent]{};
            sin_independent::EntityHandleEntry entries[4]{};
            unsigned base_calls=0,expected=0,mode=0;
        } f;
        const auto activate=reinterpret_cast<ActivateCampaignFilter>(GetProcAddress(server,filter_activation_export));
        assert(activate&&!GetProcAddress(client,filter_activation_export));
        put(f.trigger,0,dynamic_trigger_vtable);put(f.trigger,0x410,0xffffffffu);
        put(f.filters[0],0,name_filter_vtable);put(f.filters[1],0,class_filter_vtable);put(f.filters[2],0,multi_filter_vtable);
        put(f.relay,0,relay_vtable);
        void* actors[]{f.relay,f.filters[0],f.filters[1],f.filters[2]};
        for(unsigned i=0;i<4;++i){
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=3+i;
            put(actors[i],0x378,((3+i)<<12)|i);put(actors[i],0xc0,10+i);
            if(i)f.entries[i].previous=&f.entries[i-1];
            if(i<3)f.entries[i].next=&f.entries[i+1];
        }
        nw::ServerEntitySearchBindings search{};search.context=&f;search.head=f.entries;search.entities.entries=f.entries;search.entities.count=4;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<ActivationFixture*>(p);
            if(a==f.relay){*bytes=sizeof(f.relay);return 1;}
            for(auto& row:f.filters)if(a==row){*bytes=sizeof(row);return 1;}
            return 0;
        };
        search.string_text=[](void*,unsigned token,const char** value,unsigned* bytes) noexcept ->int{
            constexpr const char* names[]{"relay","name_filter","class_filter","multi_filter","missing","name*"};
            if(token<10||token>15)return 0;*value=names[token-10];*bytes=static_cast<unsigned>(std::strlen(*value)+1);return 1;
        };
        CampaignFilterActivationBindings b{};b.context=&f;b.search=&search;
        b.base_activate=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<ActivationFixture*>(p);++f.base_calls;
            unsigned h=0;
            if(actor==f.trigger)std::memcpy(&h,f.trigger+0x410,4);
            else {assert(actor==f.filters[2]);std::memcpy(&h,f.filters[2]+0x3e0,4);}
            assert(h==f.expected);
            return f.mode!=1;
        };
        CampaignFilterActivationResult r{};
        put(f.trigger,0x40c,11);f.expected=0x4001;
        assert(activate(f.trigger,sizeof(f.trigger),&b,&r)&&r.resolved==1&&r.base_called&&word(f.trigger,0x410)==0x4001);
        put(f.trigger,0x40c,15);assert(activate(f.trigger,sizeof(f.trigger),&b,&r)&&r.resolved==1&&word(f.trigger,0x410)==0x4001);
        // The first result is a non-filter; do not keep searching for
        // a second actor with the same targetname.
        put(f.relay,0xc0,11);put(f.trigger,0x40c,11);f.expected=0xffffffffu;
        assert(activate(f.trigger,sizeof(f.trigger),&b,&r)&&r.wrong_class==1&&r.lookups==1&&word(f.trigger,0x410)==0xffffffffu);
        put(f.relay,0xc0,10);put(f.trigger,0x40c,14);
        assert(activate(f.trigger,sizeof(f.trigger),&b,&r)&&r.missing==1&&word(f.trigger,0x410)==0xffffffffu);
        put(f.trigger,0x40c,0);put(f.trigger,0x410,0x1234);f.expected=0x1234;
        assert(activate(f.trigger,sizeof(f.trigger),&b,&r)&&r.preserved==1&&!r.lookups&&word(f.trigger,0x410)==0x1234);
        put(f.trigger,0x40c,12);f.expected=0x5002;f.mode=1;
        assert(!activate(f.trigger,sizeof(f.trigger),&b,&r)&&r.resolved==1&&r.written==1&&word(f.trigger,0x410)==0x5002);
        f.mode=0;
        auto* m=f.filters[2];
        put(m,0x3cc,11);put(m,0x3d0,12);put(m,0x3d4,0);put(m,0x3d8,14);put(m,0x3dc,10);
        put(m,0x3e0,0xdead);put(m,0x3e8,0x1234);f.expected=0xdead;
        assert(activate(m,sizeof(f.filters[2]),&b,&r)&&r.lookups==4&&r.resolved==3&&r.missing==1&&r.preserved==1);
        assert(word(m,0x3e0)==0x4001&&word(m,0x3e4)==0x5002&&word(m,0x3e8)==0x1234&&word(m,0x3ec)==0xffffffffu&&word(m,0x3f0)==0x3000);
        f.expected=0x4001;f.mode=1;put(m,0x3cc,14);
        assert(!activate(m,sizeof(f.filters[2]),&b,&r)&&r.base_called&&!r.lookups&&word(m,0x3e0)==0x4001);
    }

// Four independent filter factories and real native keyvalue parsing.
// A separate creation adapter avoids expanding the ABI-pinned dictionary.
    {
        struct FilterFactoryFixture {
            alignas(4) unsigned char storage[4][0x3f4]{};
            unsigned allocated=0,constructed=0,initialized=0,accepted=0,cleared=0,foreign=0;
            ActionPool pool{};
        } f;
        const auto make=reinterpret_cast<CreateCampaignFilterEntity>(GetProcAddress(server,filter_factory_export));
        const auto map_create=reinterpret_cast<CreateCampaignFilterForMap>(GetProcAddress(server,filter_map_create_export));
        assert(make&&map_create&&!GetProcAddress(client,filter_factory_export)&&!GetProcAddress(client,filter_map_create_export));
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterFactoryFixture*>(p);constexpr unsigned extents[]{0x3c8,0x3f4,0x3cc,0x3cc};
            assert(f.allocated<4&&n==extents[f.allocated]);*actor=f.storage[f.allocated++];*bytes=n;return 1;
        };
        owner.base_construct=[](void* p,void* actor,int arg) noexcept ->int{
            auto& f=*static_cast<FilterFactoryFixture*>(p);assert(arg==1&&actor==f.storage[f.constructed]);++f.constructed;return 1;
        };
        owner.initialize=[](void* p,void* actor,const char* name) noexcept ->int{
            auto& f=*static_cast<FilterFactoryFixture*>(p);constexpr unsigned vts[]{base_filter_vtable,multi_filter_vtable,name_filter_vtable,class_filter_vtable};
            unsigned vt=0;std::memcpy(&vt,actor,4);assert(actor==f.storage[f.initialized]&&vt==vts[f.initialized]&&name);++f.initialized;return 1;
        };
        CampaignFilterFactoryBindings fb{};fb.owner=&owner;fb.fallback_context=&f;
        fb.fallback_create=[](void* p,const char* name,void** actor) noexcept ->int{
            auto& f=*static_cast<FilterFactoryFixture*>(p);assert(!std::strcmp(name,"unknown"));++f.foreign;*actor=nullptr;return 1;
        };
        FactoryResult missing{};assert(make("unknown",&fb,&missing)&&missing.completed&&!missing.matched&&!f.allocated);
        OutputActionBindings actions=output;actions.context=&f.pool;actions.next_id=&f.pool.id;
        KeyBindings keys{};keys.context=&f;keys.native_output=&actions;
        keys.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterFactoryFixture*>(p);constexpr unsigned extents[]{0x3c8,0x3f4,0x3cc,0x3cc};
            for(unsigned i=0;i<4;++i)if(actor==f.storage[i]){*bytes=extents[i];return 1;}return 0;
        };
        keys.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
            *token=!std::strcmp(value,"name1")?21:!std::strcmp(value,"class1")?22:
                !std::strcmp(value,"hero")?23:!std::strcmp(value,"player")?24:
                !std::strcmp(value,"relay")?25:!std::strcmp(value,"Trigger")?26:27;return 1;
        };
        Bindings parse{};parse.context=&fb;parse.create=map_create;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{
            ++static_cast<FilterFactoryFixture*>(static_cast<CampaignFilterFactoryBindings*>(p)->owner->context)->cleared;return 1;
        };
        parse.accept_entity=[](void* p,void* actor,const char*,unsigned,const EntitySlice*) noexcept ->int{
            auto& f=*static_cast<FilterFactoryFixture*>(static_cast<CampaignFilterFactoryBindings*>(p)->owner->context);
            assert(actor==f.storage[f.accepted]);++f.accepted;return 1;
        };
        parse.missing=[](void*,const char* name) noexcept ->int{return name&&!std::strcmp(name,"unknown");};
        const char lump[]="{ classname filter_base Negated 1 OnPass \"relay,Trigger,,0,-1\" OnFail \"relay,Trigger,,0,-1\" } "
            "{ classname filter_multi FilterType 1 Filter01 name1 Filter02 class1 } "
            "{ classname filter_activator_name targetname name1 filtername hero } "
            "{ classname filter_activator_class targetname class1 filterclass player } { classname unknown }";
        Result result{};
        assert(api->parse(lump,sizeof(lump),&parse,&result)&&result.created==4&&result.missing_factory==1&&f.allocated==4&&f.initialized==4&&f.accepted==4&&f.foreign==1&&f.cleared==5);
        assert(f.storage[0][0x394]==1&&word(f.storage[0],0x3a4)==0xffffffffu&&word(f.storage[0],0x3bc)==0xffffffffu&&f.pool.used==2);
        assert(word(f.storage[0],0x3ac)==ptrword(f.pool.rows[0])&&word(f.storage[0],0x3c4)==ptrword(f.pool.rows[1]));
        assert(word(f.storage[1],0x3c8)==1&&word(f.storage[1],0x3cc)==21&&word(f.storage[1],0x3d0)==22);
        for(unsigned i=0;i<5;++i)assert(word(f.storage[1],0x3e0+4*i)==0xffffffffu);
        assert(word(f.storage[2],0x3c8)==23&&word(f.storage[3],0x3c8)==24);
        KeyResult key{};
        assert(api->key_value(f.storage[2],"Negated#1","1",&keys,&key)&&key.base_field&&f.storage[2][0x394]);
        assert(!api->key_value(f.storage[2],"FilterType","1",&keys,&key));
    }
