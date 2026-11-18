#include "client_screen_random_core.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
using namespace sin_independent;
class EngineFixture {
public:
    virtual void __thiscall Guard0() noexcept {bad=true;}
    virtual void __thiscall Guard1() noexcept {bad=true;}
    virtual void __thiscall Guard2() noexcept {bad=true;}
    virtual void __thiscall Guard3() noexcept {bad=true;}
    virtual void __thiscall Guard4() noexcept {bad=true;}
    virtual void __thiscall GetScreenSize(int& w,int& h) noexcept {
        if(w!=0 || h!=0 || &w==&h)bad=true;
        w=width;h=height;++calls;
    }
    int width=1920,height=1080;
    unsigned calls=0;
    bool bad=false;
};
static unsigned constructed=0,attached=0;
static void* uniformExpected=nullptr;
static GaussianStorage* expected=nullptr;
// Fixture-only native methods with the recovered nonvirtual calling
// convention. Never substitute these for vstdlib in a retail process.
class GaussianFixture {
public:
    void* __thiscall Construct(void* stream) noexcept {
        if(stream || reinterpret_cast<void*>(this)!=expected)return nullptr;
        ++constructed;
        auto* storage=reinterpret_cast<GaussianStorage*>(this);
        storage->uniform=stream;storage->haveValue=true;storage->cached=13.0f;
        return this;
    }
    void __thiscall Attach(void* stream) noexcept {
        if(constructed!=1 || stream!=uniformExpected ||
           reinterpret_cast<void*>(this)!=expected)return;
        auto* storage=reinterpret_cast<GaussianStorage*>(this);
        storage->uniform=stream;storage->haveValue=false;
        ++attached;
    }
};
static_assert(sizeof(void (GaussianFixture::*)())==sizeof(void*),"single inheritance native pointer");
template<class Fn,class Member> Fn Address(Member ptr) noexcept {
    static_assert(sizeof(Fn)==sizeof(Member));
    Fn fn=nullptr;
    // MSVC x86 single-inheritance nonvirtual member representation
    // has one executable address.
    std::memcpy(&fn,&ptr,sizeof(fn));
    return fn;
}
int main() {
    EngineFixture engine;
    ClientScreenRandomState state;
    const auto ctor=Address<GaussianCtor>(&GaussianFixture::Construct);
    const auto attach=Address<GaussianAttach>(&GaussianFixture::Attach);
    int uniform=0;
    expected=&state.gaussian;uniformExpected=&uniform;
    if(state.Attach(&uniform,ctor,attach) || constructed ||
       state.screen.Query(nullptr) || state.screen.queried)return 1;
    if(!state.screen.Query(&engine) || engine.bad || engine.calls!=1 ||
       state.screen.width!=1920 || state.screen.height!=1080 ||
       state.screen.Query(&engine) || engine.calls!=1)return 2;
    if(state.Attach(nullptr,ctor,attach) || state.Attach(&uniform,nullptr,attach) ||
       state.Attach(&uniform,ctor,nullptr) || constructed)return 3;
    if(!state.Attach(&uniform,ctor,attach) || constructed!=1 || attached!=1 ||
       !state.attached || state.gaussian.uniform!=&uniform || state.gaussian.haveValue ||
       state.Attach(&uniform,ctor,attach))return 4;
    EngineFixture zero;zero.width=0;zero.height=0;
    ScreenState screen;
    if(!screen.Query(&zero) || !screen.queried || screen.width || screen.height ||
       zero.bad || zero.calls!=1)return 5;
    void* table[6]={};void** empty=table;
    ScreenState missing;
    if(missing.Query(&empty) || missing.queried)return 6;
    std::puts("PASS native client screen slot 5, exact output references and owned Gaussian constructor/Attach ABI");
    return 0;
}
