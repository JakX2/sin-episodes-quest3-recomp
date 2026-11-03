#include "sin_compat/original_init_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <array>
#include <string>
#include <string_view>
#include <vector>
using namespace sin_compat::init;

int main(){
    static_assert(client_core_requests.size()==21 && server_core_requests.size()==16);
    assert(client_core_requests[0].name=="VEngineClient012");
    assert(server_core_requests[0].name=="VEngineServer021");
    assert(client_core_requests[16].name=="VFileSystem012");
    assert(client_core_requests[16].source==FactoryArgument::First);
    assert(server_core_requests[10].name=="VFileSystem012");
    assert(server_core_requests[10].source==FactoryArgument::Third);

    static int memory[64]{};
    std::vector<std::string> called;
    std::array<void*,21> client{};
    auto first=[&](std::string_view name)->void* {
        called.emplace_back(name);return &memory[called.size()];
    };
    auto forbidden=[](std::string_view)->void* { assert(false);return nullptr; };
    const auto client_result=resolve_required_core(client_core_requests,first,forbidden,client);
    assert(client_result.resolved && client_result.first_failed_index==21);
    assert(called.size()==21);
    for(std::size_t i=0;i<21;++i) {
        assert(called[i]==client_core_requests[i].name);
        assert(client[i]!=nullptr);
    }
    assert(may_continue_after_emitter_gate(client_result,true));
    assert(!may_continue_after_emitter_gate(client_result,false));

    std::array<void*,16> server{};
    called.clear();std::size_t third_calls=0;
    auto server_first=[&](std::string_view name)->void* {
        called.emplace_back(name);return &memory[called.size()];
    };
    auto server_third=[&](std::string_view name)->void* {
        ++third_calls;called.emplace_back("third:");
        called.back()+=name;return &memory[50];
    };
    const auto server_result=resolve_required_core(server_core_requests,server_first,server_third,server);
    assert(server_result.resolved && server_result.first_failed_index==16);
    assert(third_calls==1 && called.size()==16);
    assert(called[10]=="third:VFileSystem012" && server[10]==&memory[50]);

    // Original pre-error pointer writes are not rolled back.
    for(std::size_t missing : std::array<std::size_t,4>{0,7,10,15}) {
        server.fill(nullptr);called.clear();third_calls=0;
        std::size_t invoked=0;
        auto a=[&](std::string_view name)->void* {
            called.emplace_back(name);
            return invoked++==missing?nullptr:&memory[4];
        };
        auto b=[&](std::string_view name)->void* {
            ++third_calls;called.emplace_back(name);
            return invoked++==missing?nullptr:&memory[5];
        };
        const auto result=resolve_required_core(server_core_requests,a,b,server);
        assert(!result.resolved && result.first_failed_index==missing);
        assert(called.size()==missing+1);
        for(std::size_t i=0;i<missing;++i)assert(server[i]!=nullptr);
        assert(server[missing]==nullptr);
        for(std::size_t i=missing+1;i<16;++i)assert(server[i]==nullptr);
        assert(!may_continue_after_emitter_gate(result,true));
        assert(third_calls==(missing>=10?1u:0u));
    }

    std::array<void*,21> incompatible{};
    auto only_modern=[](std::string_view name)->void* {
        return name=="VClient017"?static_cast<void*>(&memory[0]):nullptr;
    };
    const auto old_client=resolve_required_core(client_core_requests,only_modern,forbidden,incompatible);
    assert(!old_client.resolved && old_client.first_failed_index==0);
}
