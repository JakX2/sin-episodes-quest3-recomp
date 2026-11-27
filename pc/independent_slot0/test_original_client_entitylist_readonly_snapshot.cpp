// Phase 1656. Native Win32 x86 tests of the exact owner-original
// VClientEntityList003 memory readback contract using invented, bounded
// in-process DWORD records. NO original DLL is linked, loaded or executed.
#include "original_client_entitylist_readonly_snapshot.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase 1656 owner readback regression requires MSVC native Windows x86
#endif

namespace abi=sin_independent::original_client_entitylist_snapshot;
namespace {
constexpr std::uintptr_t kFakeModuleBase=0x50000000u;
struct Entry {
    std::uintptr_t at=0;
    std::uint32_t value=0;
    bool readable=true;
};
struct FakeImage {
    std::array<Entry,14> entries{};
    unsigned reads=0;
};
bool fakeRead(void* context,std::uintptr_t address,std::uint32_t* out) noexcept {
    auto* image=static_cast<FakeImage*>(context);
    if(!image || !out)return false;
    ++image->reads;
    for(const auto& record:image->entries) {
        if(record.at==address) {
            if(!record.readable)return false;
            *out=record.value;
            return true;
        }
    }
    return false;
}
FakeImage inventedImage() {
    FakeImage out{};
    unsigned at=0;
    out.entries[at++]={kFakeModuleBase+abi::kParentObjectRva,
                       static_cast<std::uint32_t>(kFakeModuleBase+
                       abi::kParentVtableRva),true};
    out.entries[at++]={kFakeModuleBase+abi::kSecondaryObjectRva,
                       static_cast<std::uint32_t>(kFakeModuleBase+
                       abi::kSecondaryVtableRva),true};
    for(unsigned i=0u;i<9u;++i)
        out.entries[at++]={
            kFakeModuleBase+abi::kSecondaryVtableRva+4u*i,
            static_cast<std::uint32_t>(kFakeModuleBase+
                                       abi::kSecondaryMethodRvas[i]),true};
    for(unsigned i=0u;i<3u;++i)
        out.entries[at++]={
            kFakeModuleBase+abi::kParentVtableRva+4u*i,
            static_cast<std::uint32_t>(kFakeModuleBase+
                                       abi::kParentMethodRvas[i]),true};
    return out;
}
abi::Result probe(FakeImage& image) noexcept {
    return abi::Observe(kFakeModuleBase,{&image,&fakeRead});
}
int fail(unsigned code,const char* why) {
    std::fprintf(stderr,
        "FAIL Phase1656 original-free client entity-list memory readback "
        "%u: %s\n",code,why);
    return static_cast<int>(code);
}
} // namespace

int main() {
    auto image=inventedImage();
    const auto verified=probe(image);
    if(verified.status!=abi::Status::Verified ||
       verified.secondary_slots_matched!=9u ||
       verified.parent_slots_matched!=3u || image.reads!=14u)
        return fail(1,"correct nine/three table must pass in exactly 14 reads");
    if(abi::Observe(kFakeModuleBase,{nullptr,nullptr}).status!=
       abi::Status::MissingReader)
        return fail(2,"missing reader was accepted");
    image.reads=0;
    if(abi::Observe(0u,{&image,&fakeRead}).status!=
       abi::Status::InvalidBase ||
       abi::Observe(0xfffffff0u,{&image,&fakeRead}).status!=
       abi::Status::InvalidBase || image.reads!=0u)
        return fail(3,"invalid 32-bit module base was dereferenced");
    for(std::size_t i=0u;i<image.entries.size();++i) {
        auto changed=inventedImage();
        changed.entries[i].value^=0x01u;
        const auto result=probe(changed);
        const auto expected=(i==0)?abi::Status::ParentVptrMismatch:
            (i==1)?abi::Status::SecondaryVptrMismatch:
            (i<11)?abi::Status::SecondarySlotMismatch:
                   abi::Status::ParentSlotMismatch;
        if(result.status!=expected || result.status==abi::Status::Verified)
            return fail(4,"corrupted single pointer was accepted");
        changed=inventedImage();
        changed.entries[i].readable=false;
        const auto missing=probe(changed);
        const auto missingStatus=(i==0)?abi::Status::ParentVptrUnreadable:
            (i==1)?abi::Status::SecondaryVptrUnreadable:
            (i<11)?abi::Status::SecondarySlotUnreadable:
                   abi::Status::ParentSlotUnreadable;
        if(missing.status!=missingStatus)
            return fail(5,"unreadable original memory accepted");
    }
    // A forged primary-interface alias must never pass the embedded
    // object's +0x10014 vptr check, even when every virtual entry matches.
    image=inventedImage();
    image.entries[1].value=image.entries[0].value;
    if(probe(image).status!=abi::Status::SecondaryVptrMismatch)
        return fail(6,"primary/secondary pointer confusion not rejected");
    // Inter-table DWORD 0x242973B4 is intentionally NOT read as a tenth
    // secondary method. Owner readback does not guess its RTTI semantics.
    if(abi::kParentVtableRva-abi::kSecondaryVtableRva!=40u)
        return fail(7,"nine virtuals + four inter-table bytes boundary drift");
    std::puts("PASS Phase1656 original-free owner client entity-list snapshot: "
              "14 exact DWORD reads, primary/secondary pointer offsets, "
              "nine virtuals, three distinct parent slots, every pointer "
              "mutation/refusal and overflow-safe bounds");
    return 0;
}
