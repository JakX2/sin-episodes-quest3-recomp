#pragma once
// Independent behavioral model of original 2006 SiN server.dll 0x2213D960.
// No proprietary engine ABI, original DLL bytes, or Ritual source included.
#include "sin_compat/original_world_transform.hpp"
#include <cstdint>
#include <optional>
namespace sin_compat::jessica {
struct OriginalParentBinding {
    std::optional<WorldEntityId> parent{};
    std::uint8_t attachment=0;
};
struct OriginalParentWorldState {
    WorldMatrix3x4 matrix{};
    bool dirty=false;
};
struct OriginalAttachmentPose {
    TossVec3 origin{},angles{};
};
class IOriginalParentMatrixHost {
public:
    virtual ~IOriginalParentMatrixHost()=default;
    virtual std::optional<OriginalParentBinding> binding(WorldEntityId child)=0;
    // Native host must check original +0xF0 EHANDLE slot and generation.
    virtual bool parent_is_live(WorldEntityId parent)=0;
    // Original parent vtable byte +0xC4 yields animating interface.
    virtual bool parent_has_animating_interface(WorldEntityId parent)=0;
    // The original 0x220ED470 returns a boolean. Nullopt represents false.
    virtual std::optional<OriginalAttachmentPose> attachment_pose(
        WorldEntityId parent,std::uint8_t attachment)=0;
    virtual std::optional<OriginalParentWorldState> parent_world_state(
        WorldEntityId parent)=0;
    virtual bool refresh_parent_world(WorldEntityId parent)=0;
};
enum class OriginalParentMatrixRoute : std::uint8_t {
    Identity,ParentMatrix,AttachmentMatrix,MissingChild,InvalidTransform,HostRejected
};
struct OriginalParentMatrixResult {
    OriginalParentMatrixRoute route=OriginalParentMatrixRoute::HostRejected;
    WorldMatrix3x4 matrix{};
    bool successful() const noexcept {
        return route==OriginalParentMatrixRoute::Identity ||
               route==OriginalParentMatrixRoute::ParentMatrix ||
               route==OriginalParentMatrixRoute::AttachmentMatrix;
    }
};
// Safe value-copy model. Original 2006 code may return borrowed parent
// matrix pointer (+0x268) or a scratch buffer, not an owned matrix.
OriginalParentMatrixResult resolve_original_parent_matrix(
    IOriginalParentMatrixHost&,WorldEntityId child) noexcept;
class IOriginalNativeAttachmentSource {
public:
    virtual ~IOriginalNativeAttachmentSource()=default;
    virtual bool has_animating_interface(WorldEntityId parent)=0;
    virtual std::optional<OriginalAttachmentPose> get_attachment_pose(
        WorldEntityId parent,std::uint8_t slot)=0;
};
class OriginalParentMatrixWorldAdapter final:public IOriginalParentMatrixHost {
public:
    OriginalParentMatrixWorldAdapter(IOriginalWorldTransformHost& world,
                                    IOriginalNativeAttachmentSource& attachments)
        : world_(world),attachments_(attachments){}
    std::optional<OriginalParentBinding> binding(WorldEntityId child) override;
    bool parent_is_live(WorldEntityId parent) override;
    bool parent_has_animating_interface(WorldEntityId parent) override;
    std::optional<OriginalAttachmentPose> attachment_pose(
        WorldEntityId parent,std::uint8_t slot) override;
    std::optional<OriginalParentWorldState> parent_world_state(
        WorldEntityId parent) override;
    bool refresh_parent_world(WorldEntityId parent) override;
private:
    IOriginalWorldTransformHost& world_;
    IOriginalNativeAttachmentSource& attachments_;
};
// Adapts original 0x220ED470 success gate to Phase 108 world refresh.
class OriginalAttachmentWorldTransformAdapter final:public IOriginalWorldTransformHost {
public:
    OriginalAttachmentWorldTransformAdapter(IOriginalWorldTransformHost& world,
                                            IOriginalNativeAttachmentSource& source)
        : world_(world),source_(source){}
    std::optional<WorldTransformSnapshot> snapshot(WorldEntityId id) override;
    std::optional<WorldMatrix3x4> attachment_parent_matrix(
        WorldEntityId child,WorldEntityId parent,std::uint8_t slot) override;
    bool commit_world_transform(WorldEntityId id,
                                const WorldTransformSnapshot& updated) override;
    bool on_world_transform_rebuilt(WorldEntityId id) override;
private:
    IOriginalWorldTransformHost& world_;
    IOriginalNativeAttachmentSource& source_;
};
} // namespace sin_compat::jessica
