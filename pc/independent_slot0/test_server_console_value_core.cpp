#include "server_console_value_core.h"
#include <cstring>
#include <cstdio>
#include <limits>
using namespace sin_independent;
struct CallbackState {
    unsigned calls=0;
    char old[128]{};
    float seen=0;
    ConsoleValue* receiver=nullptr;
};
static void Changed(ConsoleValue& value,const char* old,void* p) noexcept {
    auto& state=*static_cast<CallbackState*>(p);
    ++state.calls;
    std::snprintf(state.old,sizeof(state.old),"%s",old);
    state.seen=value.Float();
    state.receiver=&value;
}
int main() {
    ConsoleValue root,alias,other,hidden;
    CallbackState s;
    const ConsoleValue::Bounds bounds{true,0,true,10};
    if(!root.Initialize("12",0,bounds) || root.Int()!=12 ||
       !alias.Initialize("1",0,{}) || !other.Initialize("0",0,{}) ||
       !alias.LinkTo(root) || root.LinkTo(other) || other.LinkTo(alias) ||
       root.LinkTo(root) || alias.Int()!=12 || std::strcmp(alias.Default(),"12"))
        return 1;
    if(!alias.InstallChanged(&Changed,&s) || s.calls!=1 ||
       s.receiver!=&root || std::strcmp(s.old,"12") ||
       root.InstallChanged(&Changed,&s))return 2;
    if(!alias.SetString(" 3.75tail") || root.Float()!=3.75f ||
       root.Int()!=3 || std::strcmp(root.String()," 3.75tail") ||
       s.calls!=2 || std::strcmp(s.old,"12") || s.seen!=3.75f)return 3;
    if(!root.SetString(root.String()) || s.calls!=3 ||
       std::strcmp(s.old," 3.75tail"))return 4;
    if(!alias.SetString("-5") || root.Int()!=0 ||
       std::strcmp(root.String(),"0.000000") || s.calls!=4)return 5;
    if(!alias.SetInt(21) || root.Int()!=10 ||
       std::strcmp(root.String(),"10") || s.calls!=5 ||
       !alias.SetInt(10) || !alias.SetFloat(10) || s.calls!=5)return 6;
    if(!alias.SetFloat(4.5f) || root.Float()!=4.5f ||
       root.Int()!=4 || std::strcmp(root.String(),"4.500000") ||
       s.calls!=6 || std::strcmp(s.old,"10"))return 7;
    if(!alias.Revert() || root.Int()!=10 || std::strcmp(root.Default(),"12") ||
       std::strcmp(root.String(),"10.000000") || s.calls!=7)return 8;
    if(alias.SetFloat(std::numeric_limits<float>::infinity()) ||
       alias.SetString("nan") || alias.SetString(nullptr) ||
       s.calls!=7 || root.Int()!=10)return 9;
    if(!hidden.Initialize("2",kConsoleNeverAsString,{}) ||
       hidden.InstallChanged(&Changed,&s) || !hidden.SetInt(15) ||
       hidden.Int()!=15 || std::strcmp(hidden.String(),"FCVAR_NEVER_AS_STRING") ||
       s.calls!=7)return 10;
    ConsoleValue huge;
    if(!huge.Initialize("0",0,{}) || !huge.SetString("1e20") ||
       huge.Int()!=std::numeric_limits<int>::min() ||
       !huge.SetInt(std::numeric_limits<int>::max()) ||
       huge.Int()!=std::numeric_limits<int>::max())return 11;
    ConsoleValue invalid;
    if(invalid.Initialize("0",0,{true,10,true,0}) ||
       invalid.Initialize("nan",0,{}) || invalid.SetInt(1))return 12;
    std::puts("PASS console values: parent aliases, original setter distinctions, bounds, callbacks, default revert and numeric conversion");
    return 0;
}
