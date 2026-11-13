#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_allocator_game_callers.hpp"

namespace game=sin_compat::original_allocator_game_callers;
namespace cross=sin_compat::original_cross_dll_tier0_allocator;

struct SyntheticHost {
    cross::ModuleState* allocator_state{};
    game::ClientBufferSpan32* buffer_span{};
    bool track_allocations{};
    bool allocation_fails{};
    std::uint32_t next_token=0x10000u,owner_id=0xa5a5u;
    std::uint32_t fixed_random{};
    int metadata_queries=0,metadata_created=0;
    int basic_allocations=0,tagged_allocations=0;
    int zeroed=0,store_calls=0,random_calls=0,virtual_5b0_calls=0;
    int client_queries=0,client_preinits=0;
    std::uint32_t dispatched_token{};
    std::vector<std::string> events{};
    std::map<std::uint32_t,std::vector<std::uint8_t>> storage{};
    std::vector<game::ClientDescriptor> descriptor_results{};

    bool metadata_enabled() {
        assert(allocator_state && (allocator_state->original.alloc_flags_04&1u)!=0u);
        ++metadata_queries;return track_allocations;
    }
    std::uint32_t make_metadata() {
        ++metadata_created;return owner_id;
    }
    std::uint32_t allocate_basic(std::uint32_t bytes) {
        events.push_back("allocate-basic");++basic_allocations;
        return allocate(bytes);
    }
    std::uint32_t allocate_tagged(std::uint32_t bytes,
                                  std::uint32_t owner,std::uint32_t zero) {
        assert(owner==owner_id && zero==0u);
        events.push_back("allocate-tagged");++tagged_allocations;
        return allocate(bytes);
    }
    std::uint32_t allocate(std::uint32_t bytes) {
        if(allocation_fails)return 0u;
        next_token+=0x1000u;
        storage.emplace(next_token,std::vector<std::uint8_t>(bytes,0xedu));
        return next_token;
    }
    void store_byte(std::uint32_t token,std::uint32_t index,std::uint8_t b) {
        auto it=storage.find(token);
        assert(it!=storage.end() && index<it->second.size());
        it->second[index]=b;
        ++store_calls;
        events.push_back("store");
    }
    int random_int(int first,int last) {
        assert(first==0 && last==3);
        ++random_calls;events.push_back("random-int");
        return static_cast<int>(fixed_random);
    }
    std::uint32_t dispatch_virtual_5b0(std::uint32_t token) {
        ++virtual_5b0_calls;dispatched_token=token;
        events.push_back("virtual-5b0");
        assert(storage.count(token)==1u);
        return 0x5b005b00u;
    }
    game::ClientDescriptor query_virtual_40() {
        events.push_back("descriptor-query");
        const auto index=static_cast<std::size_t>(client_queries++);
        return index<descriptor_results.size() ?
            descriptor_results[index] : game::ClientDescriptor{};
    }
    void invoke_helper_240c57b0(const game::ClientDescriptor& source) {
        assert(source.present && source.flag_at_11==0u);
        ++client_preinits;events.push_back("conditional-preinit");
    }
    void zero_bytes(std::uint32_t token,std::uint32_t bytes) {
        auto it=storage.find(token);
        assert(it!=storage.end() && it->second.size()==bytes);
        assert(buffer_span!=nullptr);
        // Original MOV publishes the destination pointer BEFORE REP STOSD.
        const bool published=buffer_span->sentinel_454==token ||
            std::find(buffer_span->pointers_2ec_to_450.begin(),
                      buffer_span->pointers_2ec_to_450.end(),token)!=
            buffer_span->pointers_2ec_to_450.end();
        assert(published);
        ++zeroed;events.push_back("zero-bytes");
        std::fill(it->second.begin(),it->second.end(),static_cast<std::uint8_t>(0u));
    }
};

static void server_three_byte_random_token() {
    for(std::uint32_t random=0;random<4u;++random) {
        SyntheticHost host;
        host.fixed_random=random;
        cross::ModuleState module{};
        host.allocator_state=&module;
        const auto result=game::server_make_and_send_token(module,host);
        assert(result==0x5b005b00u);
        assert(host.virtual_5b0_calls==1 && host.random_calls==1);
        assert(host.store_calls==3 && host.storage.size()==1u);
        const auto& bytes=host.storage.at(host.dispatched_token);
        assert(bytes.size()==3u);
        assert(bytes[0]=='_' && bytes[1]==static_cast<std::uint8_t>('A'+random));
        assert(bytes[2]==0u);
        assert(host.events.size()==6u);
        assert(host.events[0]=="allocate-basic");
        assert(host.events[1]=="store");
        assert(host.events[2]=="random-int");
        assert(host.events[3]=="store" && host.events[4]=="store");
        assert(host.events[5]=="virtual-5b0");
        assert(module.original.alloc_flags_04==1u);
        // Native caller does NOT free the token; the callee's ownership
        // interpretation is unknown until original virtual +0x5B0 is traced.
        assert(host.storage.count(host.dispatched_token)==1u);
    }
    {
        SyntheticHost host;
        host.track_allocations=true;
        cross::ModuleState state{};
        host.allocator_state=&state;
        (void)game::server_make_and_send_token(state,host);
        assert(host.tagged_allocations==1 && host.basic_allocations==0);
        assert(host.metadata_created==1 && host.metadata_queries==1);
        assert(state.original.alloc_context_00==host.owner_id);
        host.allocation_fails=true;
        bool rejected=false;
        try {(void)game::server_make_and_send_token(state,host);}
        catch(const std::runtime_error&){rejected=true;}
        assert(rejected && host.random_calls==1 && host.virtual_5b0_calls==1);
    }
}

static void client_91_individual_zeroed_buffers() {
    cross::ModuleState module{};
    game::ClientBufferSpan32 span{};
    SyntheticHost host;
    host.buffer_span=&span;host.allocator_state=&module;
    host.descriptor_results={
        {true,0u,123},   // first query triggers original preinit
        {true,1u,7},     // second query determines signed byte length
    };
    game::client_init_91_buffers(span,module,host);
    assert(host.client_queries==2 && host.client_preinits==1);
    assert(host.zeroed==91 && host.basic_allocations==91);
    assert(host.tagged_allocations==0);
    assert(host.metadata_queries==1 && host.metadata_created==0);
    assert(host.storage.size()==91u);
    assert(span.sentinel_454!=0u);
    assert(host.storage.at(span.sentinel_454).size()==7u);
    std::set<std::uint32_t> tokens{span.sentinel_454};
    for(auto token:span.pointers_2ec_to_450) {
        assert(token!=0u);
        assert(host.storage.at(token).size()==7u);
        const auto& bytes=host.storage.at(token);
        assert(std::all_of(bytes.begin(),bytes.end(),
                           [](std::uint8_t b){return b==0u;}));
        tokens.insert(token);
    }
    assert(tokens.size()==91u);
    assert(host.events[0]=="descriptor-query");
    assert(host.events[1]=="conditional-preinit");
    assert(host.events[2]=="descriptor-query");
    const auto calls=host.basic_allocations;
    game::client_init_91_buffers(span,module,host); // native guard fast path
    assert(host.client_queries==2 && host.basic_allocations==calls);
}

static void client_min_size_descriptor_variants_and_guards() {
    for(const std::int32_t size:{-5,0,4}) {
        SyntheticHost host;
        cross::ModuleState state{};
        game::ClientBufferSpan32 span{};
        host.buffer_span=&span;host.allocator_state=&state;
        host.track_allocations=true;
        host.descriptor_results={{true,1u,800},{true,1u,size}};
        game::client_init_91_buffers(span,state,host);
        assert(host.client_preinits==0 && host.client_queries==2);
        assert(host.tagged_allocations==91u && host.metadata_created==1);
        for(const auto& item:host.storage) {
            assert(item.second.size()==4u);
            assert(std::all_of(item.second.begin(),item.second.end(),
                          [](std::uint8_t b){return b==0u;}));
        }
    }
    {
        SyntheticHost host;
        cross::ModuleState state{};
        game::ClientBufferSpan32 span{};
        host.buffer_span=&span;host.allocator_state=&state;
        host.descriptor_results={{false,0u,0},{true,0u,6}};
        game::client_init_91_buffers(span,state,host);
        assert(host.client_preinits==0 && host.zeroed==91);
    }
    {
        SyntheticHost host;
        cross::ModuleState state{};
        game::ClientBufferSpan32 span{};
        host.buffer_span=&span;host.allocator_state=&state;
        host.descriptor_results={{true,1u,0},{false,0u,0}};
        bool rejected=false;
        try {game::client_init_91_buffers(span,state,host);}
        catch(const std::runtime_error&){rejected=true;}
        assert(rejected && host.storage.empty() && span.sentinel_454==0u);
    }
    {
        SyntheticHost host;
        cross::ModuleState state{};
        game::ClientBufferSpan32 span{};
        host.buffer_span=&span;host.allocator_state=&state;
        host.descriptor_results={{true,1u,0},{true,0u,100000000}};
        bool rejected=false;
        try {game::client_init_91_buffers(span,state,host);}
        catch(const std::overflow_error&){rejected=true;}
        assert(rejected && host.storage.empty());
    }
}

int main() {
    static_assert(game::kServerStringCallerVA==0x22358870u);
    static_assert(game::kClientBufferInitVA==0x240c8720u);
    static_assert(game::kServerRandomIntIATVA==0x223d21acu);
    static_assert(game::kClientBufferCount==90u);
    static_assert(game::kClientTotalAllocationCount==91u);
    static_assert(game::kClientSentinelObjectByteOffset-
                  game::kClientPointerArrayObjectByteOffset==90u*4u);
    server_three_byte_random_token();
    client_91_individual_zeroed_buffers();
    client_min_size_descriptor_variants_and_guards();
}
