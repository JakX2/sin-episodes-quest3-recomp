#include "sin_compat/stat_schema.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <map>
#include <string>
using namespace sin_compat;
int main() {
  // Self-authored fixture; not original game text.
  const char* example = R"(// tests comments and repeated stat blocks
  "StatDefinitions" {
    "stat" { "name" "TIME" "short_text" "#TIME_S" "long_text" "#TIME_L" }
    "stat" { "name" "COMBAT" "short_text" "#COMBAT_S" "long_text" "#COMBAT_L"
       "percentage_of" "TIME" }
    "stat" { "name" "SHOTS" "short_text" "#SHOTS_S" "long_text" "#SHOTS_L" }
  })";
  StatSchema s;std::string error;
  assert(s.load(example,&error) && error.empty());
  assert(s.definitions().size()==3);
  assert(s.definitions()[1].name=="COMBAT");
  assert(s.find("TIME") && s.find("TIME")->long_text=="#TIME_L");
  assert(s.find("COMBAT")->percentage_of=="TIME");
  assert(!s.find("missing"));
  std::map<std::string,double> values={{"TIME",20},{"COMBAT",10}};
  auto result=s.percentage("COMBAT",values);
  assert(result && result.percent==50.0);
  assert(s.percentage("TIME",values).status==StatPercentage::Status::NotPercentageStat);
  assert(s.percentage("no",values).status==StatPercentage::Status::UnknownStat);
  values.erase("TIME");
  assert(s.percentage("COMBAT",values).status==StatPercentage::Status::MissingDenominator);
  values["TIME"]=0;
  assert(s.percentage("COMBAT",values).status==StatPercentage::Status::ZeroDenominator);
  values["TIME"]=INFINITY;
  assert(s.percentage("COMBAT",values).status==StatPercentage::Status::InvalidValue);
  values["TIME"]=10; values.erase("COMBAT");
  assert(s.percentage("COMBAT",values).status==StatPercentage::Status::MissingNumerator);
  values["COMBAT"]=3;
  assert(s.percentage("COMBAT",values).percent==30.0);
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "long_text" "L" } })",&error));
  assert(error=="missing required stat value" && s.definitions().empty());
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "short_text" "S" "long_text" "L" }
             "stat" { "name" "X" "short_text" "T" "long_text" "U" } })",&error));
  assert(error=="duplicate stat name");
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "short_text" "S"
             "long_text" "L" "percentage_of" "MISSING" } })",&error));
  assert(error=="unknown percentage_of reference");
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "name" "Y"
             "short_text" "S" "long_text" "L" } })",&error));
  assert(error=="duplicate stat field");
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "short_text" "S"
             "long_text" "L" "strange" "F" } })",&error));
  assert(error=="unknown stat field");
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "short_text" "S"
             "long_text" "L" } } trailing)",&error));
  assert(error=="trailing content after StatDefinitions");
  assert(!s.load(R"("StatDefinitions" { "stat" { "name" "X" "short_text" "S"
             "long_text" "L\q" } })",&error));
  assert(!s.load(std::string(262145,'x'),&error));
  assert(error=="stat definition size limit");
  assert(s.load(R"("StatDefinitions" { "stat" { "name" "A" "short_text" "S" "long_text" "L"
      "percentage_of" "B" } "stat" { "name" "B" "short_text" "S" "long_text" "L" } })"));
  assert(s.find("A") && s.find("B")); // forward reference allowed
  return 0;
}
