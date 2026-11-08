#include "sin_compat/original_parent_attachment.hpp"
#include <cmath>
namespace sin_compat::jessica {
namespace {
bool finite_matrix(const WorldMatrix3x4& m) noexcept {
    for(const float x:m.row_major)if(!std::isfinite(x))return false;
    return true;
}
WorldMatrix3x4 identity_matrix() noexcept {
    WorldMatrix3x4 m{};
    m.row_major[0]=1;m.row_major[5]=1;m.row_major[10]=1;
    return m;
}
}
OriginalParentMatrixResult resolve_original_parent_matrix(
    IOriginalParentMatrixHost& host,WorldEntityId child) noexcept {
    using Route=OriginalParentMatrixRoute;
    try {
        const auto entry=host.binding(child);
        if(!entry)return {Route::MissingChild,{}};
        // Invalid/stale/nonexistent EHANDLE -> original 0x22269F60 identity.
        if(!entry->parent||!host.parent_is_live(*entry->parent))
            return {Route::Identity,identity_matrix()};
        const auto parent=*entry->parent;
        // Original attachment index +0xEC; parent animating vtable +0xC4.
        if(entry->attachment && host.parent_has_animating_interface(parent)) {
            const auto pose=host.attachment_pose(parent,entry->attachment);
            // 0x220ED470 returns false on attachment failure, even after
            // writing fallback parent position/angles. Discard those outputs.
            if(pose) {
                const auto matrix=make_local_angle_matrix(
                    pose->angles,pose->origin);
                if(!matrix)return {Route::InvalidTransform,{}};
                return {Route::AttachmentMatrix,*matrix};
            }
        }
        auto state=host.parent_world_state(parent);
        if(!state)return {Route::HostRejected,{}};
        if(state->dirty) {
            if(!host.refresh_parent_world(parent))
                return {Route::HostRejected,{}};
            state=host.parent_world_state(parent);
            if(!state||state->dirty)return {Route::HostRejected,{}};
        }
        if(!finite_matrix(state->matrix))return {Route::InvalidTransform,{}};
        return {Route::ParentMatrix,state->matrix};
    }catch(...){return {Route::HostRejected,{}};}
}
std::optional<OriginalParentBinding> OriginalParentMatrixWorldAdapter::binding(
    WorldEntityId id) {
    const auto s=world_.snapshot(id);
    if(!s)return std::nullopt;
    return OriginalParentBinding{s->parent,s->parent_attachment};
}
bool OriginalParentMatrixWorldAdapter::parent_is_live(WorldEntityId id) {
    return world_.snapshot(id).has_value();
}
bool OriginalParentMatrixWorldAdapter::parent_has_animating_interface(
    WorldEntityId id) {return attachments_.has_animating_interface(id);}
std::optional<OriginalAttachmentPose> OriginalParentMatrixWorldAdapter::attachment_pose(
    WorldEntityId id,std::uint8_t slot) {
    return attachments_.get_attachment_pose(id,slot);
}
std::optional<OriginalParentWorldState>
OriginalParentMatrixWorldAdapter::parent_world_state(WorldEntityId id) {
    const auto s=world_.snapshot(id);
    if(!s)return std::nullopt;
    return OriginalParentWorldState{s->world_matrix,s->dirty};
}
bool OriginalParentMatrixWorldAdapter::refresh_parent_world(WorldEntityId id) {
    return refresh_original_world_transform(world_,id).successful();
}
std::optional<WorldTransformSnapshot>
OriginalAttachmentWorldTransformAdapter::snapshot(WorldEntityId id) {
    return world_.snapshot(id);
}
std::optional<WorldMatrix3x4>
OriginalAttachmentWorldTransformAdapter::attachment_parent_matrix(
    WorldEntityId,WorldEntityId parent,std::uint8_t slot) {
    if(!slot||!source_.has_animating_interface(parent))return std::nullopt;
    const auto pose=source_.get_attachment_pose(parent,slot);
    if(!pose)return std::nullopt;
    return make_local_angle_matrix(pose->angles,pose->origin);
}
bool OriginalAttachmentWorldTransformAdapter::commit_world_transform(
    WorldEntityId id,const WorldTransformSnapshot& s) {
    return world_.commit_world_transform(id,s);
}
bool OriginalAttachmentWorldTransformAdapter::on_world_transform_rebuilt(
    WorldEntityId id) {return world_.on_world_transform_rebuilt(id);}
} // namespace sin_compat::jessica
