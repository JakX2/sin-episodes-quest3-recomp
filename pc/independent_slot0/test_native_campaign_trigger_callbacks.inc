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

// InputTestActivator via actual native datamap AcceptInput, not a
// handler callback. Variant payload is irrelevant; caller is self.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct TestFilterFixture {
            alignas(4) unsigned char actor[0x3f4]{},player[0x37c]{};
            unsigned rows[2][7]{},calls=0,last=0;
            void* activator=nullptr;
        } f;
        const auto leaf=reinterpret_cast<CampaignFilterTestActivator>(GetProcAddress(server,filter_test_activator_export));
        const auto accept=reinterpret_cast<nw::AcceptCampaignFilterInput>(GetProcAddress(server,nw::accept_campaign_filter_export));
        assert(leaf&&accept&&!GetProcAddress(client,filter_test_activator_export)&&!GetProcAddress(client,nw::accept_campaign_filter_export));
        put(f.player,0,0x224aec3cu);put(f.player,0xc0,1);put(f.player,0xc,2);
        for(unsigned i=0;i<2;++i){f.rows[i][0]=201+i;f.rows[i][1]=203;f.rows[i][4]=0xffffffffu;f.rows[i][5]=80+i;}
        CampaignCustomFilterBindings pred{};pred.context=&f;
        pred.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<TestFilterFixture*>(p);*bytes=a==f.actor?sizeof(f.actor):a==f.player?sizeof(f.player):0;return *bytes!=0;
        };
        pred.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token==1)*text="hero";else if(token==2)*text="player";else if(token==3)*text="wrong";else return 0;
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntityVoidOutputBindings out{};out.context=&f;
        out.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<TestFilterFixture*>(p);if(token<11||token>12)return 0;*row=f.rows[token-11];*bytes=0x1c;return 1;
        };
        out.enqueue=[](void* p,void* activator,void* caller,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<TestFilterFixture*>(p);assert(activator==f.activator&&caller==f.actor&&action->delay==0.f);++f.calls;f.last=action->target;return 1;
        };
        CampaignFilterInputBindings b{&pred,&out,nullptr};
        nw::ServerNpcActivePostAnimationBindings storage{};
        nw::ServerEntityAcceptInputBindings ai{};ai.storage=&storage;
        ai.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token!=90)return 0;*text="tEsTaCtIvAtOr";*bytes=14;return 1;
        };
        nw::ServerEntityAcceptInputResult ar{};CampaignFilterInputResult r{};
        unsigned variant[5]{123,0,0,0,5};
        for(unsigned vt:{base_filter_vtable,name_filter_vtable,class_filter_vtable,multi_filter_vtable}){
            std::memset(f.actor,0,sizeof(f.actor));put(f.actor,0,vt);put(f.actor,0x3ac,11);put(f.actor,0x3c4,12);
            put(f.actor,0x3c8,vt==name_filter_vtable?1:vt==class_filter_vtable?2:0);
            for(unsigned i=0;i<5;++i)put(f.actor,0x3e0+4*i,0xffffffffu);
            f.actor[0x1f0]=1;f.activator=f.player;f.calls=0;
            const unsigned extent=vt==base_filter_vtable?0x3c8u:vt==multi_filter_vtable?0x3f4u:0x3ccu;
            assert(accept(f.actor,extent,vt,90,f.player,nullptr,variant,sizeof(variant),13,&ai,&ar,&b,&r)&&ar.accepted&&ar.native_handler&&ar.dirty&&ar.handler==0x2216b300u&&ar.map==0x224f8a44u&&ar.actual_type==5&&ar.expected_type==20);
            assert(r.completed&&r.pass_output&&!r.fail_output&&r.queued==1&&f.calls==1&&f.last==201&&f.actor[0x200]);
            f.actor[0x394]=1;
            assert(leaf(f.actor,extent,f.player,&b,&r)&&r.fail_output&&!r.pass_output&&f.calls==2&&f.last==202);
            assert(!leaf(f.actor,extent-1,f.player,&b,&r)&&f.calls==2);
            f.actor[0x394]=0;f.activator=nullptr;
            if(vt==base_filter_vtable||vt==multi_filter_vtable)assert(leaf(f.actor,extent,nullptr,&b,&r)&&r.pass_output&&f.calls==3);
            else assert(!leaf(f.actor,extent,nullptr,&b,&r)&&f.calls==2);
        }
    }

// Native queue -> TestActivator -> nested native output -> relay.
// No hosted enqueue, target search or input dispatch.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct FilterQueueFixture {
            alignas(4) unsigned char filter[0x3cc]{},player[0x37c]{},relay[relay_extent]{};
            unsigned outputs[2][7]{};
            alignas(4) unsigned char root[0x38]{},rows[4][0x38]{};
            sin_independent::EntityHandleEntry entries[3]{};
            float now=20.f;unsigned allocated=0,released=0;bool locked=false;
        } f;
        const auto service=reinterpret_cast<nw::ServiceCampaignFilterEvents>(GetProcAddress(server,nw::service_campaign_filters_export));
        const auto queue_get=reinterpret_cast<nw::GetServerEntityNamedEventQueueApi>(GetProcAddress(server,nw::server_entity_named_event_queue_export));
        assert(service&&queue_get&&!GetProcAddress(client,nw::service_campaign_filters_export));
        const auto* qa=queue_get(1,sizeof(nw::ServerEntityNamedEventQueueApi));assert(qa);
        nw::ServerEntityNamedEventQueueBindings queue{};queue.context=&f;
        queue.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterQueueFixture*>(p);if(!token||token>f.allocated)return 0;*row=f.rows[token-1];*bytes=0x38;return 1;
        };
        queue.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterQueueFixture*>(p);if(f.allocated==4)return 0;*row=f.rows[f.allocated++];*token=f.allocated;*bytes=0x38;return 1;
        };
        queue.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=201&&token<=206;return 1;};
        nw::ServerEntityVoidOutputBindings ob{};ob.context=&f;
        ob.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterQueueFixture*>(p);if(token<11||token>12)return 0;*row=f.outputs[token-11];*bytes=0x1c;return 1;
        };
        RelayQueueBindings route{};route.context=&f;route.root=f.root;route.root_bytes=sizeof(f.root);route.root_token=99;route.queue=&queue;
        route.handle=[](void* p,void* actor,unsigned* handle) noexcept ->int{
            auto& f=*static_cast<FilterQueueFixture*>(p);if(actor==f.filter)*handle=0x3000;else if(actor==f.player)*handle=0x5001;else if(actor==f.relay)*handle=0x7002;else return 0;return 1;
        };
        void* actors[]{f.filter,f.player,f.relay};const unsigned serials[]{3,5,7};
        for(unsigned i=0;i<3;++i){
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=serials[i];
            put(actors[i],0x378,(serials[i]<<12)|i);
            if(i)f.entries[i].previous=&f.entries[i-1];if(i<2)f.entries[i].next=&f.entries[i+1];
        }
        put(f.filter,0,name_filter_vtable);put(f.filter,0xc0,201);put(f.filter,0x3c8,206);
        put(f.filter,0x3ac,11);put(f.filter,0x3c4,12);
        put(f.player,0,0x224aec3cu);put(f.player,0xc0,206);
        put(f.relay,0,relay_vtable);put(f.relay,0xc0,203);f.relay[0x3ac]=1;
        for(unsigned i=0;i<2;++i){f.outputs[i][0]=203;f.outputs[i][1]=204+i;f.outputs[i][4]=0xffffffffu;f.outputs[i][5]=80+i;}
        auto extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterQueueFixture*>(p);*bytes=actor==f.filter?sizeof(f.filter):actor==f.player?sizeof(f.player):actor==f.relay?sizeof(f.relay):0;return *bytes!=0;
        };
        auto text=[](void*,unsigned token,const char** value,unsigned* bytes) noexcept ->int{
            constexpr const char* names[]{"gate_filter","TestActivator","relay","Enable","Disable","hero"};
            if(token<201||token>206)return 0;*value=names[token-201];*bytes=static_cast<unsigned>(std::strlen(*value)+1);return 1;
        };
        CampaignCustomFilterBindings pred{};pred.context=&f;pred.extent=extent;pred.string_text=text;
        CampaignFilterInputBindings filters{&pred,&ob,&route};
        nw::ServerEntitySearchBindings search{};search.context=&f;search.head=f.entries;search.extent=extent;search.string_text=text;
        nw::ServerNpcActivePostAnimationBindings storage{};RelayBindings rb{};
        nw::ServerEntityAcceptInputBindings input{};input.storage=&storage;input.relay=&rb;input.string_text=text;
        nw::ServerEntityEventServiceBindings sb{};sb.context=&f;sb.entities.entries=f.entries;sb.entities.count=3;sb.resolve=queue.resolve;
        sb.native_search=&search;sb.native_input=&input;sb.live_now=&f.now;
        sb.lock=[](void* p,bool enter) noexcept ->int{auto& f=*static_cast<FilterQueueFixture*>(p);assert(f.locked!=enter);f.locked=enter;return 1;};
        sb.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& f=*static_cast<FilterQueueFixture*>(p);assert(f.locked&&row==f.rows[token-1]);++f.released;return 1;};
        nw::ServerEntityVoidOutputAction action{201,202,0,0,99,0.f};
        nw::ServerEntityNamedEventQueueResult qr{};nw::ServerEntityEventServiceResult sr{};nw::CampaignFilterServiceSummary summary{};
        int steps=1;
        auto enqueue=[&](){return qa->enqueue(f.root,sizeof(f.root),99,f.now,0x5001,0x5001,&action,&queue,&qr);};
        auto tick=[&](bool step){return service(f.root,sizeof(f.root),99,f.now,step,&steps,&sb,&sr,&filters,&summary);};
        assert(enqueue()&&tick(true)&&sr.events==1&&sr.native_inputs==1&&sr.step_stopped&&summary.passed==1&&summary.queued==1&&f.allocated==2&&f.released==1);
        assert(word(f.rows[1],0xc)==0x5001&&word(f.rows[1],0x10)==0x3000&&word(f.rows[1],8)==204&&f.relay[0x3ac]);
        assert(tick(false)&&sr.events==1&&sr.native_inputs==1&&!f.relay[0x3ac]&&f.released==2);
        f.filter[0x394]=1;
        assert(enqueue()&&tick(false)&&sr.events==2&&sr.native_inputs==2&&!sr.foreign_inputs&&summary.failed==1&&summary.queued==1&&f.relay[0x3ac]&&f.released==4&&!word(f.root,0x30));
    }

// Shared inherited Kill -> deferred networkable release -> filter
// destructor. The four destructors have identical member release order.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct FilterDestroyFixture {
            alignas(4) unsigned char actor[0x3f4]{};
            unsigned actions[3][7]{},updates=0,base=0,freed=0,releases=0,order[3]{},network=0;
            void* slots[1]{};
            const sin_port::native_map::Api* api;
            const LogicDestroyBindings* destroy=nullptr;
            unsigned extent=0;
        } f;f.api=api;
        const auto destroy_factory=reinterpret_cast<DestroyCampaignFilterEntity>(GetProcAddress(server,destroy_filter_factory_export));
        const auto input_get=reinterpret_cast<nw::GetServerEntityAcceptInputApi>(GetProcAddress(server,nw::server_entity_accept_input_export));
        assert(destroy_factory&&input_get&&!GetProcAddress(client,destroy_filter_factory_export));
        const auto* input_api=input_get(1,sizeof(nw::ServerEntityAcceptInputApi));assert(input_api);
        LogicDestroyBindings db{};db.context=&f;f.destroy=&db;
        db.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);if(token<11||token>13)return 0;*row=f.actions[token-11];*bytes=0x1c;return 1;
        };
        db.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);assert(f.releases<3&&row==f.actions[token-11]&&!f.base);f.order[f.releases++]=token;return 1;
        };
        db.base_destroy=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);assert(actor==f.actor&&f.releases==3&&f.order[0]==12&&f.order[1]==13&&f.order[2]==11);++f.base;return 1;
        };
        db.free_actor=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);assert(actor==f.actor&&f.base==1);++f.freed;return 1;
        };
        RelayFactoryBindings factory{};factory.context=&f;
        factory.release_networkable=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);assert(net==f.actor+0x160);++f.network;return 1;
        };
        DeferredRemovalQueue queue{};queue.slots=f.slots;queue.capacity=1;
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&queue;
        deferred.update=[](void* p,void* actor,unsigned method) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);assert(actor==f.actor&&method==0x221446f0u&&(f.actor[0xb8]&1));++f.updates;return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<FilterDestroyFixture*>(p);assert(net==f.actor+0x160);
            LogicDestroyResult result{};return f.api->destroy_logic(f.actor,f.extent,1,f.destroy,&result)&&result.completed&&result.outputs==2&&result.released==3&&result.freed;
        };
        EntityKillBindings kb{};kb.deferred=&deferred;
        nw::ServerNpcActivePostAnimationBindings storage{};nw::ServerEntityAcceptInputBindings ai{};ai.storage=&storage;ai.logic_kill=&kb;
        ai.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token!=90)return 0;*text="Kill";*bytes=5;return 1;
        };
        unsigned variant[5]{};
        for(unsigned vt:{base_filter_vtable,multi_filter_vtable,name_filter_vtable,class_filter_vtable}){
            std::memset(f.actor,0,sizeof(f.actor));std::memset(f.actions,0,sizeof(f.actions));
            f.updates=f.base=f.freed=f.releases=f.network=0;f.extent=campaign_filter_extent(vt);
            put(f.actor,0,vt);put(f.actor,0x378,0x3000);put(f.actor,0xc0,99);put(f.actor,0x160,0x22454f00);put(f.actor,0x168,ptrword(f.actor));put(f.actor,0x20c,0xffffffffu);
            put(f.actor,0x3ac,11);put(f.actor,0x3c4,12);f.actions[1][6]=13;
            assert(destroy_factory(nullptr,0,nullptr,nullptr)&&!destroy_factory(f.actor,f.extent,f.actor+0x164,&factory)&&!f.network);
            assert(destroy_factory(f.actor,f.extent,f.actor+0x160,&factory)&&f.network==1);
            // Aliased output lists refused before release.
            put(f.actor,0x3ac,13);LogicDestroyResult dr{};
            assert(!api->destroy_logic(f.actor,f.extent,1,&db,&dr)&&!f.releases&&!f.base&&!f.freed);
            put(f.actor,0x3ac,11);
            nw::ServerEntityAcceptInputResult ar{};
            assert(input_api->run(f.actor,f.extent,vt,90,nullptr,nullptr,variant,sizeof(variant),0,&ai,&ar)&&ar.accepted&&ar.native_handler&&ar.handler==0x2213cdc0u);
            assert(queue.count==1&&f.updates==1&&!word(f.actor,0xc0)&&!f.releases);
            DeferredRemovalResult rr{};
            assert(api->defer_logic_remove(f.actor,f.extent,&deferred,&rr)&&rr.already_marked&&queue.count==1);
            assert(api->cleanup_deferred(&deferred,&rr)&&rr.released==1&&!queue.count&&f.base==1&&f.freed==1&&f.releases==3);
        }
    }

// Retail scripted_sequence factory, external datamap names and
// numbered script event output. No NPC schedule or animation claim.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct SequenceFixture {
            alignas(4) unsigned char actor[scripted_sequence_extent]{};
            unsigned rows[8][7]{},calls=0,last=0,base=0,initialized=0;
        } f;
        const auto make=reinterpret_cast<CreateScriptedSequence>(GetProcAddress(server,scripted_sequence_factory_export));
        const auto emit=reinterpret_cast<FireScriptedSequenceEvent>(GetProcAddress(server,scripted_sequence_event_export));
        assert(make&&emit&&!GetProcAddress(client,scripted_sequence_factory_export)&&!GetProcAddress(client,scripted_sequence_event_export));
        RelayFactoryBindings owner{};owner.context=&f;
        owner.allocate=[](void* p,unsigned n,void** actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<SequenceFixture*>(p);assert(n==sizeof(f.actor));*actor=f.actor;*bytes=n;return 1;
        };
        owner.base_construct=[](void* p,void* actor,int arg) noexcept ->int{
            auto& f=*static_cast<SequenceFixture*>(p);assert(actor==f.actor&&arg==0);++f.base;return 1;
        };
        owner.initialize=[](void* p,void* actor,const char* cls) noexcept ->int{
            auto& f=*static_cast<SequenceFixture*>(p);assert(actor==f.actor&&f.base==1&&!std::strcmp(cls,"scripted_sequence"));++f.initialized;return 1;
        };
        FactoryResult fr{};assert(make(&owner,&fr)&&fr.completed&&fr.matched&&fr.actor==f.actor&&fr.networkable==f.actor+0x160&&fr.spec->original_object==0x22505008&&f.initialized==1);
        assert(word(f.actor,0)==scripted_sequence_vtable&&word(f.actor,0x3d0)==0xffffffffu&&word(f.actor,0x3d4)==0xffffffffu&&word(f.actor,0x3dc)==0xffffffffu);
        for(unsigned i=0;i<10;++i)assert(word(f.actor,0x3e0+i*0x18+0xc)==0xffffffffu&&!word(f.actor,0x3e0+i*0x18+0x14));
        KeyBindings keys{};keys.context=&f;
        keys.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{auto& f=*static_cast<SequenceFixture*>(p);if(actor!=f.actor)return 0;*bytes=sizeof(f.actor);return 1;};
        keys.intern=[](void*,const char* value,unsigned* token) noexcept ->int{*token=!std::strcmp(value,"idle")?10:!std::strcmp(value,"walk")?11:12;return 1;};
        keys.output=[](void* p,void* actor,void* out,const char* value) noexcept ->int{
            auto& f=*static_cast<SequenceFixture*>(p);assert(actor==f.actor&&!std::strcmp(value,"action"));
            const auto offset=static_cast<unsigned>(static_cast<unsigned char*>(out)-f.actor);
            assert(offset>=0x410&&offset<=0x4b8&&!((offset-0x410)%0x18));
            const unsigned i=(offset-0x410)/0x18,token=101+i;
            std::memcpy(static_cast<unsigned char*>(out)+0x14,&token,4);
            f.rows[i][0]=201+i;f.rows[i][1]=220;f.rows[i][4]=0xffffffffu;return 1;
        };
        KeyResult key{};
        assert(api->key_value(f.actor,"m_iszIdle","idle",&keys,&key)&&key.offset==0x394&&word(f.actor,0x394)==10);
        assert(!api->key_value(f.actor,"m_iszPreIdle","idle",&keys,&key));
        assert(api->key_value(f.actor,"m_iszPlay","walk",&keys,&key)&&word(f.actor,0x398)==11);
        assert(api->key_value(f.actor,"m_fMoveTo","2",&keys,&key)&&word(f.actor,0x3ac)==2);
        assert(api->key_value(f.actor,"m_flRadius","128",&keys,&key)&&word(f.actor,0x3b0)==0x43000000u);
        nw::ServerEntityVoidOutputBindings fire{};fire.context=&f;
        fire.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<SequenceFixture*>(p);if(token<101||token>108)return 0;*row=f.rows[token-101];*bytes=0x1c;return 1;
        };
        fire.enqueue=[](void* p,void* activator,void* caller,const nw::ServerEntityVoidOutputAction* action) noexcept ->int{
            auto& f=*static_cast<SequenceFixture*>(p);assert(activator==f.actor&&caller==f.actor&&action->delay==0.f);++f.calls;f.last=action->target;return 1;
        };
        ScriptedSequenceEventBindings b{&fire,nullptr};ScriptedSequenceEventResult result{};
        for(int i=1;i<=8;++i){
            char name[]="OnScriptEvent00";name[14]=char('0'+i);
            assert(api->key_value(f.actor,name,"action",&keys,&key)&&key.offset==0x3f8u+0x18u*i);
            assert(emit(f.actor,sizeof(f.actor),i,&b,&result)&&result.output_called&&result.queued==1&&f.last==200u+i&&f.calls==static_cast<unsigned>(i));
        }
        assert(emit(f.actor,sizeof(f.actor),0,nullptr,&result)&&result.completed&&!result.output_called&&f.calls==8);
        assert(emit(f.actor,sizeof(f.actor),9,nullptr,&result)&&result.completed&&!result.output_called&&f.calls==8);
        assert(!emit(f.actor,sizeof(f.actor)-1,1,&b,&result)&&f.calls==8);
    }

// ScriptedSequence input ordering and native NPC permission, including
// live current/next sequence policy and original default think cadence.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct ControlFixture {
            alignas(4) unsigned char seq[scripted_sequence_extent]{},npc[0x974]{},current[scripted_sequence_extent]{},next[scripted_sequence_extent]{};
            sin_independent::EntityHandleEntry entries[3]{};
            unsigned possess=0,notify=0,warnings=0,reason=0,cancels=0,cancel_warn=0,expected_wait=0;
            float now=10.25f,interval=.125f;
        } f;
        const auto control=reinterpret_cast<ControlScriptedSequence>(GetProcAddress(server,scripted_sequence_control_export));
        assert(control&&!GetProcAddress(client,scripted_sequence_control_export));
        put(f.seq,0,scripted_sequence_vtable);put(f.seq,0x3d0,0xffffffffu);put(f.seq,0x74,0xffffffffu);put(f.seq,0xb8,0x400000u);
        put(f.npc,0,0x224954a4u);put(f.npc,0x970,0xffffffffu);
        put(f.current,0,scripted_sequence_vtable);put(f.current,0x3d4,0xffffffffu);
        put(f.next,0,scripted_sequence_vtable);
        void* actors[]{f.npc,f.current,f.next};for(unsigned i=0;i<3;++i){f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=3+i;}
        nw::ServerNpcOuterCadenceBindings cadence{};cadence.context=&f;
        cadence.notify_think_flags=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<ControlFixture*>(p);assert(actor==f.seq&&!f.seq[0x3c0]&&f.seq[0x3d8]);++f.notify;return 1;
        };
        ScriptedSequenceControlBindings b{};b.context=&f;b.entries=f.entries;b.entry_count=3;b.now=&f.now;b.interval=&f.interval;b.cadence=&cadence;
        b.extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<ControlFixture*>(p);*bytes=actor==f.npc?sizeof(f.npc):actor==f.current||actor==f.next?sizeof(f.current):0;return *bytes!=0;
        };
        b.possess=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<ControlFixture*>(p);assert(actor==f.seq&&f.seq[0x3c0]==f.expected_wait);++f.possess;return 1;
        };
        b.linked_warning=[](void* p,void* next,unsigned reason) noexcept ->int{
            auto& f=*static_cast<ControlFixture*>(p);assert(next==f.next);++f.warnings;f.reason=reason;return 1;
        };
        b.cancel_warning=[](void* p,unsigned token) noexcept ->int{
            auto& f=*static_cast<ControlFixture*>(p);assert(token==7&&f.seq[0x3d8]);++f.cancel_warn;return 1;
        };
        b.cancel_group=[](void* p,void* actor) noexcept ->int{
            auto& f=*static_cast<ControlFixture*>(p);assert(actor==f.seq&&!f.seq[0x3d8]&&f.cancel_warn==1);++f.cancels;return 1;
        };
        using I=ScriptedSequenceInput;ScriptedSequenceControlResult o{};
        assert(control(f.seq,sizeof(f.seq),I::MoveToPosition,&b,&o)&&o.think_installed&&o.tick_scheduled&&o.due_tick==82&&f.seq[0x3c0]&&f.notify==1&&word(f.seq,8)==0x221afc90u&&word(f.seq,0x74)==82);
        put(f.seq,0x3d0,0x3000u);put(f.npc,0x96c,1);
        assert(control(f.seq,sizeof(f.seq),I::BeginSequence,&b,&o)&&o.state_blocked&&!f.seq[0x3c0]&&!f.possess);
        put(f.npc,0x96c,2);put(f.npc,0x850,4);
        assert(control(f.seq,sizeof(f.seq),I::MoveToPosition,&b,&o)&&o.permission_blocked&&!f.seq[0x3c0]&&!f.possess);
        put(f.seq,0xb4,0x40u);
        assert(control(f.seq,sizeof(f.seq),I::MoveToPosition,&b,&o)&&o.possessed&&o.permission==1&&f.seq[0x3c0]&&f.possess==1);
        put(f.npc,0x970,0x4001u);put(f.current,0x3d4,0x5002u);put(f.current,0x3a4,9u);
        assert(control(f.seq,sizeof(f.seq),I::BeginSequence,&b,&o)&&o.linked_blocked&&f.warnings==1&&f.reason==1&&!f.seq[0x3c0]);
        put(f.current,0x3a4,0u);put(f.next,0xb4,0x200u);
        assert(control(f.seq,sizeof(f.seq),I::BeginSequence,&b,&o)&&o.linked_blocked&&f.warnings==2&&f.reason==2);
        put(f.next,0xb4,0u);
        assert(control(f.seq,sizeof(f.seq),I::BeginSequence,&b,&o)&&o.possessed&&o.permission==2&&f.possess==2);
        put(f.npc,0x8c,1);
        assert(control(f.seq,sizeof(f.seq),I::BeginSequence,&b,&o)&&o.permission_blocked&&f.possess==2);
        put(f.seq,0x398,7u);f.seq[0x3d9]=1;
        assert(control(f.seq,sizeof(f.seq),I::CancelSequence,&b,&o)&&o.suppressed&&!f.cancels);
        f.seq[0x3d9]=0;
        assert(control(f.seq,sizeof(f.seq),I::CancelSequence,&b,&o)&&o.think_cleared&&o.cancelled&&f.cancels==1&&!word(f.seq,8)&&word(f.seq,0x74)==82);
    }

    // Actual 221ae8f0 branches: first ownership, all six modes, invalid
    // mode, successor replacement and full serial handles.
    {
        struct PossessFixture {
            alignas(4) unsigned char seq[scripted_sequence_extent]{},npc[0x974]{},old[scripted_sequence_extent]{},next[scripted_sequence_extent]{};
            sin_independent::EntityHandleEntry entries[4]{};
            float now=23.5f;unsigned flag_calls=0,prep=0,reset=0,angles=0,replaced=0;
        } f;
        const auto possess=reinterpret_cast<PossessScriptedSequence>(GetProcAddress(server,scripted_sequence_possession_export));
        const auto callback=reinterpret_cast<PossessScriptedSequenceCallback>(GetProcAddress(server,scripted_sequence_possession_callback_export));
        assert(possess&&callback&&!GetProcAddress(client,scripted_sequence_possession_export));
        void* actors[]{f.seq,f.npc,f.old,f.next};
        for(unsigned i=0;i<4;++i){f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=3+i;put(actors[i],0x378,((3+i)<<12)|i);}
        for(void* a:{static_cast<void*>(f.seq),static_cast<void*>(f.old),static_cast<void*>(f.next)})put(a,0,scripted_sequence_vtable);
        put(f.npc,0,0x224954a4u);put(f.seq,0x3d0,0x4001u);put(f.seq,0x78,2u);
        ScriptedSequencePossessionBindings b{};b.context=&f;b.entries=f.entries;b.entry_count=4;b.now=&f.now;
        b.extent=[](void* p,void* a,unsigned* n) noexcept ->int{
            auto& f=*static_cast<PossessFixture*>(p);*n=a==f.npc?sizeof(f.npc):a==f.seq||a==f.old||a==f.next?sizeof(f.seq):0;return *n!=0;
        };
        b.add_flags=[](void* p,void* npc,unsigned flags) noexcept ->int{
            auto& f=*static_cast<PossessFixture*>(p);assert(npc==f.npc);unsigned old=0;std::memcpy(&old,f.npc+0x78,4);old|=flags;std::memcpy(f.npc+0x78,&old,4);++f.flag_calls;const unsigned effects=0xabcdu;std::memcpy(f.npc+0xbc,&effects,4);return 1;
        };
        b.move4_prepare=[](void* p,void* npc,const void* pos,unsigned yaw) noexcept ->int{
            auto& f=*static_cast<PossessFixture*>(p);assert(npc==f.npc&&pos==f.seq+0x360&&yaw==0x42b40000u);++f.prep;return 1;
        };
        b.move4_reset=[](void* p,void* npc) noexcept ->int{auto& f=*static_cast<PossessFixture*>(p);assert(npc==f.npc&&f.prep);++f.reset;return 1;};
        b.move4_angles=[](void* p,void* npc,const float* xyz) noexcept ->int{auto& f=*static_cast<PossessFixture*>(p);assert(npc==f.npc&&f.reset&&xyz[0]==1.f&&xyz[1]==90.f&&xyz[2]==3.f);++f.angles;return 1;};
        b.replaced=[](void* p,void* seq,void* displaced) noexcept ->int{auto& f=*static_cast<PossessFixture*>(p);assert(seq==f.seq&&displaced==f.next);++f.replaced;return 1;};
        ScriptedSequencePossessionResult o{};
        for(unsigned mode=0;mode<=6;++mode){
            put(f.npc,0x970,0xffffffffu);put(f.npc,0x78,1u);put(f.npc,0x96c,9u);put(f.npc,0x858,2u);
            put(f.npc,0x864,5u);put(f.npc,0x85c,6u);put(f.seq,0x3d4,0x6003u);put(f.seq,0x3ac,mode);
            put(f.npc,0x36c,0x3f800000u);put(f.npc,0x370,0x40000000u);put(f.npc,0x374,0x40400000u);put(f.seq,0x370,0x42b40000u);
            assert(possess(f.seq,sizeof(f.seq),&b,&o)&&o.claimed&&!o.queued&&o.flags_calls==(mode==4?2u:1u)&&o.group_updates==1);
            assert(word(f.npc,0x970)==0x3000&&word(f.npc,0x90c)==0x3000&&word(f.npc,0x8a8)==0x3000&&!word(f.npc,0x864)&&!word(f.npc,0x85c));
            assert(word(f.seq,0x3c4)==1&&word(f.seq,0x3c8)==0xabcd&&word(f.seq,0x3d4)==0xffffffffu&&word(f.npc,0x858)==4);
            assert(o.state==(mode==6?9:mode>=1&&mode<=3?int(mode+3):1)&&word(f.seq,0x3b8)==1&&word(f.seq,0x3bc)==0x41bc0000u);
        }
        assert(f.prep==1&&f.reset==1&&f.angles==1&&f.flag_calls==8);
        put(f.npc,0x970,0x5002u);put(f.old,0x3d4,0x6003u);put(f.next,0x3d0,0x4001u);
        assert(possess(f.seq,sizeof(f.seq),&b,&o)&&o.queued&&o.displaced&&!o.claimed&&f.replaced==1&&word(f.old,0x3d4)==0x3000&&word(f.next,0x3d0)==0xffffffffu);
        put(f.old,0x3d4,0x5002u);put(f.old,0x3d0,0x4001u);
        b.replaced=[](void*,void*,void*) noexcept ->int{return 1;};
        assert(possess(f.seq,sizeof(f.seq),&b,&o)&&o.queued&&!o.displaced&&word(f.old,0x3d0)==0x4001);
        put(f.seq,0x3d0,0x9001u);
        assert(callback(&b,f.seq)&&possess(f.seq,sizeof(f.seq),&b,&o)&&o.missing&&!o.claimed&&!o.queued);
    }

    // Original name-first selection, fallback permission 2, strict radius
    // and class-search mode. Full packed handles including serial bits.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct AcquireFixture {
            alignas(4) unsigned char seq[scripted_sequence_extent]{},npc[3][0x974]{},old[scripted_sequence_extent]{};
            sin_independent::EntityHandleEntry entries[5]{};
            unsigned warnings=0,refresh=0;
        } f;
        const auto acquire=reinterpret_cast<AcquireScriptedSequenceNpc>(GetProcAddress(server,scripted_sequence_acquire_export));
        assert(acquire&&!GetProcAddress(client,scripted_sequence_acquire_export));
        void* actors[]{f.npc[0],f.npc[1],f.npc[2],f.old,f.seq};
        for(unsigned i=0;i<5;++i){
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=3+i;
            put(actors[i],0x378,((3+i)<<12)|i);
            if(i)f.entries[i].previous=&f.entries[i-1];if(i<4)f.entries[i].next=&f.entries[i+1];
        }
        put(f.seq,0,scripted_sequence_vtable);put(f.old,0,scripted_sequence_vtable);
        put(f.seq,0x3a8,21u);put(f.seq,0x3dc,0xffffffffu);put(f.old,0x3d4,0xffffffffu);
        for(unsigned i=0;i<3;++i){
            put(f.npc[i],0,0x224954a4u);put(f.npc[i],0xc,22u);put(f.npc[i],0xc0,21u);
            put(f.npc[i],0x970,0x6003u);put(f.npc[i],0x16c,1u);const float position=float(i+1);std::memcpy(f.npc[i]+0x2bc,&position,4);
        }
        put(f.npc[2],0x970,0xffffffffu);
        auto extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<AcquireFixture*>(p);*bytes=actor==f.seq||actor==f.old?sizeof(f.seq):
                actor==f.npc[0]||actor==f.npc[1]||actor==f.npc[2]?sizeof(f.npc[0]):0;return *bytes!=0;
        };
        nw::ServerEntitySearchBindings search{};search.context=&f;search.head=f.entries;search.extent=extent;
        search.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==21?"target":token==22?"npc_sintek_grunt":token==0?"":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        ScriptedSequenceControlBindings c{};c.context=&f;c.entries=f.entries;c.entry_count=5;c.extent=extent;
        ScriptedSequenceAcquireBindings b{&c,&search,&f};
        b.denied_warning=[](void* p,unsigned) noexcept ->int{++static_cast<AcquireFixture*>(p)->warnings;return 1;};
        b.refresh_origin=[](void* p,void*) noexcept ->int{++static_cast<AcquireFixture*>(p)->refresh;return 1;};
        ScriptedSequenceAcquireResult o{};
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&o.named_path&&o.preferred&&o.fallback_candidates==2&&o.actor==f.npc[2]&&o.handle==0x5002u);
        put(f.npc[2],0x8c,1);
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&!o.preferred&&o.denied==1&&o.actor==f.npc[1]&&f.warnings==1);
        // Radius exactly 2 excludes the second at distance 2, and
        // skips non-edict first before considering any class fallback.
        put(f.seq,0x3b0,0x40000000u);put(f.seq,0xb8,0x800u);put(f.npc[0],0xb8,0x800u);
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&o.actor==f.npc[0]&&o.radius_skips==2&&f.refresh>=2);
        put(f.npc[0],0x16c,0u);
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&o.class_path&&!o.found&&word(f.seq,0x3d0)==0xffffffffu);
        // Targetname miss -> classname; activity 2 allowed only by
        // mode >=1 (named route), unless force is set.
        put(f.seq,0xb8,0u);put(f.seq,0x3b0,0u);put(f.seq,0x3a8,22u);
        put(f.npc[0],0x850,2);put(f.npc[1],0x850,2);put(f.npc[2],0x8c,0);put(f.npc[2],0x850,2);
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&o.class_path&&o.denied==3&&!o.found);
        put(f.seq,0xb4,0x440u);
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&o.class_path&&o.preferred&&o.cached&&word(f.seq,0x3dc)==0x5002u);
        // Starting after cached candidate, no wrap to earlier rows.
        assert(acquire(f.seq,sizeof(f.seq),&b,&o)&&!o.found&&word(f.seq,0x3dc)==0xffffffffu);
    }

    // Actual acquisition think and cancel leaf: rules gate, retry
    // using clock mutated during warning, successful no-reschedule,
    // normal cleanup and scripted-state cancellation.
    {
        namespace nw=sin_port::native_weapon_tables;
        struct ThinkFixture {
            alignas(4) unsigned char seq[scripted_sequence_extent]{},npc[0x974]{},other[scripted_sequence_extent]{};
            sin_independent::EntityHandleEntry entries[3]{};
            float now=10.f,interval=.125f;
            unsigned reports=0,flags=0,restores=0,cleanup=0,schedule=0;
            unsigned char rules=0;
        } f;
        const auto think=reinterpret_cast<RunScriptedSequenceAcquisitionThink>(GetProcAddress(server,scripted_sequence_think_export));
        const auto cancel=reinterpret_cast<CancelScriptedSequenceNpc>(GetProcAddress(server,scripted_sequence_cancel_export));
        assert(think&&cancel&&!GetProcAddress(client,scripted_sequence_think_export));
        void* actors[]{f.seq,f.npc,f.other};
        for(unsigned i=0;i<3;++i){
            f.entries[i].entity=reinterpret_cast<CBaseEntity*>(actors[i]);f.entries[i].serial=3+i;put(actors[i],0x378,((3+i)<<12)|i);
            if(i)f.entries[i].previous=&f.entries[i-1];if(i<2)f.entries[i].next=&f.entries[i+1];
        }
        put(f.seq,0,scripted_sequence_vtable);put(f.seq,0xc,0x2242ee1cu);put(f.seq,8,0x221afc90u);put(f.seq,0x3a8,21u);
        put(f.seq,0x3d0,0xffffffffu);put(f.seq,0x3dc,0xffffffffu);
        put(f.other,0,scripted_sequence_vtable);put(f.other,0xc,0x2242ee1cu);put(f.other,0x3d0,0xffffffffu);
        put(f.npc,0,0x224954a4u);put(f.npc,0x970,0xffffffffu);put(f.npc,0xc,22u);
        auto extent=[](void* p,void* actor,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<ThinkFixture*>(p);*bytes=actor==f.seq||actor==f.other?sizeof(f.seq):actor==f.npc?sizeof(f.npc):0;return *bytes!=0;
        };
        nw::ServerEntitySearchBindings search{};search.context=&f;search.head=f.entries;search.extent=extent;
        search.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==21?"target":token==22?"npc_sintek_grunt":token==23?"group":token==0x2242ee1c?"scripted_sequence":token==0?"":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerNpcOuterCadenceBindings cadence{};
        ScriptedSequenceControlBindings c{};c.context=&f;c.entries=f.entries;c.entry_count=3;c.extent=extent;c.now=&f.now;c.interval=&f.interval;c.cadence=&cadence;
        ScriptedSequenceAcquireBindings ac{&c,&search,&f};
        ScriptedSequencePossessionBindings pos{};pos.context=&f;pos.entries=f.entries;pos.entry_count=3;pos.extent=extent;pos.now=&f.now;pos.search=&search;
        pos.add_flags=[](void* p,void*,unsigned) noexcept ->int{++static_cast<ThinkFixture*>(p)->flags;return 1;};
        ScriptedSequenceCancelBindings cb{&c,&search,&f};
        cb.restore_flags=[](void* p,void* npc,unsigned flags) noexcept ->int{auto& f=*static_cast<ThinkFixture*>(p);assert(npc==f.npc);std::memcpy(f.npc+0x78,&flags,4);++f.restores;return 1;};
        cb.cleanup_script_state=[](void* p,void* npc) noexcept ->int{auto& f=*static_cast<ThinkFixture*>(p);assert(npc==f.npc&&f.npc[0x96c]==3);++f.cleanup;return 1;};
        cb.cancel_schedule=[](void* p,void* seq,void* npc,unsigned flags) noexcept ->int{auto& f=*static_cast<ThinkFixture*>(p);assert(seq==f.seq&&npc==f.npc&&flags==0x1234&&f.cleanup==1);++f.schedule;return 1;};
        ScriptedSequenceThinkBindings b{&ac,&pos,&cb,&f.rules,&f};
        b.report=[](void* p,void* seq,unsigned stage,void* npc,unsigned) noexcept ->int{
            auto& f=*static_cast<ThinkFixture*>(p);assert(seq==f.seq);
            if(stage==1)assert(npc==f.npc);else {assert(!npc);if(stage==3)f.now+=.25f;}
            ++f.reports;return 1;
        };
        ScriptedSequenceThinkResult o{};ScriptedSequenceCancelResult cr{};
        assert(think(f.seq,sizeof(f.seq),&b,&o)&&o.rules_blocked&&o.retry_scheduled&&o.due_tick==88&&!f.reports);
        f.rules=1;put(f.seq,0xc0,23u);put(f.other,0xc0,23u);put(f.seq,0x3b8,2u);put(f.other,0x3b8,3u);
        assert(think(f.seq,sizeof(f.seq),&b,&o)&&!o.acquired&&o.cancelled==2&&o.retry_time==11.25f&&o.due_tick==90&&f.reports==2&&!word(f.seq,0x3b8)&&!word(f.other,0x3b8));
        put(f.npc,0xc0,21u);put(f.seq,0x74,0xffffffffu);
        assert(think(f.seq,sizeof(f.seq),&b,&o)&&o.acquired&&o.possessed&&!o.retry_scheduled&&f.flags==1&&f.reports==3&&word(f.seq,0x74)==0xffffffffu&&word(f.seq,0x3b8)==2);
        put(f.seq,0x3c4,7u);
        assert(cancel(f.seq,sizeof(f.seq),&cb,&cr)&&cr.cleared&&cr.restored_flags&&f.restores==1&&word(f.npc,0x78)==7&&word(f.npc,0x970)==0xffffffffu&&word(f.seq,0x3d0)==0xffffffffu);
        put(f.seq,0x3d0,0x4001u);put(f.npc,0x970,0x5002u);put(f.other,0x3c8,0x1234u);put(f.npc,0x850,4);
        assert(cancel(f.seq,sizeof(f.seq),&cb,&cr)&&cr.scripted_state&&cr.cleanup&&!cr.cleared&&f.cleanup==1&&f.schedule==1);
        // Actual native AcceptInput datamap traversal, no hosted possession,
        // cancel group or handler callbacks. Exact external input names.
        const auto accept=reinterpret_cast<nw::AcceptScriptedSequenceInput>(GetProcAddress(server,nw::accept_scripted_sequence_export));
        assert(accept&&!GetProcAddress(client,nw::accept_scripted_sequence_export));
        c.cancel_warning=[](void*,unsigned) noexcept ->int{return 1;};
        ScriptedSequenceNativeControlBindings route{&c,&pos,&cb};
        nw::ServerNpcActivePostAnimationBindings storage{};
        nw::ServerEntityAcceptInputBindings input{};input.storage=&storage;input.entities.entries=f.entries;input.entities.count=3;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==71?"mOvEtOpOsItIoN":token==72?"BeginSequence":token==73?"CancelSequence":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntityAcceptInputResult ar{};ScriptedSequenceControlResult sr{};
        unsigned value[5]{42,0,0,0,5};
        auto send=[&](unsigned token){return accept(f.seq,sizeof(f.seq),scripted_sequence_vtable,token,f.npc,nullptr,value,sizeof(value),99,&input,&ar,&route,&sr);};
        put(f.seq,0xc0,0u);put(f.seq,0x3d0,0x4001u);put(f.npc,0x970,0xffffffffu);
        put(f.npc,0x850,0u);put(f.npc,0x96c,0u);f.seq[0x1f0]=1;
        assert(send(71)&&ar.completed&&ar.accepted&&ar.native_handler&&ar.dirty&&ar.map==0x22504fc0u&&ar.handler==0x221b0430u&&ar.expected_type==0&&ar.actual_type==5&&ar.converted&&sr.possessed&&f.seq[0x3c0]&&f.seq[0x200]);
        assert(send(73)&&ar.native_handler&&ar.handler==0x221af970u&&sr.cancelled&&word(f.seq,0x3d0)==0xffffffffu&&f.restores==2);
        assert(send(72)&&ar.native_handler&&ar.handler==0x221b0500u&&sr.think_installed&&sr.tick_scheduled&&!f.seq[0x3c0]);
        f.seq[0x3d9]=1;
        assert(send(73)&&ar.accepted&&sr.suppressed&&!sr.cancelled);
        // Same native input graph through original-shaped delayed queue.
        // Deliberately clear owner registry/clock: service must rebind.
        struct SequenceQueue {
            alignas(4) unsigned char root[0x38]{},rows[4][0x38]{};
            unsigned allocated=0,released=0;bool locked=false;
        } q;
        auto get_queue=reinterpret_cast<nw::GetServerEntityNamedEventQueueApi>(GetProcAddress(server,nw::server_entity_named_event_queue_export));
        const auto service=reinterpret_cast<nw::ServiceScriptedSequenceEvents>(GetProcAddress(server,nw::service_scripted_sequences_export));
        assert(get_queue&&service&&!GetProcAddress(client,nw::service_scripted_sequences_export));
        const auto* queue=get_queue(1,sizeof(nw::ServerEntityNamedEventQueueApi));assert(queue);
        nw::ServerEntityNamedEventQueueBindings qb{};qb.context=&q;
        qb.allocate=[](void* p,unsigned* token,void** row,unsigned* bytes) noexcept ->int{
            auto& q=*static_cast<SequenceQueue*>(p);if(q.allocated==4)return 0;*row=q.rows[q.allocated++];*token=q.allocated;*bytes=0x38;return 1;
        };
        qb.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& q=*static_cast<SequenceQueue*>(p);if(!token||token>q.allocated)return 0;*row=q.rows[token-1];*bytes=0x38;return 1;
        };
        qb.string_nonempty=[](void*,unsigned token,bool* yes) noexcept ->int{*yes=token>=71&&token<=74;return 1;};
        auto native_search=search;native_search.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==21?"target":token==74?"seq":token==22?"npc_sintek_grunt":token==0x2242ee1c?"scripted_sequence":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        nw::ServerEntityEventServiceBindings eb{};eb.context=&q;eb.entities=input.entities;eb.resolve=qb.resolve;eb.native_input=&input;eb.native_search=&native_search;
        eb.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<SequenceQueue*>(p);assert(q.locked!=enter);q.locked=enter;return 1;};
        eb.release=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& q=*static_cast<SequenceQueue*>(p);assert(q.locked&&token&&token<=q.allocated&&row==q.rows[token-1]);++q.released;return 1;
        };
        f.seq[0x3d9]=0;put(f.seq,0xc0,74u);put(f.seq,0x3d0,0x4001u);put(f.npc,0x970,0xffffffffu);put(f.npc,0x96c,0u);
        auto stale_c=c;stale_c.entries=nullptr;stale_c.entry_count=0;stale_c.now=nullptr;
        auto stale_pos=pos;stale_pos.entries=nullptr;stale_pos.entry_count=0;stale_pos.now=nullptr;
        auto stale_cb=cb;stale_cb.control=&stale_c;
        ScriptedSequenceNativeControlBindings stale{&stale_c,&stale_pos,&stale_cb};
        nw::ServerEntityNamedEventQueueResult qr{};
        unsigned empty[5]{0,0,0,0xffffffffu,0};
        assert(queue->enqueue_direct(q.root,sizeof(q.root),99,20.f,0x3000u,71,0x4001u,0x5002u,empty,sizeof(empty),.125f,99,&qb,&qr));
        assert(queue->enqueue_direct(q.root,sizeof(q.root),99,20.f,0x3000u,73,0x4001u,0x5002u,empty,sizeof(empty),.25f,99,&qb,&qr));
        nw::ServerEntityVoidOutputAction begin{74,72,0,0,99,.25f};
        assert(queue->enqueue(q.root,sizeof(q.root),99,20.f,0x4001u,0x5002u,&begin,&qb,&qr));
        float live=20.f;eb.live_now=&live;int steps=0;std::uint8_t single=0;eb.live_single_step=&single;
        nw::ServerEntityEventServiceResult er{};nw::ScriptedSequenceServiceSummary sum{};
        auto tick=[&](){return service(q.root,sizeof(q.root),99,-99.f,false,&steps,&eb,&er,&stale,&sum,nullptr,nullptr);};
        assert(tick()&&er.future_stopped&&!sum.attempted&&!q.released);
        live=20.125f;assert(tick()&&er.direct==1&&sum.possessed==1&&f.seq[0x3c0]&&q.released==1);
        live=20.25f;single=1;steps=1;
        assert(tick()&&er.step_stopped&&sum.cancelled==1&&word(f.seq,0x3d0)==0xffffffffu&&q.released==2);
        single=0;
        assert(tick()&&er.native_named_searches==2&&er.named==1&&sum.scheduled==1&&!f.seq[0x3c0]&&word(f.seq,0x74)==162&&q.released==3&&!q.locked&&!word(q.root,0x30));
    }
