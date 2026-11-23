#define SIN_ELITE_SOURCE_GATE_C_EXPORT
#include "sin_compat/elite_native_source_gate_phase630.h"
#include "sin_compat/original_elite_event43_chaingun_gate_phase629.hpp"
#include <cstdint>
namespace source=sin_compat::original_elite_event43_chaingun_gate_phase629;
namespace {
bool valid(const sin_elite_source_gate_callbacks_v1* c,
           const sin_elite_source_gate_result_v1* out)noexcept {
  return c && out && c->version==SIN_ELITE_SOURCE_GATE_ABI_V1 &&
    c->struct_size==sizeof(*c) &&
    out->version==SIN_ELITE_SOURCE_GATE_ABI_V1 &&
    out->struct_size==sizeof(*out) &&
    c->read_current_serial_handle_fdc && c->resolve_serial_handle &&
    c->read_resolved_state && c->write_phase_594 &&
    c->write_time_590 && c->write_time_58c &&
    c->virtual_3c4 && c->virtual_3c8 &&
    c->read_original_engine_time &&
    c->publish_elite_fd9 && c->forward_inherited_event43;
}
void reset(sin_elite_source_gate_result_v1* out)noexcept {
  out->source_status=SIN_ELITE_SOURCE_NOT_INVOKED_V1;
  out->failed_stage=0;
  out->phase_before=0u;
  out->phase_after=0u;
  out->current_fdc=SIN_ELITE_HANDLE_INVALID_V1;
  out->target_valid=0;
}
struct Context {
  const sin_elite_source_gate_callbacks_v1& cb;
  void* userdata;
  sin_elite_source_gate_result_v1& out;
  std::uint64_t token=0u;
  explicit Context(const sin_elite_source_gate_callbacks_v1& callbacks,
                   void* u,sin_elite_source_gate_result_v1& result)noexcept
    :cb(callbacks),userdata(u),out(result){}
};
class Host final:public source::IHost {
public:
  explicit Host(Context& ctx)noexcept:ctx_(ctx){}
  bool native_virtual_3c4(std::uint32_t command,
                          std::uint32_t zero)override {
    return ctx_.cb.virtual_3c4(
      ctx_.userdata,ctx_.token,command,zero)==1;
  }
  bool native_virtual_3c8(std::uint32_t command)override {
    return ctx_.cb.virtual_3c8(
      ctx_.userdata,ctx_.token,command)==1;
  }
  bool read_original_engine_time(float& time)override {
    return ctx_.cb.read_original_engine_time(ctx_.userdata,&time)==1;
  }
  bool write_original_phase_594(std::uint32_t phase)override {
    return ctx_.cb.write_phase_594(
      ctx_.userdata,ctx_.token,phase)==1;
  }
  bool write_original_time_590(float time)override {
    return ctx_.cb.write_time_590(
      ctx_.userdata,ctx_.token,time)==1;
  }
  bool write_original_time_58c(float time)override {
    return ctx_.cb.write_time_58c(
      ctx_.userdata,ctx_.token,time)==1;
  }
private:
  Context& ctx_;
};
bool load_state(Context& ctx,source::State& state) {
  if(ctx.token==0u) {
    ctx.out.source_status=SIN_ELITE_SOURCE_TARGET_MISSING_V1;
    return false;
  }
  const int got=ctx.cb.read_resolved_state(
      ctx.userdata,ctx.token,&state.original_phase_594,
      &state.original_time_590,&state.original_time_58c);
  if(got!=1) {
    ctx.out.source_status=SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1;
    return false;
  }
  ctx.out.phase_before=state.original_phase_594;
  ctx.out.phase_after=state.original_phase_594;
  ctx.out.target_valid=1;
  return true;
}
int32_t run_original_gate(Context& ctx) {
  source::State state{};
  if(!load_state(ctx,state))return -1;
  Host host(ctx);
  source::Reference model(host);
  const source::Result result=model.gate(state,true);
  ctx.out.failed_stage=static_cast<std::int32_t>(result.failed_stage);
  ctx.out.phase_after=state.original_phase_594;
  if(!result.native_call_accepted()) {
    ctx.out.source_status=SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1;
    return -1;
  }
  if(result.event_permitted()) {
    ctx.out.source_status=SIN_ELITE_SOURCE_GATE_ALLOWED_V1;
    return 1;
  }
  ctx.out.source_status=SIN_ELITE_SOURCE_GATE_BLOCKED_V1;
  return 0;
}
int32_t run_original_update(Context& ctx) {
  source::State state{};
  if(!load_state(ctx,state))
    return SIN_ELITE_NATIVE_HOST_REJECTED;
  Host host(ctx);
  source::Reference model(host);
  const source::Result result=model.update(state,true);
  ctx.out.failed_stage=static_cast<std::int32_t>(result.failed_stage);
  ctx.out.phase_after=state.original_phase_594;
  if(!result.native_call_accepted()) {
    ctx.out.source_status=SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1;
    return SIN_ELITE_NATIVE_HOST_REJECTED;
  }
  if(result.code==source::Code::Advanced) {
    ctx.out.source_status=SIN_ELITE_SOURCE_UPDATE_ADVANCED_V1;
    return SIN_ELITE_SOURCE_UPDATE_ADVANCED_V1;
  }
  ctx.out.source_status=SIN_ELITE_SOURCE_UPDATE_UNCHANGED_V1;
  return SIN_ELITE_SOURCE_UPDATE_UNCHANGED_V1;
}
std::int32_t native_read_handle(void* userdata,std::uint32_t* out) {
  auto& ctx=*static_cast<Context*>(userdata);
  const std::int32_t code=ctx.cb.read_current_serial_handle_fdc(
      ctx.userdata,out);
  if(code==1)ctx.out.current_fdc=*out;
  return code;
}
std::int32_t native_resolve(void* userdata,std::uint32_t handle,
                            std::uint64_t* out) {
  auto& ctx=*static_cast<Context*>(userdata);
  const std::int32_t code=ctx.cb.resolve_serial_handle(
      ctx.userdata,handle,out);
  if(code==1 && *out==0u) {
    ctx.out.source_status=SIN_ELITE_SOURCE_TARGET_MISSING_V1;
    return -1;
  }
  return code;
}
std::int32_t native_gate(void* userdata,std::uint64_t token) {
  auto& ctx=*static_cast<Context*>(userdata);
  ctx.token=token;
  return run_original_gate(ctx);
}
std::int32_t native_publish(void* userdata,std::uint8_t value) {
  auto& ctx=*static_cast<Context*>(userdata);
  return ctx.cb.publish_elite_fd9(ctx.userdata,value);
}
std::int32_t native_forward(void* userdata,float parameter) {
  auto& ctx=*static_cast<Context*>(userdata);
  return ctx.cb.forward_inherited_event43(ctx.userdata,parameter);
}
} // namespace
extern "C" {
std::uint32_t sin_elite_source_gate_abi_version_v1(void) {
  return SIN_ELITE_SOURCE_GATE_ABI_V1;
}
std::int32_t sin_elite_source_gate_event43_v1(
    sin_elite_native_registry_v1* registry,sin_elite_actor_v1 actor,
    float parameter,const sin_elite_source_gate_callbacks_v1* cb,
    void* userdata,sin_elite_source_gate_result_v1* out) {
  if(!registry || !valid(cb,out))return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  reset(out);
  try {
    Context ctx(*cb,userdata,*out);
    const sin_elite_native_event43_callbacks_v1 native{
      SIN_ELITE_NATIVE_ABI_V1,
      static_cast<std::uint32_t>(sizeof(sin_elite_native_event43_callbacks_v1)),
      native_read_handle,native_resolve,native_gate,native_publish,native_forward
    };
    return sin_elite_native_event43_v1(
        registry,actor,parameter,&native,&ctx);
  } catch(...) {
    out->source_status=SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1;
    return SIN_ELITE_NATIVE_HOST_REJECTED;
  }
}
std::int32_t sin_elite_source_gate_update_v1(
    sin_elite_native_registry_v1* registry,sin_elite_actor_v1 actor,
    const sin_elite_source_gate_callbacks_v1* cb,
    void* userdata,sin_elite_source_gate_result_v1* out) {
  if(!registry || !valid(cb,out))return SIN_ELITE_NATIVE_INVALID_ARGUMENT;
  reset(out);
  try {
    // Use the same Phase626 ownership and exact-generation validation,
    // including its spawn-completed guard. No private C++ registry access.
    sin_elite_native_state_v1 native{};
    native.version=SIN_ELITE_NATIVE_ABI_V1;
    native.struct_size=sizeof(native);
    const auto verified=sin_elite_native_snapshot_v1(
        registry,actor,&native);
    if(verified!=SIN_ELITE_NATIVE_OK)return verified;
    if(!native.spawn_completed)return SIN_ELITE_NATIVE_SPAWN_REQUIRED;
    Context ctx(*cb,userdata,*out);
    std::uint32_t handle=SIN_ELITE_HANDLE_INVALID_V1;
    if(cb->read_current_serial_handle_fdc(userdata,&handle)!=1) {
      out->source_status=SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1;
      return SIN_ELITE_NATIVE_HOST_REJECTED;
    }
    out->current_fdc=handle;
    if(handle==SIN_ELITE_HANDLE_INVALID_V1) {
      out->source_status=SIN_ELITE_SOURCE_TARGET_MISSING_V1;
      return SIN_ELITE_NATIVE_HOST_REJECTED;
    }
    std::uint64_t resolved=0u;
    if(cb->resolve_serial_handle(userdata,handle,&resolved)!=1 ||
       resolved==0u) {
      out->source_status=SIN_ELITE_SOURCE_TARGET_MISSING_V1;
      return SIN_ELITE_NATIVE_HOST_REJECTED;
    }
    ctx.token=resolved;
    return run_original_update(ctx);
  } catch(...) {
    out->source_status=SIN_ELITE_SOURCE_NATIVE_CALLBACK_FAILED_V1;
    return SIN_ELITE_NATIVE_HOST_REJECTED;
  }
}
} // extern C
