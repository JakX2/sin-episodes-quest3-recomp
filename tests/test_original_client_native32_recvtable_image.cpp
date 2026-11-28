#include "sin_compat/original_client_native32_recvtable_image.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

namespace native=sin_compat::original_client_native32_recvtable_image;
namespace schema=sin_compat::original_client_native32_recvtable_manifest;
namespace classes=sin_compat::original_client_class_catalog;
namespace staticLinks=sin_compat::original_client_static_recvtable_links;
namespace classArrays=sin_compat::original_client_nested_array_table;
namespace auxArrays=sin_compat::original_client_aux_dynamic_receivers;

template<typename F>
static void expect_exception(F&& func) {
    bool rejected=false;
    try{func();}
    catch(const std::exception&){rejected=true;}
    assert(rejected);
}
static void check_original_source_counts() {
    static_assert(schema::kStaticTables==211u);
    static_assert(schema::kStaticPropertySlots==1110u);
    static_assert(schema::kStaticClassTables==191u);
    static_assert(schema::kStaticAuxiliaryTables==20u);
    static_assert(schema::kGeneratedGenericSubtables==14u);
    static_assert(schema::kGeneratedGenericSubtableProps==691u);
    static_assert(schema::kNativeTypeFiveProps==207u);
    static_assert(schema::kOriginalRecvPropStride==60u);
    static_assert(schema::kOriginalRecvTableBytes==20u);
    static_assert(schema::kOriginalClientClassPrefixBytes==20u);
    static_assert(schema::kTypePrefixes[211]==1110u);
    static_assert(schema::EveryNativeTypeFiveRouteSourceClassified());
    static_assert(schema::SourceCardinality());
    assert(!schema::CanExposeToRetailEngine());
}

static void test_owned_arena_refusal() {
    native::Arena arena;
    assert(arena.Allocate(0u)==0u);
    assert(arena.Size()==0u);
    const auto first=arena.Allocate(4u);
    assert(first==native::kArenaBaseVA);
    arena.Write32(first,0xC0FFEE99u);
    assert(arena.Read8(first)==0x99u);
    assert(arena.Read32(first)==0xC0FFEE99u);
    expect_exception([&]{(void)arena.Read32(0x2437F720u);});
    expect_exception([&]{arena.Write32(first+2u,0x1234u);});
    expect_exception([&]{arena.Write8(0u,42u);});
    expect_exception([&]{(void)arena.Read32(0u);});
    expect_exception([&]{(void)arena.StoreString("");});
    expect_exception([&]{(void)arena.StoreString(std::string(128,'x'));});
    const auto name=arena.StoreString("DT_Fabricated");
    assert(arena.ReadString(name)=="DT_Fabricated");
    assert(arena.Read8(name+13u)==0u);
}

static void test_exact_native32_image(const char* sourceManifest) {
    std::ifstream original(sourceManifest);
    assert(original.good());
    classes::Catalog catalog;
    assert(catalog.Load(original));
    assert(catalog.Size()==191u);
    assert(catalog.Head() && catalog.Head()->networkName=="CWeaponMelee");
    assert(!catalog.CanExposeToRetailEngine());

    auto image=native::Image::Build(catalog);
    assert(!image.CanExposeToRetailEngine());
    assert(image.classHeadToken==image.classNodeTokens.front());
    assert(image.arena.Size()>1110u*60u+691u*60u);
    assert(image.arena.Size()<400000u);

    // All 191 original linked ClientClass head prefixes, in exact
    // retail source order, hold host-safe 32-bit arena tokens.
    std::size_t cls=0u;
    std::uint32_t current=image.classHeadToken;
    for(const auto* source=catalog.Head();source;source=source->next) {
        assert(cls<191u && current==image.classNodeTokens[cls]);
        assert(image.arena.Read32(current+0u)==0u); // creators NOT rebuilt
        assert(image.arena.Read32(current+4u)==0u); // temp-event NOT rebuilt
        const auto label=image.arena.Read32(current+native::kNativeClassNameOffset);
        assert(image.arena.ReadString(label)==source->networkName);
        assert(image.arena.Read32(current+native::kNativeClassRecvTableOffset)==
               image.staticTableTokens[cls]);
        current=image.arena.Read32(current+native::kNativeClassNextOffset);
        ++cls;
    }
    assert(cls==191u && current==0u);
    assert(image.arena.Read32(image.classNodeTokens.back()+
                              native::kNativeClassNextOffset)==0u);

    // Each of the 211 original static tables is the proper 20-byte
    // source layout header, references the right 60-byte descriptor
    // array and contains original per-slot type DWORDs 0..5.
    std::array<std::size_t,6> originalNativeTypes{};
    std::size_t counted=0u;
    for(std::size_t table=0u;table<schema::kStaticTables;++table) {
        const auto tableToken=image.staticTableTokens[table];
        const auto props=image.arena.Read32(tableToken+0u);
        const auto count=image.arena.Read32(tableToken+4u);
        assert(count==schema::kCounts[table]);
        assert(props==image.staticPropArrayTokens[table]);
        assert(image.arena.Read32(tableToken+8u)==0u);
        assert(image.arena.Read8(tableToken+17u)==0u);
        for(std::uint32_t slot=0u;slot<count;++slot) {
            const auto prop=props+slot*60u;
            const auto type=image.arena.Read32(prop+4u);
            assert(type<6u && type==schema::kTypes[
                schema::kTypePrefixes[table]+slot]);
            ++originalNativeTypes[type];
            ++counted;
        }
    }
    assert(counted==1110u);
    assert((originalNativeTypes==std::array<std::size_t,6>{{
        414u,300u,164u,12u,13u,207u}}));
    // All 192 real original static type5 edges point to valid owned
    // source-specific child table tokens, never original PE32 VAs.
    for(const auto& edge:staticLinks::kOriginalSourceEdges) {
        const auto parent=image.staticPropArrayTokens[edge.parentTableOrdinal]+
            std::uint32_t(edge.parentPropertySlot)*60u;
        assert(image.arena.Read32(parent+4u)==5u);
        assert(image.arena.Read32(parent+40u)==
               image.staticTableTokens[edge.childTableOrdinal]);
    }

    // All 14 original generic dynamically generated child RecvTables
    // retain actual native source counts, type0/1, original three-digit
    // names, child-local byte +44 offsets, and separate OUTER object base.
    std::size_t elements=0u;
    for(std::size_t i=0u;i<schema::kGeneratedGenericSubtables;++i) {
        const auto& edge=schema::kGeneratedOuters[i];
        const bool classSource=i<classArrays::kOriginalShapes.size();
        const auto count=classSource?
            classArrays::kOriginalShapes[i].elements:
            auxArrays::kSourceShapes[i-classArrays::kOriginalShapes.size()].elements;
        const auto stride=classSource?
            classArrays::kOriginalShapes[i].byteStride:
            auxArrays::kSourceShapes[i-classArrays::kOriginalShapes.size()].byteStride;
        const auto base=classSource?
            classArrays::kOriginalShapes[i].firstX86Offset:
            auxArrays::kSourceShapes[i-classArrays::kOriginalShapes.size()].originalOuterParentX86Offset;
        const auto type=classSource?
            classArrays::kOriginalShapes[i].elementNativeType:
            auxArrays::kSourceShapes[i-classArrays::kOriginalShapes.size()].originalElementType;
        const auto parent=image.staticPropArrayTokens[edge.parentTableOrdinal]+
            std::uint32_t(edge.parentSlot)*60u;
        const auto table=image.dynamicTableTokens[i];
        const auto props=image.dynamicPropArrayTokens[i];
        assert(image.arena.Read32(parent+4u)==5u);
        assert(image.arena.Read32(parent+40u)==table);
        assert(image.arena.Read32(parent+44u)==base);
        assert(image.arena.Read32(table+0u)==props);
        assert(image.arena.Read32(table+4u)==count);
        for(std::uint32_t slot=0u;slot<count;++slot) {
            const auto prop=props+slot*60u;
            assert(image.arena.Read32(prop+4u)==type);
            assert(image.arena.Read32(prop+44u)==slot*stride);
            const auto name=image.arena.Read32(prop+0u);
            const std::string expected={
                static_cast<char>('0'+slot/100u),
                static_cast<char>('0'+(slot/10u)%10u),
                static_cast<char>('0'+slot%10u)
            };
            assert(image.arena.ReadString(name)==expected);
            ++elements;
        }
    }
    assert(elements==691u);

    // The genuine specialized overlay source has a unique native type5
    // property but lacks executable callbacks, so the child pointer
    // MUST stay NULL and NEVER create a fake success for the retail engine.
    assert(image.unresolvedOverlayDescriptorToken==
           image.staticPropArrayTokens[197u]);
    assert(image.arena.Read32(image.unresolvedOverlayDescriptorToken+4u)==5u);
    assert(image.arena.Read32(image.unresolvedOverlayDescriptorToken+40u)==0u);
    const auto walk=image.WalkAllStaticTables();
    assert(walk.visitedTables==225u);          // 211 static + 14 generic child
    assert(walk.visitedProperties==1801u);     // 1110 static + 691 generated
    assert(walk.traversedNativeType5==207u);   // 192 +14 +1 specialized
    assert(walk.blockedUnimplementedNativeType5==1u);
    assert(walk.duplicateSubtableReferences>0u);
    assert(!walk.CompleteRetailRecursion());
    assert(!image.CanExposeToRetailEngine());

    // Changes in source-mapped pointers/counts/type values are REFUSED,
    // not silently treated as a successful engine table traversal.
    {
        const auto first=image.staticPropArrayTokens[0u];
        const auto save=image.arena.Read32(first+40u);
        image.arena.Write32(first+40u,0xDEADBEEFu);
        expect_exception([&]{(void)image.WalkAllStaticTables();});
        image.arena.Write32(first+40u,save);
    }
    {
        const auto prop=image.staticPropArrayTokens[0u];
        const auto save=image.arena.Read32(prop+4u);
        image.arena.Write32(prop+4u,99u);
        expect_exception([&]{(void)image.WalkAllStaticTables();});
        image.arena.Write32(prop+4u,save);
    }
    {
        const auto table=image.staticTableTokens[0u];
        image.arena.Write32(table+4u,257u);
        expect_exception([&]{(void)image.WalkAllStaticTables();});
        image.arena.Write32(table+4u,schema::kCounts[0u]);
    }
    {
        const auto overlay=image.unresolvedOverlayDescriptorToken;
        image.arena.Write32(overlay+40u,0x2437F720u);
        expect_exception([&]{(void)image.WalkAllStaticTables();});
        image.arena.Write32(overlay+40u,0u);
    }
    const auto repaired=image.WalkAllStaticTables();
    assert(repaired.visitedTables==225u &&
           repaired.visitedProperties==1801u &&
           repaired.blockedUnimplementedNativeType5==1u);
}
int main(int argc,char** argv) {
    assert(argc==2);
    check_original_source_counts();
    test_owned_arena_refusal();
    test_exact_native32_image(argv[1]);
    std::puts("PASS Phase1700 2006 PE32 owned-token ABI image: 191 real class nodes, 211 static tables, 1110 original static RecvProps, 14 generated tables / 691 child props, 207 type5 edges with one blocked overlay; NO retail client slot5 exposure");
    return 0;
}
