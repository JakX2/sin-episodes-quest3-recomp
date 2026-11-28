#pragma once
// Phase1696: metadata-only ordered 2006 ClientClass catalog.
// Original VClient011.slot5 returns a full linked registry whose node+16
// is the predecessor. CRT constructor order is reversed by prepend.
// Original PE32 virtual addresses are NOT usable host pointers.
// This model is deliberately NOT a Source Engine ClientClass* or RecvTable*.
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sin_compat::original_client_class_catalog {
inline constexpr std::size_t kOriginalCRTInitializers=1582;
inline constexpr std::size_t kOriginalRegisteredClasses=191;
inline constexpr std::size_t kOriginalEntityCreators=149;
inline constexpr std::size_t kOriginalTempEventCreators=42;
inline constexpr std::uint32_t kOriginalListHeadVA=0x2433fd90u;
inline constexpr std::uint32_t kOriginalNextNodeOffset=16u;

struct Entry {
    std::string networkName;
    std::uint32_t sourceCRTIndex=0;
    bool originalTempEvent=false;
    std::uint32_t originalNodeVA=0;
    std::uint32_t originalRecvTableVA=0;
    const Entry* next=nullptr;
};

// Source metadata alone has no executable creation factories, native
// RecvTables or engine-assigned class IDs. Loading does NOT authorize
// a VClient011.slot5 retail pointer.
class Catalog final {
public:
    Catalog()=default;
    Catalog(const Catalog&)=delete;
    Catalog& operator=(const Catalog&)=delete;
    Catalog(Catalog&&)=delete;
    Catalog& operator=(Catalog&&)=delete;

    bool Load(std::istream& input) {
        if(loaded_)return false;
        std::vector<Entry> candidate;
        candidate.reserve(kOriginalRegisteredClasses);
        std::unordered_set<std::string> names;
        std::unordered_set<std::uint32_t> nodes;
        std::unordered_set<std::uint32_t> tables;
        std::string line;
        std::size_t tempCount=0;
        while(std::getline(input,line)) {
            if(!line.empty() && line.back()=='\r')line.pop_back();
            if(line.empty() || line.front()=='#')continue;
            if(candidate.size()>=kOriginalRegisteredClasses)return false;
            std::string_view cells[5];
            if(!Split(line,cells))return false;
            const auto name=cells[0];
            if(name.empty() || name.size()>99)return false;
            for(const char c:name) {
                const bool valid=(c>='a'&&c<='z')||
                    (c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';
                if(!valid)return false;
            }
            std::uint32_t crt=0,node=0,table=0;
            if(!ParseUnsigned(cells[1],10,crt) ||
               !ParseUnsigned(cells[3],16,node) ||
               !ParseUnsigned(cells[4],16,table) ||
               crt>=kOriginalCRTInitializers ||
               !SourceDataVA(node) || !SourceDataVA(table))return false;
            if(!candidate.empty() && candidate.back().sourceCRTIndex<=crt)
                return false;
            if(cells[2]!="E" && cells[2]!="T")return false;
            const bool temp=cells[2]=="T";
            if(temp)++tempCount;
            const auto n=std::string(name);
            if(!names.emplace(n).second || !nodes.emplace(node).second ||
               !tables.emplace(table).second)return false;
            candidate.push_back({n,crt,temp,node,table,nullptr});
        }
        if(input.bad() || candidate.size()!=kOriginalRegisteredClasses ||
           tempCount!=kOriginalTempEventCreators ||
           candidate.front().networkName!="CWeaponMelee" ||
           candidate.back().networkName!="CBaseTempEntity")return false;
        entries_.swap(candidate);
        for(std::size_t i=0;i<entries_.size();++i)
            entries_[i].next=(i+1<entries_.size())?&entries_[i+1]:nullptr;
        loaded_=true;
        return true;
    }

    const Entry* Head() const noexcept {
        return loaded_ && !entries_.empty()?&entries_[0]:nullptr;
    }
    const Entry* Find(std::string_view name) const noexcept {
        if(!loaded_)return nullptr;
        for(const auto& item:entries_)
            if(item.networkName==name)return &item;
        return nullptr;
    }
    std::size_t Size() const noexcept {return entries_.size();}
    std::size_t TempEventCount() const noexcept {
        if(!loaded_)return 0;
        std::size_t n=0;
        for(const auto& item:entries_)if(item.originalTempEvent)++n;
        return n;
    }
    bool IsLoaded() const noexcept {return loaded_;}
    // STRICT refusal until live factory callbacks and all 191 genuine
    // receive-table descriptors, proxies and class IDs are reconstructed.
    bool CanExposeToRetailEngine() const noexcept {return false;}

private:
    static bool SourceDataVA(std::uint32_t value) noexcept {
        return value>=0x242f3000u && value<0x2438f320u;
    }
    static bool ParseUnsigned(std::string_view s,int base,
                              std::uint32_t& result) noexcept {
        if(s.empty())return false;
        const char* const begin=s.data();
        const auto parsed=std::from_chars(begin,begin+s.size(),result,base);
        return parsed.ec==std::errc{} && parsed.ptr==begin+s.size();
    }
    static bool Split(const std::string& line,
                      std::string_view (&out)[5]) noexcept {
        std::size_t start=0;
        for(unsigned i=0;i<5;++i) {
            const auto end=line.find('|',start);
            if(i==4 && end!=std::string::npos)return false;
            if(i!=4 && end==std::string::npos)return false;
            out[i]=std::string_view(line.data()+start,
                                    (end==std::string::npos?line.size():end)-start);
            start=end+1;
        }
        return true;
    }
    std::vector<Entry> entries_;
    bool loaded_=false;
};
} // namespace sin_compat::original_client_class_catalog
