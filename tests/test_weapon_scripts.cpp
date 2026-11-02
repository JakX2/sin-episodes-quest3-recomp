#include "sin_compat/weapon_scripts.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace sin_compat;
namespace {
const char* example=R"kv(
WeaponData {
 "viewmodel" "models/weapons/v_example/example.mdl"
 "ZoomFOV" "42"
 "clip_size" "17"
 "clip2_size" "-1"
 "primary_ammo" "TEST"
 "secondary_ammo" "TEST_ALT"
 "viewmodelOriginOffset" "-0.125 0 0.375"
 SoundData { "shoot" "Example.Single" }
 TextureData { "icon" { "file" "sprites/example" } }
}
)kv";
void rejects(const std::string& src) {
 bool rejected=false;
 try { (void)parse_weapon_script(src); }
 catch(const std::runtime_error&) { rejected=true; }
 assert(rejected);
}
void test_parser() {
 auto c=parse_weapon_script(example);
 assert(c.viewmodel_name=="models/weapons/v_example/example.mdl");
 assert(c.zoom_fov==42 && c.ammo.clip_size==17 && c.ammo.clip2_size==-1);
 assert(c.primary_ammo_name=="TEST" && c.secondary_ammo_name=="TEST_ALT");
 assert(c.origin_offset[0]==-0.125f && c.origin_offset[2]==0.375f);
 assert(parse_weapon_script(std::string("\xEF\xBB\xBF")+example).zoom_fov==42);
 rejects("WeaponData { \"clip_size\" \"12\" }");
 rejects("WeaponData { \"viewmodel\" \"broken ");
 rejects(std::string(example).replace(std::string(example).find("\"42\""),4,"\"xx\""));
 rejects(std::string(example).replace(std::string(example).find("\"17\""),4,"\"-8\""));
 rejects(std::string(example).replace(std::string(example).find("-0.125 0 0.375"),14,"nan 0 1"));
 rejects("WeaponData { SoundData { \"shoot\" \"hi\" }");
 rejects(std::string(example)+"extra");
 rejects(std::string(1024*1024+1,'A'));
}
void test_disk() {
 auto dir=std::filesystem::temp_directory_path() /
 ("sin_weapon_script_fixture_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(dir/"scripts");
 { std::ofstream f(dir/"scripts"/"weapon_magnum.txt"); f<<example; }
 assert(load_weapon_script(dir,WeaponKind::Magnum).ammo.clip_size==17);
 std::filesystem::remove_all(dir);
 bool failed=false;
 try { (void)load_weapon_script(dir,WeaponKind::Magnum); }
 catch(const std::runtime_error&) { failed=true; }
 assert(failed);
}
}
int main(){test_parser();test_disk();std::cout<<"weapon script tests passed\n";}
