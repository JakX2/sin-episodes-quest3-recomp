// Phase1704: source-authenticated original RecvProp names, metadata only.
#include "sin_compat/original_client_native32_recvtable_image.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace image32=sin_compat::original_client_native32_recvtable_image;
namespace catalog=sin_compat::original_client_class_catalog;
namespace src=sin_compat::original_client_native32_recvtable_manifest;

template<class F> void MustRefuse(F&& action) {
    bool rejected=false;
    try{action();}catch(const std::exception&){rejected=true;}
    assert(rejected);
}

int main(int argc,char** argv) {
    assert(argc==3);
    std::ifstream classes(argv[1]);
    assert(classes.good());
    catalog::Catalog originalCatalog;
    assert(originalCatalog.Load(classes));
    assert(originalCatalog.Size()==191u);

    std::ifstream csv(argv[2],std::ios::binary);
    assert(csv.good());
    std::ostringstream stream;
    stream<<csv.rdbuf();
    assert(csv.good() || csv.eof());
    const std::string originalCsv=stream.str();
    assert(originalCsv.size()>60000u);

    auto image=image32::Image::Build(originalCatalog);
    assert(!image.StaticSourceNamesAttached());
    assert(!image.CanExposeToRetailEngine());

    // A corrupted, still parseable name must not partly mutate the arena.
    {
        auto wrong=originalCsv;
        const auto first=wrong.find("baseclass");
        assert(first!=std::string::npos);
        wrong[first]='X';
        std::istringstream tampered(wrong);
        const auto previousSize=image.arena.Size();
        MustRefuse([&]{image.AttachStaticSourceNames(tampered);});
        assert(!image.StaticSourceNamesAttached());
        assert(image.arena.Size()==previousSize);
        assert(image.arena.Read32(image.staticPropArrayTokens[0u])==0u);
    }
    // A structurally valid but incorrect source DWORD at +44 must also
    // be rejected transactionally before any name pointer is installed.
    {
        auto wrong=originalCsv;
        const auto marker=wrong.find(",5,0,member_offset,");
        assert(marker!=std::string::npos);
        wrong.replace(marker,std::string(",5,0,member_offset,").size(),
                      ",5,1,member_offset,");
        std::istringstream tampered(wrong);
        MustRefuse([&]{image.AttachStaticSourceNames(tampered);});
        assert(!image.StaticSourceNamesAttached());
        assert(image.arena.Read32(image.staticPropArrayTokens[0u])==0u);
    }
    {
        std::istringstream authenticated(originalCsv);
        image.AttachStaticSourceNames(authenticated);
        assert(image.StaticSourceNamesAttached());
    }
    assert(!image.CanExposeToRetailEngine());
    std::unordered_set<std::string> unique;
    std::size_t populated=0u;
    for(std::size_t t=0u;t<src::kStaticTables;++t) {
        for(std::size_t p=0u;p<src::kCounts[t];++p) {
            const auto idx=src::kTypePrefixes[t]+p;
            const auto prop=image.staticPropArrayTokens[t]+
                static_cast<std::uint32_t>(p*src::kOriginalRecvPropStride);
            const auto namePtr=image.arena.Read32(prop);
            assert(namePtr>=image32::kArenaBaseVA);
            const auto name=image.arena.ReadString(namePtr);
            assert(name==image.originalStaticNames[idx]);
            assert(name.size()<=127u && !name.empty());
            unique.insert(name);
            ++populated;
        }
    }
    assert(populated==1110u && unique.size()==665u);
    const auto team=image.staticPropArrayTokens[81u]+4u*60u;
    assert(image.arena.ReadString(image.arena.Read32(team))==
           "\"player_array\"");
    assert(image.arena.Read32(team+4u)==4u);
    assert(image.arena.Read32(team+48u)==0u);
    assert(image.arena.Read32(team+52u)==32u);

    // Phase1705 original 621-byte overlay constructor zeroes its native
    // descriptor byte +44 before copying 60 bytes. The parent object
    // offset 0x7A4 belongs to an auxiliary callback record, NOT +44.
    const auto overlay=image.unresolvedOverlayDescriptorToken;
    assert(overlay==image.staticPropArrayTokens[197u]);
    assert(image.arena.Read32(overlay+4u)==5u);
    assert(image.arena.Read32(overlay+44u)==0u);
    assert(image.arena.Read32(overlay+40u)==0u); // callback/child unresolved
    assert(!image.CanExposeToRetailEngine());

    const auto rooted=image.WalkFromClientClassRoots();
    assert(rooted.visitedTables==224u);
    assert(rooted.visitedProperties==1796u);
    assert(rooted.blockedUnimplementedNativeType5==1u);
    const auto all=image.WalkAllStaticTables();
    assert(all.visitedTables==225u && all.visitedProperties==1801u);

    // The source-proven special overlay has +44 ZERO, even though its
    // type5 child table and callbacks remain intentionally unimplemented.
    {
        const auto overlay=image.unresolvedOverlayDescriptorToken;
        image.arena.Write32(overlay+44u,0x7A4u);
        MustRefuse([&]{(void)image.WalkFromClientClassRoots();});
        image.arena.Write32(overlay+44u,0u);
    }
    // Every reached name must be original, not just a readable token.
    {
        const auto prop=image.staticPropArrayTokens[0u];
        const auto token=image.arena.Read32(prop);
        const auto originalFirst=image.arena.Read8(token);
        image.arena.Write8(token,static_cast<std::uint8_t>('Z'));
        MustRefuse([&]{(void)image.WalkFromClientClassRoots();});
        image.arena.Write8(token,originalFirst);
    }
    // Wrong-but-owned alternative name pointers are not acceptable.
    {
        const auto prop=image.staticPropArrayTokens[0u];
        const auto saved=image.arena.Read32(prop);
        const auto alternative=image.arena.Read32(
            image.staticPropArrayTokens[43u]+31u*60u);
        assert(saved!=alternative);
        assert(image.arena.ReadString(alternative)!="baseclass");
        image.arena.Write32(prop,alternative);
        MustRefuse([&]{(void)image.WalkFromClientClassRoots();});
        image.arena.Write32(prop,saved);
    }
    // The source-root graph deliberately does not walk auxiliary
    // DT_Animationlayer ordinal196; exhaustive inventory does.
    {
        const auto prop=image.staticPropArrayTokens[196u];
        const auto token=image.arena.Read32(prop);
        const auto first=image.arena.Read8(token);
        image.arena.Write8(token,static_cast<std::uint8_t>('Z'));
        const auto rootedStillValid=image.WalkFromClientClassRoots();
        assert(rootedStillValid.visitedTables==224u);
        MustRefuse([&]{(void)image.WalkAllStaticTables();});
        image.arena.Write8(token,first);
    }
    MustRefuse([&]{
        std::istringstream duplicate(originalCsv);
        image.AttachStaticSourceNames(duplicate);
    });
    const auto restored=image.WalkFromClientClassRoots();
    assert(restored.visitedTables==224u &&
           restored.visitedProperties==1796u);
    assert(!restored.CompleteRetailRecursion());
    assert(!image.CanExposeToRetailEngine());
    std::puts("PASS Phase1704 original 1110 static RecvProp names / 665 unique, "
              "224 rooted / 225 inventory, source-exact mutation refusal, "
              "NO retail slot5");
    return 0;
}
