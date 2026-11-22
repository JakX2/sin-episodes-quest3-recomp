// Phase617: source-authenticated original Assault lifecycle virtual methods.
// No game engine, retail binary or original MSVC vtable is linked.
#include "sin_compat/original_grunt_assault_activation_phase617.hpp"
#include <cassert>
namespace activation=sin_compat::grunt_assault_activation_phase617;
struct Bridge final:activation::Host {
  int calls=0;
  void invoke_behavior_bridge_plus_0c_virtual_0() override {++calls;}
};
int main() {
  Bridge b;
  // Whole SHA-authenticated original Assault+0x1C and +0x28 both RET.
  activation::enter_1c_noop();
  activation::virtual_28_noop();
  assert(b.calls==0);
  // Original +0x24 dispatches through its +0x0C bridge vtable[0] exactly once.
  activation::enter_24_bridge(b);
  assert(b.calls==1);
  activation::enter_1c_noop();
  activation::virtual_28_noop();
  assert(b.calls==1);
  activation::enter_24_bridge(b);
  assert(b.calls==2);
}
