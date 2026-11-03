#include "sin_compat/original_factory_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
using namespace sin_compat::original_factory;
static int a=1,b=2,c=3;
static void* primary(){return &a;}
static void* secondary(){return &b;}
static void* shared(){return &c;}
int main(){
  Registry client,server;
  Registration ca{primary,client_interface};
  Registration cb{secondary,"GameClientExports001"};
  Registration sa{secondary,server_interface};
  Registration sb{shared,server_ents_interface};
  Registration sc{primary,server_clients_interface};
  client.add(ca);client.add(cb);
  server.add(sa);server.add(sb);server.add(sc);
  assert(client.first==&cb && cb.next==&ca);
  int code=-1;
  assert(client.query("VClient011",&code)==&a && code==0);
  assert(client.query("GameClientExports001",&code)==&b && code==0);
  assert(server.query("ServerGameDLL003",&code)==&b && code==0);
  assert(server.query("ServerGameEnts001",&code)==&c && code==0);
  assert(server.query("ServerGameClients003",&code)==&a && code==0);
  assert(client.query("VClient017",&code)==nullptr && code==1);
  assert(server.query("ServerGameDLL010",&code)==nullptr && code==1);
  assert(server.query("servergamedll003",&code)==nullptr && code==1);
  assert(client.query("VCLIENT011",&code)==nullptr && code==1);
  assert(client.query("VClient011",nullptr)==&a);
  assert(client.query("unknown",nullptr)==nullptr);
  assert(client.query(nullptr,&code)==nullptr && code==1);
  Registration duplicate{shared,"VClient011"};client.add(duplicate);
  assert(client.query("VClient011",&code)==&c && code==0);
  Registry empty;
  assert(empty.query("VClient011",&code)==nullptr && code==1);
}
