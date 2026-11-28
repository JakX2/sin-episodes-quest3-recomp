#include "sin_compat/original_client_class_catalog.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
using namespace sin_compat::original_client_class_catalog;

int main(int argc,char**argv) {
    assert(argc==2);
    std::ifstream source(argv[1]);
    assert(source.good());
    Catalog catalog;
    assert(!catalog.IsLoaded() && !catalog.Head());
    assert(!catalog.CanExposeToRetailEngine());
    assert(catalog.Load(source));
    assert(catalog.Size()==191 && catalog.TempEventCount()==42);
    assert(catalog.IsLoaded() && catalog.Head());
    assert(catalog.Head()->networkName=="CWeaponMelee");
    assert(!catalog.CanExposeToRetailEngine());
    assert(catalog.Find("CNPC_GruntBase"));
    assert(catalog.Find("CSE1Proxy"));
    assert(catalog.Find("CWeaponMagnum"));
    assert(catalog.Find("CPropSinVehicle"));
    assert(catalog.Find("CTEExplosion") &&
           catalog.Find("CTEExplosion")->originalTempEvent);
    assert(catalog.Find("CBaseEntity") &&
           !catalog.Find("CBaseEntity")->originalTempEvent);
    assert(!catalog.Find("UnregisteredFakeClass"));
    std::unordered_set<const Entry*> visited;
    std::unordered_set<std::string> names;
    std::size_t count=0;
    std::uint32_t lastIndex=0xffffffffu;
    const Entry* tail=nullptr;
    for(auto* node=catalog.Head();node;node=node->next) {
        assert(visited.emplace(node).second);
        assert(names.emplace(node->networkName).second);
        assert(node->sourceCRTIndex<lastIndex);
        assert(node->originalNodeVA>=0x242f3000u);
        assert(node->originalRecvTableVA>=0x242f3000u);
        lastIndex=node->sourceCRTIndex;
        tail=node;
        ++count;
        assert(count<=kOriginalRegisteredClasses);
    }
    assert(count==kOriginalRegisteredClasses && visited.size()==count);
    assert(tail && tail->networkName=="CBaseTempEntity" && tail->next==nullptr);
    assert(catalog.Find("CWeaponMelee")==catalog.Head());
    assert(catalog.Find("CBaseTempEntity")==tail);
    std::ifstream again(argv[1]);
    assert(!catalog.Load(again)); // no mutation after publishing pointers

    const std::string example=
        "CFakeB|30|T|2437f708|2437f720\n"
        "CFakeA|10|E|2437f6d0|2437f6e8\n";
    {
        std::istringstream tooSmall(example);
        Catalog reject;
        assert(!reject.Load(tooSmall) && !reject.Head());
    }
    {
        std::istringstream repeated(
           "CFakeB|30|T|2437f708|2437f720\n"
           "CFakeB|10|E|2437f6d0|2437f6e8\n");
        Catalog reject;
        assert(!reject.Load(repeated) && reject.Size()==0);
    }
    {
        std::istringstream wrongOrder(
           "CFakeB|10|T|2437f708|2437f720\n"
           "CFakeA|30|E|2437f6d0|2437f6e8\n");
        Catalog reject;
        assert(!reject.Load(wrongOrder) && !reject.IsLoaded());
    }
    {
        std::istringstream badHex("CFakeB|30|T|NOT_HEX|2437f720\n");
        Catalog reject;
        assert(!reject.Load(badHex));
    }
    {
        std::istringstream extra("CFakeB|30|T|2437f708|2437f720|EXTRA\n");
        Catalog reject;
        assert(!reject.Load(extra));
    }
    std::puts("PASS original 191 ClientClass metadata order and pointer topology; 149 entities, 42 temp events, strict invalid source refusal; retail slot5 remains gated");
}
