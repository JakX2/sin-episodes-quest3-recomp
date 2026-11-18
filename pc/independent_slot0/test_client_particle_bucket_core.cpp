#include "client_particle_bucket_core.h"
#include <array>
#include <cstdlib>
#include <string_view>
using namespace sin_independent;
#define CHECK(x) do { if(!(x)) std::abort(); } while(false)
struct Backend final : ParticleBucketBackend {
    alignas(std::max_align_t) std::array<std::byte,48> storage{};
    std::array<char,32> events{};
    std::size_t count=0;
    bool allocated=false,constructed=false,failAllocate=false,failConstruct=false;
    ParticleBucketOwner* owner=nullptr;
    void Mark(char c) noexcept { CHECK(count<events.size()); events[count++]=c; }
    void Expect(std::string_view sequence) {
        CHECK(std::string_view(events.data(),count)==sequence); count=0;
    }
    void ResetManager() noexcept override {
        Mark('R'); CHECK(!allocated);
        if(owner){CHECK(owner->State()==ParticleBucketState::Starting);
            CHECK(owner->Start()==ParticleBucketResult::AlreadyAttempted);
            CHECK(!owner->Shutdown());}
    }
    void* Allocate(std::size_t bytes) noexcept override {
        Mark('A'); CHECK(bytes==48 && !allocated);
        if(failAllocate)return nullptr;
        allocated=true;return storage.data();
    }
    bool Construct(void* p,std::size_t arg,std::string_view label) noexcept override {
        Mark('C');CHECK(p==storage.data() && allocated && !constructed);
        CHECK(arg==96 && label=="CParticleMgr::m_pParticleBucket");
        if(failConstruct)return false;
        constructed=true;return true;
    }
    void Destroy(void* p) noexcept override {
        Mark('D');CHECK(p==storage.data() && allocated && constructed);
        if(owner){CHECK(owner->Bucket()==nullptr);
            CHECK(owner->State()==ParticleBucketState::Stopping);
            CHECK(!owner->Shutdown());}
        constructed=false;
    }
    void Deallocate(void* p) noexcept override {
        Mark('F');CHECK(p==storage.data() && allocated && !constructed);allocated=false;
    }
};
int main(){
    static_assert(ParticleBucketObjectBytes==48 && ParticleBucketConstructorArgument==96);
    static_assert(OriginalParticleBucketMemberOffset==0x84);
    Backend b;
    {
        ParticleBucketOwner o(b);b.owner=&o;
        CHECK(o.State()==ParticleBucketState::Idle && !o.Bucket());
        CHECK(!o.Shutdown());
        b.failAllocate=true;
        CHECK(o.Start()==ParticleBucketResult::AllocationFailed);
        CHECK(!o.Bucket() && o.State()==ParticleBucketState::Failed);
        CHECK(o.Start()==ParticleBucketResult::AlreadyAttempted);
        b.Expect("RA");CHECK(o.Shutdown());b.Expect("");
        b.failAllocate=false;b.failConstruct=true;
        CHECK(o.Start()==ParticleBucketResult::ConstructionFailed);
        CHECK(!o.Bucket() && !b.allocated);
        b.Expect("RACF");CHECK(o.Shutdown());
        b.failConstruct=false;
        CHECK(o.Start()==ParticleBucketResult::Started);
        CHECK(o.Bucket()==b.storage.data() && o.State()==ParticleBucketState::Ready);
        CHECK(o.Start()==ParticleBucketResult::AlreadyAttempted);
        b.Expect("RAC");CHECK(o.Shutdown());b.Expect("DF");
        CHECK(!o.Shutdown());CHECK(!b.allocated);
        CHECK(o.Start()==ParticleBucketResult::Started);b.Expect("RAC");
        // Destructor must release exactly once if caller omits explicit Shutdown.
    }
    b.owner=nullptr;b.Expect("DF");CHECK(!b.allocated && !b.constructed);
}
