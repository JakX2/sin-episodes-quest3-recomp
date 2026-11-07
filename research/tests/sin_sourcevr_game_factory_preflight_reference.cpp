#include "sin_compat/sourcevr_game_factory_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <string_view>
using namespace sin_compat::sourcevr_game_factory;

int main() {
    constexpr auto native = sourcevr_055_native_game_modules;
    static_assert(native.size() == 2);
    static_assert(!safe_to_bind(native));
    assert(native[0].original_required == "ServerGameDLL003");
    assert(native[0].native_registered == "ServerGameDLL010");
    assert(native[1].original_required == "VClient011");
    assert(native[1].native_registered == "VClient017");
    for (const auto& evidence : native) {
        assert(evidence.registration_proven);
        assert(!evidence.method_abi_proven);
        assert(gate_factory(evidence) == FactoryGate::VersionMismatch);
    }
    // Changing only a factory name does not justify calling any native
    // methods; same names with unproven ABI must also remain gated.
    constexpr InterfaceEvidence names_only{
        "ServerGameDLL010", "ServerGameDLL010", true, false
    };
    assert(gate_factory(names_only) == FactoryGate::MethodABIUnverified);
    constexpr InterfaceEvidence unobserved{
        "ServerGameDLL003", "", false, false
    };
    assert(gate_factory(unobserved) == FactoryGate::MissingRegistration);
    constexpr InterfaceEvidence wrong_alias{
        "ServerGameDLL003", "ServerGameDLL010", true, true
    };
    assert(gate_factory(wrong_alias) == FactoryGate::VersionMismatch);
}
