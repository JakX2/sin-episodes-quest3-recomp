#include "sin_compat/original_pose_driver_first_pass.hpp"
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace sin_compat::jessica {
namespace {
using Code=OriginalPoseFirstPassCode;
constexpr std::size_t kMaxTracks=4096,kMaxCommands=1'000'000,kCommandStride=0x7c;
bool finite(float value) noexcept {return std::isfinite(value);}
bool finite(const TossVec3& v) noexcept {
    return finite(v.x)&&finite(v.y)&&finite(v.z);
}
bool finite(const OriginalPoseDriverCommand& c) noexcept {
    if(!finite(c.origin)||!finite(c.secondary)||!finite(c.influence)||
       !finite(c.weight)||!finite(c.limit)) return false;
    for(float f:c.values)if(!finite(f))return false;
    for(float f:c.quaternion)if(!finite(f))return false;
    return true;
}
bool finite(const OriginalPoseDriverTrack& t) noexcept {
    if(!finite(t.origin)||!finite(t.cap)||!finite(t.secondary)||
       !finite(t.influence)||!finite(t.type3_height)||
       !finite(t.type3_reset_marker))return false;
    for(float f:t.values)if(!finite(f))return false;
    for(float f:t.quaternion)if(!finite(f))return false;
    return true;
}
float blend(float old,float next,float alpha) noexcept {
    // Corresponds to old + (new-old)*weight. Native x87 precision may differ.
    return old+(next-old)*alpha;
}
std::uint32_t read_u32(const std::vector<std::uint8_t>& b,std::size_t i) noexcept {
    return std::uint32_t(b[i])|std::uint32_t(b[i+1])<<8|
           std::uint32_t(b[i+2])<<16|std::uint32_t(b[i+3])<<24;
}
float read_f32(const std::vector<std::uint8_t>& b,std::size_t i) noexcept {
    std::uint32_t bits=read_u32(b,i);float f=0;
    static_assert(sizeof(f)==sizeof(bits),"requires 32-bit float");
    std::memcpy(&f,&bits,sizeof(f));return f;
}
} // namespace
std::optional<OriginalPoseDriverCommand> parse_original_pose_command(
    const std::vector<std::uint8_t>& b,std::size_t offset) noexcept {
    if(offset>b.size() || b.size()-offset<kCommandStride)return std::nullopt;
    OriginalPoseDriverCommand c;
    c.type=read_u32(b,offset+0x04);
    c.selector=read_u32(b,offset+0x08);
    const std::uint32_t raw_index=read_u32(b,offset+0x10);
    std::memcpy(&c.track_index,&raw_index,4);
    for(std::size_t j=0;j<3;++j)c.values[j]=read_f32(b,offset+0x14+j*4);
    c.origin={read_f32(b,offset+0x20),read_f32(b,offset+0x24),
              read_f32(b,offset+0x28)};
    for(std::size_t j=0;j<4;++j)c.quaternion[j]=read_f32(b,offset+0x2c+j*4);
    c.secondary=read_f32(b,offset+0x50);
    c.influence=read_f32(b,offset+0x54);
    c.weight=read_f32(b,offset+0x58);
    c.limit=read_f32(b,offset+0x5c);
    c.optional_payload=read_u32(b,offset+0x60);
    return c;
}
OriginalPoseFirstPassResult apply_original_pose_driver_first_pass(
    std::size_t count,const std::vector<OriginalPoseCommandGroup>& groups,
    float height,IOriginalPoseQuaternionMixer* mixer,
    const std::vector<OriginalPoseDriverTrack>& prior) noexcept {
    try {
        if(count>kMaxTracks||(!prior.empty()&&prior.size()!=count))
            return {Code::InvalidTrack,{}};
        if(!finite(height))return {Code::InvalidFloat,{}};
        OriginalPoseFirstPassResult out{};
        out.tracks=prior.empty()?std::vector<OriginalPoseDriverTrack>(count):prior;
        for(auto& track:out.tracks) {
            // Original loop does NOT reset prior type/selector/auxiliary
            // fields, nor the type-3-only trailing metadata.
            track.origin={};track.quaternion={};
            track.cap=1.f;track.secondary=1.f;track.values={};
            track.influence=0.f;
        }
        for(const auto& group:groups) {
            if(group.size()>kMaxCommands-out.commands_seen)
                return {Code::InvalidCommand,{}};
            for(const auto& cmd:group) {
                ++out.commands_seen;
                // Original jump table has handlers only for 3..6; other
                // types increment the 0x7C command stride and are ignored.
                if(cmd.type<3||cmd.type>6) {++out.commands_ignored;continue;}
                if(cmd.track_index<0||static_cast<std::size_t>(cmd.track_index)>=count)
                    return {Code::InvalidTrack,{}};
                if(!finite(cmd))return {Code::InvalidFloat,{}};
                auto& track=out.tracks[static_cast<std::size_t>(cmd.track_index)];
                if(cmd.type==4||cmd.type==6) {
                    // Both types cap track+0x44 against (1-cmd+0x54)
                    // unless cmd+0x5C is zero, when it is cleared.
                    track.cap=cmd.limit==0.f?0.f:
                              std::fmin(track.cap,1.f-cmd.influence);
                    if(cmd.type==4)
                        track.influence*=1.f-cmd.weight*cmd.influence;
                }else{
                    track.selector=cmd.selector;
                    track.type=cmd.type;
                    track.optional_payload=cmd.type==5?cmd.optional_payload:0;
                    if(cmd.weight!=1.f&&track.influence!=0.f) {
                        if(!mixer)return {Code::MissingQuaternionMixer,{}};
                        const auto blended=mixer->mix(track.quaternion,cmd.quaternion,cmd.weight);
                        if(!blended)return {Code::QuaternionMixRejected,{}};
                        track.quaternion=*blended;
                        track.origin={blend(track.origin.x,cmd.origin.x,cmd.weight),
                                      blend(track.origin.y,cmd.origin.y,cmd.weight),
                                      blend(track.origin.z,cmd.origin.z,cmd.weight)};
                        // Native +0x4C/+0x50/+0x54 uses command
                        // +0x14/+0x1C/+0x18, respectively.
                        track.values[0]=blend(track.values[0],cmd.values[0],cmd.weight);
                        track.values[1]=blend(track.values[1],cmd.values[2],cmd.weight);
                        track.values[2]=blend(track.values[2],cmd.values[1],cmd.weight);
                        track.cap=std::fmin(track.cap,cmd.limit);
                        track.secondary=blend(track.secondary,cmd.secondary,cmd.weight);
                        track.influence=blend(track.influence,cmd.influence,cmd.weight);
                    }else {
                        track.quaternion=cmd.quaternion;
                        track.origin=cmd.origin;
                        track.values={cmd.values[0],cmd.values[2],cmd.values[1]};
                        track.cap=cmd.limit*cmd.weight;
                        track.secondary=cmd.secondary;
                        track.influence=cmd.influence*cmd.weight;
                    }
                    if(cmd.type==3){
                        track.type3_reset_marker=0.f;
                        track.type3_height=height+track.values[1];
                    }
                }
                if(!finite(track))return {Code::InvalidFloat,{}};
                ++out.commands_applied;
            }
        }
        out.code=Code::Applied;
        return out;
    }catch(...){return {Code::HostRejected,{}};}
}
} // namespace sin_compat::jessica
