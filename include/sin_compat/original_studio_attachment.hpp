#pragma once
// Independent C++17 model of the original SiN (2006) CBaseAnimating
// attachment getter at 0x220ED530 (virtual +0x2C4). No original source.
#include "sin_compat/original_parent_attachment.hpp"
#include <cstdint>
#include <optional>

namespace sin_compat::jessica {
// The original mstudioattachment-style record uses a 92-byte stride. Its
// short flags include bit 0x100 (+6), bone ID is at +8, and 3x4 local
// attachment matrix begins at +0xC. Native host owns studiohdr & MDL cache.
struct OriginalStudioAttachmentRecord {
    std::uint16_t flags = 0;
    std::uint32_t bone_index = 0;
    WorldMatrix3x4 bone_local_matrix{};
};
class IOriginalStudioAttachmentHost {
public:
    virtual ~IOriginalStudioAttachmentHost()=default;
    // Nullopt means no valid studio resource, including mdlcache rejection.
    // In the original GetAttachment(1..count), zero and >count fail.
    virtual std::optional<std::uint32_t> attachment_count(WorldEntityId id)=0;
    virtual std::optional<OriginalStudioAttachmentRecord> attachment_record(
        WorldEntityId id,std::uint32_t zero_based_index)=0;
    // Original vtable +0x2B0 obtains the selected bone world transform.
    // Owner implements correct animation setup, model cache and bone ABI.
    virtual std::optional<WorldMatrix3x4> bone_world_transform(
        WorldEntityId id,std::uint32_t bone_index)=0;
};

enum class OriginalStudioAttachmentCode : std::uint8_t {
    Composed, WorldAligned, InvalidIndex, MissingStudio,
    MissingRecord, MissingBone, InvalidMatrix, HostRejected
};
struct OriginalStudioAttachmentResult {
    OriginalStudioAttachmentCode code=OriginalStudioAttachmentCode::HostRejected;
    WorldMatrix3x4 matrix{};
    bool successful()const noexcept {
        return code==OriginalStudioAttachmentCode::Composed ||
               code==OriginalStudioAttachmentCode::WorldAligned;
    }
};
OriginalStudioAttachmentResult resolve_original_studio_attachment(
    IOriginalStudioAttachmentHost&, WorldEntityId entity,
    std::uint32_t one_based_attachment_index) noexcept;

// Connect the new bone-aware getter to Phase 109's boolean-success gate.
// This does not reinterpret or call a 2006 x86 vtable on ARM64.
class OriginalStudioNativeAttachmentAdapter final:public IOriginalNativeAttachmentSource {
public:
    explicit OriginalStudioNativeAttachmentAdapter(IOriginalStudioAttachmentHost& host)
        :host_(host){}
    bool has_animating_interface(WorldEntityId parent) override;
    std::optional<OriginalAttachmentPose> get_attachment_pose(
        WorldEntityId parent,std::uint8_t attachment) override;
private:
    IOriginalStudioAttachmentHost& host_;
};
} // namespace sin_compat::jessica
