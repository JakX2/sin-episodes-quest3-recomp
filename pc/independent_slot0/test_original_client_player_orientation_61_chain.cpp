// Phase1662: complete ORIGINAL-FREE MSVC x86 prediction -> player chain.
//
// Real compiler-generated virtual dispatch at C_BasePlayer vtable
// slots57-60 (getter RET0 / setter RET4), plus existing 23-slot
// CPrediction slots9-12 (one-pointer RET4), all with x86 ECX/ESP
// invariants. No retail DLL or engine code, no production interface.
#include "native_client_player_orientation_61_probe.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1662 original-free native player prediction chain requires MSVC x86
#endif
namespace play=sin_independent::original_client_player_orientation_61;
namespace pred=sin_independent::client_prediction23_diagnostic;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
       "FAIL Phase1662 original-free 61-slot player x 23-slot prediction "
       "native ABI #%d: %s\n",code,why);
    return code;
}
std::uintptr_t distance_from(void* object,const void* field) noexcept {
    return reinterpret_cast<std::uintptr_t>(field)-
           reinterpret_cast<std::uintptr_t>(object);
}
bool equal(const play::Vec3Bits& a,const play::Vec3Bits& b) noexcept {
    return a.x==b.x && a.y==b.y && a.z==b.z;
}
bool get_native(void* object,unsigned slot,
                const play::Vec3Bits*& result) noexcept {
    auto** table=*reinterpret_cast<void***>(object);
    void* fn=table[slot];
    void* out=nullptr;
    std::uintptr_t esp_before=0u,esp_after=0u;
    __asm {
        mov esp_before,esp
        mov ecx,object
        mov eax,fn
        call eax
        mov out,eax
        mov esp_after,esp
    }
    result=static_cast<const play::Vec3Bits*>(out);
    return esp_before==esp_after;
}
bool set_native(void* object,unsigned slot,
                play::Vec3Bits* input) noexcept {
    auto** table=*reinterpret_cast<void***>(object);
    void* fn=table[slot];
    std::uintptr_t esp_before=0u,esp_after=0u;
    __asm {
        mov esp_before,esp
        push input
        mov ecx,object
        mov eax,fn
        call eax
        mov esp_after,esp
    }
    return esp_before==esp_after;
}
bool notify_native(play::Notifier4* notifier) noexcept {
    auto** table=*reinterpret_cast<void***>(notifier);
    void* fn=table[3];
    std::uintptr_t esp_before=0u,esp_after=0u;
    __asm {
        mov esp_before,esp
        mov ecx,notifier
        mov eax,fn
        call eax
        mov esp_after,esp
    }
    return esp_before==esp_after;
}
struct Trace {
    std::array<char,64> log{};
    unsigned count=0u;
    unsigned abi_fail=0u;
    unsigned dirty_count_a=0u;
    unsigned dirty_count_b=0u;
    play::CBasePlayer61Fixture* player=nullptr;
    void push(char event) noexcept {
        if(count<log.size())log[count++]=event;
    }
    bool matches(const char* sequence,unsigned length) const noexcept {
        if(count!=length)return false;
        for(unsigned i=0u;i<length;++i)
            if(log[i]!=sequence[i])return false;
        return true;
    }
    void reset() noexcept {
        count=0u;
        log.fill(0);
    }
};
void on_dirty(void* context,unsigned flag) noexcept {
    auto* t=static_cast<Trace*>(context);
    if(flag==play::kDirtyFlagA) {
        ++t->dirty_count_a;t->push('1');
    } else if(flag==play::kDirtyFlagB) {
        ++t->dirty_count_b;t->push('2');
    } else {
        ++t->abi_fail;t->push('?');
    }
}
void on_notifier(void* context,unsigned id) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->push(id==1u?'A':id==2u?'B':'?');
    if(id!=1u && id!=2u)++t->abi_fail;
}
const pred::ViewBits3* prediction_get_a(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    const play::Vec3Bits* out=nullptr;
    if(!get_native(t->player,play::kGetterSlotA,out))++t->abi_fail;
    return out;
}
const pred::ViewBits3* prediction_get_b(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    const play::Vec3Bits* out=nullptr;
    if(!get_native(t->player,play::kGetterSlotB,out))++t->abi_fail;
    return out;
}
void prediction_set_a(void* context,pred::ViewBits3* input) noexcept {
    auto* t=static_cast<Trace*>(context);
    if(!set_native(t->player,play::kSetterSlotA,input))++t->abi_fail;
}
void prediction_set_b(void* context,pred::ViewBits3* input) noexcept {
    auto* t=static_cast<Trace*>(context);
    if(!set_native(t->player,play::kSetterSlotB,input))++t->abi_fail;
}
void prediction_mirror_a(void* context,const pred::ViewBits3* v) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->push('m');
    t->player->mirror_a(v);
}
void prediction_mirror_b(void* context,const pred::ViewBits3* v) noexcept {
    auto* t=static_cast<Trace*>(context);
    t->push('M');
    t->player->mirror_b(v);
}
void prediction_notify_a(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    if(!equal(*t->player->mirrored_a(),*t->player->a()))
        ++t->abi_fail;
    if(!notify_native(t->player->notifier_a()))++t->abi_fail;
}
void prediction_notify_b(void* context) noexcept {
    auto* t=static_cast<Trace*>(context);
    if(!equal(*t->player->mirrored_b(),*t->player->b()))
        ++t->abi_fail;
    if(!notify_native(t->player->notifier_b()))++t->abi_fail;
}
pred::OrientationOps ops(Trace& t) noexcept {
    pred::OrientationOps cb{};
    cb.user=&t;
    cb.get_a=&prediction_get_a;
    cb.get_b=&prediction_get_b;
    cb.set_a=&prediction_set_a;
    cb.set_b=&prediction_set_b;
    cb.mirror_a=&prediction_mirror_a;
    cb.mirror_b=&prediction_mirror_b;
    cb.notify_a=&prediction_notify_a;
    cb.notify_b=&prediction_notify_b;
    return cb;
}
} // namespace
int main() {
    pred::events={};
    auto player=std::make_unique<play::CBasePlayer61Fixture>();
    auto* base=player.get();
    if(distance_from(base,base->dirty_flags_for_test_only())!=
           play::kDirtyBits ||
       distance_from(base,base->a())!=play::kPlayerFieldA ||
       distance_from(base,base->notifier_a())!=play::kNotifierA ||
       distance_from(base,base->b())!=play::kPlayerFieldB ||
       distance_from(base,base->notifier_b())!=play::kNotifierB ||
       distance_from(base,base->mirrored_a())!=play::kMirrorA ||
       distance_from(base,base->mirrored_b())!=play::kMirrorB)
        return fail(1,"synthetic player offsets did not match original x86");
    auto** table=*reinterpret_cast<void***>(base);
    for(unsigned i=0u;i<=play::kSetterSlotB;++i)
        if(!table[i])return fail(2,"missing original C_BasePlayer vtable slot");
    if(table[57]==table[58] || table[58]==table[59] ||
       table[59]==table[60])
        return fail(3,"MSVC player orientation methods unexpectedly folded");

    Trace t{};
    t.player=base;
    base->set_dirty_hook({&t,&on_dirty});
    base->set_notifier_hooks(&t,&on_notifier);
    const play::Vec3Bits* get_a=nullptr;
    const play::Vec3Bits* get_b=nullptr;
    if(!get_native(base,57,get_a) || get_a!=base->a() ||
       !get_native(base,59,get_b) || get_b!=base->b() ||
       !equal(*get_a,{0u,0u,0u}) || !equal(*get_b,{0u,0u,0u}))
        return fail(4,"player native getter vtable index/ECX/RET0/offset mismatch");

    // x87 compares signed zeroes numerically: identical to +0,
    // MUST NOT dirty or overwrite original +0 DWORD bits.
    play::Vec3Bits minus_zero{0x80000000u,0u,0u};
    if(!set_native(base,58,&minus_zero) ||
       !equal(*base->a(),{0u,0u,0u}) ||
       t.count!=0u)
        return fail(5,"signed zero x87 equality incorrectly marked dirty");

    // First actual changed A vector, including a quiet NaN and -0.
    play::Vec3Bits changed_a{0x3F800000u,0x7FC01234u,0x80000000u};
    if(!set_native(base,58,&changed_a) ||
       !equal(*base->a(),changed_a) || !t.matches("1",1u) ||
       t.dirty_count_a!=1u)
        return fail(6,"player A setter x87 changed/dirty-before-copy");
    // NaN unordered: even the identical bit pattern triggers dirty.
    if(!set_native(base,58,&changed_a) ||
       !equal(*base->a(),changed_a) || !t.matches("11",2u) ||
       t.dirty_count_a!=2u)
        return fail(7,"NaN unordered compare must always mark dirty");
    // B setter dirty flag is 2. +0/-0 in the third component
    // remains numerically equal, so this value only changes due B.x.
    play::Vec3Bits changed_b{0xBF800000u,0u,0x80000000u};
    if(!set_native(base,60,&changed_b) ||
       !equal(*base->b(),changed_b) || !t.matches("112",3u) ||
       t.dirty_count_b!=1u)
        return fail(8,"player B setter distinct dirty flag or raw value");
    if(!set_native(base,60,&changed_b) ||
       !t.matches("112",3u))
        return fail(9,"all-three-equal non-NaN setter incorrectly dirtied");

    // Prepare zero state for full two-vtable prediction/player chain.
    play::Vec3Bits zero{};
    if(!set_native(base,58,&zero) || !set_native(base,60,&zero))
        return fail(10,"resetting synthetic player did not honor RET4");
    t.reset();

    auto prediction=std::make_unique<pred::Prediction23Fixture>(nullptr);
    prediction->set_orientation_ops(ops(t));
    auto** ptable=*reinterpret_cast<void***>(prediction.get());
    for(unsigned slot=9u;slot<=12u;++slot)
        if(!ptable[slot])return fail(11,"prediction virtual slot9-12 missing");
    // Source scalar pointers travel through: prediction virtual slot10
    // -> original player virtual slot58 (+E8) -> dirty(flag1)
    // -> player +0x2C8 mirror -> notifier at player+0x22C slot3.
    pred::ViewBits3 first{0x3F800000u,0u,0x80000000u};
    if(!set_native(prediction.get(),10,&first) ||
       !equal(*base->a(),first) ||
       !equal(*base->mirrored_a(),first) ||
       !t.matches("1mA",3u) || t.abi_fail)
        return fail(12,"full prediction A setter to player dirty/mirror/notifier chain");

    // CPrediction continues mirroring and notifying EVEN IF
    // C_BasePlayer's inner setter finds all 3 floats unchanged.
    if(!set_native(prediction.get(),10,&first) ||
       !t.matches("1mAmA",5u) || t.abi_fail)
        return fail(13,"unchanged player setter must still cause outer notify");
    pred::ViewBits3 read_a{};
    if(!set_native(prediction.get(),9,&read_a) ||
       !equal(read_a,first))
        return fail(14,"prediction getter slot9 did not chain to player slot57");
    pred::ViewBits3 second{0xBF800000u,0x7F800000u,0x80000000u};
    if(!set_native(prediction.get(),12,&second) ||
       !equal(*base->b(),second) ||
       !equal(*base->mirrored_b(),second) ||
       !t.matches("1mAmA2MB",8u) || t.abi_fail)
        return fail(15,"prediction B setter/player slot60 + notifier chain");
    pred::ViewBits3 read_b{};
    if(!set_native(prediction.get(),11,&read_b) ||
       !equal(read_b,second))
        return fail(16,"prediction getter slot11 did not chain to player slot59");

    // Host fixture refuses null, which native original would deref.
    if(!set_native(base,58,nullptr) ||
       !set_native(base,60,nullptr) ||
       base->refused_null_inputs()!=2u)
        return fail(17,"null raw player setter pointers were not refused");
    // Detach before destroying host objects: synthetic lifetime safety.
    prediction->set_orientation_ops({});
    player.reset();
    read_a={0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu};
    if(!set_native(prediction.get(),9,&read_a) ||
       !equal(read_a,{0u,0u,0u}))
        return fail(18,"retired player callback still accessible");
    prediction.reset();
    if(pred::events.destructor_calls!=1u)
        return fail(19,"prediction synthetic deleting-destructor count changed");
    std::puts("PASS Phase1662 original-free MSVC x86: actual 61-slot "
              "C_BasePlayer and 23-slot CPrediction virtual chain, "
              "player +0x220/+0x25C pointer getters RET0, setters "
              "RET4 x87 ordered equality/signed-zero/NaN changed "
              "dirty-helper flag1/2, nested prediction mirror "
              "+0x2C8/+0x2D4 and notifier +0x22C/+0x268 slot3, "
              "zero/unchanged/retired host, exact raw DWORDs and ESP");
    return 0;
}
