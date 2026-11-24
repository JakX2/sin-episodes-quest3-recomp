#include "original_ammo_definitions.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include "test_inventory_network_contract_core.h"
#include "test_native_weapon_datatables.h"
#include <cstring>
#include <cstdio>
namespace d=sin_port::original_ammo;
int main(int argc,char** argv){
    assert(argc==3);
    const auto client=LoadLibraryA(argv[1]),server=LoadLibraryA(argv[2]);
    assert(client&&server);
    const auto gc=reinterpret_cast<d::GetApi>(GetProcAddress(client,d::export_name));
    const auto gs=reinterpret_cast<d::GetApi>(GetProcAddress(server,d::export_name));
    assert(gc&&gs&&!gc(0,sizeof(d::Api))&&!gs(1,0));
    const auto* c=gc(1,sizeof(d::Api));const auto* s=gs(1,sizeof(d::Api));
    assert(c&&s&&c->definition_count()==17&&s->definition_count()==17);
    const char* names[]={"MAG","AR","BUCK","SR","ROCKET","ARNADE","GRENADE","FLARE","DUAG","FLAK",
        "CHAIN","PULSE","HELIGUN","HELIMISL","TURRET1","TURRET2","TURRET3"};
    const int maxima[]={80,330,30,30,5,3,5,30,8,10,750,10,320,320,320,320,320};
    for(int i=1;i<=17;++i){
        d::Definition x{},y{};
        assert(c->definition(i,&x)&&s->definition(i,&y));
        assert(std::memcmp(&x,&y,sizeof(x))==0&&std::strcmp(x.name,names[i-1])==0);
        assert(x.max_carry==maxima[i-1]&&c->max_carry(i)==x.max_carry&&s->max_carry(i)==x.max_carry);
        assert(c->index(x.name)==i&&s->index(y.name)==i&&x.min_splash==4&&x.max_splash==8);
    }
    assert(c->index("duag")==9&&s->index("aRnAdE")==6&&c->index(nullptr)==-1);
    d::Definition unchanged{};unchanged.max_carry=123;
    assert(!c->definition(0,&unchanged)&&!s->definition(18,&unchanged)&&unchanged.max_carry==123);
    assert(!s->definition(1,nullptr)&&c->max_carry(0)==0&&s->max_carry(99)==0);
    namespace wn=sin_port::weapon_net;
    const auto nc=reinterpret_cast<wn::GetApi>(GetProcAddress(client,wn::export_name));
    const auto ns=reinterpret_cast<wn::GetApi>(GetProcAddress(server,wn::export_name));
    assert(nc&&ns);
    const auto* clientNet=nc(wn::version,sizeof(wn::Api));const auto* serverNet=ns(wn::version,sizeof(wn::Api));
    assert(clientNet&&serverNet);
    wn::test::check(*serverNet,*clientNet);
    std::puts("PASS actual client/server weapon field payload contract (13/13)");
    const auto ic=reinterpret_cast<wn::GetInventoryApi>(GetProcAddress(client,wn::inventory_export_name));
    const auto is=reinterpret_cast<wn::GetInventoryApi>(GetProcAddress(server,wn::inventory_export_name));
    assert(ic&&is);
    const auto* ci=ic(wn::version,sizeof(wn::InventoryApi));const auto* si=is(wn::version,sizeof(wn::InventoryApi));
    assert(ci&&si);wn::test::check_inventory(*si,*ci);
    std::puts("PASS actual client/server inventory scalar contract (32 ammo + 48 handles + 2 scalar fields)");
    namespace tables=sin_port::native_weapon_tables;
    const auto sendApi=reinterpret_cast<tables::GetSendApi>(GetProcAddress(server,tables::send_export));
    const auto recvApi=reinterpret_cast<tables::GetRecvApi>(GetProcAddress(client,tables::recv_export));
    assert(sendApi&&recvApi&&!GetProcAddress(client,tables::send_export)&&!GetProcAddress(server,tables::recv_export));
    assert(!sendApi(0,sizeof(tables::SendApi))&&!recvApi(1,0));
    const auto* sa=sendApi(1,sizeof(tables::SendApi));const auto* ra=recvApi(1,sizeof(tables::RecvApi));
    assert(sa&&ra);
    tables::test::check(*sa,*ra,*serverNet,wn::test::example());
    std::puts("PASS native PE32 weapon SendTable/RecvTable graphs and scalar proxy dispatch (9 tables, 13 scalar fields)");
    const auto getResolved=reinterpret_cast<tables::GetResolvedSendApi>(GetProcAddress(server,tables::resolved_send_export));
    assert(getResolved&&!GetProcAddress(client,tables::resolved_send_export)&&!getResolved(0,sizeof(tables::ResolvedSendApi)));
    const auto* resolved=getResolved(1,sizeof(tables::ResolvedSendApi));assert(resolved);
    tables::test::check_resolved(*resolved);
    std::puts("PASS native resolved weapon owner/recipient callbacks and stale handle rejection");
    const auto getInventorySend=reinterpret_cast<tables::GetInventorySendApi>(GetProcAddress(server,tables::inventory_send_export));
    const auto getInventoryRecv=reinterpret_cast<tables::GetInventoryRecvApi>(GetProcAddress(client,tables::inventory_recv_export));
    assert(getInventorySend&&getInventoryRecv);
    const auto* invSend=getInventorySend(1,sizeof(tables::InventorySendApi));
    const auto* invRecv=getInventoryRecv(1,sizeof(tables::InventoryRecvApi));
    assert(invSend&&invRecv);
    tables::test::check_inventory_graph(*invSend,*invRecv);
    std::puts("PASS native paired inventory Array3 graphs (80 numbered elements, BCC recipient callbacks)");
    const auto getAnimSend=reinterpret_cast<tables::GetAnimSendApi>(GetProcAddress(server,tables::anim_send_export));
    const auto getAnimRecv=reinterpret_cast<tables::GetAnimRecvApi>(GetProcAddress(client,tables::anim_recv_export));
    assert(getAnimSend&&getAnimRecv&&!GetProcAddress(client,tables::anim_send_export)&&!GetProcAddress(server,tables::anim_recv_export));
    const auto* animSend=getAnimSend(1,sizeof(tables::AnimSendApi));
    const auto* animRecv=getAnimRecv(1,sizeof(tables::AnimRecvApi));
    assert(animSend&&animRecv&&!getAnimSend(0,sizeof(tables::AnimSendApi))&&!getAnimRecv(1,0));
    tables::test::check_anim_graph(*animSend,*animRecv,*sa,*ra);
    std::puts("PASS native paired BaseAnimating and ServerAnimationData graphs (28 numbered elements)");
    const auto getFlexSend=reinterpret_cast<tables::GetFlexSendApi>(GetProcAddress(server,tables::flex_send_export));
    const auto getFlexRecv=reinterpret_cast<tables::GetFlexRecvApi>(GetProcAddress(client,tables::flex_recv_export));
    assert(getFlexSend&&getFlexRecv&&!GetProcAddress(client,tables::flex_send_export)&&!GetProcAddress(server,tables::flex_recv_export));
    const auto* flexSend=getFlexSend(1,sizeof(tables::FlexSendApi));
    const auto* flexRecv=getFlexRecv(1,sizeof(tables::FlexRecvApi));
    assert(flexSend&&flexRecv&&!getFlexSend(0,sizeof(tables::FlexSendApi))&&!getFlexRecv(1,0));
    tables::test::check_flex_graph(*flexSend,*flexRecv,*animSend,*animRecv,*invSend,*invRecv);
    std::puts("PASS native paired BaseFlex and BaseAnimatingOverlay graphs (64 numbered elements)");
    const auto getOverlaySend=reinterpret_cast<tables::GetOverlaySendApi>(GetProcAddress(server,tables::overlay_send_export));
    const auto getOverlayRecv=reinterpret_cast<tables::GetOverlayRecvApi>(GetProcAddress(client,tables::overlay_recv_export));
    assert(getOverlaySend&&getOverlayRecv&&!GetProcAddress(client,tables::overlay_send_export)&&!GetProcAddress(server,tables::overlay_recv_export));
    const auto* overlaySend=getOverlaySend(1,sizeof(tables::OverlaySendApi));
    const auto* overlayRecv=getOverlayRecv(1,sizeof(tables::OverlayRecvApi));
    assert(overlaySend&&overlayRecv&&!getOverlaySend(0,sizeof(tables::OverlaySendApi))&&!getOverlayRecv(1,0));
    tables::test::check_overlay_graph(*overlaySend,*overlayRecv,*animSend,*animRecv,*flexSend,*flexRecv);
    std::puts("PASS native paired dynamic animation-layer vector graph and host capacity/length callbacks");
    const auto getLocalSend=reinterpret_cast<tables::GetPlayerLocalSendApi>(GetProcAddress(server,tables::player_local_send_export));
    const auto getLocalRecv=reinterpret_cast<tables::GetPlayerLocalRecvApi>(GetProcAddress(client,tables::player_local_recv_export));
    assert(getLocalSend&&getLocalRecv&&!GetProcAddress(client,tables::player_local_send_export)&&!GetProcAddress(server,tables::player_local_recv_export));
    const auto* localSend=getLocalSend(1,sizeof(tables::PlayerLocalSendApi));
    const auto* localRecv=getLocalRecv(1,sizeof(tables::PlayerLocalRecvApi));
    assert(localSend&&localRecv&&!getLocalSend(0,sizeof(tables::PlayerLocalSendApi))&&!getLocalRecv(1,0));
    tables::test::check_player_local_graph(*localSend,*localRecv,*invSend,*invRecv);
    std::puts("PASS native paired DT_LocalPlayerExclusive descriptors and velocity invalidation callbacks");
    const auto getBasePlayerSend=reinterpret_cast<tables::GetBasePlayerSendApi>(GetProcAddress(server,tables::base_player_send_export));
    const auto getBasePlayerRecv=reinterpret_cast<tables::GetBasePlayerRecvApi>(GetProcAddress(client,tables::base_player_recv_export));
    assert(getBasePlayerSend&&getBasePlayerRecv&&!GetProcAddress(client,tables::base_player_send_export)&&!GetProcAddress(server,tables::base_player_recv_export));
    const auto* basePlayerSend=getBasePlayerSend(1,sizeof(tables::BasePlayerSendApi));
    const auto* basePlayerRecv=getBasePlayerRecv(1,sizeof(tables::BasePlayerRecvApi));
    assert(basePlayerSend&&basePlayerRecv&&!getBasePlayerSend(0,sizeof(tables::BasePlayerSendApi))&&!getBasePlayerRecv(1,0));
    const auto getLocalDataSend=reinterpret_cast<tables::GetLocalDataSendApi>(GetProcAddress(server,tables::local_data_send_export));
    const auto getLocalDataRecv=reinterpret_cast<tables::GetLocalDataRecvApi>(GetProcAddress(client,tables::local_data_recv_export));
    assert(getLocalDataSend&&getLocalDataRecv&&!GetProcAddress(client,tables::local_data_send_export)&&!GetProcAddress(server,tables::local_data_recv_export));
    const auto* dataSend=getLocalDataSend(1,sizeof(tables::LocalDataSendApi));
    const auto* dataRecv=getLocalDataRecv(1,sizeof(tables::LocalDataRecvApi));
    assert(dataSend&&dataRecv&&!getLocalDataSend(0,sizeof(tables::LocalDataSendApi))&&!getLocalDataRecv(1,0));
    tables::test::check_local_data_graph(*dataSend,*dataRecv);
    std::puts("PASS native paired DT_Local 48/49 slots, 32 byte area bits and FOV transitions");
    const auto getEntitySend=reinterpret_cast<tables::GetBaseEntitySendApi>(GetProcAddress(server,tables::entity_send_export));
    const auto getEntityRecv=reinterpret_cast<tables::GetBaseEntityRecvApi>(GetProcAddress(client,tables::entity_recv_export));
    assert(getEntitySend&&getEntityRecv&&!GetProcAddress(client,tables::entity_send_export)&&!GetProcAddress(server,tables::entity_recv_export));
    const auto* entitySend=getEntitySend(1,sizeof(tables::BaseEntitySendApi));
    const auto* entityRecv=getEntityRecv(1,sizeof(tables::BaseEntityRecvApi));
    assert(entitySend&&entityRecv&&!getEntitySend(0,sizeof(tables::BaseEntitySendApi))&&!getEntityRecv(1,0));
    tables::test::check_entity_graph(*entitySend,*entityRecv);
    std::puts("PASS native paired BaseEntity time, prediction and notification network graph");
    const auto getDerivedSend=reinterpret_cast<tables::GetDerivedPlayerSendApi>(GetProcAddress(server,tables::derived_player_send_export));
    const auto getDerivedRecv=reinterpret_cast<tables::GetDerivedPlayerRecvApi>(GetProcAddress(client,tables::derived_player_recv_export));
    assert(getDerivedSend&&getDerivedRecv&&!GetProcAddress(client,tables::derived_player_send_export)&&!GetProcAddress(server,tables::derived_player_recv_export));
    const auto* derivedSend=getDerivedSend(1,sizeof(tables::DerivedPlayerSendApi));
    const auto* derivedRecv=getDerivedRecv(1,sizeof(tables::DerivedPlayerRecvApi));
    assert(derivedSend&&derivedRecv&&!getDerivedSend(0,sizeof(tables::DerivedPlayerSendApi))&&!getDerivedRecv(1,0));
    const auto getServerClass=reinterpret_cast<tables::GetServerClassApi>(GetProcAddress(server,tables::server_class_export));
    const auto getClientClass=reinterpret_cast<tables::GetClientClassApi>(GetProcAddress(client,tables::client_class_export));
    assert(getServerClass&&getClientClass&&!GetProcAddress(client,tables::server_class_export)&&!GetProcAddress(server,tables::client_class_export));
    const auto* serverClass=getServerClass(1,sizeof(tables::ServerClassApi));
    const auto* clientClass=getClientClass(1,sizeof(tables::ClientClassApi));
    assert(serverClass&&clientClass&&!getServerClass(0,sizeof(tables::ServerClassApi))&&!getClientClass(1,0));
    tables::test::check_base_player_graph(*basePlayerSend,*basePlayerRecv,*animSend,*animRecv,*overlaySend,*overlayRecv,
        *flexSend,*flexRecv,*invSend,*invRecv,*localSend,*localRecv,*dataSend,*dataRecv,*entitySend,*entityRecv,*derivedSend,*derivedRecv,*resolved,*ra,*serverClass,*clientClass);
    std::puts("PASS native paired BasePlayer, HL2_Player, HL2Local, PlayerHand and PlayerState graph composition");
    std::puts("PASS native paired 15-class registration subset, ordered lists and real creator ABI");
    const auto getAggregateSend=reinterpret_cast<tables::GetAggregateSendApi>(GetProcAddress(server,tables::aggregate_send_export));
    const auto getAggregateRecv=reinterpret_cast<tables::GetAggregateRecvApi>(GetProcAddress(client,tables::aggregate_recv_export));
    assert(getAggregateSend&&getAggregateRecv&&!GetProcAddress(client,tables::aggregate_send_export)&&!GetProcAddress(server,tables::aggregate_recv_export));
    const auto* aggregateSend=getAggregateSend(1,sizeof(tables::AggregateSendApi));
    const auto* aggregateRecv=getAggregateRecv(1,sizeof(tables::AggregateRecvApi));
    assert(aggregateSend&&aggregateRecv&&!getAggregateSend(0,sizeof(tables::AggregateSendApi))&&!getAggregateRecv(1,0));
    tables::test::check_aggregate(*aggregateSend,*aggregateRecv);
    std::puts("PASS native paired network aggregate (35 owned tables, 15 class records, transactional construction)");
    const auto getCreator=reinterpret_cast<tables::GetClientCreatorApi>(GetProcAddress(client,tables::client_creator_export));
    assert(getCreator&&!GetProcAddress(server,tables::client_creator_export));
    const auto* creator=getCreator(1,sizeof(tables::ClientCreatorApi));
    assert(creator&&!getCreator(0,sizeof(tables::ClientCreatorApi))&&!getCreator(1,0));
    tables::test::check_client_creator(*creator);
    std::puts("PASS native client entity creator boundary, five object sizes and constructor defaults");
    const auto getRegistry=reinterpret_cast<tables::GetClientRegistryApi>(GetProcAddress(client,tables::client_registry_export));
    assert(getRegistry&&!GetProcAddress(server,tables::client_registry_export));
    const auto* registry=getRegistry(1,sizeof(tables::ClientRegistryApi));
    assert(registry&&!getRegistry(0,sizeof(tables::ClientRegistryApi))&&!getRegistry(1,0));
    tables::test::check_client_registry(*registry);
    std::puts("PASS native client entity registry, active list, handle serial and notifications");
    const auto getInit=reinterpret_cast<tables::GetClientInitApi>(GetProcAddress(client,tables::client_init_export));
    assert(getInit&&!GetProcAddress(server,tables::client_init_export));
    const auto* init=getInit(1,sizeof(tables::ClientInitApi));
    assert(init&&!getInit(0,sizeof(tables::ClientInitApi))&&!getInit(1,0));
    tables::test::check_client_init(*registry,*init);
    std::puts("PASS native client slot37 initialization, map traversal and rollback");
    const auto getSpawn=reinterpret_cast<tables::GetClientSpawnApi>(GetProcAddress(client,tables::spawn_session_export));
    assert(getSpawn&&!GetProcAddress(server,tables::spawn_session_export));
    const auto* spawn=getSpawn(1,sizeof(tables::ClientSpawnApi));
    assert(spawn&&!getSpawn(0,sizeof(tables::ClientSpawnApi))&&!getSpawn(1,0));
    tables::test::check_spawn_session(*aggregateSend,*aggregateRecv,*registry,*spawn);
    std::puts("PASS native client class creators -> registry -> shared initializer -> paired property callbacks -> release");
    assert(FreeLibrary(server)&&FreeLibrary(client));
    std::puts("PASS actual native client/server ammo definition parity (17/17)");
}
