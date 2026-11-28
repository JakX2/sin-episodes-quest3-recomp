#pragma once
// Phase1697: bounded SOURCE-CONTRACT preflight for 2006 SiN RecvTables.
//
// Original engine 0x20023BB0 gathers ClientClass::recvtable at +0x0C,
// walks ClientClass::next +0x10, and refuses after its 1024-entry count.
// Engine 0x20085630 marks RecvTable byte +0x11 before walking 60-byte
// RecvProps; type DWORD +4 == 5 means nested table at prop +0x28.
// This host model copies the ordering and cycle-termination semantics
// using SAFE STABLE INDICES, not executable original pointers.
// It deliberately CANNOT expose a retail RecvTable or ClientClass ABI.
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace sin_compat::original_client_recvtable_preflight {

inline constexpr std::size_t kClassRootLimit=1024u;
inline constexpr std::size_t kOriginalClassRoots=191u;
inline constexpr std::size_t kOriginalAttachedPropSlots=939u;
inline constexpr std::size_t kOriginalRecvPropStride=60u;
inline constexpr std::size_t kOriginalClassRecvTableOffset=12u;
inline constexpr std::size_t kOriginalClassNextOffset=16u;
inline constexpr std::size_t kOriginalRecvTableVisitedByteOffset=17u;
inline constexpr std::size_t kOriginalRecvPropTypeOffset=4u;
inline constexpr std::size_t kOriginalNestedRecvTableOffset=40u;
inline constexpr int kUnresolvedSourceType=-1;
inline constexpr int kNativeNestedTableType=5;
inline constexpr std::size_t kNoTable=std::numeric_limits<std::size_t>::max();

struct Property final {
    std::string name;
    int nativeSourceType=kUnresolvedSourceType;
    std::size_t nestedTable=kNoTable; // only valid for original native type 5
};

struct Table final {
    std::string name;
    std::vector<Property> props;
};

// Designed to express exact source facts and refuse incomplete metadata.
// Unlike the engine's in-place visited flag at table+17, verification does
// not mutate user data; temporary tri-color state is maintained separately.
class Graph final {
public:
    enum class Problem {
        None, EmptyRoots, TooManyRoots, InvalidRoot,
        InvalidTable, UnknownType, InvalidType, MissingNestedTable,
        UnexpectedNestedTable, ExcessiveTraversal
    };
    struct Report final {
        Problem problem=Problem::None;
        std::size_t visitedTables=0;
        std::size_t seenProperties=0;
        std::size_t nestedEdges=0;
        std::size_t revisitEdges=0;
        bool good() const noexcept {return problem==Problem::None;}
    };

    std::size_t AddTable(std::string name) {
        const auto index=tables_.size();
        tables_.push_back(Table{std::move(name),{}});
        return index;
    }

    bool AddProperty(std::size_t table,std::string name,int nativeType) {
        if(table>=tables_.size() || name.empty())return false;
        // Allow unknown in the intermediate graph, never in Validate.
        if(nativeType<kUnresolvedSourceType || nativeType>kNativeNestedTableType)
            return false;
        tables_[table].props.push_back(
            Property{std::move(name),nativeType,kNoTable});
        return true;
    }

    bool LinkNested(std::size_t table,std::size_t prop,std::size_t child) {
        if(table>=tables_.size() || child>=tables_.size())return false;
        if(prop>=tables_[table].props.size())return false;
        auto& p=tables_[table].props[prop];
        if(p.nativeSourceType!=kNativeNestedTableType ||
           p.nestedTable!=kNoTable)return false;
        p.nestedTable=child;
        return true;
    }

    const Table* FindTable(std::size_t index) const noexcept {
        return index<tables_.size()?&tables_[index]:nullptr;
    }
    std::size_t TableCount() const noexcept {return tables_.size();}

    Report Validate(const std::vector<std::size_t>& roots) const {
        Report r{};
        if(roots.empty()) {
            r.problem=Problem::EmptyRoots;return r;
        }
        // Native engine uses a 4096-byte stack staging area and
        // compares against 1024. The possible diagnostic callback's
        // return is unknown: a safe host MUST reject >= 1024.
        if(roots.size()>=kClassRootLimit) {
            r.problem=Problem::TooManyRoots;return r;
        }
        std::vector<std::uint8_t> state(tables_.size(),0u);
        // Each (index, next property) frame is a single recursion level.
        std::vector<std::pair<std::size_t,std::size_t>> stack;
        for(const auto root:roots) {
            if(root>=tables_.size()) {
                r.problem=Problem::InvalidRoot;return r;
            }
            if(state[root]!=0u) {
                ++r.revisitEdges;continue;
            }
            state[root]=1u; ++r.visitedTables;
            stack.emplace_back(root,0u);
            while(!stack.empty()) {
                if(stack.size()>kClassRootLimit ||
                   r.visitedTables>kClassRootLimit) {
                    r.problem=Problem::ExcessiveTraversal;return r;
                }
                auto& frame=stack.back();
                const auto& table=tables_[frame.first];
                if(table.name.empty()) {
                    r.problem=Problem::InvalidTable;return r;
                }
                if(frame.second==table.props.size()) {
                    state[frame.first]=2u;stack.pop_back();continue;
                }
                const auto& p=table.props[frame.second++];
                ++r.seenProperties;
                if(p.name.empty()) {
                    r.problem=Problem::InvalidTable;return r;
                }
                if(p.nativeSourceType==kUnresolvedSourceType) {
                    r.problem=Problem::UnknownType;return r;
                }
                if(p.nativeSourceType<0 ||
                   p.nativeSourceType>kNativeNestedTableType) {
                    r.problem=Problem::InvalidType;return r;
                }
                if(p.nativeSourceType==kNativeNestedTableType) {
                    ++r.nestedEdges;
                    if(p.nestedTable==kNoTable ||
                       p.nestedTable>=tables_.size()) {
                        r.problem=Problem::MissingNestedTable;return r;
                    }
                    if(state[p.nestedTable]!=0u) {
                        ++r.revisitEdges;continue;
                    }
                    state[p.nestedTable]=1u;
                    ++r.visitedTables;
                    stack.emplace_back(p.nestedTable,0u);
                } else if(p.nestedTable!=kNoTable) {
                    r.problem=Problem::UnexpectedNestedTable;return r;
                }
            }
        }
        return r;
    }

    // Even a green source-graph preflight is NOT a complete native
    // mutable RecvTable, RecvProxy, class creator or wire-format ABI.
    bool CanExposeToRetailEngine() const noexcept {return false;}
private:
    std::vector<Table> tables_;
};

} // namespace sin_compat::original_client_recvtable_preflight
