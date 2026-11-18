#include "server_console_object_abi.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
using namespace sin_independent::console_abi;

static Variable* changedObject=nullptr;
static unsigned changedCount=0,dispatched=0;
static char oldValue[128]{};
static float changedFloat=0;
static void __cdecl Changed(Variable* var,const char* old) {
    changedObject=var;++changedCount;changedFloat=var->real;
    std::snprintf(oldValue,sizeof(oldValue),"%s",old);
}
static void __cdecl Execute(){++dispatched;}
static int __cdecl Complete(const char* prefix,char (*out)[64]) {
    if(std::strcmp(prefix,"te"))return -1;
    std::strcpy(out[0],"test");return 1;
}
struct Registration {
    CommandBase* head=nullptr;
    unsigned calls=0;
    bool detached=true;
};
static bool __cdecl Register(CommandBase* p,void* context) noexcept {
    auto& r=*static_cast<Registration*>(context);
    r.detached=r.detached && !p->registered;
    p->next=r.head;r.head=p;++r.calls;
    return false; // original Init sets registered regardless of return
}
static int fail(int code,const char* message) {
    std::fprintf(stderr,"FAIL console x86 ABI %d: %s\n",code,message);return code;
}
template<class T> static void** Vtable(T& t) {
    return *reinterpret_cast<void***>(&t);
}
template<class T,class Field> static std::size_t Offset(T& o,Field& v) {
    return reinterpret_cast<unsigned char*>(&v)-reinterpret_cast<unsigned char*>(&o);
}
int main() {
    Variable root,alias;
    Command cmd,plain;
    if(Offset(root,root.next)!=4 || Offset(root,root.registered)!=8 ||
       Offset(root,root.name)!=12 || Offset(root,root.help)!=16 ||
       Offset(root,root.flags)!=20 || Offset(root,root.parent)!=24 ||
       Offset(root,root.defaultValue)!=28 || Offset(root,root.string)!=32 ||
       Offset(root,root.capacity)!=36 || Offset(root,root.real)!=40 ||
       Offset(root,root.integer)!=44 || Offset(root,root.hasMin)!=48 ||
       Offset(root,root.min)!=52 || Offset(root,root.hasMax)!=56 ||
       Offset(root,root.max)!=60 || Offset(root,root.callback)!=64 ||
       Offset(cmd,cmd.execute)!=24 || Offset(cmd,cmd.completion)!=28 ||
       Offset(cmd,cmd.hasCompletion)!=32)return fail(1,"record layout");
    auto** vt=Vtable(root);auto** ct=Vtable(cmd);
    using Create=void (__thiscall *)(void*,const char*,const char*,int,const char*,bool,float,bool,float,ChangeCallback);
    reinterpret_cast<Create>(vt[17])(&root,"test_var","12",0x2000,"root help",true,0,true,10,&Changed);
    reinterpret_cast<Create>(vt[17])(&alias,"test_var","1",0,"alias help",false,0,false,0,nullptr);
    if(!root.string || root.parent!=&root || root.integer!=12 || root.capacity!=3 ||
       changedCount || std::strcmp(root.String(),"12") || !alias.string)
        return fail(2,"actual slot 17 construction");
    alias.parent=&root;
    using Bool=bool (__thiscall *)(void*);
    using Flags=bool (__thiscall *)(void*,int);
    using Add=void (__thiscall *)(void*,int);
    using Text=const char* (__thiscall *)(void*);
    if(reinterpret_cast<Bool>(vt[1])(&alias) ||
       !reinterpret_cast<Flags>(vt[2])(&alias,0x2000) ||
       std::strcmp(reinterpret_cast<Text>(vt[4])(&alias),"test_var") ||
       std::strcmp(reinterpret_cast<Text>(vt[5])(&alias),"root help"))
        return fail(3,"root-forwarded base calls");
    reinterpret_cast<Add>(vt[3])(&alias,4);
    if(root.flags!=0x2004 || alias.flags)return fail(4,"root flags");
    using Int=void (__thiscall *)(void*,int);
    using Float=void (__thiscall *)(void*,float);
    using String=void (__thiscall *)(void*,const char*);
    reinterpret_cast<Int>(vt[9])(&alias,7);
    if(root.integer!=7 || std::strcmp(root.String(),"7") ||
       changedObject!=&root || changedCount!=1 || std::strcmp(oldValue,"12"))
        return fail(5,"public int slot");
    reinterpret_cast<Float>(vt[10])(&alias,5.5f);
    if(root.real!=5.5f || root.integer!=5 || changedFloat!=5.5f || changedCount!=2)
        return fail(6,"public float slot");
    reinterpret_cast<String>(vt[11])(&alias,"-1");
    if(root.real || root.integer || std::strcmp(root.String(),"0.000000") ||
       changedCount!=3 || std::strcmp(oldValue,"5.500000"))return fail(7,"public string slot");
    reinterpret_cast<String>(vt[12])(&root,"2.75");
    reinterpret_cast<Float>(vt[13])(&root,3.25f);
    reinterpret_cast<Int>(vt[14])(&root,4);
    if(root.real!=4 || changedCount!=6 || std::strcmp(root.String(),"4"))
        return fail(8,"internal setter slots");
    float number=21;
    if(!reinterpret_cast<bool (__thiscall *)(void*,float&)>(vt[15])(&root,number)||number!=10)
        return fail(9,"clamp slot");
    reinterpret_cast<String>(vt[16])(&root,"literal");
    if(root.real!=4 || changedCount!=7 || std::strcmp(root.String(),"literal"))
        return fail(10,"direct string replacement slot");
    reinterpret_cast<String>(vt[11])(&root,root.String());
    if(root.real!=0 || changedCount!=8 || std::strcmp(oldValue,"literal"))return fail(11,"aliased input");
    void* target=vt[9];void* self=&alias;
    std::uintptr_t before=0,after=0;
    __asm {
        mov before,esp
        push 9
        mov ecx,self
        mov eax,target
        call eax
        mov after,esp
    }
    if(before!=after || root.integer!=9 || changedCount!=9)return fail(12,"x86 thiscall int stack");
    using Init=void (__thiscall *)(void*);
    reinterpret_cast<Init>(vt[8])(&root);
    if(root.registered)return fail(13,"no implicit accessor");
    Registration registration;
    {
        AccessorScope scope(&Register,&registration);
        reinterpret_cast<Init>(vt[8])(&root);
        reinterpret_cast<Init>(vt[8])(&root);
        using CreateCommand=void (__thiscall *)(void*,const char*,CommandCallback,const char*,int,CompletionCallback);
        reinterpret_cast<CreateCommand>(ct[12])(&cmd,"test",&Execute,"test help",0,&Complete);
        reinterpret_cast<CreateCommand>(ct[12])(&plain,"plain",&Execute,nullptr,0,nullptr);
    }
    if(registration.calls!=3 || !registration.detached ||
       registration.head!=&plain || plain.next!=&cmd || cmd.next!=&root ||
       !reinterpret_cast<Bool>(vt[6])(&alias) ||
       !reinterpret_cast<Bool>(ct[1])(&cmd) || !reinterpret_cast<Bool>(ct[10])(&cmd) ||
       reinterpret_cast<Bool>(ct[10])(&plain) || !plain.completion)
        return fail(14,"synthetic registration and command flags");
    char outputs[64][64]{};
    using Completion=int (__thiscall *)(void*,const char*,char (*)[64]);
    if(reinterpret_cast<Completion>(ct[9])(&cmd,"te",outputs)!=1 ||
       std::strcmp(outputs[0],"test") ||
       reinterpret_cast<Completion>(ct[9])(&plain,"te",outputs)!=0)
        return fail(15,"command completion");
    reinterpret_cast<Init>(ct[11])(&cmd);
    if(dispatched!=1)return fail(16,"command dispatch");
    // Exercise the actual scalar-deleting destructor on independently
    // allocated heap objects. A matching module owns the allocation.
    auto* heap=new Variable;
    heap->CreateVariable("heap","1",0,nullptr,false,0,false,0,nullptr);
    using Delete=void* (__thiscall *)(void*,unsigned);
    reinterpret_cast<Delete>(Vtable(*heap)[0])(heap,1);
    std::puts("PASS native console ABI: 24/68/36-byte records, original virtual slots, root forwarding, callbacks, registration and deletion");
    return 0;
}
