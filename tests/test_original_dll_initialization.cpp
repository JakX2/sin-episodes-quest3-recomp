#include "sin_compat/original_dll_initialization.hpp"
// Win32 MSVC CI builds Release with NDEBUG; these assertions are the test,
// so ensure they remain active rather than silently passing an empty run.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
using namespace sin_compat::original_dll_initialization;

struct MockHost {
    struct Query { Supplier supplier; std::string name; };
    std::vector<Query> queried;
    std::vector<std::pair<std::uint32_t,std::uintptr_t>> published;
    std::size_t fail_at=static_cast<std::size_t>(-1);
    bool check_store_order=true;
    std::uintptr_t query(Supplier origin, const char* name) {
        if (check_store_order) assert(queried.size()==published.size());
        queried.push_back({origin,name});
        return queried.size()-1==fail_at?0u:0x90000000u+queried.size();
    }
    void publish(std::uint32_t va, std::uintptr_t value) {
        assert(queried.size()==published.size()+1);
        published.push_back({va,value});
    }
};

template<std::size_t N>
void verify_prefix(const std::array<Request,N>& rows) {
    for(const auto& row : rows) {
        assert(row.version[0]);
        assert(row.required);
        assert(row.native_request_va);
        assert(row.native_string_va);
        assert(row.native_storage_va);
        assert(row.supplier!=Supplier::DerivedVirtual);
    }
    for(std::size_t failure=0;failure<=N;++failure) {
        MockHost host;
        host.fail_at=failure;
        const bool ok=query_mandatory_prefix(host,rows);
        assert(ok==(failure==N));
        const auto expected=(failure==N?N:failure+1);
        assert(host.queried.size()==expected);
        assert(host.published.size()==expected);
        for(std::size_t i=0;i<expected;++i) {
            assert(host.queried[i].supplier==rows[i].supplier);
            assert(host.queried[i].name==rows[i].version);
            assert(host.published[i].first==rows[i].native_storage_va);
            assert(host.published[i].second==(i==failure?0u:0x90000001u+i));
        }
    }
}

int main() {
    static_assert(kServerPrefixRequests.size()==16,"original server prefix count");
    static_assert(kClientPrefixRequests.size()==21,"original client prefix count");
    static_assert(kServerLateRequests.size()==1,"server late optional");
    static_assert(kClientLateRequests.size()==3,"client late lookups");
    static_assert(kServerInitOriginalLength+kClientInitOriginalLength==2095,"whole body lengths");
    verify_prefix(kServerPrefixRequests);
    verify_prefix(kClientPrefixRequests);
    assert(std::strcmp(kServerPrefixRequests[0].version,"VEngineServer021")==0);
    assert(std::strcmp(kClientPrefixRequests[0].version,"VEngineClient012")==0);
    assert(std::strcmp(kServerPrefixRequests[10].version,"VFileSystem012")==0);
    assert(kServerPrefixRequests[10].supplier==Supplier::FilesystemFactory);
    assert(kServerPrefixRequests[9].supplier==Supplier::EngineFactory);
    assert(kServerPrefixRequests[11].supplier==Supplier::EngineFactory);
    assert(std::strcmp(kClientPrefixRequests[16].version,"VFileSystem012")==0);
    assert(kClientPrefixRequests[16].supplier==Supplier::EngineFactory);
    assert(kServerLateRequests[0].required==false);
    assert(kClientLateRequests[0].required==false);
    assert(kClientLateRequests[1].required);
    assert(kClientLateRequests[2].required);
    assert(kClientLateRequests[1].supplier==Supplier::DerivedVirtual);
    assert(kClientLateRequests[2].supplier==Supplier::DerivedVirtual);
    for(const auto& row:kServerLateRequests) {
        MockHost host; host.fail_at=0;
        assert(query_late_request(host,row));
        assert(host.published.size()==1 && host.published[0].second==0);
    }
    for(const auto& row:kClientLateRequests) {
        MockHost host; host.fail_at=0;
        assert(query_late_request(host,row)==!row.required);
        assert(host.published.size()==1 && host.published[0].first==row.native_storage_va);
    }
    MockHost host;
    assert(query_late_request(host,kClientLateRequests[2]));
    assert(host.published[0].second!=0);
    assert(kServerFailureReturnVA==0x221e79a1u);
    assert(kClientFailureReturnVA==0x240fe59eu);
}