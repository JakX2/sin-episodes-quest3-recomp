#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_prediction_snapshot_dispatch.hpp"
#include "sin_compat/original_client_prediction_savedata.hpp"
namespace snap=sin_compat::original_client_prediction_snapshot_dispatch;
namespace savedata=sin_compat::original_client_prediction_savedata;

struct FakeSerializerHost {
    std::uint32_t* process_count{};
    std::map<std::uint32_t,snap::RecordView32> records{};
    std::vector<std::string> events{};
    std::vector<std::uint32_t> visited{};
    std::vector<std::uint32_t> depths{};
    bool mutate_next_after_first{};
    bool force_extra_counter_during_prepare{};
    int pre_calls=0,prepare_calls=0,write_calls=0;
    std::uint32_t arg_one{},arg_two{},desc_token{};
    snap::RecordView32 load_record(std::uint32_t token) {
        const auto it=records.find(token);
        if(it==records.end())throw std::out_of_range("fake record token missing");
        return it->second;
    }
    void preprocess_24197f50(std::uint32_t token) {
        assert(process_count && *process_count!=0xffffffffu);
        assert(records.count(token)==1u);
        assert(records.at(token).byte_10==0u);
        events.emplace_back("pre");
        ++pre_calls;
    }
    void prepare_24199100(snap::ArgumentBlock32& state,
                          std::uint32_t first,std::uint32_t second,
                          std::uint32_t descriptor) {
        assert(process_count);
        events.emplace_back("prepare");
        ++prepare_calls;arg_one=first;arg_two=second;desc_token=descriptor;
        assert(state.get_word(snap::kResultWordOffset)==0u);
        if(force_extra_counter_during_prepare)*process_count+=7u;
        state.put_word(snap::kResultWordOffset,0x6500u);
    }
    void write_record_241996f0(snap::ArgumentBlock32& state,
                                std::uint32_t iteration,
                                std::uint32_t token,
                                std::uint32_t first,
                                std::uint32_t second) {
        assert(process_count && iteration==*process_count);
        const auto node=records.at(token);
        assert(first==node.word_00 && second==node.word_04);
        events.emplace_back("write");
        visited.push_back(token);
        depths.push_back(iteration);
        ++write_calls;
        state.put_word(snap::kResultWordOffset,
                       state.get_word(snap::kResultWordOffset)+first+second);
        if(mutate_next_after_first && token==0x100u)
            records[token].next_token_0c=0x300u;
    }
    // Phase210's host callback routes the selected slot through the two
    // authentic Phase211 wrappers. This is an ORIGINAL-FREE integration,
    // not a reproduction of the unknown engine serializer internals.
    std::uint32_t save_data_opaque(std::uint32_t selected,
                                   const std::string& label,
                                   std::int32_t index,
                                   std::uint32_t flag) {
        assert(index>=0 && flag==0u);
        assert(label==savedata::format_savedata_label("InitPredictable",index));
        snap::ArgumentBlock32 block{};
        const std::array<std::uint32_t,10> args{
            flag,selected,1u,0xabcd0000u,0u,0u,0u,1u,0u,0u};
        snap::construct_argument_block(block,args);
        assert(block.get_word(4u)==selected);
        return snap::dispatch_original_snapshot(block,*this,*process_count,
                                                0x4444u,0x7777u,0x100u);
    }
};

static void test_original_argument_builder_every_written_byte() {
    static_assert(sizeof(snap::ArgumentBlock32)==52u);
    snap::ArgumentBlock32 block{};
    block.bytes.fill(0xa5u);
    const auto inputs=std::array<std::uint32_t,10>{
        0x11223344u,0x55667788u,0x100u,0x99aabbccu,0x101u,
        0xdeadbe1fu,0x133u,0x144u,0x155u,0x66778899u
    };
    auto& alias=snap::construct_argument_block(block,inputs);
    assert(&alias==&block);
    assert(block.get_word(0x00u)==inputs[0]);
    assert(block.get_word(0x04u)==inputs[1]);
    assert(block.get_word(0x08u)==inputs[3]);
    assert(block.get_word(0x0cu)==0u);  // LOW BYTE of 0x100 is zero
    assert(block.get_word(0x10u)==1u);  // LOW BYTE of 0x101 is one
    assert(block.bytes[0x14u]==0x1fu);
    assert(block.bytes[0x15u]==0x33u);
    assert(block.bytes[0x16u]==0x55u); // original ninth stack argument
    assert(block.get_word(0x18u)==0u);
    assert(block.get_word(0x1cu)==0u);
    assert(block.get_word(0x20u)==0u);
    assert(block.bytes[0x24u]==0u && block.bytes[0x25u]==0u);
    assert(block.get_word(0x28u)==0u);
    assert(block.bytes[0x2cu]==0x44u); // original eighth stack argument
    assert(block.get_word(0x30u)==inputs[9]);
    for(const auto unchanged:{0x17u,0x26u,0x27u,0x2du,0x2eu,0x2fu})
        assert(block.bytes[unchanged]==0xa5u);
    bool threw=false;
    try {(void)block.get_word(50u);}
    catch(const std::out_of_range&){threw=true;}
    assert(threw);
}

static FakeSerializerHost simple_host(std::uint32_t* counter) {
    FakeSerializerHost host;
    host.process_count=counter;
    host.records[0x100u]={5u,7u,0x200u,0u};
    host.records[0x200u]={11u,13u,0u,1u};
    host.records[0x300u]={17u,19u,0u,1u};
    return host;
}
static void test_original_dispatch_order_and_counter() {
    std::uint32_t counter=40u;
    auto host=simple_host(&counter);
    snap::ArgumentBlock32 block{};
    const auto ret=snap::dispatch_original_snapshot(
        block,host,counter,0x1111u,0x2222u,0x100u);
    assert(counter==41u);
    assert(host.pre_calls==1 && host.prepare_calls==1 && host.write_calls==2);
    assert((host.events==std::vector<std::string>{"pre","prepare","write","write"}));
    assert((host.visited==std::vector<std::uint32_t>{0x100u,0x200u}));
    assert((host.depths==std::vector<std::uint32_t>{41u,41u}));
    assert(host.arg_one==0x1111u && host.arg_two==0x2222u);
    assert(host.desc_token==0x100u);
    assert(ret==0x6500u+5u+7u+11u+13u);
    assert(ret==block.get_word(0x28u));
}
static void test_native_counter_read_after_prepare_and_record_mutation() {
    std::uint32_t counter=2u;
    auto host=simple_host(&counter);
    host.records[0x100u].byte_10=1u;
    host.mutate_next_after_first=true;
    host.force_extra_counter_during_prepare=true;
    snap::ArgumentBlock32 block{};
    const auto result=snap::dispatch_original_snapshot(
        block,host,counter,1u,2u,0x100u);
    assert(counter==10u); // 2 + 1 native ADD + 7 mock nested activity
    assert(host.pre_calls==0);
    assert((host.visited==std::vector<std::uint32_t>{0x100u,0x300u}));
    assert((host.depths==std::vector<std::uint32_t>{10u,10u}));
    assert(result==0x6500u+5u+7u+17u+19u);
}
static void test_wrapping_and_host_only_loop_guard() {
    std::uint32_t counter=std::numeric_limits<std::uint32_t>::max();
    auto host=simple_host(&counter);
    host.records[0x100u].byte_10=1u;
    snap::ArgumentBlock32 block{};
    (void)snap::dispatch_original_snapshot(block,host,counter,1u,2u,0x100u);
    assert(counter==0u && host.depths.front()==0u);

    std::uint32_t counter2=0u;
    auto looping=simple_host(&counter2);
    looping.records[0x200u].next_token_0c=0x100u;
    bool threw=false;
    snap::ArgumentBlock32 second_block{};
    try {(void)snap::dispatch_original_snapshot(second_block,looping,counter2,
                                                1u,2u,0x100u);}
    catch(const std::runtime_error&){threw=true;}
    assert(threw && counter2==1u && looping.write_calls==2);

    threw=false;
    try {snap::walk_original_records(block,looping,0u,0u);}
    catch(const std::invalid_argument&){threw=true;}
    assert(threw);
}
static void test_real_save_data_reference_chaining() {
    std::uint32_t counter=0u;
    auto host=simple_host(&counter);
    savedata::PredictableSlice32 source{};
    source.savedata.sentinel_454=0xff00u;
    for(std::uint32_t i=0u;i<90u;++i)
        source.savedata.pointers_2ec_to_450[i]=0x10000u+i;
    auto result=savedata::save_data_slot(source,host,
                                        "InitPredictable",0,0u);
    assert(result==0x6500u+5u+7u+11u+13u);
    assert(counter==1u);
    assert(host.prepare_calls==1 && host.write_calls==2);
    result=savedata::save_data_slot(source,host,
                                   "InitPredictable",89,0u);
    assert(result==0x6500u+5u+7u+11u+13u);
    assert(counter==2u && host.write_calls==4);
}
int main() {
    static_assert(snap::kOriginalArgPackConstructorVA==0x241978a0u);
    static_assert(snap::kOriginalDispatcherVA==0x24199ba0u);
    static_assert(snap::kOriginalRecordWalkVA==0x24199b50u);
    static_assert(snap::kExternalRecordWriterVA==0x241996f0u);
    static_assert(snap::kProcessGlobalCounterVA==0x242fdb50u);
    test_original_argument_builder_every_written_byte();
    test_original_dispatch_order_and_counter();
    test_native_counter_read_after_prepare_and_record_mutation();
    test_wrapping_and_host_only_loop_guard();
    test_real_save_data_reference_chaining();
}
