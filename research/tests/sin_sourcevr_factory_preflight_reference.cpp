#include "sin_compat/sourcevr_factory_preflight.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
using namespace sin_compat::phase96;
int main() {
    static_assert(sourcevr_055_matrix.size()==3);
    static_assert(!safe_to_bind(sourcevr_055_matrix));
    assert(gate_factory(sourcevr_055_matrix[0])==FactoryGate::VersionMismatch);
    assert(gate_factory(sourcevr_055_matrix[1])==FactoryGate::MethodABIUnverified);
    assert(gate_factory(sourcevr_055_matrix[2])==FactoryGate::MethodABIUnverified);
    assert(gate_name(FactoryGate::VersionMismatch)=="factory_version_mismatch");
    constexpr InterfaceEvidence unknown{"VClient011","",false,false};
    assert(gate_factory(unknown)==FactoryGate::MissingRegistration);
    constexpr InterfaceEvidence renamed{"VSoundEmitter001","VSoundEmitter002",true,true};
    assert(gate_factory(renamed)==FactoryGate::VersionMismatch);
    constexpr InterfaceEvidence approved{"VGUI_System010","VGUI_System010",true,true};
    assert(gate_factory(approved)==FactoryGate::VerifiedCompatible);
    constexpr std::array<InterfaceEvidence,1> approved_group{{approved}};
    static_assert(safe_to_bind(approved_group));
}
