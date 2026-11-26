// Real native deferred removal entry and cleanup, using synthetic owned actors.
    {
        struct Removal {
            alignas(4) unsigned char storage[4][case_extent]{};
            void* slots[4]{};
            DeferredRemovalQueue queue{slots,4};
            DeferredRemovalBindings bindings{};
            const Api* api;
            std::string order;
            unsigned released=0;bool append=false,refuse=false;
            Removal(const Api* a):api(a){}
            unsigned index(void* a) noexcept{for(unsigned i=0;i<4;++i)if(a==storage[i])return i;assert(false);return 4;}
        } f(api);
        for(unsigned i=0;i<4;++i){
            auto* a=f.storage[i];put(a,0,i==1?navigation_vtable:case_vtable);put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));
            put(a,0x378,0x1000u+i);put(a,0xb8,0x80u);put(a,0xc0,0x1234u+i);
        }
        auto& b=f.bindings;b.context=&f;b.queue=&f.queue;
        b.update=[](void* p,void* a,unsigned handler) noexcept ->int{
            auto& f=*static_cast<Removal*>(p);const auto i=f.index(a);
            unsigned flags=0;std::memcpy(&flags,static_cast<unsigned char*>(a)+0xb8,4);
            assert(flags==0x81u&&!f.queue.chained_update&&handler==(i==1?0x22177f50u:0x221446f0u));
            f.order+=char('a'+i);f.queue.chained_update=true;
            DeferredRemovalResult again{};
            assert(f.api->defer_logic_remove(a,case_extent,&f.bindings,&again)&&again.already_marked&&!again.queued);
            return 1;
        };
        b.scope=[](void* p,bool enter) noexcept ->int{
            auto& f=*static_cast<Removal*>(p);assert(!f.queue.cleanup_active&&!f.queue.handle_access_disabled);f.order+=enter?'[':']';return 1;
        };
        b.pre_cleanup=[](void* p) noexcept ->int{
            auto& f=*static_cast<Removal*>(p);assert(f.queue.cleanup_active&&!f.queue.handle_access_disabled);f.order+='P';return 1;
        };
        b.release=[](void* p,void* net) noexcept ->int{
            auto& f=*static_cast<Removal*>(p);auto* a=static_cast<unsigned char*>(net)-0x160;const auto i=f.index(a);
            assert(f.queue.cleanup_active&&f.queue.handle_access_disabled);
            if(f.refuse&&i==1){f.order+='!';return 0;}
            f.order+=char('A'+i);++f.released;
            if(f.append&&i==0){
                DeferredRemovalResult r{};assert(!f.api->cleanup_deferred(&f.bindings,&r));
                assert(!f.api->discard_deferred(&f.queue,&r));
                assert(f.api->defer_logic_remove(f.storage[2],case_extent,&f.bindings,&r)&&r.queued);
            }
            // Poison synthetic storage immediately; cleanup must not read it.
            std::memset(a,0xdd,case_extent);return 1;
        };
        DeferredRemovalResult r{};
        assert(api->defer_logic_remove(nullptr,0,nullptr,&r)&&r.completed);
        assert(!api->defer_logic_remove(f.storage[0],case_extent-1,&b,&r)&&!f.queue.count);
        put(f.storage[0],0x168,0);assert(!api->defer_logic_remove(f.storage[0],case_extent,&b,&r)&&!f.queue.count);
        put(f.storage[0],0x168,ptrword(f.storage[0]));
        f.queue.capacity=0;assert(!api->defer_logic_remove(f.storage[0],case_extent,&b,&r)&&word(f.storage[0],0xb8)==0x80u);
        f.queue.capacity=4;
        assert(api->defer_logic_remove(f.storage[0],case_extent,&b,&r)&&r.marked&&r.updated&&r.name_cleared&&r.queued&&f.queue.count==1&&!word(f.storage[0],0xc0));
        assert(api->defer_logic_remove(f.storage[1],navigation_extent,&b,&r)&&r.queued&&f.queue.count==2);
        assert(f.slots[0]==f.storage[0]+0x160&&f.slots[1]==f.storage[1]+0x160&&f.order=="ab");
        put(f.storage[3],0x378,0xffffffffu);
        assert(api->defer_logic_remove(f.storage[3],case_extent,&b,&r)&&r.invalid_handle&&!r.queued&&r.updated&&!word(f.storage[3],0xc0)&&f.queue.count==2);
        assert(api->defer_logic_remove(f.storage[3],case_extent,&b,&r)&&r.already_marked);
        f.append=true;f.refuse=true;
        assert(!api->cleanup_deferred(&b,&r)&&r.released==1&&r.remaining==2&&r.closed&&!f.queue.cleanup_active&&!f.queue.handle_access_disabled);
        assert(f.queue.count==2&&f.slots[0]==f.storage[1]+0x160&&f.slots[1]==f.storage[2]+0x160&&f.order=="abd[PAc!]");
        f.refuse=false;
        assert(api->cleanup_deferred(&b,&r)&&r.released==2&&r.completed&&r.closed&&!f.queue.count&&f.released==3&&f.order=="abd[PAc!][PBC]");
        assert(api->cleanup_deferred(&b,&r)&&!r.released&&r.pre_called&&f.order=="abd[PAc!][PBC][P]");
        // Original discard helper clears count without invoking Release.
        f.slots[0]=f.storage[3]+0x160;f.queue.count=1;
        assert(api->discard_deferred(&f.queue,&r)&&r.discarded==1&&!f.queue.count&&f.released==3&&f.slots[0]==f.storage[3]+0x160);
        // Recreate a synthetic owned actor after the earlier release.
        auto* a=f.storage[0];std::memset(a,0,case_extent);
        put(a,0,case_vtable);put(a,0x160,0x22454f00u);put(a,0x168,ptrword(a));put(a,0x378,0x2000u);
        put(a,0x20c,0xffffffffu);put(a,0xb8,0x80u);put(a,0xc0,7u);a[0x1f0]=1;
        f.order.clear();f.append=false;
        EntityKillBindings kill{};kill.deferred=&b;
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.logic_kill=&kill;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token!=0x2240b624u)return 0;*text="Kill";*bytes=5;return 1;
        };
        nw::ServerEntityAcceptInputResult accepted{};
        unsigned empty[5]{0,0,0,0xffffffffu,0};
        for(unsigned i=0;i<2;++i){
            assert(timer_accept->run(a,case_extent,case_vtable,0x2240b624u,nullptr,nullptr,empty,sizeof(empty),i,&input,&accepted)&&accepted.native_handler&&accepted.dirty);
            assert(f.queue.count==1&&f.slots[0]==a+0x160&&f.order=="a"&&word(a,0xb8)==0x81u&&!word(a,0xc0)&&a[0x200]);
        }
        assert(api->cleanup_deferred(&b,&r)&&r.released==1&&f.order=="a[PA]"&&f.released==4);
        // One-shot logic_auto can also remove through the queue with no
        // remove callback, after invoking its original outputs.
        std::memset(a,0,case_extent);put(a,0,auto_vtable);put(a,0x160,0x22454f00u);
        put(a,0x168,ptrword(a));put(a,0x378,0x3000u);put(a,0xb8,0x80u);put(a,0xb4,1u);put(a,0xc0,7u);
        AutoBindings startup{};int load=0;startup.load_type=&load;startup.deferred=&b;
        nw::ServerEntityVoidOutputBindings fire{};
        fire.enqueue=[](void*,void*,void*,const nw::ServerEntityVoidOutputAction*) noexcept ->int{assert(false);return 0;};
        startup.output=&fire;startup.low_violence=[](void*,bool* low) noexcept ->int{*low=false;return 1;};
        AutoResult ar{};f.order.clear();
        assert(api->auto_think(a,auto_extent,&startup,&ar)&&ar.outputs==3&&!ar.queued&&ar.removed&&f.order=="a"&&f.queue.count==1&&!word(a,0xc0));
        assert(api->cleanup_deferred(&b,&r)&&r.released==1&&f.order=="a[PA]"&&f.released==5);
    }
