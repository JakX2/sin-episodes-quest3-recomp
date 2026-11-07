#include "sin_compat/original_dllinit_factory_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string_view>
#include <vector>

using namespace sin_compat::original_dllinit;

int main() {
    std::vector<std::string_view> engine_names;
    int file_calls=0, material_calls=0, surface_calls=0;
    const auto server=check_original_server_interfaces(
        [&](auto n){engine_names.push_back(n);return true;},
        [&](auto n){assert(n=="VFileSystem012");++file_calls;return true;});
    assert(server.present && server.checked==16);
    assert(file_calls==1 && engine_names.size()==15);
    assert(engine_names[0]=="VEngineServer021");
    assert(engine_names[13]=="ServerUploadGameStats001");

    engine_names.clear();file_calls=0;
    const auto server_blocked=check_original_server_interfaces(
        [&](auto n){engine_names.push_back(n);return n!="VSoundEmitter001";},
        [&](auto){++file_calls;return true;});
    assert(!server_blocked.present && server_blocked.checked==14);
    assert(server_blocked.missing_name=="VSoundEmitter001");
    assert(file_calls==1 && engine_names.size()==13);
    assert(server_blocked.missing_provider==FactoryProvider::Engine);

    engine_names.clear();
    const auto client=check_original_client_interfaces(
        [&](auto n){engine_names.push_back(n);return true;},
        [&](auto n){assert(n=="MaterialSystemHardwareConfig012");++material_calls;return true;},
        [&](auto n){assert(n=="MatSystemSurface003");++surface_calls;return true;});
    assert(client.present && client.checked==23);
    assert(engine_names.size()==21 && material_calls==1 && surface_calls==1);

    engine_names.clear();material_calls=surface_calls=0;
    const auto client_blocked=check_original_client_interfaces(
        [&](auto n){engine_names.push_back(n);return n!="VSoundEmitter001";},
        [&](auto){++material_calls;return true;},
        [&](auto){++surface_calls;return true;});
    assert(!client_blocked.present && client_blocked.checked==21);
    assert(client_blocked.missing_name=="VSoundEmitter001");
    assert(material_calls==0 && surface_calls==0);

    const auto derived_blocked=check_original_client_interfaces(
        [](auto){return true;},[](auto){return false;},[](auto){return true;});
    assert(!derived_blocked.present && derived_blocked.checked==22);
    assert(derived_blocked.missing_provider==FactoryProvider::MaterialSystemMethod);

    const auto surface_blocked=check_original_client_interfaces(
        [](auto){return true;},[](auto){return true;},[](auto){return false;});
    assert(!surface_blocked.present && surface_blocked.checked==23);
    assert(surface_blocked.missing_provider==FactoryProvider::SurfaceMethod);
    assert(optional_probes[0].name=="VDebugOverlay003");
    assert(optional_probes[1].name=="VMaterialSystemStub001");

    // Callbacks that only satisfy the newer 002 version must NOT silently
    // satisfy the original 001 interface. The native 002 method ABI is
    // unproven against the original; no guessed alias is permitted.
    const auto wrong_version=check_original_server_interfaces(
        [](auto n){return n!="VSoundEmitter001";},
        [](auto){return true;});
    assert(!wrong_version.present &&
           wrong_version.missing_name=="VSoundEmitter001");
}
