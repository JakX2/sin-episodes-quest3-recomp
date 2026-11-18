#include "server_send_datatable_core.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
using namespace sin_independent::send_schema;
static int rulesValue=123;
static void* __cdecl RuleProxy(const SendProp* prop,const void*,const void*,Recipients* recipients,int objectId) {
    if(!prop || !recipients || objectId!=7)return nullptr;
    return AllRecipientsGameRules(&rulesValue,*recipients);
}
static int fail(int code,const char* message) {
    std::fprintf(stderr,"FAIL send table %d: %s\n",code,message);return code;
}
int main() {
    SendProp layout;
    auto address=reinterpret_cast<std::uintptr_t>(&layout);
#define CHECK_OFFSET(field,offset) \
    if(reinterpret_cast<std::uintptr_t>(&layout.field)-address!=offset) \
        return fail(1,#field " offset")
    CHECK_OFFSET(matchingRecvProp,4);
    CHECK_OFFSET(type,8);
    CHECK_OFFSET(bits,12);
    CHECK_OFFSET(low,16);
    CHECK_OFFSET(high,20);
    CHECK_OFFSET(arrayProp,24);
    CHECK_OFFSET(arrayLength,28);
    CHECK_OFFSET(elements,32);
    CHECK_OFFSET(stride,36);
    CHECK_OFFSET(excludeOrParent,40);
    CHECK_OFFSET(name,44);
    CHECK_OFFSET(multiplier,48);
    CHECK_OFFSET(flags,52);
    CHECK_OFFSET(scalarProxy,56);
    CHECK_OFFSET(tableProxy,60);
    CHECK_OFFSET(table,64);
    CHECK_OFFSET(offset,68);
    CHECK_OFFSET(extra,72);
#undef CHECK_OFFSET
    SendTable table;
    int fakePrecalc=123;
    table.precalc=&fakePrecalc;table.initialized=true;table.written=true;
    if(!table.Construct(&layout,1,"DT_Test") || table.props!=&layout ||
       table.count!=1 || std::strcmp(table.name,"DT_Test") ||
       table.precalc || table.initialized || table.written)
        return fail(2,"original Construct resets engine state");
    if(table.Construct(nullptr,1,"DT_Bad") ||
       table.Construct(&layout,-1,"DT_Bad") ||
       table.Construct(&layout,1,nullptr) || table.props!=&layout ||
       table.count!=1 || std::strcmp(table.name,"DT_Test"))
        return fail(3,"invalid construction is nonmutating");
    SendProp p;
    p.bits=32;p.flags=0xFF;p.extra=&fakePrecalc;p.arrayProp=&layout;
    if(!MakeDataTableProp(p,"nested",12,&table,&DirectTable) ||
       p.type!=5 || p.bits || p.low || p.high || p.arrayProp ||
       p.arrayLength || p.elements!=1 || p.stride!=-1 ||
       p.excludeOrParent || std::strcmp(p.name,"nested") || p.multiplier ||
       p.flags!=0x200 || p.scalarProxy || p.tableProxy!=&DirectTable ||
       p.table!=&table || p.offset!=12 || p.extra || p.matchingRecvProp)
        return fail(4,"standard data-table property defaults");
    if(MakeDataTableProp(p,nullptr,0,&table,&DirectTable) ||
       MakeDataTableProp(p,"new",0,nullptr,&DirectTable) ||
       MakeDataTableProp(p,"new",0,&table,nullptr) ||
       p.offset!=12 || std::strcmp(p.name,"nested"))
        return fail(5,"invalid prop nonmutation");
    int object=5;void* pointer=&object;
    Recipients recipients;
    recipients.words[0]=0x12345678u;
    if(p.tableProxy(&p,nullptr,&object,&recipients,0)!=&object ||
       !MakeDataTableProp(p,"pointer",4,&table,&PointerTable) ||
       p.flags!=0x200 || p.tableProxy(&p,nullptr,&pointer,&recipients,0)!=&object ||
       p.tableProxy(&p,nullptr,nullptr,&recipients,0)!=nullptr ||
       recipients.words[0]!=0x12345678u)
        return fail(6,"original standard proxy identities and results");
    SE1ProxyTables graph;
    if(graph.Construct(nullptr) || graph.Construct(&DirectTable) ||
       graph.proxy.props || !graph.Construct(&RuleProxy))
        return fail(7,"explicit custom rules dependency");
    if(std::strcmp(graph.proxy.name,"DT_SE1Proxy") || graph.proxy.count!=2 ||
       graph.proxy.props!=graph.properties ||
       std::strcmp(graph.base.name,"DT_GameRulesProxy") || graph.base.count ||
       std::strcmp(graph.rules.name,"DT_SE1GameRules") || graph.rules.count ||
       std::strcmp(graph.properties[0].name,"baseclass") ||
       graph.properties[0].table!=&graph.base || graph.properties[0].flags!=0x200 ||
       std::strcmp(graph.properties[1].name,"se1_gamerules_data") ||
       graph.properties[1].table!=&graph.rules || graph.properties[1].flags ||
       graph.properties[1].offset || graph.properties[1].tableProxy!=&RuleProxy)
        return fail(8,"SE1Proxy exact direct property graph");
    if(graph.properties[1].tableProxy(&graph.properties[1],nullptr,nullptr,&recipients,7)
       !=&rulesValue)return fail(9,"custom rule callback");
    for(auto word:recipients.words)if(word!=0xFFFFFFFFu)
        return fail(10,"all 256 recipient bits");
    std::puts("PASS original x86 SendTable and SendProp layout, construction, proxies, and SE1Proxy graph");
    return 0;
}
