#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_prediction_savedata.hpp"
namespace predict=sin_compat::original_client_prediction_savedata;
namespace game=sin_compat::original_allocator_game_callers;
namespace cross=sin_compat::original_cross_dll_tier0_allocator;

struct Host {
    predict::PredictableSlice32* owner{};
    cross::ModuleState* allocator{};
    std::uint32_t next=0x10000u;
    std::uint32_t debug_owner=0xa500u;
    bool tagged{};
    int metadata_checks=0,metadata_creations=0,descriptor_queries=0;
    int alloc_calls=0,zero_calls=0,optional_preinit=0;
    int init44=0,init8f00=0,global_init=0,local_init=0,save_calls=0;
    std::vector<std::string> events{};
    std::vector<std::int32_t> slot_ids{};
    std::vector<std::uint32_t> used_tokens{};
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks{};
    std::uint32_t member_seen{};
    bool metadata_enabled() {
        assert(allocator && (allocator->original.alloc_flags_04&1u)!=0u);
        ++metadata_checks;return tagged;
    }
    std::uint32_t make_metadata() {
        ++metadata_creations;return debug_owner;
    }
    std::uint32_t allocate_basic(std::uint32_t size) {
        ++alloc_calls;events.emplace_back("alloc");return reserve(size);
    }
    std::uint32_t allocate_tagged(std::uint32_t size,std::uint32_t context,
                                  std::uint32_t unused) {
        assert(context==debug_owner && unused==0u);
        ++alloc_calls;events.emplace_back("alloc");return reserve(size);
    }
    std::uint32_t reserve(std::uint32_t size) {
        next+=0x1000u;
        blocks.emplace(next,std::vector<std::uint8_t>(size,0xedu));
        return next;
    }
    game::ClientDescriptor query_virtual_40() {
        ++descriptor_queries;events.emplace_back("descriptor40");
        return {true,static_cast<std::uint8_t>(descriptor_queries==1 ? 0:1),7};
    }
    void invoke_helper_240c57b0(const game::ClientDescriptor& desc) {
        assert(desc.present && desc.flag_at_11==0u);
        ++optional_preinit;events.emplace_back("preinit");
    }
    void zero_bytes(std::uint32_t token,std::uint32_t size) {
        assert(owner!=nullptr);
        auto it=blocks.find(token);
        assert(it!=blocks.end() && it->second.size()==size);
        // Native pointer MOV occurs before REP STOSD, including sentinel.
        const bool published=owner->savedata.sentinel_454==token ||
          std::find(owner->savedata.pointers_2ec_to_450.begin(),
                    owner->savedata.pointers_2ec_to_450.end(),token)!=
          owner->savedata.pointers_2ec_to_450.end();
        assert(published);
        ++zero_calls;events.emplace_back("zero");
        std::fill(it->second.begin(),it->second.end(),std::uint8_t{0});
    }
    std::uint32_t query_virtual_44() {
        assert(owner && owner->predictable_flag_2e8==1u);
        ++init44;events.emplace_back("virtual44");
        return 0xb441u;
    }
    void initialize_240c8f00(std::uint32_t returned) {
        assert(returned==0xb441u);
        ++init8f00;events.emplace_back("helper8f00");
    }
    void global_240cac00(std::uint32_t member) {
        assert(owner!=nullptr && member==owner->member_b4);
        member_seen=member;++global_init;events.emplace_back("global-init");
    }
    void local_240c6900(std::uint32_t zero) {
        assert(zero==0u);
        ++local_init;events.emplace_back("local-init");
    }
    std::uint32_t save_data_opaque(std::uint32_t token,
                                    const std::string& label,
                                    std::int32_t index,std::uint32_t flag) {
        assert(owner && index>=0 && index<90 && flag==0u);
        assert(label==predict::format_savedata_label("InitPredictable",index));
        assert(owner->savedata.pointers_2ec_to_450[
            static_cast<std::size_t>(index)]==token);
        assert(blocks.count(token)==1u);
        assert(std::all_of(blocks[token].begin(),blocks[token].end(),
                [](std::uint8_t byte){return byte==0u;}));
        ++save_calls;events.emplace_back("save-slot");
        slot_ids.push_back(index);used_tokens.push_back(token);
        return static_cast<std::uint32_t>(index);
    }
};

static void test_savedata_slots_and_formats() {
    predict::PredictableSlice32 object{};
    assert(predict::select_savedata_buffer(object.savedata,5)==0u);
    object.savedata.sentinel_454=0xa001u;
    for(std::uint32_t i=0;i<90;++i)
        object.savedata.pointers_2ec_to_450[i]=0xb000u+i;
    assert(predict::select_savedata_buffer(object.savedata,-1)==0xa001u);
    assert(predict::select_savedata_buffer(object.savedata,0)==0xb000u);
    assert(predict::select_savedata_buffer(object.savedata,89)==0xb059u);
    assert(predict::select_savedata_buffer(object.savedata,90)==0xb000u);
    assert(predict::select_savedata_buffer(object.savedata,181)==0xb001u);
    bool rejected=false;
    try {(void)predict::select_savedata_buffer(object.savedata,-2);}
    catch(const std::out_of_range&){rejected=true;}
    assert(rejected);
    assert(predict::format_savedata_label("InitPredictable",0)=="InitPredictable SaveData(slot 00)");
    assert(predict::format_savedata_label("InitPredictable",89)=="InitPredictable SaveData(slot 89)");
    assert(predict::format_savedata_label("InitPredictable",-1)=="InitPredictable SaveData(original)");
    std::string long_prefix(120,'X');
    assert(predict::format_savedata_label(long_prefix.c_str(),3).size()==63u);
}

static void test_original_init_predictable_chain() {
    predict::PredictableSlice32 object{};
    object.member_b4=0x12345678u;
    cross::ModuleState alloc{};
    Host host;
    host.owner=&object;host.allocator=&alloc;host.tagged=true;
    predict::init_predictable(object,alloc,host);
    assert(object.predictable_flag_2e8==1u);
    assert(host.init44==1 && host.init8f00==1);
    assert(host.optional_preinit==1 && host.descriptor_queries==2);
    assert(host.global_init==1 && host.local_init==1);
    assert(host.member_seen==0x12345678u);
    assert(host.zero_calls==91 && host.alloc_calls==91);
    assert(host.metadata_checks==1 && host.metadata_creations==1);
    assert(alloc.original.alloc_flags_04==1u);
    assert(alloc.original.alloc_context_00==host.debug_owner);
    assert(host.blocks.size()==91u && host.save_calls==90);
    for(const auto& pair:host.blocks)assert(pair.second.size()==7u);
    assert(host.slot_ids.front()==0 && host.slot_ids.back()==89);
    assert(host.used_tokens[0]==object.savedata.pointers_2ec_to_450[0]);
    assert(host.used_tokens[89]==object.savedata.pointers_2ec_to_450[89]);
    assert(host.events[0]=="virtual44");
    assert(host.events[1]=="helper8f00");
    auto global=std::find(host.events.begin(),host.events.end(),"global-init");
    auto local=std::find(host.events.begin(),host.events.end(),"local-init");
    auto saved=std::find(host.events.begin(),host.events.end(),"save-slot");
    assert(global!=host.events.end() && global<local && local<saved);
    // This 90B caller does NOT own the sentinel guard: the inner 156B
    // Phase209 initializer skips allocation on repeats, but all 90
    // SaveData calls and the other outer lifecycle steps repeat.
    predict::init_predictable(object,alloc,host);
    assert(host.init44==2 && host.init8f00==2);
    assert(host.alloc_calls==91 && host.zero_calls==91);
    assert(host.descriptor_queries==2 && host.optional_preinit==1);
    assert(host.global_init==2 && host.local_init==2 && host.save_calls==180);
}

int main() {
    static_assert(predict::kOriginalInitPredictableVA==0x240caf40u);
    static_assert(predict::kOriginalSaveDataVA==0x240c5960u);
    static_assert(predict::kSaveDataNamedSlots==90u);
    static_assert(predict::kOriginalVirtualInitializeOffset==0x44u);
    static_assert(predict::kOriginalDescriptorVirtualOffset==0x40u);
    static_assert(predict::kOriginalArrayVtableOffset==0x24u);
    test_savedata_slots_and_formats();
    test_original_init_predictable_chain();
}
