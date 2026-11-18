// Synthetic engine command-line provider for the server DLL's actual
// GetTickInterval callback test. This is NOT Valve's vstdlib.dll and must
// never be staged into the owner's game copy or included in its artifact.
#include <cstring>
namespace {
int g_present=0, g_rate=0;
unsigned g_checks=0,g_values=0,g_invalid=0,g_provider=0;
class Fixture final {
public:
    virtual void __thiscall Slot0() noexcept { ++g_invalid; }
    virtual void __thiscall Slot1() noexcept { ++g_invalid; }
    virtual void __thiscall Slot2() noexcept { ++g_invalid; }
    virtual const char* __thiscall CheckParm(const char* name,const char** output) const noexcept {
        ++g_checks;
        if (!name || std::strcmp(name,"-tickrate") || output) ++g_invalid;
        return g_present ? "-tickrate" : nullptr;
    }
    virtual void __thiscall Slot4() noexcept { ++g_invalid; }
    virtual void __thiscall Slot5() noexcept { ++g_invalid; }
    virtual void __thiscall Slot6() noexcept { ++g_invalid; }
    virtual int __thiscall ParmValue(const char* name,int fallback) const noexcept {
        ++g_values;
        if (!name || std::strcmp(name,"-tickrate") || fallback!=0 || !g_present)
            ++g_invalid;
        return g_rate;
    }
};
Fixture object;
}
extern "C" __declspec(dllexport) void* __cdecl CommandLine() noexcept {
    ++g_provider;
    return &object;
}
extern "C" __declspec(dllexport) void __cdecl SetTickFixture(int present,int rate) noexcept {
    g_present=present;g_rate=rate;
    g_checks=g_values=g_invalid=g_provider=0;
}
extern "C" __declspec(dllexport) bool __cdecl VerifyTickFixture() noexcept {
    return !g_invalid && g_checks==1 &&
        g_values==(g_present?1u:0u) && g_provider==(g_present?2u:1u);
}
