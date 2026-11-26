// Trigger-specific destructor: vector purge, derived outputs, registry,
// base outputs, owner suffix and scalar-delete free.
    {
        struct TriggerTeardown {
            alignas(4) unsigned char storage[dynamic_trigger_extent]{},rows[4][0x1c]{},vector[16]{};
            sin_independent::EntityHandleEntry entry{};
            RemovalRegistry registry{&entry,1,{&entry,&entry},{},1,0,false};
            RegistryRemovalBindings registry_bind{};
            RegistryDestroyBindings base{};
            LogicDestroyBindings shared{};
            TriggerDestroyBindings trigger{};
            std::string order;
            bool external=false,alias=false;
        } f;
        auto* a=f.storage;
        f.registry_bind.registry=&f.registry;f.base.registry=&f.registry_bind;f.base.context=&f;f.base.native_outputs=true;
        f.base.prefix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);assert(a==f.storage&&f.order==(f.external?"321":"V321")&&f.entry.entity);
            f.order+='P';return 1;
        };
        f.base.suffix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);assert(a==f.storage&&!f.entry.entity&&f.order==(f.external?"321P4":"V321P4"));f.order+='S';return 1;
        };
        f.shared.context=&f;f.shared.native_base=&f.base;
        f.shared.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);if(token<1||token>4)return 0;*row=f.rows[token-1];*bytes=0x1c;return 1;
        };
        f.shared.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);assert(token>=1&&token<=4&&row==f.rows[token-1]);
            assert(bool(f.entry.entity)==(token!=4));f.order+=char('0'+token);return 1;
        };
        f.shared.free_actor=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);assert(a==f.storage&&!f.entry.entity);f.order+='F';return 1;
        };
        f.trigger.context=&f;f.trigger.shared=&f.shared;
        f.trigger.resolve_vector=[](void* p,unsigned token,void** ptr,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);assert(token==9);*ptr=f.alias?f.rows[0]:f.vector;*bytes=16;return 1;
        };
        f.trigger.free_vector=[](void* p,unsigned token,void* ptr) noexcept ->int{
            auto& f=*static_cast<TriggerTeardown*>(p);unsigned size=99;std::memcpy(&size,f.storage+0x468,4);
            assert(token==9&&ptr==f.vector&&!size&&f.order.empty()&&f.entry.entity&&!f.external);f.order+='V';return 1;
        };
        for(unsigned vt:{dynamic_trigger_vtable,secret_trigger_vtable}){
            f.external=vt==secret_trigger_vtable;f.order.clear();
            std::memset(a,0,dynamic_trigger_extent);put(a,0,vt);put(a,0x378,0x3000u);
            f.entry={};f.entry.serial=3;f.entry.entity=reinterpret_cast<CBaseEntity*>(a);
            f.registry.active={&f.entry,&f.entry};f.registry.entity_count=1;
            put(a,0x428,1u);put(a,0x440,2u);put(a,0x458,3u);put(a,0x2f0,4u);
            put(a,0x45c,9u);put(a,0x460,4u);put(a,0x464,f.external?0xffffffffu:0u);put(a,0x468,2u);put(a,0x46c,9u);
            const unsigned n=removal_entity_extent(vt);LogicDestroyResult dr{};
            assert(!api->destroy_logic(a,n,1,&f.shared,&dr)&&f.order.empty());
            assert(!api->destroy_trigger_members(a,n-1,1,&f.trigger,&dr)&&f.order.empty());
            put(a,0x468,5u);assert(!api->destroy_trigger_members(a,n,1,&f.trigger,&dr)&&!dr.validated&&f.order.empty());
            put(a,0x468,2u);
            if(!f.external){f.alias=true;assert(!api->destroy_trigger_members(a,n,1,&f.trigger,&dr)&&!dr.validated&&f.order.empty());f.alias=false;}
            assert(api->destroy_trigger_members(a,n,f.external?0u:1u,&f.trigger,&dr)&&dr.completed&&dr.outputs==3&&dr.released==3&&dr.base_outputs==5&&dr.base_released==1&&dr.registry_unlinked&&dr.trigger_vector_purged);
            assert(!word(a,0x468)&&word(a,0x45c)==(f.external?9u:0u)&&word(a,0x460)==(f.external?4u:0u)&&word(a,0x46c)==(f.external?9u:0u));
            assert(dr.trigger_vector_external==unsigned(f.external)&&dr.trigger_vector_freed==unsigned(!f.external)&&dr.freed==unsigned(!f.external));
            assert(f.order==(f.external?"321P4S":"V321P4SF")&&!f.entry.entity&&f.entry.serial==4);
        }
    }
