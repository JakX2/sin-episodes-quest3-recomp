// Actual native map parser and original-shaped trigger allocations.
    {
        struct Triggers {
            alignas(4) unsigned char storage[2][dynamic_trigger_extent]{};
            unsigned allocated=0,base=0,initialized=0,foreign=0,accepted=0,released=0,cleared=0;
        } f;
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
            ++f.base;return 1;
        };
        trigger.initialize=[](void* p,void* a,const char* name) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);unsigned vt=0;std::memcpy(&vt,a,4);
            assert(a==f.storage[f.initialized]&&vt==(f.initialized?secret_trigger_vtable:dynamic_trigger_vtable)&&!std::strcmp(name,f.initialized?"trigger_secret":"trigger_dynamic_difficulty"));++f.initialized;return 1;
        };
        trigger.release_networkable=[](void* p,void* n) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(n==f.storage[f.released]+0x160);++f.released;return 1;
        };
        FactoryBindings factories{};factories.dictionary=&d;factories.campaign_triggers=&trigger;
        KeyBindings keys{};keys.context=&f;
        keys.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);*bytes=a==f.storage[0]?dynamic_trigger_extent:a==f.storage[1]?secret_trigger_extent:0;return *bytes!=0;
        };
        keys.intern=[](void*,const char* value,unsigned* token) noexcept ->int{
            *token=!std::strcmp(value,"trigger_dynamic_difficulty")?71u:!std::strcmp(value,"trigger_secret")?72u:!std::strcmp(value,"secret01")?73u:74u;return 1;
        };
        keys.foreign=[](void* p,void*,const char* key,const char*,bool* handled) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(!std::strcmp(key,"model"));++f.foreign;*handled=true;return 1;
        };
        Bindings parse{};parse.context=&f;parse.native_factory=&factories;parse.native_keys=&keys;
        parse.clear_factory_global=[](void* p) noexcept ->int{++static_cast<Triggers*>(p)->cleared;return 1;};
        parse.accept_entity=[](void* p,void* a,const char*,unsigned,const EntitySlice*) noexcept ->int{
            auto& f=*static_cast<Triggers*>(p);assert(a==f.storage[f.accepted]);++f.accepted;return 1;
        };
        const char source[]="{ classname trigger_dynamic_difficulty EstimatedTime 12.5 EstimatedDifficulty -3 EndOfGame 257 spawnflags 1024 model *1 } { classname trigger_secret targetname secret01 model *2 }";
        sin_port::native_map::Result result{};
        assert(api->parse(source,sizeof(source),&parse,&result)&&result.completed&&result.native_creates==2&&f.accepted==2&&f.base==2&&f.initialized==2&&f.foreign==2&&f.cleared==2);
        assert(word(f.storage[0],0x470)==0x41480000u&&word(f.storage[0],0x474)==0xfffffffdu&&word(f.storage[0],0x478)==257&&word(f.storage[0],0xb4)==1024&&word(f.storage[1],0xc0)==73);
        assert(!api->destroy_campaign_trigger(f.storage[0],dynamic_trigger_extent-1,f.storage[0]+0x160,&trigger));
        assert(api->destroy_campaign_trigger(f.storage[0],dynamic_trigger_extent,f.storage[0]+0x160,&trigger)&&api->destroy_campaign_trigger(f.storage[1],secret_trigger_extent,f.storage[1]+0x160,&trigger)&&f.released==2);
    }
