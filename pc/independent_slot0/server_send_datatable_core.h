#pragma once
// Independently owned PE32 SendTable and data-table SendProp construction.
// Original layout: SendTable::Construct RVA 0x25E850, SendPropDataTable
// RVA 0x25EDD0, SendProp copy helper RVA 0x25E6D0. SDK 2004 dt_send.h
// corroborates the field roles. Only data-table properties are supported;
// no claim of complete scalar/array encoding or engine-ready schema.
#include <cstdint>
#include <cstddef>
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Original Win32 x86 layout only
#endif
namespace sin_independent {
namespace send_schema {
struct SendTable;
class SendProp;
struct Recipients { std::uint32_t words[8]{}; };
using TableProxy=void* (__cdecl *)(const SendProp*,const void*,const void*,Recipients*,int);
using ArrayLengthProxy=int (__cdecl *)(const void*,int);
struct DVariant;
using VarProxy=void (__cdecl *)(const SendProp*,const void*,const void*,DVariant*,int,int);
constexpr int kDataTableType=5;
constexpr int kProxyAlwaysYes=0x200;
class SendProp {
public:
    virtual ~SendProp()=default;
    void* matchingRecvProp=nullptr;
    int type=0;
    int bits=0;
    float low=0,high=0;
    SendProp* arrayProp=nullptr;
    ArrayLengthProxy arrayLength=nullptr;
    int elements=1;
    int stride=-1;
    const char* excludeOrParent=nullptr;
    const char* name=nullptr;
    float multiplier=0;
    int flags=0;
    VarProxy scalarProxy=nullptr;
    TableProxy tableProxy=nullptr;
    SendTable* table=nullptr;
    int offset=0;
    const void* extra=nullptr;
};
struct SendTable {
    SendProp* props=nullptr;
    int count=0;
    const char* name=nullptr;
    void* precalc=nullptr;
    bool initialized=false;
    bool written=false;

    // Original construct resets the engine-owned precalc and both flags.
    // Additional validation rejects invalid storage in independent callers.
    bool Construct(SendProp* p,int n,const char* netName) noexcept {
        if(n<0 || (n && !p) || !netName || !*netName)return false;
        props=p;count=n;name=netName;precalc=nullptr;
        initialized=false;written=false;
        return true;
    }
};
static_assert(sizeof(SendProp)==0x4c,"original SendProp x86 stride");
static_assert(sizeof(SendTable)==0x14,"original SendTable x86 stride");
static_assert(offsetof(SendTable,props)==0 &&
              offsetof(SendTable,count)==4 &&
              offsetof(SendTable,name)==8 &&
              offsetof(SendTable,precalc)==12 &&
              offsetof(SendTable,initialized)==16 &&
              offsetof(SendTable,written)==17,"original SendTable fields");
static_assert(sizeof(Recipients)==32,"original all-recipient bitset");

// Original standard proxies at RVAs 0x25E590 and 0x25E5A0 return
// the supplied data or the pointer stored at that data address.
// The independent pointer variant also rejects a null input address.
inline void* __cdecl DirectTable(const SendProp*,const void*,const void* data,
                                 Recipients*,int) noexcept {
    return const_cast<void*>(data);
}
inline void* __cdecl PointerTable(const SendProp*,const void*,const void* data,
                                  Recipients*,int) noexcept {
    return data ? *static_cast<void* const*>(data) : nullptr;
}
inline bool MakeDataTableProp(SendProp& out,const char* name,int offset,
                              SendTable* table,TableProxy proxy) noexcept {
    if(!name || !*name || !table || !proxy)return false;
    SendProp fresh;
    fresh.type=kDataTableType;
    fresh.name=name;
    fresh.offset=offset;
    fresh.table=table;
    fresh.tableProxy=proxy;
    // Original marks precisely the two standard proxy identities.
    if(proxy==&DirectTable || proxy==&PointerTable)
        fresh.flags=kProxyAlwaysYes;
    out=fresh;
    return true;
}
// Original CSE1Proxy custom callback RVA 0x2D5C00 sets all eight
// recipient words and returns the current game-rules singleton.
// The owner of that singleton supplies it explicitly here.
inline void* AllRecipientsGameRules(void* currentRules,Recipients& recipients) noexcept {
    for(auto& word:recipients.words)word=0xffffffffu;
    return currentRules;
}

// Original DT_SE1Proxy declares baseclass and se1_gamerules_data in
// that order; DT_GameRulesProxy and DT_SE1GameRules declare zero direct
// properties. Independent owner must provide the real rules callback.
// This graph is not returned to the retail engine.
struct SE1ProxyTables {
    SendTable base{},rules{},proxy{};
    SendProp properties[2];
    SE1ProxyTables()=default;
    SE1ProxyTables(const SE1ProxyTables&)=delete;
    SE1ProxyTables& operator=(const SE1ProxyTables&)=delete;
    bool Construct(TableProxy rulesProxy) noexcept {
        if(!rulesProxy || rulesProxy==&DirectTable || rulesProxy==&PointerTable)
            return false;
        return base.Construct(nullptr,0,"DT_GameRulesProxy") &&
            rules.Construct(nullptr,0,"DT_SE1GameRules") &&
            MakeDataTableProp(properties[0],"baseclass",0,&base,&DirectTable) &&
            MakeDataTableProp(properties[1],"se1_gamerules_data",0,&rules,rulesProxy) &&
            proxy.Construct(properties,2,"DT_SE1Proxy");
    }
};
} // namespace send_schema
} // namespace sin_independent
