// Shared inherited Kill projection, including classes with no own input rows.
    {
        struct Layout {unsigned vt,extent,rows;};
        constexpr Layout classes[]{
            {relay_vtable,relay_extent,9},{auto_vtable,auto_extent,8},{timer_vtable,timer_extent,14},
            {branch_vtable,branch_extent,8},{case_vtable,case_extent,35},{compare_vtable,compare_extent,10},
            {counter_vtable,counter_extent,15},{menu_vtable,menu_extent,16},{navigation_vtable,navigation_extent,5},
            {autosave_vtable,autosave_extent,2},{multicompare_vtable,multicompare_extent,7},
            {remap_vtable,remap_extent,6},{colorblend_vtable,colorblend_extent,6}
        };
        alignas(4) unsigned char actor[case_extent]{},attached[4]{};
        struct Kill {void* actor;void* attached;std::string order;bool refuse=false;} f{actor,attached};
        EntityKillBindings kill{};kill.context=&f;
        kill.attachment_notice=[](void* p,void* a,void* entity) noexcept ->int{
            auto& f=*static_cast<Kill*>(p);assert(a==f.attached&&entity==f.actor);f.order+='N';return !f.refuse;
        };
        kill.clear_attachment=[](void* p,void* entity) noexcept ->int{
            auto& f=*static_cast<Kill*>(p);assert(entity==f.actor&&f.order=="N");f.order+='C';
            unsigned invalid=0xffffffffu;std::memcpy(static_cast<unsigned char*>(entity)+0x20c,&invalid,4);return 1;
        };
        kill.remove=[](void* p,void* entity) noexcept ->int{
            auto& f=*static_cast<Kill*>(p);assert(entity==f.actor);f.order+='R';return 1;
        };
        sin_independent::EntityHandleEntry entries[2]{};
        entries[0].serial=3;entries[0].entity=reinterpret_cast<CBaseEntity*>(attached);
        entries[1].serial=4;entries[1].entity=reinterpret_cast<CBaseEntity*>(actor);
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.entities.entries=entries;input.entities.count=2;input.logic_kill=&kill;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            if(token==405||token==0x2240b624u)*text="kIlL";else if(token==406)*text="Unrelated";else return 0;
            *bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        const unsigned nonvoid[5]{17,0x11223344,0x55667788,0xffffffffu,5};
        nw::ServerEntityAcceptInputResult result{};
        for(const auto& cls:classes){
            std::memset(actor,0,sizeof(actor));put(actor,0,cls.vt);put(actor,0x378,0x4001u);put(actor,0x20c,0x3000u);actor[0x1f0]=1;
            if(cls.vt==navigation_vtable)put(actor,0x394,navigation_listener_vtable);
            if(cls.vt==menu_vtable)put(actor,0x394,menu_listener_vtable);
            f.order.clear();
            assert(logic_kill_extent(cls.vt)==cls.extent);
            assert(timer_accept->run(actor,cls.extent,cls.vt,405,attached,attached,nonvoid,sizeof(nonvoid),51,&input,&result));
            assert(result.completed&&result.accepted&&result.converted&&result.expected_type==0&&result.actual_type==0&&result.native_handler&&result.native_maps==2&&result.row==78&&result.rows==cls.rows+79&&result.map==0x224f0350u&&result.handler==0x2213cdc0u);
            assert(f.order=="NCR"&&word(actor,0x20c)==0xffffffffu&&actor[0x200]);
            f.order.clear();actor[0x200]=0;put(actor,0x20c,0x9000u);
            assert(timer_accept->run(actor,cls.extent,cls.vt,0x2240b624u,attached,attached,nonvoid,sizeof(nonvoid),52,&input,&result)&&result.native_handler);
            assert(f.order=="R"&&word(actor,0x20c)==0x9000u&&actor[0x200]);
            f.order.clear();actor[0x200]=0;put(actor,0x20c,0x3000u);f.refuse=true;
            assert(!timer_accept->run(actor,cls.extent,cls.vt,405,attached,attached,nonvoid,sizeof(nonvoid),53,&input,&result)&&f.order=="N"&&!actor[0x200]&&word(actor,0x20c)==0x3000u);
            f.refuse=false;f.order.clear();
            assert(!timer_accept->run(actor,cls.extent-1,cls.vt,405,attached,attached,nonvoid,sizeof(nonvoid),54,&input,&result)&&f.order.empty());
            EntityKillResult leaf{};
            assert(!api->logic_kill(actor,cls.extent,0,entries,2,&kill,&leaf)&&!leaf.completed);
        }
        // Kill-only binding works through the real event service, without
        // installing a class-specific behavior or accepting unrelated input.
        std::memset(actor,0,sizeof(actor));put(actor,0,auto_vtable);put(actor,0x378,0x4001u);put(actor,0x20c,0x3000u);actor[0x1f0]=1;
        struct QueueKill {ConnectedOutput* q;Kill* f;unsigned foreign=0;} qk{&co,&f};
        nw::ServerEntitySearchBindings search{};search.context=&qk;
        search.extent=[](void* p,void* a,unsigned* bytes) noexcept ->int{auto& f=*static_cast<QueueKill*>(p)->f;*bytes=a==f.actor?auto_extent:0;return *bytes!=0;};
        nw::ServerEntityEventServiceBindings delivery{};delivery.context=&qk;delivery.entities.entries=entries;delivery.entities.count=2;delivery.native_input=&input;delivery.native_search=&search;delivery.live_now=&co.now;
        delivery.resolve=[](void* p,unsigned token,void** row,unsigned* bytes) noexcept ->int{auto& q=*static_cast<QueueKill*>(p)->q;return q.queue.resolve(q.queue.context,token,row,bytes);};
        delivery.lock=[](void* p,bool enter) noexcept ->int{auto& q=*static_cast<QueueKill*>(p)->q;assert(q.locked!=enter);q.locked=enter;return 1;};
        delivery.accept=[](void* p,void* a,unsigned token,void*,void*,const void*,unsigned) noexcept ->int{auto& f=*static_cast<QueueKill*>(p);assert(a==f.f->actor&&token==406&&f.q->locked);++f.foreign;return 1;};
        delivery.release=[](void* p,unsigned token,void* row) noexcept ->int{auto& q=*static_cast<QueueKill*>(p)->q;assert(q.locked&&token&&token<=q.alloc&&row==q.rows[token-1]);++q.released;return 1;};
        std::memset(co.root,0,sizeof(co.root));std::memset(co.rows,0,sizeof(co.rows));co.alloc=0;co.released=0;co.now=220.f;
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,406,0x3000,0x3000,nonvoid,sizeof(nonvoid),0.f,50,&co.queue,&cancel_qr));
        assert(queue_api->enqueue_direct(co.root,sizeof(co.root),99,co.now,0x4001,405,0x3000,0x3000,nonvoid,sizeof(nonvoid),0.f,51,&co.queue,&cancel_qr));
        assert(service_api->service(co.root,sizeof(co.root),99,co.now,false,&budget,&delivery,&es)&&es.events==2&&es.native_inputs==1&&es.foreign_inputs==1&&!es.rejected_inputs&&qk.foreign==1&&co.released==2);
        assert(f.order=="NCR"&&actor[0x200]&&word(actor,0x20c)==0xffffffffu&&!word(co.root,0x30));
    }
