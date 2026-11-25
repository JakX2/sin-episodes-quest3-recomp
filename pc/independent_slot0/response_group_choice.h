#pragma once
#include <cstdint>
#include <cstring>
namespace sin_port::response_rule {
inline constexpr unsigned group_stride=0x58,group_bytes=0x44,response_row_bytes=0x14;
struct GroupChoice {
    void* context=nullptr;
    bool (*filter)(void*,unsigned type,std::uint32_t value,unsigned* accepted) noexcept=nullptr;
    bool (*random)(void*,float low,float high,float*) noexcept=nullptr;
};
template<class T> inline T group_get(const unsigned char* p,unsigned off) noexcept {
    T v{};std::memcpy(&v,p+off,sizeof(v));return v;
}
template<class T> inline void group_put(unsigned char* p,unsigned off,T v) noexcept {
    std::memcpy(p+off,&v,sizeof(v));
}
// 22099730 and the sequential branches of 2209afe0/2209ae50.
// Caller resolves +0 vector pointer and owns mutable group/entry storage.
inline bool choose_group(unsigned char* g,unsigned bytes,unsigned char* rows,unsigned row_bytes,
    const GroupChoice& h,int* rejected,unsigned capacity,int& selected) noexcept {
    if(!g||bytes<group_bytes)return false;
    const int count=group_get<int>(g,0xc);
    if(count<0||count>4096||(count&&(!rows||row_bytes<unsigned(count)*response_row_bytes)))return false;
    if(!count||!g[0x3c]){selected=-1;return true;}
    auto entry=[&](int i){return rows+unsigned(i)*response_row_bytes;};
    auto accept=[&](int i,unsigned& yes){
        yes=1;const auto* r=entry(i);
        return !h.filter||h.filter(h.context,group_get<unsigned>(r,0),group_get<std::uint32_t>(r,4),&yes);};
    if(g[0x3a]){
        const int initial=group_get<int>(g,0x40);
        if(initial<0||initial>count)return false;
        int cursor=initial;
        for(int attempts=0;attempts<=count;++attempts){
            int i=cursor++;
            group_put(g,0x40,cursor);
            if(i>=count){
                if(g[0x3b]){g[0x3c]=0;selected=-1;return true;}
                i=cursor=0;group_put(g,0x40,cursor);
            }
            unsigned yes=0;if(!accept(i,yes))return false;
            if(yes){selected=i;return true;}
            if(cursor==initial){selected=-1;return true;}
        }
        // Bound malformed/reentrant filter state rather than loop forever.
        selected=-1;return true;
    }
    if(h.filter&&g[0x30]&&(!rejected||capacity<unsigned(count)*2))return false;
    unsigned reject_count=0;
    auto generation=group_get<std::uint32_t>(g,0x34);
    auto used=[&](int i){return group_get<std::uint32_t>(entry(i),0xc)==generation;};
    auto prefilter=[&](bool skip_used){
        if(!h.filter||!g[0x30])return true;
        for(int i=0;i<count;++i){
            if(skip_used&&used(i))continue;
            unsigned yes=0;if(!accept(i,yes))return false;
            if(!yes){rejected[reject_count++]=i;group_put(entry(i),0xc,generation);}
        }
        return true;};
    auto exhausted=[&](){for(int i=0;i<count;++i)if(!used(i))return false;return true;};
    if(!prefilter(true))return false;
    if(g[0x30]&&exhausted()){
        ++generation;group_put(g,0x34,generation);
        if(!prefilter(false))return false;
        // Original early returns retain filter-rejection stamps.
        if(g[0x30]&&exhausted()){selected=-1;return true;}
        if(g[0x3b]){g[0x3c]=0;selected=-1;return true;}
    }
    const bool deplete=g[0x30]!=0;
    int pick=-1;
    if(deplete){
        int last=-1;bool ordinary=false;
        for(int i=0;i<count;++i){
            if(used(i))continue;
            if(entry(i)[0x10]){pick=i;break;}
            if(entry(i)[0x11]){if(last<0)last=i;}else ordinary=true;
        }
        if(pick<0&&!ordinary)pick=last;
    }
    if(pick<0){
        float total=0;
        for(int i=0;i<count;++i){
            auto* row=entry(i);
            if(deplete&&(used(i)||row[0x11]))continue;
            const int previous=pick;
            if(total==0.f)pick=i;
            const float weight=group_get<float>(row,8);
            total=static_cast<float>(double(total)+weight);
            if(total==0.f)pick=i;
            else{
                float draw=0;if(!h.random||!h.random(h.context,0.f,total,&draw))return false;
                if(draw<weight)pick=i;
            }
            if(!deplete&&pick!=previous&&h.filter){
                unsigned yes=0;if(!accept(i,yes))return false;
                if(!yes){pick=previous;total=static_cast<float>(double(total)-weight);}
            }
        }
    }
    if(deplete&&pick>=0)group_put(entry(pick),0xc,generation);
    if(h.filter&&g[0x30])
        for(unsigned i=0;i<reject_count;++i)group_put(entry(rejected[i]),0xc,0u);
    selected=pick;return true;
}
// 220996c0 resets each response group; no change to other flags/weights.
inline bool reset_group(unsigned char* g,unsigned bytes,unsigned char* rows,unsigned row_bytes) noexcept {
    if(!g||bytes<group_bytes)return false;
    const int count=group_get<int>(g,0xc);
    if(count<0||count>4096||(count&&(!rows||row_bytes<unsigned(count)*response_row_bytes)))return false;
    g[0x3c]=1;group_put(g,0x34,1u);group_put(g,0x40,0u);
    for(int i=0;i<count;++i)group_put(rows+unsigned(i)*response_row_bytes,0xc,0u);
    return true;
}
}
