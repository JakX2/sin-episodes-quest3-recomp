#pragma once
// Phase1700: source-backed 32-bit LITTLE-ENDIAN PE32 layout sandbox for
// all 191 original ClientClass heads, 211 static RecvTable headers, all
// 1110 static source-typed native 60-byte RecvProp records and fourteen
// dynamic generic child tables (691 generated RecvProps).
//
// Every pointer below is an ARENA TOKEN in a private host byte-vector,
// not a process pointer. No native 2006 PE32 function bytes, class creator,
// RecvProxy, classname factory, engine callback, renderer or original DLL
// is executed. DO NOT use it to answer real engine VClient011.slot5.
// The incomplete special animation overlay remains an explicit
// unresolved native type5 pointer. Fail CLOSED, never invent a child.
//
// Original 2006 source field ABI:
// ClientClass prefix [name +8, recvtable +12, next +16], 20 bytes;
// RecvTable [props +0,count +4, aux +8,name +12, visited +17], 20B;
// RecvProp (60B) [name +0,type +4,nested pointer +40,member offset +44].
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "sin_compat/original_client_class_catalog.hpp"
#include "sin_compat/original_client_aux_recvtable_metadata.hpp"
#include "sin_compat/original_client_native32_recvtable_manifest.hpp"
#include "sin_compat/original_client_nested_array_table.hpp"
#include "sin_compat/original_client_aux_dynamic_receivers.hpp"
#include "sin_compat/original_client_native_member_offsets.hpp"
#include "sin_compat/original_client_native_type4_array_fields.hpp"

namespace sin_compat::original_client_native32_recvtable_image {
namespace src=sin_compat::original_client_native32_recvtable_manifest;
namespace classes=sin_compat::original_client_class_catalog;
namespace aux=sin_compat::original_client_aux_recvtable_metadata;
namespace classArrays=sin_compat::original_client_nested_array_table;
namespace auxArrays=sin_compat::original_client_aux_dynamic_receivers;
namespace links=sin_compat::original_client_static_recvtable_links;
namespace sourceOffsets=sin_compat::original_client_native_member_offsets;
namespace nativeArrays=sin_compat::original_client_native_type4_array_fields;

inline constexpr std::uint32_t kArenaBaseVA=0x10000000u;
inline constexpr std::uint32_t kNativeRecvPropTypeOffset=4u;
inline constexpr std::uint32_t kNativeRecvPropNestedTableOffset=40u;
inline constexpr std::uint32_t kNativeRecvPropMemberOffset=44u;
inline constexpr std::uint32_t kNativeClassNameOffset=8u;
inline constexpr std::uint32_t kNativeClassRecvTableOffset=12u;
inline constexpr std::uint32_t kNativeClassNextOffset=16u;
inline constexpr std::uint32_t kNativeRecvTablePropsOffset=0u;
inline constexpr std::uint32_t kNativeRecvTablePropCountOffset=4u;
inline constexpr std::uint32_t kNativeRecvTableVisitedOffset=17u;

// Source 32-bit process addressing is encoded into this model, NOT
// host pointers: original offsets and fake arena tokens are distinct.
class Arena final {
public:
    std::uint32_t Allocate(std::size_t n) {
        if(n==0u)return 0u;
        const auto start=(data_.size()+3u)&~std::size_t(3u);
        if(n>0x0FFFFFFFu || start>0x0FFFFFFFu-n)
            throw std::overflow_error("32-bit sandbox arena exhausted");
        data_.resize(start+n,0u);
        return static_cast<std::uint32_t>(kArenaBaseVA+start);
    }
    std::uint32_t StoreString(const std::string& value) {
        if(value.empty()||value.size()>127u)
            throw std::invalid_argument("untrusted native string length");
        const auto result=Allocate(value.size()+1u);
        const auto at=Offset(result,value.size()+1u);
        for(std::size_t i=0u;i<value.size();++i)
            data_[at+i]=static_cast<std::uint8_t>(value[i]);
        data_[at+value.size()]=0u;
        return result;
    }
    void Write8(std::uint32_t addr,std::uint8_t value) {
        data_[Offset(addr,1u)]=value;
    }
    std::uint8_t Read8(std::uint32_t addr) const {
        return data_[Offset(addr,1u)];
    }
    void Write32(std::uint32_t addr,std::uint32_t value) {
        const auto at=Offset(addr,4u);
        for(unsigned i=0u;i<4u;++i)
            data_[at+i]=static_cast<std::uint8_t>(value>>(i*8u));
    }
    std::uint32_t Read32(std::uint32_t addr) const {
        const auto at=Offset(addr,4u);
        std::uint32_t result=0;
        for(unsigned i=0;i<4u;++i)
            result|=std::uint32_t(data_[at+i])<<(i*8u);
        return result;
    }
    std::string ReadString(std::uint32_t addr) const {
        std::string s;
        for(std::size_t i=0u;i<128u;++i) {
            const auto v=Read8(addr+static_cast<std::uint32_t>(i));
            if(v==0u)return s;
            s.push_back(static_cast<char>(v));
        }
        throw std::runtime_error("native string missing terminator");
    }
    std::size_t Size() const noexcept {return data_.size();}
private:
    std::size_t Offset(std::uint32_t addr,std::size_t n) const {
        if(addr<kArenaBaseVA)
            throw std::out_of_range("not an owned native32 arena token");
        const auto at=static_cast<std::size_t>(addr-kArenaBaseVA);
        if(at>data_.size() || n>data_.size()-at)
            throw std::out_of_range("native32 memory token outside arena");
        return at;
    }
    std::vector<std::uint8_t> data_;
};

struct IndexedRegistrationNode final {
    // Host token standing in for original native 8-byte record +0.
    std::uint32_t tableToken=0u;
    // Original 2006 PE32 0x20086930 stores source prev index at +4
    // and source next index at +6, with FFFF as the empty sentinel.
    std::uint16_t previous=0xFFFFu;
    std::uint16_t next=0xFFFFu;
};
static_assert(sizeof(IndexedRegistrationNode)==8u);
struct WalkReport final {
    std::size_t visitedTables=0u;
    // Cold-entry SEQUENTIAL HOST model of original engine global indexed
    // double-linked list at 0x202FDC48. The genuine heap allocator and
    // original free-list reuse have NOT been reconstructed.
    std::vector<IndexedRegistrationNode> registrationNodes;
    std::uint16_t registrationHead=0xFFFFu;
    std::uint16_t registrationTail=0xFFFFu;
    std::size_t visitedProperties=0u;
    std::size_t traversedNativeType5=0u;
    std::size_t blockedUnimplementedNativeType5=0u;
    std::size_t duplicateSubtableReferences=0u;
    bool CompleteRetailRecursion() const noexcept {
        // A proven host-only traversal is NEVER sufficient for real
        // engine networking/RecvProxy, even if all pointers resolve.
        return false;
    }
};

// Safe arena image of original STATIC + generic-DYNAMIC schema.
struct Image final {
    Arena arena;
    std::array<std::uint32_t,src::kStaticTables> staticTableTokens{};
    std::array<std::uint32_t,src::kStaticTables> staticPropArrayTokens{};
    std::array<std::uint32_t,src::kGeneratedGenericSubtables> dynamicTableTokens{};
    std::array<std::uint32_t,src::kGeneratedGenericSubtables> dynamicPropArrayTokens{};
    std::array<std::uint32_t,src::kStaticClassTables> classNodeTokens{};
    std::uint32_t classHeadToken=0u;
    std::uint32_t unresolvedOverlayDescriptorToken=0u;

    // This branch is ALWAYS closed until all native Win32 creator/proxy
    // callbacks, 2006 RecvTables and lifecycle rules are implemented.
    bool CanExposeToRetailEngine() const noexcept {return false;}

    static Image Build(const classes::Catalog& catalog) {
        if(!catalog.IsLoaded()||catalog.Size()!=src::kStaticClassTables)
            throw std::invalid_argument("191 original registered classes required");
        Image image{};
        // Allocate all 211 native RecvTable headers with real original
        // source property COUNT and type DWORD layout. Non-type fields
        // remain unknown, and MUST NOT be published as live memory.
        for(std::size_t i=0u;i<src::kStaticTables;++i) {
            const auto count=src::kCounts[i];
            image.staticTableTokens[i]=image.arena.Allocate(src::kOriginalRecvTableBytes);
            image.staticPropArrayTokens[i]=image.arena.Allocate(
                std::size_t(count)*src::kOriginalRecvPropStride);
            const auto table=image.staticTableTokens[i];
            image.arena.Write32(table+kNativeRecvTablePropsOffset,
                                image.staticPropArrayTokens[i]);
            image.arena.Write32(table+kNativeRecvTablePropCountOffset,count);
            image.arena.Write32(table+8u,0u);  // original table ctor
            image.arena.Write8(table+16u,0u);
            image.arena.Write8(table+kNativeRecvTableVisitedOffset,0u);
            for(std::size_t j=0u;j<count;++j) {
                const auto type=src::kTypes[src::kTypePrefixes[i]+j];
                const auto prop=image.staticPropArrayTokens[i]+
                    static_cast<std::uint32_t>(j*src::kOriginalRecvPropStride);
                image.arena.Write32(prop+kNativeRecvPropTypeOffset,type);
                // 1082 original native member offsets come directly from
                // source constructor immediates. Special constructors stay
                // unresolved here: generated arrays are populated below.
                const auto sourceSlot=src::kTypePrefixes[i]+j;
                const auto sourceOffset=sourceOffsets::kSourceNativeMemberOffsets[sourceSlot];
                if(sourceOffset!=sourceOffsets::kNotDirectlyRecovered)
                    image.arena.Write32(prop+kNativeRecvPropMemberOffset,
                                        sourceOffset);
            }
        }
        // Source helper 0x24126BD0 writes native type4 descriptors:
        // byte +44 ZERO (NOT the element count), +48 element stride,
        // +52 element count. Preserve exact original network names.
        // One original Team callback at +28 is intentionally NOT
        // installed: its 2006 PE32 code cannot run inside this host.
        for(const auto& array:nativeArrays::kSourceArrays) {
            const auto parent=std::size_t(array.tableOrdinal);
            const auto slot=std::size_t(array.propertySlot);
            if(src::kTypes[src::kTypePrefixes[parent]+slot]!=4u)
                throw std::logic_error("original type4 array source index drift");
            const auto prop=image.staticPropArrayTokens[parent]+
                static_cast<std::uint32_t>(slot*src::kOriginalRecvPropStride);
            image.arena.Write32(prop,
                image.arena.StoreString(std::string(array.originalPropertyName)));
            image.arena.Write32(prop+kNativeRecvPropMemberOffset,0u);
            image.arena.Write32(prop+nativeArrays::kNativeArrayElementStrideOffset,
                                array.elementStride);
            image.arena.Write32(prop+nativeArrays::kNativeArrayElementCountOffset,
                                array.elementCount);
            // +28 remains NULL even where the original holds a callback.
        }
        std::array<std::uint8_t,src::kStaticPropertySlots> assigned{};
        // Link the ACTUAL 192 original static type5 table pointers,
        // including the one auxiliary DT_LocalPlayerExclusive.m_Local.
        for(const auto& edge:links::kOriginalSourceEdges) {
            const auto parent=std::size_t(edge.parentTableOrdinal);
            const auto slot=std::size_t(edge.parentPropertySlot);
            if(parent>=src::kStaticTables || slot>=src::kCounts[parent])
                throw std::logic_error("original nested source link index");
            const auto index=src::kTypePrefixes[parent]+slot;
            if(src::kTypes[index]!=5u || assigned[index]++)
                throw std::logic_error("original static type5 source duplicate");
            const auto prop=image.staticPropArrayTokens[parent]+
                static_cast<std::uint32_t>(slot*src::kOriginalRecvPropStride);
            image.arena.Write32(prop+kNativeRecvPropNestedTableOffset,
                                image.staticTableTokens[edge.childTableOrdinal]);
        }
        // Construct fourteen source-authenticated dynamic generic child
        // tables with their real count, type0/1, child-local byte +44
        // offsets and original 000..255 names. No unknown proxy copied.
        for(std::size_t i=0u;i<src::kGeneratedGenericSubtables;++i) {
            const auto& edge=src::kGeneratedOuters[i];
            if(edge.sourceShapeIndex!=i)
                throw std::logic_error("original nested source shape ordinal changed");
            const bool classSource=i<classArrays::kSourceArrayShapeCount;
            std::uint32_t count=0u,stride=0u,elementType=0u,outerBase=0u;
            if(classSource) {
                const auto& spec=classArrays::kOriginalShapes[i];
                count=spec.elements;
                stride=spec.byteStride;
                elementType=spec.elementNativeType;
                outerBase=spec.firstX86Offset;
            } else {
                const auto& spec=auxArrays::kSourceShapes[i-classArrays::kSourceArrayShapeCount];
                count=spec.elements;
                stride=spec.byteStride;
                elementType=spec.originalElementType;
                outerBase=spec.originalOuterParentX86Offset;
            }
            if(count==0u || count>256u || (stride!=1u&&stride!=4u) ||
               elementType>1u)
                throw std::logic_error("native dynamic source shape unsupported");
            image.dynamicTableTokens[i]=image.arena.Allocate(src::kOriginalRecvTableBytes);
            image.dynamicPropArrayTokens[i]=image.arena.Allocate(
                count*src::kOriginalRecvPropStride);
            const auto table=image.dynamicTableTokens[i];
            image.arena.Write32(table+kNativeRecvTablePropsOffset,
                                image.dynamicPropArrayTokens[i]);
            image.arena.Write32(table+kNativeRecvTablePropCountOffset,count);
            image.arena.Write32(table+8u,0u);
            image.arena.Write8(table+16u,0u);
            image.arena.Write8(table+kNativeRecvTableVisitedOffset,0u);
            for(std::uint32_t j=0u;j<count;++j) {
                const auto p=image.dynamicPropArrayTokens[i]+
                    j*src::kOriginalRecvPropStride;
                image.arena.Write32(p+kNativeRecvPropTypeOffset,elementType);
                image.arena.Write32(p+kNativeRecvPropMemberOffset,j*stride);
                const std::string originalIndexName={
                    static_cast<char>('0'+j/100u),
                    static_cast<char>('0'+((j/10u)%10u)),
                    static_cast<char>('0'+(j%10u))
                };
                image.arena.Write32(p,image.arena.StoreString(originalIndexName));
            }
            const auto parent=std::size_t(edge.parentTableOrdinal);
            const auto slot=std::size_t(edge.parentSlot);
            const auto index=src::kTypePrefixes[parent]+slot;
            if(src::kTypes[index]!=5u || assigned[index]++)
                throw std::logic_error("native dynamic source index conflict");
            const auto parentProp=image.staticPropArrayTokens[parent]+
                static_cast<std::uint32_t>(slot*src::kOriginalRecvPropStride);
            // The OUTER source native +44 contains the original parent
            // object base; the generated child descriptor +44 starts at
            // ZERO and increments by the element byte stride. The two
            // offsets must never be conflated.
            image.arena.Write32(parentProp+kNativeRecvPropMemberOffset,
                                outerBase);
            image.arena.Write32(parentProp+kNativeRecvPropNestedTableOffset,table);
        }
        // The 621-byte original specialized OVERLAY builder has unknown
        // full callback side effects. Keep this ONLY remaining native
        // type5 nested pointer NULL and block complete engine traversal.
        constexpr std::size_t overlayTable=197u;
        constexpr std::size_t overlaySlot=0u;
        const auto overlayIndex=src::kTypePrefixes[overlayTable]+overlaySlot;
        if(src::kTypes[overlayIndex]!=5u || assigned[overlayIndex]++)
            throw std::logic_error("unrecognized original overlay slot");
        image.unresolvedOverlayDescriptorToken=
            image.staticPropArrayTokens[overlayTable];
        if(image.arena.Read32(image.unresolvedOverlayDescriptorToken+
                              kNativeRecvPropNestedTableOffset)!=0u)
            throw std::logic_error("unimplemented overlay must remain null");
        for(std::size_t i=0u;i<src::kStaticPropertySlots;++i) {
            if((src::kTypes[i]==5u)!=(assigned[i]!=0u))
                throw std::logic_error("original native type5 route uncovered");
        }

        // Recreate only the first 20 bytes of each original ClientClass,
        // with real source class names and linked-list order. Original
        // executable entity/temp-event creator functions intentionally
        // remain NULL, NOT simulated or exposed to the game engine.
        std::size_t i=0u;
        for(auto* c=catalog.Head();c;c=c->next,++i) {
            if(i>=src::kStaticClassTables)
                throw std::logic_error("original ClientClass list too long");
            image.classNodeTokens[i]=image.arena.Allocate(
                src::kOriginalClientClassPrefixBytes);
            const auto node=image.classNodeTokens[i];
            image.arena.Write32(node+0u,0u); // creator not reconstructed
            image.arena.Write32(node+4u,0u); // temp-event creator not reconstructed
            image.arena.Write32(node+kNativeClassNameOffset,
                                image.arena.StoreString(c->networkName));
            image.arena.Write32(node+kNativeClassRecvTableOffset,
                                image.staticTableTokens[i]);
        }
        if(i!=src::kStaticClassTables)
            throw std::logic_error("original ClientClass list too short");
        for(std::size_t j=0u;j<image.classNodeTokens.size();++j)
            image.arena.Write32(image.classNodeTokens[j]+kNativeClassNextOffset,
                j+1u<image.classNodeTokens.size()?image.classNodeTokens[j+1u]:0u);
        image.classHeadToken=image.classNodeTokens[0];
        return image;
    }

    // Real original engine start (0x20023BB0): obtain VClient011.slot5,
    // enumerate actual head-to-tail ClientClass::next +16 and extract
    // table from ClientClass +12. Do not treat all 211 source static
    // tables as class roots: exactly ONE original auxiliary table
    // (DT_Animationlayer, static ordinal 196) is NOT reachable by the
    // authenticated 191 source class heads and native type5 links.
    WalkReport WalkFromClientClassRoots() {
        std::vector<std::uint32_t> roots;
        roots.reserve(classNodeTokens.size());
        auto node=classHeadToken;
        for(std::size_t i=0u;i<classNodeTokens.size();++i) {
            if(node!=classNodeTokens[i])
                throw std::logic_error("original ClientClass head/next chain drift");
            const auto table=arena.Read32(node+kNativeClassRecvTableOffset);
            if(table!=staticTableTokens[i])
                throw std::logic_error("original ClientClass source table pointer drift");
            roots.push_back(table);
            node=arena.Read32(node+kNativeClassNextOffset);
        }
        if(node!=0u || roots.size()!=src::kStaticClassTables)
            throw std::logic_error("original 191 ClientClass root chain malformed");
        return WalkRoots(roots);
    }

    // Exhaustive INVENTORY preflight, not the retail engine startup
    // traversal. This visits all 211 static tables regardless of
    // whether original VClient011 head->child links reach them.
    WalkReport WalkAllStaticTables() {
        return WalkRoots(std::vector<std::uint32_t>(
            staticTableTokens.begin(),staticTableTokens.end()));
    }

private:
    // Replays ONLY original 0x20085630's authenticated +17 visited mark,
    // count, 60-byte RecvProp stride and type5 child+40 recursion.
    // The original helper 0x20086930 records each table in an engine
    // global linked list, but its allocator/free-list and callbacks
    // remain out of scope; NO 2006 engine helper or proxy is invoked.
    // Phase1701: verify each visited table against the authenticated
    // immutable SOURCE mapping before accepting an otherwise in-bounds
    // pointer. A wrong-but-valid native type, count, child table token or
    // generated child offset must fail, not pass a mere memory-safety scan.
    // Called only for reached tables: the genuine 191-root startup must
    // continue to ignore the one unreachable DT_Animationlayer table.
    void AuditVisitedSourceTable(std::uint32_t table) const {
        for(std::size_t i=0u;i<src::kStaticTables;++i) {
            if(table!=staticTableTokens[i])continue;
            const auto count=src::kCounts[i];
            const auto props=staticPropArrayTokens[i];
            if(arena.Read32(table+kNativeRecvTablePropsOffset)!=props ||
               arena.Read32(table+kNativeRecvTablePropCountOffset)!=count)
                throw std::logic_error("static RecvTable source header drift");
            for(std::uint32_t j=0u;j<count;++j) {
                const auto p=props+j*src::kOriginalRecvPropStride;
                const auto originalType=src::kTypes[src::kTypePrefixes[i]+j];
                if(arena.Read32(p+kNativeRecvPropTypeOffset)!=originalType)
                    throw std::logic_error("source RecvProp native type drift");
                const auto knownOffset=sourceOffsets::kSourceNativeMemberOffsets[
                    src::kTypePrefixes[i]+j];
                if(knownOffset!=sourceOffsets::kNotDirectlyRecovered &&
                   arena.Read32(p+kNativeRecvPropMemberOffset)!=knownOffset)
                    throw std::logic_error("original PE32 member offset drift");
                if(originalType==4u) {
                    unsigned matches=0u;
                    for(const auto& array:nativeArrays::kSourceArrays) {
                        if(array.tableOrdinal!=i || array.propertySlot!=j)
                            continue;
                        ++matches;
                        if(arena.Read32(p+kNativeRecvPropMemberOffset)!=0u ||
                           arena.Read32(p+nativeArrays::kNativeArrayElementStrideOffset)
                               !=array.elementStride ||
                           arena.Read32(p+nativeArrays::kNativeArrayElementCountOffset)
                               !=array.elementCount ||
                           arena.ReadString(arena.Read32(p))!=
                               std::string(array.originalPropertyName) ||
                           arena.Read32(p+nativeArrays::kNativeArrayCallbackOffset)!=0u)
                            throw std::logic_error("original type4 array shape drift");
                    }
                    if(matches!=1u)
                        throw std::logic_error("unclassified native type4 array");
                }
                if(originalType!=5u)continue;
                std::uint32_t expectedChild=0u;
                unsigned routes=0u;
                for(const auto& edge:links::kOriginalSourceEdges)
                    if(edge.parentTableOrdinal==i &&
                       edge.parentPropertySlot==j) {
                        expectedChild=staticTableTokens[edge.childTableOrdinal];
                        ++routes;
                    }
                for(std::size_t k=0u;k<src::kGeneratedGenericSubtables;++k) {
                    const auto& outer=src::kGeneratedOuters[k];
                    if(outer.parentTableOrdinal==i && outer.parentSlot==j) {
                        expectedChild=dynamicTableTokens[k];
                        ++routes;
                    }
                }
                // The specialized animation-overlay child is a KNOWN
                // unresolved path, not permission to insert a fake table.
                if(i==197u && j==0u)++routes;
                if(routes!=1u ||
                   arena.Read32(p+kNativeRecvPropNestedTableOffset)!=
                       expectedChild)
                    throw std::logic_error("original nested source edge drift");
            }
            return;
        }
        for(std::size_t k=0u;k<src::kGeneratedGenericSubtables;++k) {
            if(table!=dynamicTableTokens[k])continue;
            const bool classSource=k<classArrays::kSourceArrayShapeCount;
            const auto count=classSource?
                classArrays::kOriginalShapes[k].elements:
                auxArrays::kSourceShapes[
                    k-classArrays::kSourceArrayShapeCount].elements;
            const auto stride=classSource?
                classArrays::kOriginalShapes[k].byteStride:
                auxArrays::kSourceShapes[
                    k-classArrays::kSourceArrayShapeCount].byteStride;
            const auto nativeType=classSource?
                classArrays::kOriginalShapes[k].elementNativeType:
                auxArrays::kSourceShapes[
                    k-classArrays::kSourceArrayShapeCount].originalElementType;
            const auto props=dynamicPropArrayTokens[k];
            if(arena.Read32(table+kNativeRecvTablePropsOffset)!=props ||
               arena.Read32(table+kNativeRecvTablePropCountOffset)!=count)
                throw std::logic_error("generated RecvTable source header drift");
            for(std::uint32_t j=0u;j<count;++j) {
                const auto p=props+j*src::kOriginalRecvPropStride;
                const std::string sourceIndexName={
                    static_cast<char>('0'+j/100u),
                    static_cast<char>('0'+(j/10u)%10u),
                    static_cast<char>('0'+j%10u)
                };
                if(arena.Read32(p+kNativeRecvPropTypeOffset)!=nativeType ||
                   arena.Read32(p+kNativeRecvPropMemberOffset)!=j*stride ||
                   arena.ReadString(arena.Read32(p))!=sourceIndexName)
                    throw std::logic_error("source generated child descriptor drift");
            }
            return;
        }
        throw std::out_of_range("table token has no authenticated source identity");
    }

    WalkReport WalkRoots(const std::vector<std::uint32_t>& roots) {
        if(roots.empty() || roots.size()>=1024u)
            throw std::out_of_range("original 2006 client root bounds");
        for(const auto table:staticTableTokens)
            arena.Write8(table+kNativeRecvTableVisitedOffset,0u);
        for(const auto table:dynamicTableTokens)
            arena.Write8(table+kNativeRecvTableVisitedOffset,0u);
        std::unordered_set<std::uint32_t> allowed;
        for(const auto t:staticTableTokens)allowed.insert(t);
        for(const auto t:dynamicTableTokens)allowed.insert(t);
        WalkReport out{};
        std::function<void(std::uint32_t)> visit=[&](std::uint32_t table) {
            if(!allowed.count(table))
                throw std::out_of_range("native 32-bit table token not owned");
            AuditVisitedSourceTable(table);
            if(arena.Read8(table+kNativeRecvTableVisitedOffset)!=0u) {
                ++out.duplicateSubtableReferences;return;
            }
            arena.Write8(table+kNativeRecvTableVisitedOffset,1u);
            ++out.visitedTables;
            // Original 0x20085630 now calls 0x20086930 immediately
            // AFTER setting the visited byte, BEFORE processing props.
            // That helper appends this table pointer into an indexed
            // 8-byte doubly linked list with prev/next 0xFFFF sentinels.
            // Simulate only its cold sequential append effect, NEVER
            // call its original 2006 allocator/free-index logic.
            const auto nextIndex=out.registrationNodes.size();
            if(nextIndex>=0xFFFFu)
                throw std::out_of_range("source engine 16-bit list index exhausted");
            const auto assigned=static_cast<std::uint16_t>(nextIndex);
            if(out.registrationTail!=0xFFFFu)
                out.registrationNodes[out.registrationTail].next=assigned;
            else
                out.registrationHead=assigned;
            out.registrationNodes.push_back(
                {table,out.registrationTail,0xFFFFu});
            out.registrationTail=assigned;
            const auto props=arena.Read32(table+kNativeRecvTablePropsOffset);
            const auto count=arena.Read32(table+kNativeRecvTablePropCountOffset);
            if(count>256u || (count!=0u&&props==0u))
                throw std::out_of_range("invalid native 2006 table descriptor count");
            for(std::uint32_t j=0u;j<count;++j) {
                const auto ptr=props+j*src::kOriginalRecvPropStride;
                const auto type=arena.Read32(ptr+kNativeRecvPropTypeOffset);
                if(type>5u)
                    throw std::out_of_range("unproven native RecvProp type");
                ++out.visitedProperties;
                if(type==5u) {
                    ++out.traversedNativeType5;
                    const auto child=arena.Read32(
                        ptr+kNativeRecvPropNestedTableOffset);
                    if(child==0u) {
                        ++out.blockedUnimplementedNativeType5;continue;
                    }
                    visit(child);
                }
            }
        };
        for(const auto table:roots)visit(table);
        return out;
    }
};
} // namespace sin_compat::original_client_native32_recvtable_image
