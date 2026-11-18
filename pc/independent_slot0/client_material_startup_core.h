#pragma once
// Independently authored first post-Connect client acquisition boundary.
// Observed original VClient011 Init RVAs 0xFE32A..0xFE371.
// Engine objects are opaque; only the observed virtual slots are called.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original Win32 x86 ABI only
#endif
namespace sin_independent {
using ClientFactory=void* (__cdecl *)(const char*,int*);
using ClientCommandLine=void* (__cdecl *)();
using FindParmSlot=int (__thiscall *)(const void*,const char*);
using HardwareConfigSlot=void* (__thiscall *)(void*,const char*,int*);
using MaterialTrace=void (*)(const char*,const char*) noexcept;
struct ClientMaterialState {
    bool textMode=false;
    void* stub=nullptr;
    void* hardwareConfig=nullptr;
};
enum class ClientMaterialResult { Acquired, InvalidCommandLine, InvalidMaterial, MissingHardware };
inline const char* ClientMaterialResultName(ClientMaterialResult r) noexcept {
    switch(r) {
    case ClientMaterialResult::Acquired:return "acquired";
    case ClientMaterialResult::InvalidCommandLine:return "invalid-commandline";
    case ClientMaterialResult::InvalidMaterial:return "invalid-material";
    case ClientMaterialResult::MissingHardware:return "missing-hardware";
    }
    return "unknown";
}
inline ClientMaterialResult AcquireClientMaterialState(ClientCommandLine provider,
    ClientFactory app,void* materials,ClientMaterialState& out,MaterialTrace trace) noexcept {
    if(!provider || !app || !trace)return ClientMaterialResult::InvalidCommandLine;
    const void* commandLine=provider();
    if(!commandLine)return ClientMaterialResult::InvalidCommandLine;
    const auto* table=*reinterpret_cast<void* const* const*>(commandLine);
    if(!table || !table[10])return ClientMaterialResult::InvalidCommandLine;
    auto find=reinterpret_cast<FindParmSlot>(table[10]);
    out.textMode=find(commandLine,"-textmode")!=0;
    trace("textmode-checked",out.textMode?"present":"absent");
    trace("optional-request","VMaterialSystemStub001");
    out.stub=app("VMaterialSystemStub001",nullptr);
    trace(out.stub?"optional-found":"optional-missing","VMaterialSystemStub001");
    if(!materials)return ClientMaterialResult::InvalidMaterial;
    auto** mt=*reinterpret_cast<void***>(materials);
    if(!mt || !mt[8])return ClientMaterialResult::InvalidMaterial;
    trace("hardware-config-entered","MaterialSystemHardwareConfig012");
    auto get=reinterpret_cast<HardwareConfigSlot>(mt[8]);
    out.hardwareConfig=get(materials,"MaterialSystemHardwareConfig012",nullptr);
    trace(out.hardwareConfig?"hardware-config-found":"hardware-config-missing",
          "MaterialSystemHardwareConfig012");
    return out.hardwareConfig?ClientMaterialResult::Acquired:ClientMaterialResult::MissingHardware;
}
} // namespace sin_independent
