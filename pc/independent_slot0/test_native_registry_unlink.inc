// Original active/free-chain removal, serial mask, edict mirror.
    {
        using Entry=sin_independent::EntityHandleEntry;
        Entry entries[2051]{};
        alignas(4) unsigned char edict_actor[relay_extent]{},first[relay_extent]{},middle[relay_extent]{},last[relay_extent]{},edict[8]{};
        auto setup=[&](unsigned i,void* a,unsigned serial){
            put(a,0,relay_vtable);put(a,0x378,(serial<<12)|i);entries[i].entity=reinterpret_cast<CBaseEntity*>(a);entries[i].serial=serial;
        };
        setup(1,edict_actor,3);setup(2048,first,0x7fff);setup(2049,middle,7);setup(2050,last,9);
        put(edict_actor,0x16c,123u);put(edict,4,3u);
        auto link=[&](unsigned x,unsigned y){entries[x].next=&entries[y];entries[y].previous=&entries[x];};
        link(1,2048);link(2048,2049);link(2049,2050);
        RemovalRegistry reg{entries,2051,{&entries[1],&entries[2050]},{},4,1,false};
        RegistryRemovalBindings bind{};bind.context=edict;bind.registry=&reg;
        bind.edict=[](void* p,unsigned token,void** out,unsigned* bytes) noexcept ->int{if(token!=123)return 0;*out=p;*bytes=8;return 1;};
        RegistryRemovalResult rr{};
        const unsigned old=0x7fff800u;
        assert(!api->registry_unlink_logic(first,sizeof(first),old-0x1000,&bind,&rr)&&entries[2048].entity&&reg.entity_count==4);
        // Reject malformed neighboring links before changing actor/registry.
        entries[2049].previous=nullptr;
        assert(!api->registry_unlink_logic(first,sizeof(first),old,&bind,&rr)&&word(first,0x378)==old&&entries[2048].serial==0x7fff);
        entries[2049].previous=&entries[2048];
        assert(api->registry_unlink_logic(first,sizeof(first),old,&bind,&rr)&&rr.unlinked&&rr.returned_to_free&&rr.new_serial==0&&rr.old_handle==old);
        assert(!sin_independent::EntityHandleView(entries,2051).Resolve(old)&&word(first,0x378)==0xffffffffu&&reg.entity_count==3);
        assert(entries[1].next==&entries[2049]&&entries[2049].previous==&entries[1]&&reg.free_nonnetworked.head==&entries[2048]&&reg.free_nonnetworked.tail==&entries[2048]);
        assert(api->registry_unlink_logic(first,sizeof(first),old,&bind,&rr)&&rr.empty&&reg.entity_count==3);
        auto bad=bind;bad.edict=nullptr;
        assert(!api->registry_unlink_logic(edict_actor,sizeof(edict_actor),0x3001u,&bad,&rr)&&reg.edict_count==1&&word(edict_actor,0x378)==0x3001u);
        assert(api->registry_unlink_logic(edict_actor,sizeof(edict_actor),0x3001u,&bind,&rr)&&rr.edict_cleared&&!rr.returned_to_free&&rr.new_serial==4);
        assert(word(edict,4)==0x3ffu&&!reg.edict_count&&reg.active.head==&entries[2049]&&!entries[2049].previous&&entries[1].previous==&entries[1]&&entries[1].next==&entries[1]);
        // Original base destructor vtable is also accepted.
        put(last,0,0x2240ac2cu);
        assert(api->registry_unlink_logic(last,sizeof(last),0x9802u,&bind,&rr)&&rr.returned_to_free&&reg.active.tail==&entries[2049]&&!entries[2049].next);
        assert(api->registry_unlink_logic(middle,sizeof(middle),0x7801u,&bind,&rr)&&!reg.entity_count&&!reg.active.head&&!reg.active.tail);
        assert(reg.free_nonnetworked.head==&entries[2048]&&reg.free_nonnetworked.tail==&entries[2049]&&entries[2048].next==&entries[2050]&&entries[2050].next==&entries[2049]&&entries[2049].previous==&entries[2050]&&!entries[2049].next);
    }
