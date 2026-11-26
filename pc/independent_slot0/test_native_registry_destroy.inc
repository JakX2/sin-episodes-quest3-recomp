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
        // Five base outputs, with a second action on the highest member.
        // Tokens use the same owned 28-byte action pool as the derived output.
        for(unsigned i=0;i<5;++i)put(a,0x2dc+i*0x18+0x14,ptrword(f.pool.rows[i+1]));
        put(f.pool.rows[5],0x18,ptrword(f.pool.rows[6]));
        f.pool.used=7;
        f.registry.registry=&f.reg;f.base.registry=&f.registry;f.base.context=&f;f.base.native_outputs=true;
        f.base.prefix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(a==f.actor&&f.queue.handle_access_disabled&&f.order=="UO");
            unsigned vt=0,h=0;std::memcpy(&vt,f.actor,4);std::memcpy(&h,f.actor+0x378,4);
            assert(vt==0x2240ac2cu&&h==0x3000u&&f.entry.entity==reinterpret_cast<CBaseEntity*>(a));f.order+='P';return 1;
        };
        f.base.suffix=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(a==f.actor&&f.queue.handle_access_disabled&&f.order=="UOP564321");
            unsigned h=0;std::memcpy(&h,f.actor+0x378,4);
            assert(h==0xffffffffu&&!f.entry.entity&&f.entry.serial==4&&!f.reg.entity_count&&!f.reg.active.head&&!f.reg.active.tail);
            f.order+='S';return 1;
        };
        f.logic.context=&f;f.logic.native_base=&f.base;
        f.logic.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);
            for(unsigned i=0;i<7;++i)if(token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(f.pool.rows[i]))){*row=f.pool.rows[i];*bytes=0x1c;return 1;}
            return 0;
        };
        f.logic.release_action=[](void* p,unsigned token,void* row) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(f.queue.handle_access_disabled&&token==static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(row)));
            unsigned i=0;while(i<7&&row!=f.pool.rows[i])++i;assert(i<7);
            if(!i){assert(f.order=="U"&&f.entry.entity);f.order+='O';}
            else{assert(!f.entry.entity&&f.entry.serial==4);f.order+=char('0'+i);}
            ++f.pool.released;return 1;
        };
        f.logic.free_actor=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);unsigned vt=0;std::memcpy(&vt,a,4);assert(vt==0x2240a088u&&f.order=="UOP564321S"&&f.queue.handle_access_disabled);
            f.order+='F';std::memset(a,0xdd,relay_extent);return 1;
        };
        DeferredRemovalBindings deferred{};deferred.context=&f;deferred.queue=&f.queue;
        deferred.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(a==f.actor&&handler==0x221446f0u&&!f.queue.handle_access_disabled);f.order+='U';return 1;
        };
        deferred.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<Teardown*>(p);assert(net==f.actor+0x160&&f.queue.handle_access_disabled);
            LogicDestroyResult r{};return f.api->destroy_logic(f.actor,relay_extent,1,&f.logic,&r)&&r.completed&&r.released==1&&r.base_prefix&&r.registry_unlinked&&r.base_outputs==5&&r.base_actions==6&&r.base_released==6&&r.base_suffix&&r.freed;
        };
        DeferredRemovalResult dr{};
        assert(api->defer_logic_remove(a,sizeof(f.actor),&deferred,&dr)&&dr.queued&&f.order=="U"&&!f.pool.released);
        LogicDestroyResult invalid{};
        // A shared base/derived row must fail preflight without releasing anything.
        put(a,0x2dc+0x14,ptrword(f.pool.rows[0]));
        assert(!api->destroy_logic(a,relay_extent,1,&f.logic,&invalid)&&!invalid.validated&&!f.pool.released&&f.order=="U");
        put(a,0x2dc+0x14,ptrword(f.pool.rows[1]));
        put(f.pool.rows[6],0x18,ptrword(f.pool.rows[5]));
        assert(!api->destroy_logic(a,relay_extent,1,&f.logic,&invalid)&&!invalid.validated&&!f.pool.released&&f.order=="U");
        put(f.pool.rows[6],0x18,0u);
        assert(api->cleanup_deferred(&deferred,&dr)&&dr.released==1&&f.order=="UOP564321SF"&&f.pool.released==7&&!f.queue.count&&!f.queue.handle_access_disabled);
        assert(!sin_independent::EntityHandleView(&f.entry,1).Resolve(0x3000u)&&word(a,0)==0xddddddddu);
    }
