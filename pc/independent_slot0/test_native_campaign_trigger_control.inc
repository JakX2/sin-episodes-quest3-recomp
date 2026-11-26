// Authenticated inherited inputs traverse the derived map then CBaseTrigger.
    {
        struct Control {
            alignas(4) unsigned char actor[dynamic_trigger_extent]{};
            unsigned calls=0,refresh=0,expected=0,disabled=0;
        } f;
        TriggerControlBindings control{};control.context=&f;
        control.solid_flags=[](void* p,void* member,unsigned flags) noexcept ->int{
            auto& f=*static_cast<Control*>(p);assert(member==f.actor+0xfc&&flags==f.expected&&f.actor[0x408]==f.disabled);
            unsigned short v=static_cast<unsigned short>(flags);std::memcpy(f.actor+0x120,&v,2);++f.calls;return 1;
        };
        control.refresh=[](void* p,void* a,int arg) noexcept ->int{
            auto& f=*static_cast<Control*>(p);assert(a==f.actor&&!arg&&f.calls==f.refresh+1);
            unsigned short flags=0;std::memcpy(&flags,f.actor+0x120,2);assert(flags==f.expected);++f.refresh;return 1;
        };
        nw::ServerEntityAcceptInputBindings input{};input.storage=&tstorage;input.campaign_trigger_control=&control;
        input.string_text=[](void*,unsigned token,const char** text,unsigned* bytes) noexcept ->int{
            *text=token==0x223edd00u?"Enable":token==0x223edce8u?"Disable":token==0x223d4914u?"Toggle":token==91?"tOgGlE":nullptr;
            if(!*text)return 0;*bytes=static_cast<unsigned>(std::strlen(*text)+1);return 1;
        };
        unsigned empty[5]{0,0,0,0xffffffffu,0};nw::ServerEntityAcceptInputResult result{};
        for(unsigned vt:{dynamic_trigger_vtable,secret_trigger_vtable}){
            const auto n=removal_entity_extent(vt);auto* a=f.actor;std::memset(a,0,sizeof(f.actor));
            put(a,0,vt);put(a,0x120,0xa5a50120u);a[0x408]=1;a[0x1f0]=1;f.calls=f.refresh=0;
            f.expected=0x128;f.disabled=0;
            assert(timer_accept->run(a,n,vt,0x223edd00u,nullptr,nullptr,empty,sizeof(empty),1,&input,&result)&&result.accepted&&result.native_handler&&result.native_maps==2&&result.row==3&&result.handler==0x22060810u&&result.dirty);
            assert(word(a,0x120)==0xa5a50128u&&!a[0x408]&&a[0x200]&&f.calls==1&&f.refresh==1);
            assert(timer_accept->run(a,n,vt,0x223edd00u,nullptr,nullptr,empty,sizeof(empty),2,&input,&result)&&result.accepted&&f.calls==1);
            f.expected=0x120;f.disabled=0;
            assert(timer_accept->run(a,n,vt,91,nullptr,nullptr,empty,sizeof(empty),3,&input,&result)&&result.accepted&&result.row==5&&result.handler==0x22060830u&&f.calls==2&&!a[0x408]);
            // Disable repairs the disabled byte even when active bit is already clear.
            assert(timer_accept->run(a,n,vt,0x223edce8u,nullptr,nullptr,empty,sizeof(empty),4,&input,&result)&&result.accepted&&result.row==4&&a[0x408]&&f.calls==2);
            f.expected=0x128;f.disabled=1;
            assert(timer_accept->run(a,n,vt,0x223d4914u,nullptr,nullptr,empty,sizeof(empty),5,&input,&result)&&result.accepted&&a[0x408]&&f.calls==3);
            f.expected=0x120;f.disabled=1;
            assert(timer_accept->run(a,n,vt,0x223edce8u,nullptr,nullptr,empty,sizeof(empty),6,&input,&result)&&result.accepted&&word(a,0x120)==0xa5a50120u&&f.calls==4&&f.refresh==4);
            TriggerControlResult direct{};
            auto bad=control;bad.refresh=nullptr;
            assert(!api->trigger_control(a,n,"Enable",&bad,&direct)&&a[0x408]&&f.calls==4);
            assert(!api->trigger_control(a,n-1,"Enable",&control,&direct)&&f.calls==4);
        }
    }
