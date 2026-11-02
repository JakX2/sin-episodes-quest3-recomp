#include "sin_compat/weapon_scripts.hpp"
#include <exception>
#include <iostream>
int main(int argc,char**argv){
 if(argc!=2){std::cerr<<"Usage: sin_weapon_script_check <SiN game directory>\n";return 2;}
 try {
  for(auto kind:{sin_compat::WeaponKind::Magnum,sin_compat::WeaponKind::AssaultRifle,sin_compat::WeaponKind::Scattergun}){
   const auto c=sin_compat::load_weapon_script(argv[1],kind);
   std::cout<<sin_compat::weapon_script_filename(kind)<<": model="<<c.viewmodel_name
   <<" zoom="<<c.zoom_fov<<" clip="<<c.ammo.clip_size<<" clip2="<<c.ammo.clip2_size
   <<" ammo="<<c.primary_ammo_name<<"/"<<c.secondary_ammo_name
   <<" offset="<<c.origin_offset[0]<<","<<c.origin_offset[1]<<","<<c.origin_offset[2]<<"\n";
  }
 } catch(const std::exception& e){std::cerr<<"SiN script error: "<<e.what()<<"\n";return 1;}
}
