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

namespace sin_compat::original_client_native32_recvtable_image {
namespace src=sin_compat::original_client_native32_recvtable_manifest;
namespace classes=sin_compat::original_client_class_catalog;
namespace aux=sin_compat::original_client_aux_recvtable_metadata;
namespace classArrays=sin_compat::original_client_nested_array_table;
namespace auxArrays=sin_compat::original_client_aux_dynamic_receivers;
namespace links=sin_compat::original_client_static_recvtable_links;

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

struct WalkReport final {
    std::size_t visitedTables=0u;
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
            }
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

    // Emulates ONLY original 0x20085630's verified +17 visited mark,
    // type5 child+40 recursion and 60-byte stride/count access.
    // It does NOT run the unknown 0x20086930 helper or any proxy.
    WalkReport WalkAllStaticTables() {
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
            if(arena.Read8(table+kNativeRecvTableVisitedOffset)!=0u) {
                ++out.duplicateSubtableReferences;return;
            }
            arena.Write8(table+kNativeRecvTableVisitedOffset,1u);
            ++out.visitedTables;
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
        for(const auto table:staticTableTokens)visit(table);
        return out;
    }
};
} // namespace sin_compat::original_client_native32_recvtable_image
