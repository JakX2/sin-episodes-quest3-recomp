// Phase 1658: original-free native Win32 x86 prediction slot1 ABI fixture.
// SHA-matched retail client.dll slot1 (0x24195F80) has NO stack arguments,
// reads cl_predict's m_pParent pointer from 0x24374540
// (cl_predict ConVar object 0x24374528 + 0x18), tests that parent's
// numeric ConVar m_nValue DWORD at +0x2C, and writes prediction
// this+0x10 = (nonzero?1:0), RET 0.
//
// The original does not check whether the ConVar parent pointer is null.
// The independently owned diagnostic model refuses a missing ConVar view.
// This fixture is NOT registered in CreateInterface or an original engine.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Phase1658 prediction test requires native MSVC Windows x86
#endif

namespace sin_independent {
namespace client_prediction23_diagnostic {

constexpr std::uintptr_t kOriginalSingletonRva=0x37FA00u;
constexpr std::uintptr_t kOriginalVtableRva=0x2A363Cu;
constexpr std::uintptr_t kOriginalInitMethodRva=0x195F80u;
constexpr std::uintptr_t kOriginalClPredictConVarRva=0x374528u;
constexpr std::uintptr_t kOriginalClPredictParentSlotRva=0x374540u;
constexpr unsigned kOriginalConVarParentFieldOffset=0x18u;
constexpr unsigned kOriginalConVarNumericValueOffset=0x2Cu;
constexpr unsigned kOriginalPredictionEnabledOffset=0x10u;
constexpr unsigned kOriginalShutdownSlot=2u;
constexpr unsigned kOriginalResetSlot=7u;
constexpr unsigned kOriginalResetIndexOffset=0x24u;
constexpr unsigned kOriginalResetCountAOffset=0x28u;
constexpr unsigned kOriginalResetCountBOffset=0x2Cu;
constexpr unsigned kOriginalGetterSlot15Offset=0x08u;
constexpr unsigned kOriginalGetterSlot16Offset=0x09u;
constexpr unsigned kOriginalGetterSlot17Offset=0x0Cu;
constexpr unsigned kOriginalHostFlagSlot=8u;
constexpr unsigned kOriginalHostViewSlot=13u;
constexpr unsigned kOriginalHostFlagOffset=0x1DCu;
constexpr unsigned kOriginalHostViewOffset=0xF04u;
constexpr unsigned kOriginalPlayerGetterA=9u;
constexpr unsigned kOriginalPlayerSetterA=10u;
constexpr unsigned kOriginalPlayerGetterB=11u;
constexpr unsigned kOriginalPlayerSetterB=12u;
constexpr unsigned kOriginalPlayerVcallGetA=0xE4u;
constexpr unsigned kOriginalPlayerVcallSetA=0xE8u;
constexpr unsigned kOriginalPlayerVcallGetB=0xECu;
constexpr unsigned kOriginalPlayerVcallSetB=0xF0u;
constexpr unsigned kOriginalPlayerMirrorA=0x2C8u;
constexpr unsigned kOriginalPlayerMirrorB=0x2D4u;
constexpr unsigned kOriginalPlayerNotifierA=0x22Cu;
constexpr unsigned kOriginalPlayerNotifierB=0x268u;
constexpr unsigned kOriginalVirtualCount=23u;
constexpr unsigned kOriginalUpdateVirtualSlot=3u;
constexpr unsigned kOriginalPrePacketVirtualSlot=4u;
constexpr unsigned kOriginalPostPacketVirtualSlot=5u;
constexpr unsigned kOriginalPostNetworkVirtualSlot=6u;
constexpr std::uintptr_t kOriginalPostNetworkMethodVa=0x241960E0u;
constexpr unsigned kOriginalPostNetworkMethodBytes=1039u;
constexpr unsigned kOriginalPostNetworkStackPopBytes=4u;
constexpr unsigned kOriginalPostNetworkInputMemberOffset=0x2Cu;
constexpr unsigned kOriginalPostNetworkAnyReconciledMemberOffset=0x30u;
constexpr unsigned kOriginalEntityPredictableFlagOffset=0x2E8u;
constexpr unsigned kOriginalPacketLastArg2MemberOffset=0x0Cu;
constexpr unsigned kSyntheticPacketMaxEntities=4096u;
constexpr unsigned kOriginalInnerVirtualSlot=22u;
constexpr unsigned kOriginalUpdateLastIndexOffset=0x24u;
constexpr unsigned kOriginalUpdateScratchDwords=11u;
constexpr unsigned kOriginalUpdateNativeStackPopBytes=16u;
constexpr unsigned kOriginalUpdateInnerVirtualOffset=0x58u;

// Only invented ConVar storage is accepted. Original ConVar constructor
// sets this+0x18 (m_pParent) to self; the integer m_nValue is +0x2C.
struct ConVarView {
    std::array<std::uint8_t,0x18u> opaque_prefix{};
    const ConVarView* parent=nullptr;
    std::array<std::uint8_t,0x10u> opaque_middle{};
    std::int32_t numeric_value=0;
};
static_assert(offsetof(ConVarView,parent)==0x18u,
              "synthetic cl_predict ConVar parent pointer offset drift");
static_assert(offsetof(ConVarView,numeric_value)==0x2Cu,
              "synthetic cl_predict integer m_nValue offset drift");
static_assert(kOriginalClPredictConVarRva+
              kOriginalConVarParentFieldOffset==
              kOriginalClPredictParentSlotRva,
              "original cl_predict pointer RVA and parent offset drift");

// The retail prediction host pointer is a separate original client
// global at RVA 0x338CF0, not CPrediction.this. For original-free tests,
// construct a fully owned memory image with the two directly read fields.
struct ViewBits3 {std::uint32_t x=0u,y=0u,z=0u;};
static_assert(sizeof(ViewBits3)==12u,"3 original view DWORDs");
struct HostSnapshot {
    std::array<std::uint8_t,0x1DCu> reserved_first{};
    std::uint32_t field1dc=0u;
    std::array<std::uint8_t,0xF04u-0x1E0u> reserved_between{};
    ViewBits3 view{};
};
static_assert(offsetof(HostSnapshot,field1dc)==kOriginalHostFlagOffset,
              "native prediction host slot8 field offset drift");
static_assert(offsetof(HostSnapshot,view)==kOriginalHostViewOffset,
              "native prediction host slot13 vector offset drift");

// Artificial callbacks represent only the original C_BasePlayer virtual
// methods and embedded notifier at authentic byte offsets. They are NOT
// original player objects or claims about the engine's object layouts.
// The original setter callback executes BEFORE mirror fields are copied,
// and the notifier callback executes AFTER that copy.
struct OrientationOps {
    void* user=nullptr;
    using Getter=const ViewBits3* (*)(void*) noexcept;
    using Setter=void (*)(void*,ViewBits3*) noexcept;
    using Mirror=void (*)(void*,const ViewBits3*) noexcept;
    using Notify=void (*)(void*) noexcept;
    Getter get_a=nullptr;
    Setter set_a=nullptr;
    Getter get_b=nullptr;
    Setter set_b=nullptr;
    Mirror mirror_a=nullptr;
    Mirror mirror_b=nullptr;
    Notify notify_a=nullptr;
    Notify notify_b=nullptr;
};

// Phase1664: original CPrediction::Update(vtable[3]) saves and later
// restores exactly 11 DWORDs from the memory pointed to by the original
// global at RVA 0x2FA458. This is a SYNTHETIC ownership-safe stand-in.
// Importantly, the original code reloads the global pointer AFTER
// calling vtable[22]. The callback is allowed to switch `scratch`
// between two host-owned arrays, which we exercise in native tests.
struct UpdateFrameContext {
    std::array<std::uint32_t,11>* scratch=nullptr;
    std::int32_t update_gate_convar=1; // original ConVar +0x24374780 value
    std::int32_t cl_predict_convar=1;  // original ConVar +0x24374540 value
    void* user=nullptr;
    void (*on_scope_enter)(void*) noexcept=nullptr;
    void (*on_scope_exit)(void*) noexcept=nullptr;
    void (*on_inner_slot22)(void*,std::uint32_t,std::uint32_t,
                            std::uint32_t,std::uint32_t) noexcept=nullptr;
};
static_assert(sizeof(std::array<std::uint32_t,11>)==44u,
              "native prediction Update REP MOVSD copies eleven DWORDs");

// Phase1665: original CPrediction::PreEntityPacketReceived (slot4,
// RET8) and PostEntityPacketReceived (slot5, RET0) iterate the same
// client-wide entity list. This host-owned facade stands in for
// CClientEntityList virtual [0]=Get(index) and [1]=Count(), and
// C_BaseEntity::IsPredictable (native this+0x2E8 byte). Entity
// storage never refers to original DLL game memory in public CI.
struct PacketEntityOps {
    void* user=nullptr;
    std::int32_t cl_predict=1;
    bool local_player_present=false;
    std::int32_t (*get_count)(void*) noexcept=nullptr;
    void* (*get_entity)(void*,std::int32_t) noexcept=nullptr;
    bool (*is_predictable)(void*,void*) noexcept=nullptr;
    void (*pre_entity)(void*,void*,std::uint32_t) noexcept=nullptr;
    void (*post_entity)(void*,void*) noexcept=nullptr;
    // Exact original 0x24195720 fallback is NOT reconstructed.
    void (*pre_disabled_fallback)(void*) noexcept=nullptr;
    void (*scope_enter)(void*,unsigned) noexcept=nullptr;
    void (*scope_exit)(void*,unsigned) noexcept=nullptr;
};
// Phase1668 PARTIAL entry/reconciliation contract for the enormous
// original 1,039-byte CPrediction::PostNetworkDataReceived vtable slot6.
// Source analysis verifies the entry bookkeeping, player/cl_predict
// guards, one client entity-list Count, ascending Get(index), first
// IsPredictable and 0x240C6900(frame) per eligible entity. All later
// diagnostic/correction and OnPredictionChanged branches are OPAQUE.
// This host-only callback table does NOT implement those branches,
// native game prediction or any actual Source engine entity object.
struct PostNetworkOps {
    void* user=nullptr;
    bool local_player_present=false;
    std::int32_t cl_predict=1;
    std::int32_t cl_pred_optimize=0;
    std::int32_t (*get_count)(void*) noexcept=nullptr;
    void* (*get_entity)(void*,std::int32_t) noexcept=nullptr;
    bool (*is_predictable)(void*,void*) noexcept=nullptr;
    std::uint8_t (*reconcile_entity)(void*,void*,std::uint32_t) noexcept=nullptr;
    void (*on_opaque_entry)(void*) noexcept=nullptr;
    void (*on_opaque_remaining_branches)(void*) noexcept=nullptr;
    void (*scope_enter)(void*,unsigned) noexcept=nullptr;
    void (*scope_exit)(void*,unsigned) noexcept=nullptr;
};

struct Events {
    unsigned init_calls=0u;
    unsigned missing_provider_refusals=0u;
    unsigned destructor_calls=0u;
    unsigned missing_output_refusals=0u;
    unsigned missing_orientation_callbacks=0u;
    unsigned missing_orientation_inputs=0u;
    unsigned missing_view_setter_inputs=0u;
    unsigned missing_update_scratch=0u;
    unsigned missing_update_restore_target=0u;
    unsigned opaque_update_inner_calls=0u;
    unsigned missing_packet_context=0u;
    unsigned refused_packet_entity_backend=0u;
    unsigned refused_packet_entity_count=0u;
    unsigned missing_postnetwork_context=0u;
    unsigned refused_postnetwork_backend=0u;
    unsigned refused_postnetwork_entity_count=0u;
    unsigned postnetwork_reconcile_calls=0u;
    unsigned postnetwork_unmodeled_tail_calls=0u;
};
inline Events events{};

// MSVC slot0 deletes, slot1 Init copies cl_predict effective numeric
// value, slot2 on engine shutdown is an empty RET and slot7 resets three
// prediction state DWORDs. Slots15/16/17 are tiny native field getters;
// the other slots remain inert sentinels. This fixture is not a
// registerable implementation of IPrediction.
class Prediction23Fixture {
public:
    explicit Prediction23Fixture(const ConVarView* cl_predict) noexcept
        : cl_predict_(cl_predict) {}
    virtual ~Prediction23Fixture() noexcept { ++events.destructor_calls; }

    virtual void __thiscall InitializeFromEngine() noexcept {
        ++events.init_calls;
        if(!cl_predict_ || !cl_predict_->parent) {
            ++events.missing_provider_refusals;
            return;
        }
        enabled_=cl_predict_->parent->numeric_value!=0 ? 1u : 0u;
    }
    // Retail slot2 is a literal RET with no side effects; engine invokes
    // it during client shutdown through [vptr+8].
    virtual void __thiscall ShutdownNoop() noexcept {}
    // Retail CPrediction::Update vtable[3], 164 native bytes,
    // __thiscall RET16: four stack DWORD arguments. Enter profile,
    // compare frame ID at this+0x24, assign new frame, copy 11 DWORDs
    // from *global scratch, dispatch vtable[22] with original arg
    // ordering, reload scratch global, restore 11 DWORDs and exit
    // profile scope. The original does not guard a null scratch ptr.
    virtual void __thiscall UpdateFrame(std::uint32_t frame,
                                        std::uint32_t arg2,
                                        std::uint32_t arg3,
                                        std::uint32_t arg4) noexcept {
        if(!update_context_ || !update_context_->scratch) {
            ++events.missing_update_scratch;
            return; // synthetic-only safety refusal
        }
        auto* const ctx=update_context_;
        if(ctx->on_scope_enter)ctx->on_scope_enter(ctx->user);
        last_update_gate_flag_=
            static_cast<std::uint32_t>(highest_)!=frame ||
            ctx->update_gate_convar==0 ||
            ctx->cl_predict_convar==0;
        highest_=static_cast<std::int32_t>(frame);
        const auto copied_scratch=*ctx->scratch;
        // The first forwarded value is the first SAVED work DWORD,
        // not the frame argument. The remaining args are arg2..arg4.
        this->RunCoreFrameOpaque(copied_scratch[0],arg2,arg3,arg4);
        if(ctx->scratch) {
            *ctx->scratch=copied_scratch;
        } else {
            // The original re-read would crash on null; fail closed
            // rather than writing to any stale/unknown original ptr.
            ++events.missing_update_restore_target;
        }
        if(ctx->on_scope_exit)ctx->on_scope_exit(ctx->user);
    }
    // Retail CPrediction::PreEntityPacketReceived slot4 RET8.
    // The original stores arg2 in this+0xC before testing cl_predict.
    // If disabled it calls an opaque original native fallback
    // 0x24195720. Otherwise, when local player exists, it snapshots
    // entity count and calls +0x240C5510(arg1) for every nonnull
    // predictable entity (C_BaseEntity +0x2E8 byte != 0).
    virtual void __thiscall PreEntityPacketReceived(
            std::uint32_t arg1,std::uint32_t arg2) noexcept {
        if(!packet_context_) {
            ++events.missing_packet_context;
            return; // safe synthetic refusal; original derefs globals
        }
        auto* const ctx=packet_context_;
        if(ctx->scope_enter)ctx->scope_enter(ctx->user,4u);
        memberC_=arg2;
        if(ctx->cl_predict==0) {
            if(ctx->pre_disabled_fallback)
                ctx->pre_disabled_fallback(ctx->user);
        } else if(ctx->local_player_present) {
            const bool backend=
                ctx->get_count && ctx->get_entity &&
                ctx->is_predictable && ctx->pre_entity;
            if(!backend) {
                ++events.refused_packet_entity_backend;
            } else {
                const auto n=ctx->get_count(ctx->user);
                if(n<0 || n>static_cast<std::int32_t>(
                        kSyntheticPacketMaxEntities)) {
                    ++events.refused_packet_entity_count;
                } else for(std::int32_t i=0;i<n;++i) {
                    void* const entity=ctx->get_entity(ctx->user,i);
                    if(entity && ctx->is_predictable(ctx->user,entity))
                        ctx->pre_entity(ctx->user,entity,arg1);
                }
            }
        }
        if(ctx->scope_exit)ctx->scope_exit(ctx->user,4u);
    }
    // Retail PostEntityPacketReceived slot5 has no explicit args
    // and tail-JUMPs to CVProfile::ExitScope, cleaning no args.
    // When cl_predict is disabled or local player absent, it does
    // not iterate. Otherwise callback +0x240C68C0 for each eligible.
    virtual void __thiscall PostEntityPacketReceived() noexcept {
        if(!packet_context_) {
            ++events.missing_packet_context;
            return;
        }
        auto* const ctx=packet_context_;
        if(ctx->scope_enter)ctx->scope_enter(ctx->user,5u);
        if(ctx->cl_predict!=0 && ctx->local_player_present) {
            const bool backend=
                ctx->get_count && ctx->get_entity &&
                ctx->is_predictable && ctx->post_entity;
            if(!backend) {
                ++events.refused_packet_entity_backend;
            } else {
                const auto n=ctx->get_count(ctx->user);
                if(n<0 || n>static_cast<std::int32_t>(
                        kSyntheticPacketMaxEntities)) {
                    ++events.refused_packet_entity_count;
                } else for(std::int32_t i=0;i<n;++i) {
                    void* const entity=ctx->get_entity(ctx->user,i);
                    if(entity && ctx->is_predictable(ctx->user,entity))
                        ctx->post_entity(ctx->user,entity);
                }
            }
        }
        if(ctx->scope_exit)ctx->scope_exit(ctx->user,5u);
    }
    // Retail CPrediction::PostNetworkDataReceived slot6 takes ONE
    // stack DWORD, returns RET4, and is 1039 original bytes long.
    // This diagnostic implements ONLY the source-authenticated entry
    // and entity-reconciliation prefix. The actual retail function
    // continues into much more complex diagnostics and correction
    // logic which is not reconstructed or executed here.
    virtual void __thiscall PostNetworkDataReceived(
            std::uint32_t input_frame) noexcept {
        if(!postnetwork_context_) {
            ++events.missing_postnetwork_context;
            return; // safe-only refusal, not original native behavior
        }
        auto* const ctx=postnetwork_context_;
        if(ctx->scope_enter)ctx->scope_enter(ctx->user,6u);
        if(ctx->on_opaque_entry)
            ctx->on_opaque_entry(ctx->user);
        // Original this+0x2C and this+0x30 always written before
        // checking the local C_BasePlayer and cl_predict.
        cleared_b_=input_frame;
        post_network_any_reconciled_=0u;
        if(ctx->local_player_present && ctx->cl_predict!=0) {
            if(!ctx->get_count || !ctx->get_entity ||
               !ctx->is_predictable || !ctx->reconcile_entity) {
                ++events.refused_postnetwork_backend;
            } else {
                const auto n=ctx->get_count(ctx->user);
                if(n<0 || n>static_cast<std::int32_t>(
                        kSyntheticPacketMaxEntities)) {
                    // Original Count() result is unguarded. A fake
                    // backend reporting an unsafe count is refused.
                    ++events.refused_postnetwork_entity_count;
                } else for(std::int32_t i=0;i<n;++i) {
                    void* const entity=ctx->get_entity(ctx->user,i);
                    if(!entity || !ctx->is_predictable(ctx->user,entity))
                        continue;
                    ++events.postnetwork_reconcile_calls;
                    if(ctx->reconcile_entity(
                           ctx->user,entity,input_frame)!=0u)
                        post_network_any_reconciled_=1u;
                }
            }
        }
        // Crucially, the original function does NOT end here:
        // per-entity debug/error text, several optional correction
        // paths and prediction enable/disable reset still follow.
        // Mark this unknown boundary rather than inventing effects.
        ++events.postnetwork_unmodeled_tail_calls;
        if(ctx->on_opaque_remaining_branches)
            ctx->on_opaque_remaining_branches(ctx->user);
        if(ctx->scope_exit)ctx->scope_exit(ctx->user,6u);
    }
    // Retail slot7 is 16 bytes, zeroing [+0x28] and [+0x2C] and
    // setting [+0x24] to -1. No arguments, RET 0.
    virtual void __thiscall ResetBookkeeping() noexcept {
        cleared_a_=0u;
        cleared_b_=0u;
        highest_=-1;
    }
    // Retail slot8 reads client-global host +0x1DC, or returns zero
    // when the global host is absent. No arguments, RET0.
    virtual std::uint32_t __thiscall GetHostField1DC() noexcept {
        return view_host_?view_host_->field1dc:0u;
    }
    // Retail getter slots9/11: if no active C_BasePlayer, zero three
    // output DWORDs. Otherwise call player vptr +0xE4/+0xEC for the
    // returned pointer and copy three words into caller's OUT buffer.
    // The safe synthetic fixture refuses NULL output or returned vector.
    virtual void __thiscall CopyOrientationA(ViewBits3* out) noexcept {
        if(!out) {++events.missing_output_refusals;return;}
        *out=ViewBits3{};
        if(!orientation_.user)return;
        if(!orientation_.get_a) {
            ++events.missing_orientation_callbacks;return;
        }
        const auto* src=orientation_.get_a(orientation_.user);
        if(!src) {
            ++events.missing_orientation_callbacks;return;
        }
        *out=*src;
    }
    // Retail setter slot10: if player present, call player vptr +0xE8
    // FIRST, copy three (possibly mutated) caller DWORDs into player
    // +0x2C8/+0x2CC/+0x2D0, THEN invoke embedded player+0x22C
    // notifier's vptr +0x0C. Caller stack cleanup is RET4.
    virtual void __thiscall SetOrientationA(ViewBits3* in) noexcept {
        if(!orientation_.user)return;
        if(!in) {++events.missing_orientation_inputs;return;}
        if(!orientation_.set_a || !orientation_.mirror_a ||
           !orientation_.notify_a) {
            ++events.missing_orientation_callbacks;return;
        }
        orientation_.set_a(orientation_.user,in);
        orientation_.mirror_a(orientation_.user,in);
        orientation_.notify_a(orientation_.user);
    }
    virtual void __thiscall CopyOrientationB(ViewBits3* out) noexcept {
        if(!out) {++events.missing_output_refusals;return;}
        *out=ViewBits3{};
        if(!orientation_.user)return;
        if(!orientation_.get_b) {
            ++events.missing_orientation_callbacks;return;
        }
        const auto* src=orientation_.get_b(orientation_.user);
        if(!src) {
            ++events.missing_orientation_callbacks;return;
        }
        *out=*src;
    }
    // Retail setter slot12: player vptr +0xF0; 3 mirrored DWORDs
    // player+0x2D4/+0x2D8/+0x2DC; notifier at player+0x268.
    virtual void __thiscall SetOrientationB(ViewBits3* in) noexcept {
        if(!orientation_.user)return;
        if(!in) {++events.missing_orientation_inputs;return;}
        if(!orientation_.set_b || !orientation_.mirror_b ||
           !orientation_.notify_b) {
            ++events.missing_orientation_callbacks;return;
        }
        orientation_.set_b(orientation_.user,in);
        orientation_.mirror_b(orientation_.user,in);
        orientation_.notify_b(orientation_.user);
    }
    // Retail slot13 writes 12 bytes into its single caller-provided
    // output pointer; when host global is missing, writes all zero.
    // The independent fixture rejects an absent output pointer instead
    // of reproducing the retail null-destination crash.
    virtual void __thiscall CopyHostView(ViewBits3* out) noexcept {
        if(!out) {
            ++events.missing_output_refusals;
            return;
        }
        *out=view_host_?view_host_->view:ViewBits3{};
    }
    // Retail CPrediction slot14 is the 18-byte client-global player
    // guard and an x86 tail JMP to nonvirtual C_BasePlayer
    // 0x240D22E0. The player method copies the supplied three DWORDs
    // into the SAME +0xF04/+0xF08/+0xF0C fields read by slot13.
    // The safe original-free fixture refuses NULL source pointers.
    virtual void __thiscall SetHostView(ViewBits3* input) noexcept {
        if(!writable_view_host_)return;
        if(!input) {
            ++events.missing_view_setter_inputs;
            return;
        }
        writable_view_host_->view=*input;
    }
    // Retail slots15/16 read individual BYTEs at +8/+9 via AL,
    // and slot17 reads a DWORD at +0xC via EAX. All are RET0.
    virtual std::uint8_t __thiscall GetMember8() noexcept {return member8_;}
    virtual std::uint8_t __thiscall GetMember9() noexcept {return member9_;}
    virtual std::uint32_t __thiscall GetMemberC() noexcept {return memberC_;}
    virtual std::uint32_t __thiscall UnknownSlot18() noexcept {return 0xA012u;}
    virtual std::uint32_t __thiscall UnknownSlot19() noexcept {return 0xA013u;}
    virtual std::uint32_t __thiscall UnknownSlot20() noexcept {return 0xA014u;}
    virtual std::uint32_t __thiscall UnknownSlot21() noexcept {return 0xA015u;}
    // The full retail 0x241977B0 slot22 body is still unknown.
    // Only the four caller stack words used by the source-attested
    // Update slot3 callsite are implemented in synthetic C++.
    virtual void __thiscall RunCoreFrameOpaque(
            std::uint32_t saved_work_word0,std::uint32_t arg2,
            std::uint32_t arg3,std::uint32_t arg4) noexcept {
        ++events.opaque_update_inner_calls;
        if(update_context_ && update_context_->on_inner_slot22)
            update_context_->on_inner_slot22(update_context_->user,
                saved_work_word0,arg2,arg3,arg4);
    }

    std::uint8_t enabled() const noexcept {return enabled_;}
    std::uintptr_t enabled_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&enabled_);
    }
    void set_cl_predict(const ConVarView* cl_predict) noexcept {
        cl_predict_=cl_predict;
    }
    void set_orientation_ops(OrientationOps ops) noexcept {
        orientation_=ops;
    }
    void set_view_host(const HostSnapshot* host) noexcept {
        view_host_=host;
        // Switching to a read-only attachment revokes any previous
        // synthetic write authority. Never retain a dangling mutable
        // pointer when an owner chooses the original Phase1659 API.
        writable_view_host_=nullptr;
    }
    void set_postnetwork_ops(PostNetworkOps* ctx) noexcept {
        postnetwork_context_=ctx;
    }
    std::uint32_t postnetwork_any_reconciled() const noexcept {
        return post_network_any_reconciled_;
    }
    std::uintptr_t postnetwork_any_reconciled_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(
            &post_network_any_reconciled_);
    }
    void set_packet_entity_ops(PacketEntityOps* ctx) noexcept {
        packet_context_=ctx;
    }
    void set_update_frame_context(UpdateFrameContext* ctx) noexcept {
        update_context_=ctx;
    }
    bool last_update_gate_flag_for_test_only() const noexcept {
        return last_update_gate_flag_;
    }
    void set_writable_host_view(HostSnapshot* host) noexcept {
        // Explicit read/write ownership is separate from the Phase1659
        // read-only host attachment; cannot cast away its constness.
        writable_view_host_=host;
        view_host_=host;
    }
    void prime_early_fields(std::uint8_t a,std::uint8_t b,
                            std::uint32_t c) noexcept {
        member8_=a;
        member9_=b;
        memberC_=c;
    }
    std::uintptr_t member8_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&member8_);
    }
    std::uintptr_t member9_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&member9_);
    }
    std::uintptr_t memberC_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&memberC_);
    }
    void prime_bookkeeping(std::int32_t highest,
                           std::uint32_t a,std::uint32_t b) noexcept {
        highest_=highest;
        cleared_a_=a;
        cleared_b_=b;
    }
    std::int32_t highest() const noexcept {return highest_;}
    std::uint32_t value_a() const noexcept {return cleared_a_;}
    std::uint32_t value_b() const noexcept {return cleared_b_;}
    std::uintptr_t highest_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&highest_);
    }
    std::uintptr_t value_a_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&cleared_a_);
    }
    std::uintptr_t value_b_address() const noexcept {
        return reinterpret_cast<std::uintptr_t>(&cleared_b_);
    }
private:
    // PE32 native vfptr occupies this+0..+3. The next 12 bytes
    // preserve actual getter member positions at +8/+9/+0xC.
    std::array<std::uint8_t,4> opaque4to7_{};
    std::uint8_t member8_=0u;
    std::uint8_t member9_=0u;
    std::array<std::uint8_t,2> opaqueAtoB_{};
    std::uint32_t memberC_=0u;
    std::uint8_t enabled_=0u;
    const ConVarView* cl_predict_=nullptr; // PE32 native this+0x14
    std::array<std::uint8_t,12> unknown_prediction_fields_{};
    std::int32_t highest_=-1;
    std::uint32_t cleared_a_=0u;
    std::uint32_t cleared_b_=0u;
    // Original PostNetwork slot6 writes this+0x30. Prior synthetic
    // fixture put view_host_ here, which would be overwritten by
    // authentic post-network bookkeeping. Preserve native layout.
    std::uint32_t post_network_any_reconciled_=0u;
    const HostSnapshot* view_host_=nullptr; // diagnostic host pointer only
    HostSnapshot* writable_view_host_=nullptr; // synthetic owned write only
    OrientationOps orientation_{}; // synthetic callback contract only
    UpdateFrameContext* update_context_=nullptr; // invented owner only
    bool last_update_gate_flag_=true;
    PacketEntityOps* packet_context_=nullptr; // invented owner only
    PostNetworkOps* postnetwork_context_=nullptr; // invented owner only
};

static_assert(sizeof(void*)==4u,"original 2006 PE32 pointer width");
static_assert(std::has_virtual_destructor<Prediction23Fixture>::value,
              "original prediction vtable slot0 is a deleting destructor");
using Slot1Signature=void (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::InitializeFromEngine),
    Slot1Signature>::value,
    "original prediction slot1 is parameterless MSVC thiscall");
using NoArgVoid=void (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::ShutdownNoop),NoArgVoid>::value,
    "original prediction slot2 shutdown is parameterless RET0");
static_assert(std::is_same<
    decltype(&Prediction23Fixture::ResetBookkeeping),NoArgVoid>::value,
    "original prediction slot7 reset is parameterless RET0");
using ByteGetter=std::uint8_t (__thiscall Prediction23Fixture::*)() noexcept;
using DwordGetter=std::uint32_t (__thiscall Prediction23Fixture::*)() noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::GetMember8),
              ByteGetter>::value &&
              std::is_same<decltype(&Prediction23Fixture::GetMember9),
              ByteGetter>::value &&
              std::is_same<decltype(&Prediction23Fixture::GetMemberC),
              DwordGetter>::value,
              "native prediction slots15/16/17 byte/word getter signatures");
using HostFlagGetter=std::uint32_t (__thiscall Prediction23Fixture::*)() noexcept;
using HostVectorGetter=void (__thiscall Prediction23Fixture::*)(ViewBits3*) noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::GetHostField1DC),
              HostFlagGetter>::value &&
              std::is_same<decltype(&Prediction23Fixture::CopyHostView),
              HostVectorGetter>::value,
              "original prediction slot8 RET0 / slot13 RET4 x86 contracts");
using OrientationRef=void (__thiscall Prediction23Fixture::*)(ViewBits3*) noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::CopyOrientationA),
              OrientationRef>::value &&
              std::is_same<decltype(&Prediction23Fixture::SetOrientationA),
              OrientationRef>::value &&
              std::is_same<decltype(&Prediction23Fixture::CopyOrientationB),
              OrientationRef>::value &&
              std::is_same<decltype(&Prediction23Fixture::SetOrientationB),
              OrientationRef>::value,
              "native prediction slots9-12 each take one Vec3 pointer RET4");
static_assert(std::is_same<decltype(&Prediction23Fixture::SetHostView),
              OrientationRef>::value,
              "native CPrediction slot14 takes one Vec3 pointer, RET4");
using UpdateFourArgs=void (__thiscall Prediction23Fixture::*)(
    std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) noexcept;
static_assert(std::is_same<decltype(&Prediction23Fixture::UpdateFrame),
              UpdateFourArgs>::value &&
              std::is_same<decltype(&Prediction23Fixture::RunCoreFrameOpaque),
              UpdateFourArgs>::value,
              "original CPrediction Update slot3 / inner slot22 are four-arg RET16");
static_assert(kOriginalUpdateScratchDwords*4u==44u &&
              kOriginalInnerVirtualSlot*4u==kOriginalUpdateInnerVirtualOffset,
              "original x86 REP MOVSD/slot22 source-authenticated counts");
using PacketTwoArgs=void (__thiscall Prediction23Fixture::*)(
    std::uint32_t,std::uint32_t) noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::PreEntityPacketReceived),
    PacketTwoArgs>::value,
    "original PreEntityPacketReceived slot4 is two-arg RET8");
static_assert(std::is_same<
    decltype(&Prediction23Fixture::PostEntityPacketReceived),
    NoArgVoid>::value,
    "original PostEntityPacketReceived slot5 is no-arg RET0");
using PostNetworkOneArg=void (__thiscall Prediction23Fixture::*)(
    std::uint32_t) noexcept;
static_assert(std::is_same<
    decltype(&Prediction23Fixture::PostNetworkDataReceived),
    PostNetworkOneArg>::value,
    "original PostNetworkDataReceived slot6 is one-argument RET4");

} // namespace client_prediction23_diagnostic
} // namespace sin_independent
