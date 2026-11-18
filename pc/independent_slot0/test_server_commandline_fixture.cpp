// Synthetic engine command-line provider for the server DLL's actual
// GetTickInterval callback test. This is NOT Valve's vstdlib.dll and must
// never be staged into the owner's game copy or included in its artifact.
#include <cstring>
namespace {
int g_present=0, g_rate=0;
unsigned g_checks=0,g_values=0,g_invalid=0,g_provider=0;
int g_textMode=0; unsigned g_find=0;
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
    virtual void __thiscall Slot8() noexcept { ++g_invalid; }
    virtual void __thiscall Slot9() noexcept { ++g_invalid; }
    virtual int __thiscall FindParm(const char* name) const noexcept {
        ++g_find;
        if(!name || std::strcmp(name,"-textmode"))++g_invalid;
        return g_textMode?3:0;
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

extern "C" __declspec(dllexport) void __cdecl SetTextFixture(int present) noexcept {
    g_textMode=present;g_find=g_checks=g_values=g_invalid=g_provider=0;
}
extern "C" __declspec(dllexport) bool __cdecl VerifyTextFixture() noexcept {
    return !g_invalid && !g_checks && !g_values && g_find==1 && g_provider==1;
}

// Export exactly the original decorated constructor and attach names.
// Fixture-only matching x86 record; no real vstdlib used in CI.
class IUniformRandomStream;
static void* gaussianExpected=nullptr;
static unsigned gaussianConstructs=0,gaussianAttaches=0;
static bool gaussianRefuse=false;
extern "C" __declspec(dllexport) void __cdecl SetGaussianFixture(void* expected,int refuse) noexcept {
    gaussianExpected=expected;gaussianRefuse=refuse!=0;
    gaussianConstructs=gaussianAttaches=0;
}
class __declspec(dllexport) CGaussianRandomStream {
public:
    explicit CGaussianRandomStream(IUniformRandomStream* stream);
    void AttachToStream(IUniformRandomStream* stream);
private:
    IUniformRandomStream* uniform_;
    bool cached_;
    float value_;
};
static_assert(sizeof(CGaussianRandomStream)==12);
CGaussianRandomStream::CGaussianRandomStream(IUniformRandomStream* stream):
    uniform_(stream),cached_(false),value_(0) {
    if(!stream && g_find==1 && g_provider==1)++gaussianConstructs;
}
void CGaussianRandomStream::AttachToStream(IUniformRandomStream* stream) {
    if(gaussianConstructs==1 && stream==gaussianExpected && !gaussianAttaches) {
        ++gaussianAttaches;
        uniform_=gaussianRefuse?nullptr:stream;cached_=false;
    }
}
