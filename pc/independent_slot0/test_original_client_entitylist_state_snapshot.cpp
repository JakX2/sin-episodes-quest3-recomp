// Phase 1657: fake memory only. Four exact counter-address reads are
// tested against bounded original 2006 PE32 CClientEntityList fields.
#include "original_client_entitylist_state_snapshot.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1657 owner state test requires MSVC native Windows x86
#endif
namespace snapshot=sin_independent::original_client_entitylist_state_snapshot;
namespace {
constexpr std::uintptr_t kFakeModule=0x51000000u;
struct Word {std::uintptr_t addr=0u;std::uint32_t value=0u;bool readable=true;};
struct Fixture {
    std::array<Word,4> words{};
    unsigned read_count=0u;
};
Fixture makeFixture() noexcept {
    Fixture f{};
    f.words={{{kFakeModule+snapshot::kNetworkedCountRva,0u,true},
              {kFakeModule+snapshot::kMaxEntitiesRva,0u,true},
              {kFakeModule+snapshot::kFlaggedCountRva,0u,true},
              {kFakeModule+snapshot::kHighestIndexRva,0xFFFFFFFFu,true}}};
    return f;
}
bool readWord(void* opaque,std::uintptr_t at,std::uint32_t* result) noexcept {
    auto* f=static_cast<Fixture*>(opaque);
    if(!f || !result)return false;
    ++f->read_count;
    for(const auto& word:f->words) {
        if(word.addr==at) {
            if(!word.readable)return false;
            *result=word.value;
            return true;
        }
    }
    return false;
}
snapshot::Result inspect(Fixture& f) noexcept {
    return snapshot::Observe(kFakeModule,{&f,&readWord});
}
int fail(unsigned code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1657 native original entity-list startup snapshot %u: %s\n",
      code,why);
    return static_cast<int>(code);
}
} // namespace
int main() {
    auto f=makeFixture();
    auto result=inspect(f);
    if(result.status!=snapshot::Status::Valid ||
       result.networked_count!=0u || result.max_entities!=0u ||
       result.secondary_flagged_count!=0u ||
       result.highest_live_index!=-1 || result.successful_reads!=4u ||
       f.read_count!=4u)
        return fail(1,"owner constructor zero count/empty-highwater view");
    if(snapshot::Observe(0u,{&f,&readWord}).status!=
       snapshot::Status::InvalidBase ||
       snapshot::Observe(0xFFFFFF00u,{&f,&readWord}).status!=
       snapshot::Status::InvalidBase ||
       snapshot::Observe(kFakeModule,{nullptr,nullptr}).status!=
       snapshot::Status::MissingReader)
        return fail(2,"module base/refusal/absent reader");
    for(unsigned i=0u;i<4u;++i) {
        f=makeFixture();
        f.words[i].readable=false;
        result=inspect(f);
        const auto expected=(i==0)?snapshot::Status::LiveCountUnreadable:
            (i==1)?snapshot::Status::MaxEntitiesUnreadable:
            (i==2)?snapshot::Status::FlaggedCountUnreadable:
                   snapshot::Status::HighestIndexUnreadable;
        if(result.status!=expected || result.successful_reads!=i)
            return fail(3,"unreadable original counter field accepted");
    }
    // A legitimate populated original owner can be observed too; do not
    // require zero values outside DllMain static-constructor snapshots.
    f=makeFixture();
    f.words[0].value=3u;
    f.words[1].value=2048u;
    f.words[2].value=2u;
    f.words[3].value=7u;
    if(inspect(f).status!=snapshot::Status::Valid)
        return fail(4,"bounded nonempty entity-list state refused");
    for(unsigned i=0u;i<3u;++i) {
        f=makeFixture();
        f.words[i].value=(i==0)?2049u:4097u;
        if(inspect(f).status!=snapshot::Status::CountsOutOfRange)
            return fail(5,"unsafe original count/maximum accepted");
    }
    f=makeFixture();f.words[3].value=0u;
    if(inspect(f).status!=snapshot::Status::HighestIndexContradiction)
        return fail(6,"empty entity count with nonempty max index accepted");
    f=makeFixture();f.words[0].value=1u;
    if(inspect(f).status!=snapshot::Status::HighestIndexContradiction)
        return fail(7,"nonempty count with empty highest index accepted");
    f=makeFixture();f.words[0].value=2u;f.words[3].value=0u;
    if(inspect(f).status!=snapshot::Status::HighestIndexContradiction)
        return fail(8,"count exceeds highest slot+1");
    f=makeFixture();f.words[3].value=0xFFFFFFFEu;
    if(inspect(f).status!=snapshot::Status::HighestIndexContradiction)
        return fail(9,"negative highest out of original range accepted");
    f=makeFixture();f.words[3].value=2048u;
    if(inspect(f).status!=snapshot::Status::HighestIndexContradiction)
        return fail(10,"highest index beyond original 2047 accepted");
    std::puts("PASS Phase1657 fake-memory owner client entity-list: four "
              "constructor-state DWORDs, exact offsets, zero/initialized "
              "state, valid populated state, unreadable/overflow/counter "
              "corruption and impossible-highwater refusal");
    return 0;
}
