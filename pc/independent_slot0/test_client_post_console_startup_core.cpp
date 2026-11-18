#include "client_post_console_startup_core.h"
#include <array>
#include <cstdlib>
#include <string_view>
using namespace sin_independent;
#define CHECK(x) do { if(!(x)) std::abort(); } while(false)
enum class Failure { None, Object, Allocate, Construct, Module, Input, System, Helper };
struct Fixture : ParticleBucketBackend,ClientVGuiBackend {
    Failure failure;
    std::array<char,64> events{};
    std::size_t n=0;
    alignas(std::max_align_t) std::array<std::byte,48> storage{};
    int object=1,input=2,system=3;
    void* destination=nullptr;
    ClientPostConsoleStartup<1>* sequence=nullptr;
    explicit Fixture(Failure f):failure(f){}
    void Mark(char c) noexcept {CHECK(n<events.size());events[n++]=c;}
    void Expect(std::string_view s){CHECK(std::string_view(events.data(),n)==s);n=0;}
    void Reentry() noexcept {
        CHECK(sequence->Start()==ClientPostConsoleResult::AlreadyAttempted);
        CHECK(!sequence->Shutdown());
    }
    void ResetManager() noexcept override {Mark('R');CHECK(destination==&object);Reentry();}
    void* Allocate(std::size_t b) noexcept override {
        Mark('A');CHECK(b==48);return failure==Failure::Allocate?nullptr:storage.data();
    }
    bool Construct(void* p,std::size_t b,std::string_view l) noexcept override {
        Mark('C');CHECK(p==storage.data() && b==96 && l==ParticleBucketLabel);
        return failure!=Failure::Construct;
    }
    void Destroy(void* p) noexcept override {Mark('D');CHECK(p==storage.data());Reentry();}
    void Deallocate(void* p) noexcept override {Mark('F');CHECK(p==storage.data());}
    bool CheckModule(std::string_view n) noexcept override {
        Mark('M');CHECK(n=="CLIENT" && sequence->Bucket()==storage.data());Reentry();
        return failure!=Failure::Module;
    }
    void* Query(std::string_view version) noexcept override {
        if(version=="VGUI_InputInternal001"){Mark('I');return failure==Failure::Input?nullptr:&input;}
        CHECK(version=="VGUI_System010");Mark('S');return failure==Failure::System?nullptr:&system;
    }
    void RegisterOnce() noexcept override {Mark('G');}
    void FirstHelper() noexcept override {Mark('H');}
    bool SecondHelper() noexcept override {Mark('J');return failure!=Failure::Helper;}
    void ReleaseFixture() noexcept override {Mark('V');Reentry();}
};
static Fixture* current=nullptr;
static void* __cdecl Create(){
    current->Mark('O');current->Reentry();
    return current->failure==Failure::Object?nullptr:&current->object;
}
static void __cdecl Free(void* p){
    current->Mark('X');
    CHECK(p==(current->failure==Failure::Object?nullptr:&current->object));
    current->Reentry();
}
static void Run(Failure failure,ClientPostConsoleResult result,std::string_view expected){
    Fixture f(failure);current=&f;
    ClientInitializer record{&f.destination,Create,Free,nullptr};
    ClientInitializers<1> initializers;
    CHECK(initializers.Add(record));
    {
        ClientPostConsoleStartup<1> s(initializers,f,f);f.sequence=&s;
        CHECK(s.Start()==result);f.Expect(expected);
        CHECK(s.Start()==ClientPostConsoleResult::AlreadyAttempted);
        if(result==ClientPostConsoleResult::Ready){
            CHECK(s.State()==ClientPostConsoleState::Ready);
            CHECK(s.GuiState().input==&f.input && s.GuiState().system==&f.system);
            CHECK(s.GuiState().one_time_registered && f.destination==&f.object);
            CHECK(s.Shutdown());f.Expect("VDFX");
        }else{
            CHECK(s.State()==ClientPostConsoleState::Failed);
            CHECK(s.Shutdown());f.Expect("");
        }
        CHECK(!s.Bucket() && !f.destination);
        CHECK(!s.GuiState().input && !s.GuiState().system);
        CHECK(s.State()==ClientPostConsoleState::Finished && !s.Shutdown());
    }
    f.Expect("");current=nullptr;
}
int main(){
    using R=ClientPostConsoleResult;
    Run(Failure::None,R::Ready,"ORACMISGHJ");
    Run(Failure::Object,R::ObjectsFailed,"OX");
    Run(Failure::Allocate,R::ParticleFailed,"ORAX");
    Run(Failure::Construct,R::ParticleFailed,"ORACFX");
    Run(Failure::Module,R::VGuiFailed,"ORACMVDFX");
    Run(Failure::Input,R::VGuiFailed,"ORACMIVDFX");
    Run(Failure::System,R::VGuiFailed,"ORACMISVDFX");
    Run(Failure::Helper,R::VGuiFailed,"ORACMISGHJVDFX");
    // External objects must not be freed if this startup did not acquire them.
    Fixture f(Failure::None);current=&f;
    ClientInitializers<1> already;
    CHECK(already.Initialize()==InitializerResult::Initialized);
    ClientPostConsoleStartup<1> s(already,f,f);f.sequence=&s;
    CHECK(s.Start()==R::ObjectsFailed);
    CHECK(s.Shutdown() && already.State()==InitializerState::Ready);
    CHECK(already.Free());f.Expect("");current=nullptr;
}
