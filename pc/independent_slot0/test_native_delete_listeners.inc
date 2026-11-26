// Native removal-update prefix: reverse live listener traversal and
    // navigation unregister before notification. Registry remains live.
    {
        struct Notices {
            alignas(4) unsigned char actor[navigation_extent]{};
            unsigned ids[4]{0,1,2,3};
            void* listeners[5]{};
            unsigned count=4;
            void* deletion[1]{};
            DeferredRemovalQueue queue{deletion,1};
            sin_independent::EntityHandleEntry entry{};
            std::string order;
            bool mutate=true;
        } f;
        auto* a=f.actor;put(a,0,navigation_vtable);put(a,0x394,navigation_listener_vtable);a[0x398]=1;
        put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));put(a,0x378,0x3000u);put(a,0xc0,55u);
        f.entry.serial=3;f.entry.entity=reinterpret_cast<CBaseEntity*>(a);
        f.listeners[0]=&f.ids[0];f.listeners[1]=a+0x394;f.listeners[2]=&f.ids[1];f.listeners[3]=&f.ids[2];
        DeletionListenerBindings listeners{};listeners.context=&f;listeners.entries=&f.entry;listeners.entry_count=1;
        listeners.slots=f.listeners;listeners.count=&f.count;listeners.capacity=5;
        listeners.deleted=[](void* p,void* listener,void* actor) noexcept ->int{
            auto& f=*static_cast<Notices*>(p);assert(actor==f.actor&&f.queue.chained_update&&f.queue.reserved==1&&!f.queue.count);
            unsigned flags=0,name=0;std::memcpy(&flags,f.actor+0xb8,4);std::memcpy(&name,f.actor+0xc0,4);
            assert((flags&1u)&&name==55&&sin_independent::EntityHandleView(&f.entry,1).Resolve(0x3000u)==actor);
            auto* id=static_cast<unsigned*>(listener);assert(id>=f.ids&&id<f.ids+4);
            f.order+=char('0'+*id);
            if(f.mutate&&*id==2){f.listeners[0]=&f.ids[3];f.listeners[3]=&f.ids[0];f.count=4;}
            return 1;
        };
        NativeRemovalUpdateBindings update{};update.listeners=&listeners;update.context=&f;
        update.tail=[](void* p,void* actor) noexcept ->int{auto& f=*static_cast<Notices*>(p);assert(actor==f.actor&&f.queue.chained_update&&f.queue.reserved==1);f.order+='T';return 1;};
        DeferredRemovalBindings deferred{};deferred.queue=&f.queue;deferred.native_update=&update;
        DeletionListenerResult nr{};
        assert(api->notify_deleting(0x4000u,&listeners,&nr)&&nr.stale&&!nr.notified&&f.order.empty());
        DeferredRemovalResult dr{};
        assert(api->defer_logic_remove(a,sizeof(f.actor),&deferred,&dr)&&dr.native_update&&dr.unregistered==1&&dr.notified==3&&dr.queued);
        assert(f.order=="213T"&&f.count==4&&a[0x398]==1&&f.listeners[0]==&f.ids[3]&&f.listeners[1]==&f.ids[1]&&f.listeners[2]==&f.ids[2]&&f.listeners[3]==&f.ids[0]);
        assert(!word(a,0xc0)&&word(a,0x378)==0x3000u&&f.queue.count==1&&f.queue.chained_update);
        assert(api->defer_logic_remove(a,sizeof(f.actor),&deferred,&dr)&&dr.already_marked&&f.order=="213T");
        assert(api->discard_deferred(&f.queue,&dr));
        // Invalid handle skips all listener notifications but still runs
        // the base tail. Missing registration is a silent no-op.
        put(a,0xb8,0u);put(a,0x378,0xffffffffu);put(a,0xc0,55u);f.order.clear();
        assert(api->defer_logic_remove(a,sizeof(f.actor),&deferred,&dr)&&dr.native_update&&dr.stale_notification&&dr.invalid_handle&&!dr.queued&&!dr.unregistered&&!dr.notified);
        assert(f.order=="T"&&!word(a,0xc0)&&!f.queue.count&&a[0x398]);
    }
