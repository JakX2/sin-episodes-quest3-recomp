// Original-free Microsoft x86 synthetic guard tests for Phase520 server
// resource coupling. The VFileSystem012 read ABI with fake handles and
// original-free 93/71 source scripts is tested separately by
// test_client_original_surfaceprops_engine_fs_bridge.cpp.
#include "server_original_surfaceprops_readonly.h"
#include <cstdio>
namespace s=sin_independent::server_original_surfaceprops;
namespace p=sin_independent::server_original_physics_factory;
int main(){
    static_assert(sizeof(void*)==4,"original Win32 target");
    p::Result absent{};
    const auto no_factory=s::ReadOnlyAfterFactory(absent,nullptr);
    if(no_factory.outcome!=s::Outcome::FactoryNotVerified ||
       no_factory.attempted!=0||no_factory.read!=0||no_factory.verified())return 1;
    p::Result success{};
    success.outcome=p::Outcome::AllThreePresent;
    success.attempted=3;success.found=3;success.original_query_gate=true;
    if(!p::Consistent(success) || !success.successful())return 2;
    const auto missing_fs=s::ReadOnlyAfterFactory(success,nullptr);
    if(missing_fs.outcome!=s::Outcome::InvalidFileSystem ||
       missing_fs.attempted!=0||missing_fs.read!=0||missing_fs.verified())return 3;
    success.found=2;success.original_query_gate=false;
    const auto invalid=s::ReadOnlyAfterFactory(success,nullptr);
    if(invalid.outcome!=s::Outcome::FactoryNotVerified ||
       invalid.verified())return 4;
    if(std::string(s::Name(s::Outcome::TwoSourcesVerified))!=
       "original-server-surfaceprops-two-sources-verified")return 5;
    std::puts("PASS server original 2006 surfaceprops: cannot parse before 3/3, null FS refused, read-only bridge isolated");
    return 0;
}
