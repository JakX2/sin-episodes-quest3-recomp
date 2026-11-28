// Phase1679: original-free native Win32 x86 end-to-end record selection.
// Original CDECL name finder is SHA-bounded at retail 0x24197ED0.
// It supplies Phase1671 diagnostic +0x34 selected record for Phase1672
// typed dispatch -> Phase1675 RET16 format gate -> Phase1678 CDECL Msg.
// Every record, node, key, and output is invented and host-owned;
// original engine strcmp implementation is an opaque injected callback.
#include "native_original_postnetwork_record_name_lookup.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1679 requires MSVC native Windows x86
#endif
namespace find=sin_independent::original_postnetwork_record_name_lookup;
namespace sw=sin_independent::original_postnetwork_type_switch;
namespace fm=sin_independent::original_postnetwork_scalar_formatters;
namespace fx=sin_independent::original_postnetwork_cdecl_msg_formatter;
namespace {
int fail(int code,const char* why) {
    std::fprintf(stderr,
      "FAIL Phase1679 native record-name finder x86 #%d: %s\n",
      code,why);
    return code;
}
constexpr char kType0Name[]="SIDE";
constexpr char kGroupName[]="Group";
constexpr char kBinaryName[]="BINARY";
constexpr char kNestedName[]="nested";
constexpr char kChildOtherName[]="second";
constexpr char kSideName[]="SIDE";
constexpr char kKeyNested[]="NESTED";
constexpr char kKeyGroup[]="gROup";
constexpr char kKeyBinary[]="binary";
constexpr char kKeySide[]="side";
constexpr char kKeyMissing[]="missing";
constexpr char kDiagnosticLabel[]="network";

unsigned lower_ascii(unsigned c) noexcept {
    return c>='A' && c<='Z' ? c+('a'-'A'):c;
}
struct Trace {
    find::IntegratedFixture* owner=nullptr;
    unsigned bad=0u;
    unsigned outputs=0u;
    unsigned comparisons=0u;
    unsigned dispatch_calls=0u;
    std::array<std::array<char,128>,8> lines{};
    bool is(unsigned index,const char* literal) const noexcept {
        return index<outputs &&
               std::strcmp(lines[index].data(),literal)==0;
    }
};
int comparator(void* raw,const char* lhs,const char* rhs) noexcept {
    auto* t=static_cast<Trace*>(raw);
    if(!t || !lhs || !rhs)return 1;
    ++t->comparisons;
    // Invented callback models only basic ASCII. The 2006 retail
    // helper at 0x2425F81B is locale-sensitive and not replaced.
    for(unsigned i=0u;i<128u;++i) {
        const auto a=lower_ascii(static_cast<unsigned char>(lhs[i]));
        const auto b=lower_ascii(static_cast<unsigned char>(rhs[i]));
        if(a!=b)return a<b?-1:1;
        if(a==0u)return 0;
    }
    ++t->bad;
    return 1;
}
void emit(void* raw,const char* message) noexcept {
    auto* t=static_cast<Trace*>(raw);
    if(!t || !message || t->outputs>=t->lines.size()) {
        if(t)++t->bad;
        return;
    }
    auto& line=t->lines[t->outputs++];
    if(std::strlen(message)>=line.size()) {
        ++t->bad;
        return;
    }
    std::strncpy(line.data(),message,line.size()-1u);
}
void on_format(void* raw,void* raw_owner,fm::FormatKind kind,
               const char* text,std::int32_t value) noexcept {
    auto* t=static_cast<Trace*>(raw);
    if(!t || raw_owner!=t->owner ||
       (kind!=fm::FormatKind::Binary &&
        kind!=fm::FormatKind::Short)) {
        if(t)++t->bad;
        return;
    }
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    fx::Fixture::FormatScalar(
        static_cast<fx::Fixture*>(t->owner),text,value);
    __asm mov after,esp
    if(before!=after)++t->bad;
}
struct Data {
    std::array<sw::Record,3> main{};
    std::array<sw::Record,2> child{};
    std::array<sw::Record,1> side{};
    sw::Node nmain{};
    sw::Node nchild{};
    sw::Node nside{};
};
void setup(sw::Record& r,std::uint32_t type,const char* name,
           std::int16_t count=0) noexcept {
    r.raw.fill(0u);
    r.write32(0u,type);
    r.write32(4u,find::rawptr(name));
    r.write16(0x10u,static_cast<std::uint16_t>(count));
    r.write32(sw::kOriginalRecordSequenceOffset,0u);
}
void install(Data& d,find::Context& ctx) noexcept {
    setup(d.main[0],0u,kType0Name);
    setup(d.main[1],10u,kGroupName);
    setup(d.main[2],9u,kBinaryName,3);
    setup(d.child[0],7u,kNestedName,-32768);
    setup(d.child[1],8u,kChildOtherName,1);
    setup(d.side[0],8u,kSideName,2);
    d.nmain.word0=find::rawptr(d.main.data());
    d.nmain.word4=3u;
    d.nmain.unknown8=0x12345678u;
    d.nmain.next=&d.nside;
    d.nchild.word0=find::rawptr(d.child.data());
    d.nchild.word4=2u;
    d.nchild.unknown8=0xABC12340u;
    d.nchild.next=nullptr;
    d.nside.word0=find::rawptr(d.side.data());
    d.nside.word4=1u;
    d.nside.unknown8=0x0F0F0F0Fu;
    d.nside.next=nullptr;
    d.main[1].write32(find::kOriginalRecordNestedOffset,
                      find::rawptr(&d.nchild));
    ctx.authorize_node(&d.nmain,d.main.data(),3u);
    ctx.authorize_node(&d.nchild,d.child.data(),2u);
    ctx.authorize_node(&d.nside,d.side.data(),1u);
    ctx.authorize_name(kType0Name);
    ctx.authorize_name(kGroupName);
    ctx.authorize_name(kBinaryName);
    ctx.authorize_name(kNestedName);
    ctx.authorize_name(kChildOtherName);
    ctx.authorize_name(kSideName);
}
struct DispatchContext {
    Trace* trace=nullptr;
    Data* data=nullptr;
    std::int16_t signed_short=-32768;
};
void on_case(void* raw,std::uint8_t case_index,
             std::uint32_t type,sw::Record* record,
             std::uint32_t,std::uint32_t,std::int16_t) noexcept {
    auto* ctx=static_cast<DispatchContext*>(raw);
    if(!ctx || !ctx->trace || !ctx->data || !record)return;
    ++ctx->trace->dispatch_calls;
    auto& owner=*ctx->trace->owner;
    const char* name=nullptr;
    if(record==&ctx->data->main[2] && type==9u && case_index==9u)
        name=kBinaryName;
    else if(record==&ctx->data->side[0] && type==8u && case_index==8u)
        name=kSideName;
    else if(record==&ctx->data->child[0] && type==7u && case_index==7u)
        name=kNestedName;
    else return; // native type10 recursive specialist still opaque
    owner.authorize(record,name,kDiagnosticLabel,23);
    if(type==7u) {
        owner.authorize_short(&ctx->signed_short);
        owner.FormatShortIfSelected(
            0u,find::rawptr(&ctx->signed_short),0u,0u);
    } else {
        const auto count=static_cast<std::int16_t>(
            record->read16(0x10u));
        const auto n=static_cast<std::uint32_t>(
            static_cast<std::int32_t>(count)*(type==9u?4:1));
        owner.FormatBinaryIfSelected(0u,n,0u,0u);
    }
}
bool invoke_dispatch(find::IntegratedFixture& owner,std::uint32_t seq,
                     sw::Node* node,sw::Record* records,
                     std::int32_t count) noexcept {
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.Dispatch(seq,node,records,count);
    __asm mov after,esp
    return before==after;
}
bool lookup(find::Context& ctx,const char* key,sw::Node* root,
            sw::Record* expected) noexcept {
    ctx.authorized_key=key;
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    auto* actual=find::NativeLookup::Find(key,root);
    __asm mov after,esp
    return before==after && actual==expected;
}
bool choose(find::Context& ctx,find::IntegratedFixture& owner,
            const char* key,unsigned index,sw::Node* root,
            sw::Record* expected) noexcept {
    ctx.authorized_key=key;
    owner.configured_key=key;
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.SelectDiagnostic(kDiagnosticLabel,index,root);
    __asm mov after,esp
    return before==after &&
        owner.diagnostic_record==find::rawptr(expected) &&
        owner.diagnostic_label_word==find::rawptr(kDiagnosticLabel);
}
} // namespace

int main() {
    Data d{};
    Trace t{};
    find::Context ctx{};
    install(d,ctx);
    ctx.comparator_user=&t;
    ctx.compare=&comparator;
    find::ScopedContext scope(ctx);

    if(!lookup(ctx,kKeyNested,&d.nmain,&d.child[0]) ||
       ctx.nested_calls!=1u ||
       ctx.skipped_type0==0u || t.bad)
        return fail(1,"original type10 recursive child selected before parent");
    if(!lookup(ctx,kKeyGroup,&d.nmain,&d.main[1]))
        return fail(2,"type10 child miss must compare parent name next");
    if(!lookup(ctx,kKeyBinary,&d.nmain,&d.main[2]))
        return fail(3,"original ASCII case-insensitive name comparator handoff");
    if(!lookup(ctx,kKeySide,&d.nmain,&d.side[0]) ||
       ctx.next_hops==0u)
        return fail(4,"original node+0x0C list traversal after local misses");
    if(!lookup(ctx,kKeyMissing,&d.nmain,nullptr))
        return fail(5,"absent name should return null CDECL pointer");

    // Signed node+4 count <=0 skips all record reads and follows
    // node+0x0C. The node's record pointer remains invalid safely.
    sw::Node negative{};
    negative.word0=0xBAADF00Du;
    negative.word4=0xFFFFFFFFu;
    negative.next=&d.nside;
    if(!ctx.authorize_node(&negative,nullptr,0u) ||
       !lookup(ctx,kKeySide,&negative,&d.side[0]))
        return fail(6,"signed negative count must follow next without deref");
    negative.word4=0u;
    if(!lookup(ctx,kKeySide,&negative,&d.side[0]))
        return fail(7,"zero signed count must also follow next");

    // Synthetic-only authority: the original would try unguarded
    // pointers. Reject unknown arrays, keys and null roots.
    const auto old_word0=d.nmain.word0;
    d.nmain.word0=0xDEADBEEFu;
    const auto old_refused=ctx.refused_unowned;
    if(!lookup(ctx,kKeyBinary,&d.nmain,nullptr) ||
       ctx.refused_unowned!=old_refused+1u)
        return fail(8,"unknown record array not rejected");
    d.nmain.word0=old_word0;
    ctx.authorized_key=kKeyBinary;
    const auto before_bad=ctx.refused_unowned;
    if(find::NativeLookup::Find(kKeySide,&d.nmain)!=nullptr ||
       ctx.refused_unowned!=before_bad+1u)
        return fail(9,"ungranted lookup string not rejected");
    if(find::NativeLookup::Find(kKeyBinary,nullptr)!=nullptr)
        return fail(10,"null original root must be safely refused");

    // Source list cycles and recursive type10 loops are unsafe;
    // preserve deterministic refusal rather than infinite looping.
    const auto prior_budget=ctx.refused_budget;
    d.nside.next=&d.nside;
    if(!lookup(ctx,kKeyMissing,&d.nmain,nullptr) ||
       ctx.refused_budget!=prior_budget+1u)
        return fail(11,"cyclic next pointer bounded host-only");
    d.nside.next=nullptr;
    d.child[1].write32(0u,10u);
    d.child[1].write32(find::kOriginalRecordNestedOffset,
                       find::rawptr(&d.nchild));
    const auto budget_before=ctx.refused_budget;
    if(!lookup(ctx,kKeyMissing,&d.nmain,nullptr) ||
       ctx.refused_budget<=budget_before)
        return fail(12,"recursive type10 cycle host refusal missing");
    d.child[1].write32(0u,8u);
    d.child[1].write32(find::kOriginalRecordNestedOffset,0u);

    // From here, compose the original diagnostic core same-owner
    // this+0x34 selection -> native dispatcher -> original scalar
    // preview -> native cdecl Msg. No fake hand-picked selection.
    find::IntegratedFixture owner{};
    owner.expected_index=7;
    owner.debug_flags=0u;
    owner.configured_key=kKeyBinary;
    owner.flagged_key=kKeySide;
    owner.unknown14=1u;
    owner.correction_byte=0u;
    owner.mode=0u;
    owner.left_index=0u;
    owner.right_index=1u;
    t.owner=&owner;
    fx::OutputOps out{};
    out.user=&t;
    out.on_msg=&emit;
    owner.output_ops=out;
    fm::FormatOps format{};
    format.user=&t;
    format.on_format=&on_format;
    owner.format_ops=format;
    DispatchContext dc{};
    dc.trace=&t;dc.data=&d;
    owner.ops.user=&dc;
    owner.ops.on_opaque_case=&on_case;

    if(!choose(ctx,owner,kKeyBinary,7u,&d.nmain,&d.main[2]) ||
       !invoke_dispatch(owner,1u,&d.nmain,d.main.data(),3) ||
       t.outputs!=1u || t.bad ||
       !t.is(0u,"23 network BINARY : binary (12 bytes)\n"))
        return fail(13,"RET12 selection -> typed RET16 -> binary RET16 -> cdecl Msg");

    // Flag 0x1000 chooses a distinct source query key. The
    // original label in this+0x38 stays the first input string.
    owner.debug_flags=find::diag::kDebugMask;
    ctx.authorized_key=kKeySide;
    owner.flagged_key=kKeySide;
    std::uintptr_t before=0u,after=0u;
    __asm mov before,esp
    owner.SelectDiagnostic(kDiagnosticLabel,7u,&d.nmain);
    __asm mov after,esp
    if(before!=after ||
       owner.diagnostic_record!=find::rawptr(&d.side[0]) ||
       !invoke_dispatch(owner,2u,&d.nside,d.side.data(),1) ||
       t.outputs!=2u || t.bad ||
       !t.is(1u,"23 network SIDE : binary (2 bytes)\n"))
        return fail(14,"flagged name branch -> linked node -> integer case");

    // Recursive nested child selection now reaches signed-short
    // preview and the same original Msg template.
    owner.debug_flags=0u;
    if(!choose(ctx,owner,kKeyNested,7u,&d.nmain,&d.child[0]) ||
       !invoke_dispatch(owner,3u,&d.nchild,d.child.data(),2) ||
       t.outputs!=3u || t.bad ||
       !t.is(2u,"23 network nested : short (-32768)\n"))
        return fail(15,"recursive selected child -> RET16 signed-short Msg");

    // No chosen record means no selected preview, but the
    // dispatcher still traverses its invented typed records.
    const auto output_before=t.outputs;
    if(!choose(ctx,owner,kKeyMissing,7u,&d.nmain,nullptr) ||
       !invoke_dispatch(owner,4u,&d.nmain,d.main.data(),3) ||
       t.outputs!=output_before || t.bad)
        return fail(16,"absent lookup must suppress all previews");

    // Mismatched original debug index clears +0x34 before searching,
    // preserving the original +0x38 first-input pointer.
    ctx.authorized_key=kKeyBinary;
    owner.configured_key=kKeyBinary;
    const auto prev_calls=owner.lookup_calls;
    __asm mov before,esp
    owner.SelectDiagnostic(kDiagnosticLabel,8u,&d.nmain);
    __asm mov after,esp
    if(before!=after || owner.diagnostic_record!=0u ||
       owner.lookup_calls!=prev_calls ||
       owner.diagnostic_label_word!=find::rawptr(kDiagnosticLabel))
        return fail(17,"wrong debug index must clear selected record");
    __asm mov before,esp
    owner.SelectDiagnostic("",7u,&d.nmain);
    __asm mov after,esp
    if(before!=after || owner.diagnostic_record!=0u ||
       owner.lookup_calls!=prev_calls)
        return fail(18,"empty first diagnostic string gate");

    std::puts("PASS Phase1679 original-free native Win32 x86: "
              "authenticated 121B CDECL record-name lookup, "
              "signed node count / 0x34 record stride / type0 skip "
              "/ type10 recursive child-first / +0x0C list walk, "
              "host-only cycle/unknown-pointer refusal, "
              "RET12 diagnostic selection this+0x34 followed by "
              "native typed RET16, binary/short RET16 and native "
              "cdecl bounded Msg; no original retail method executed");
    return 0;
}
