// Native descendant snapshot, reverse requests, first-equal handle vector.
    {
        struct Hierarchy {
            alignas(4) unsigned char actors[4][relay_extent]{};
            sin_independent::EntityHandleEntry entries[4]{};
            unsigned handles[7]{0x9000u,0x3000u,0x4001u,0x5002u,0x3000u,0xffffffffu};
            unsigned handle_count=6,listener_count=0;
            void* slots[4]{};
            DeferredRemovalQueue queue{slots,4};
            std::string order;
            unsigned index(void* a) noexcept{for(unsigned i=0;i<4;++i)if(a==actors[i])return i;assert(false);return 4;}
        } f;
        auto init=[&](){
            for(unsigned i=0;i<4;++i){
                auto* a=f.actors[i];std::memset(a,0,relay_extent);put(a,0,i==3?0x12345678u:relay_vtable);
                put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));put(a,0x378,((3+i)<<12)|i);
                put(a,0xf4,0xffffffffu);put(a,0xf8,0xffffffffu);
                f.entries[i].entity=reinterpret_cast<CBaseEntity*>(a);f.entries[i].serial=3+i;
            }
            put(f.actors[0],0xf4,0x4001u);put(f.actors[1],0xf8,0x5002u);put(f.actors[1],0xf4,0x6003u);
        };
        init();
        DeletionListenerBindings listeners{};listeners.entries=f.entries;listeners.entry_count=4;listeners.count=&f.listener_count;
        RemovalMetadataBindings metadata{};
        RemovalHierarchyBindings hierarchy{};hierarchy.context=&f;hierarchy.handles=f.handles;hierarchy.count=&f.handle_count;hierarchy.capacity=7;
        hierarchy.before_collect=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Hierarchy*>(p);const auto i=f.index(a);
            f.order+=char('a'+i);
            // Owner detach removes child from parent's live sibling chain.
            unsigned invalid=0xffffffffu;
            if(i==2)std::memcpy(f.actors[1]+0xf8,&invalid,4);
            if(i==1)std::memcpy(f.actors[0]+0xf4,&invalid,4);
            return 1;
        };
        hierarchy.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{
            auto& f=*static_cast<Hierarchy*>(p);assert(f.index(a)<4);*bytes=relay_extent;return 1;
        };
        hierarchy.warning=[](void* p,void* a,unsigned count) noexcept ->int{
            auto& f=*static_cast<Hierarchy*>(p);assert(a==f.actors[0]&&count==3&&f.order=="a");f.order+='W';return 1;
        };
        hierarchy.foreign_remove=[](void* p,void* a) noexcept ->int{
            auto& f=*static_cast<Hierarchy*>(p);assert(a==f.actors[3]&&f.order=="aWcC");f.order+='d';
            unsigned invalid=0xffffffffu;std::memcpy(f.actors[1]+0xf4,&invalid,4);return 1;
        };
        NativeRemovalUpdateBindings update{};update.context=&f;update.listeners=&listeners;update.metadata=&metadata;update.hierarchy=&hierarchy;
        update.tail=[](void* p,void* a) noexcept ->int{auto& f=*static_cast<Hierarchy*>(p);f.order+=char('A'+f.index(a));return 1;};
        DeferredRemovalBindings deferred{};deferred.queue=&f.queue;deferred.native_update=&update;
        DeferredRemovalResult r{};
        // A cycle is rejected before the first child removal, even though
        // the original parent update has already started.
        put(f.actors[3],0xf8,0x4001u);
        assert(!api->defer_logic_remove(f.actors[0],relay_extent,&deferred,&r)&&r.children_collected==2&&!r.children_requested&&f.order=="a"&&!f.queue.count&&!f.queue.reserved);
        init();f.order.clear();
        assert(api->defer_logic_remove(f.actors[0],relay_extent,&deferred,&r)&&r.hierarchy&&r.children_collected==3&&r.children_requested==3&&r.children_native==2&&r.children_foreign==1&&r.handle_vector_removed&&r.handle_vector_index==1);
        assert(f.order=="aWcCdbBA"&&f.queue.count==3&&f.slots[0]==f.actors[2]+0x160&&f.slots[1]==f.actors[1]+0x160&&f.slots[2]==f.actors[0]+0x160);
        assert(f.handle_count==3&&f.handles[0]==0x9000u&&f.handles[1]==0x3000u&&f.handles[2]==0xffffffffu);
        assert(api->discard_deferred(&f.queue,&r)&&r.discarded==3);
        // Original handle comparison also equates invalid/stale to null.
        init();f.order.clear();put(f.actors[0],0x378,0xffffffffu);put(f.actors[0],0xf4,0xffffffffu);
        assert(api->defer_logic_remove(f.actors[0],relay_extent,&deferred,&r)&&r.hierarchy&&r.handle_vector_removed&&r.handle_vector_index==0&&r.invalid_handle&&!r.queued);
        assert(f.order=="aA"&&f.handle_count==2&&f.handles[0]==0x3000u&&f.handles[1]==0xffffffffu);
    }
