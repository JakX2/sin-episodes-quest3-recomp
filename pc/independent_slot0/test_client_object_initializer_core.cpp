#include "client_object_initializer_core.h"
#include <cstdio>
using namespace sin_independent;
namespace {
ClientInitializers<4>* owner=nullptr;
void* first=nullptr;
void* second=nullptr;
void* third=nullptr;
int tokens[3]{};
int events[16]{};
unsigned used=0;
bool failSecond=false,bad=false;
void Record(int e) noexcept {
    if(used>=16)bad=true;
    else events[used++]=e;
}
void* __cdecl CreateFirst(){
    Record(1);
    if(owner->State()!=InitializerState::Initializing ||
       owner->Initialize()!=InitializerResult::AlreadyAttempted || owner->Free())bad=true;
    return &tokens[0];
}
void* __cdecl CreateSecond(){Record(2);return failSecond?nullptr:&tokens[1];}
void* __cdecl CreateThird(){Record(3);return &tokens[2];}
void* __cdecl Optional(){Record(4);return nullptr;}
void __cdecl DeleteFirst(void* p){Record(10);if(p!=&tokens[0])bad=true;}
void __cdecl DeleteSecond(void* p){
    Record(20);
    if(p!=(failSecond?nullptr:&tokens[1]))bad=true;
}
void __cdecl DeleteThird(void* p){
    Record(30);
    if(p!=(failSecond?nullptr:&tokens[2]))bad=true;
}
bool Run(bool failure) {
    ClientInitializers<4> init;
    owner=&init;first=second=third=nullptr;used=0;bad=false;failSecond=failure;
    ClientInitializer a{&first,&CreateFirst,&DeleteFirst,nullptr};
    ClientInitializer b{&second,&CreateSecond,&DeleteSecond,nullptr};
    ClientInitializer c{&third,&CreateThird,&DeleteThird,nullptr};
    ClientInitializer optional{nullptr,&Optional,nullptr,nullptr};
    ClientInitializer duplicate{&first,&CreateFirst,&DeleteFirst,nullptr};
    // Original constructor prepends records.
    if(!init.Add(c) || !init.Add(b) || !init.Add(optional) || !init.Add(a) ||
       init.Head()!=&a || a.next!=&optional || optional.next!=&b || b.next!=&c ||
       init.Add(duplicate) || init.Add(a) || init.Free())return false;
    c.next=&a;
    if(init.Initialize()!=InitializerResult::InvalidList || used)return false;
    c.next=nullptr;
    first=&tokens[0];
    if(init.Initialize()!=InitializerResult::InvalidList || used)return false;
    first=nullptr;
    const auto result=init.Initialize();
    if(failure) {
        const int want[]={1,4,2,10,20,30};
        if(result!=InitializerResult::RequiredCreationFailed || used!=6 ||
           init.State()!=InitializerState::Failed || first || second || third || init.Free())
            return false;
        for(unsigned i=0;i<6;++i)if(events[i]!=want[i])return false;
    } else {
        if(result!=InitializerResult::Initialized || used!=4 ||
           init.State()!=InitializerState::Ready ||
           first!=&tokens[0] || second!=&tokens[1] || third!=&tokens[2] || !init.Free())
            return false;
        const int want[]={1,4,2,3,10,20,30};
        if(used!=7 || first || second || third || init.State()!=InitializerState::Freed || init.Free())
            return false;
        for(unsigned i=0;i<7;++i)if(events[i]!=want[i])return false;
    }
    return !bad && init.Initialize()==InitializerResult::AlreadyAttempted && !init.Add(a);
}
}
int main(){
    if(!Run(false))return 1;
    if(!Run(true))return 2;
    ClientInitializers<1> empty;
    if(empty.Initialize()!=InitializerResult::Initialized || !empty.Free())return 3;
    ClientInitializers<1> invalid;
    ClientInitializer wrong{nullptr,&CreateThird,&DeleteThird,nullptr};
    used=0;bad=false;failSecond=false;
    if(!invalid.Add(wrong) || invalid.Initialize()!=InitializerResult::InvalidOptionalResult ||
       invalid.State()!=InitializerState::Failed || used!=2 || events[0]!=3 || events[1]!=30 || bad)
        return 4;
    std::puts("PASS original client initializer: 16-byte ABI, prepended records, function-only entries, required failure and forward full cleanup");
    return 0;
}
