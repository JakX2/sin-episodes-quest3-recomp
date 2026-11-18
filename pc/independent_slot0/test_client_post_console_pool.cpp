#include "client_post_console_startup_core.h"
#include "client_particle_pool_core.h"
#include <cstdlib>
using namespace sin_independent;
#define CHECK(x) do { if(!(x)) std::abort(); } while(false)

struct Fixture final : ClientVGuiBackend {
    IndependentParticleBucketBackend backend;
    ClientInitializers<1> objects;
    ClientInitializer record{};
    void* destination=nullptr;
    int object=1,input=2,system=3;
    int creates=0,destroys=0,checks=0,queries=0,registrations=0,helpers=0,releases=0;
    bool failHelper=false;
    OwnedParticlePool::Handle live{};
    explicit Fixture(std::size_t capacity):backend(capacity){}
    bool CheckModule(std::string_view s) noexcept override {
        ++checks;CHECK(s=="CLIENT" && destination==&object && backend.Pool().Ready());
        live=backend.Pool().Acquire();CHECK(live);
        auto* p=backend.Pool().Resolve(live);CHECK(p);
        p->bytes[0]=std::byte{0x47};return true;
    }
    void* Query(std::string_view s) noexcept override {
        ++queries;
        CHECK(backend.Pool().Resolve(live)->bytes[0]==std::byte{0x47});
        if(s=="VGUI_InputInternal001")return &input;
        CHECK(s=="VGUI_System010");return &system;
    }
    void RegisterOnce() noexcept override {++registrations;}
    void FirstHelper() noexcept override {++helpers;}
    bool SecondHelper() noexcept override {++helpers;return !failHelper;}
    void ReleaseFixture() noexcept override {
        ++releases;
        CHECK(backend.Pool().Ready() && backend.Pool().Release(live));
    }
};
static Fixture* current=nullptr;
static void* __cdecl Create(){
    ++current->creates;CHECK(!current->backend.Pool().Ready());
    return &current->object;
}
static void __cdecl Free(void* p){
    ++current->destroys;CHECK(p==&current->object && !current->backend.Pool().Ready());
}
static void Run(std::size_t capacity,bool failHelper,bool explicitShutdown){
    using R=ClientPostConsoleResult;
    Fixture f(capacity);current=&f;f.failHelper=failHelper;
    f.record={&f.destination,Create,Free,nullptr};
    CHECK(f.objects.Add(f.record));
    {
        ClientPostConsoleStartup<1> startup(f.objects,f,f.backend);
        const auto expected=capacity==0?R::ParticleFailed:failHelper?R::VGuiFailed:R::Ready;
        CHECK(startup.Start()==expected);
        CHECK(f.creates==1);
        if(expected==R::Ready){
            CHECK(startup.Bucket() && f.backend.Pool().Used()==1);
            CHECK(startup.GuiState().input==&f.input && startup.GuiState().system==&f.system);
            CHECK(f.destination==&f.object);
            if(explicitShutdown)CHECK(startup.Shutdown());
        }else{
            CHECK(!startup.Bucket() && !f.backend.Pool().Ready() && !f.destination);
            CHECK(startup.Shutdown());
        }
    }
    CHECK(f.destroys==1 && !f.destination && !f.backend.Pool().Ready());
    CHECK(!f.backend.Pool().Resolve(f.live));
    if(capacity==0){
        CHECK(!f.checks && !f.queries && !f.registrations && !f.helpers && !f.releases);
    }else{
        CHECK(f.checks==1 && f.queries==2 && f.registrations==1 && f.helpers==2 && f.releases==1);
    }
    current=nullptr;
}
int main(){
    Run(2048,false,true);
    Run(8,false,false);
    Run(8,true,false);
    Run(0,false,false);
}
