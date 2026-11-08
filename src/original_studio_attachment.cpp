#include "sin_compat/original_studio_attachment.hpp"
#include <cmath>
#include <limits>

namespace sin_compat::jessica {
namespace {
using Code=OriginalStudioAttachmentCode;
bool finite(const WorldMatrix3x4& matrix) noexcept {
    for(float value:matrix.row_major) if(!std::isfinite(value))return false;
    return true;
}
std::optional<float> checked(double value) noexcept {
    if(!std::isfinite(value)||
       std::abs(value)>std::numeric_limits<float>::max())return std::nullopt;
    return static_cast<float>(value);
}
std::optional<TossVec3> transformed_origin(
    const WorldMatrix3x4& bone,const WorldMatrix3x4& attachment) noexcept {
    if(!finite(bone)||!finite(attachment))return std::nullopt;
    const auto& b=bone.row_major;
    const auto& a=attachment.row_major;
    const double px=a[3],py=a[7],pz=a[11];
    auto x=checked(double(b[0])*px+double(b[1])*py+double(b[2])*pz+b[3]);
    auto y=checked(double(b[4])*px+double(b[5])*py+double(b[6])*pz+b[7]);
    auto z=checked(double(b[8])*px+double(b[9])*py+double(b[10])*pz+b[11]);
    if(!x||!y||!z)return std::nullopt;
    return TossVec3{*x,*y,*z};
}
}
OriginalStudioAttachmentResult resolve_original_studio_attachment(
    IOriginalStudioAttachmentHost& host,WorldEntityId entity,
    std::uint32_t one_based_attachment_index) noexcept {
    try {
        const auto count=host.attachment_count(entity);
        if(!count)return {Code::MissingStudio,{}};
        if(one_based_attachment_index==0 || one_based_attachment_index>*count)
            return {Code::InvalidIndex,{}};
        const auto record=host.attachment_record(entity,one_based_attachment_index-1);
        if(!record)return {Code::MissingRecord,{}};
        const auto bone=host.bone_world_transform(entity,record->bone_index);
        if(!bone)return {Code::MissingBone,{}};
        if(!finite(*bone)||!finite(record->bone_local_matrix))
            return {Code::InvalidMatrix,{}};
        // At 0x220ED5B3, bit 0x100 is checked in the studio metadata flags.
        // If clear, original 0x22269520: boneWorld * localAttachment.
        if((record->flags & 0x100u)==0) {
            const auto combined=compose_world_matrices(*bone,record->bone_local_matrix);
            if(!combined)return {Code::InvalidMatrix,{}};
            return {Code::Composed,*combined};
        }
        // If set, original 0x22268FD0 selects the local translation column,
        // 0x22268B20 transforms the point by the bone matrix, 0x22269F60
        // makes identity orientation and 0x22269000 writes its position.
        const auto origin=transformed_origin(*bone,record->bone_local_matrix);
        if(!origin)return {Code::InvalidMatrix,{}};
        const auto aligned=make_local_angle_matrix({0,0,0},*origin);
        if(!aligned)return {Code::InvalidMatrix,{}};
        return {Code::WorldAligned,*aligned};
    }catch(...) {return {Code::HostRejected,{}};}
}
bool OriginalStudioNativeAttachmentAdapter::has_animating_interface(
    WorldEntityId parent) {
    return host_.attachment_count(parent).has_value();
}
std::optional<OriginalAttachmentPose>
OriginalStudioNativeAttachmentAdapter::get_attachment_pose(
    WorldEntityId parent,std::uint8_t slot) {
    const auto result=resolve_original_studio_attachment(host_,parent,slot);
    if(!result.successful())return std::nullopt;
    const auto angles=angles_from_world_matrix(result.matrix);
    if(!angles)return std::nullopt;
    const auto& m=result.matrix.row_major;
    return OriginalAttachmentPose{{m[3],m[7],m[11]},*angles};
}
} // namespace sin_compat::jessica
