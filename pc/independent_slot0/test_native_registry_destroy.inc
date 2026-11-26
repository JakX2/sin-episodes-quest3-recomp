// Derived output destruction -> base prefix -> native registry unlink
    // -> base suffix -> scalar-delete free, under deferred cleanup.
    {
        struct Teardown {
            alignas(4) unsigned char actor[relay_extent]{};
            ActionPool pool{};
            sin_independent::EntityHandleEntry entry{};
            RemovalRegistry reg{&entry,1,{&entry,&entry},{},1,0,false};
            RegistryRemovalBindings registry{};
            RegistryDestroyBindings base{};
            LogicDestroyBindings logic{};
            void* slots[1]{};
            DeferredRemovalQueue queue{slots,1};
            const sin_port::native_map::Api* api;
            std::string order;
            Teardown(const sin_port::native_map::Api* a):api(a){}
        } f(api);
        auto* a=f.actor;put(a,0,relay_vtable);put(a,0x378,0x3000u);put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));put(a,0xc0,55u);
        f.entry.serial=3;f.entry.entity=reinterpret_cast<CBaseEntity*>(a);
        OutputActionBindings actions=output;actions.context=&f.pool;actions.next_id=&f.pool.id;
        auto keys=kb;keys.native_output=&actions;keys.extent=[](void*,void*,unsigned* bytes) noexcept ->int{*bytes=relay_extent;return 1;};
        assert(api->key_value(a,"OnTrigger","relay,Trigger,,0,-1",&keys,&kr));
        f.registry.registry=&f.reg;f.base.registry=&f.registry;f.base.context=&f;
        f.base.prefix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(a==f.actor&&f.queue.handle_access_disabled&&f.order=="UO");
            unsigned vt=0,h=0;std::memcpy(&vt,f.actor,4);std::memcpy(&h,f.actor+0x378,4);
            assert(vt==0x2240ac2cu&&h==0x3000u&&f.entry.entity==reinterpret_cast<CBaseEntity*>(a));f.order+='P';return 1;
        };
        f.base.suffix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(a==f.actor&&f.queue.handle_access_disabled&&f.order=="UOP");
            unsigned h=0;std::memcpy(&h,f.actor+0x378,4);
            assert(h==0xffffffffu&&!f.entry.entity&&f.entry.serial==4&&!f.reg.entity_count&&!f.reg.active.head&&!f.reg.active.tail);
            f.order+='S';return 1;
        };
        f.logic.context=&f;f.logic.native_base=&f.base;
        f.logic.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);if(token!=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool.rows[0])))return 0;*row=f.pool.rows[0];*bytes=0x1c;return 1;
        };
        f.logic.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(f.queue.handle_access_disabled&&f.order=="U"&&row==f.pool.rows[0]&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));f.order+='O';++f.pool.released;return 1;
        };
        f.logic.free_actor=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);unsigned vt=0;std::memcpy(&vt,a,4);assert(vt==0x2240a088u&&f.order=="UOPS"&&f.queue.handle_access_disabled);
            f.order+='F';std::memset(a,0xdd,relay_extent);return 1;
        };
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&f.queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(a==f.actor&&handler==0x221446f0u&&!f.queue.handle_access_disabled);f.order+='U';return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(net==f.actor+0x160&&f.queue.handle_access_disabled);
            LogicDestroyResult r{};return f.api->destroy_logic(f.actor,relay_extent,1,&f.logic,&r)&&r.completed&&r.released==1&&r.base_prefix&&r.registry_unlinked&&r.base_suffix&&r.freed;
        };
        DeferredRemovalResult dr{};
        assert(api->defer_logic_remove(a,sizeof(f.actor),&deferred,&dr)&&dr.queued&&f.order=="U"&&!f.pool.released);
        assert(api->cleanup_deferred(&deferred,&dr)&&dr.released==1&&f.order=="UOPSF"&&f.pool.released==1&&!f.queue.count&&!f.queue.handle_access_disabled);
        assert(!sin_independent::EntityHandleView(&f.entry,1).Resolve(0x3000u)&&word(a,0)==0xddddddddu);
    }
