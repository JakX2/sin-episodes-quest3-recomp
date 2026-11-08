#pragma once
// Independent bounded reconstruction of the INITIALIZATION and FIRST command
// loop of SiN 2006 server.dll 0x22103A60. The second animation-effect pass,
// x87 rounding and native engine memory ownership are not reconstructed here.
#include "sin_compat/original_skeletal_setup.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace sin_compat::jessica {
struct OriginalPoseDriverCommand {
    std::uint32_t type = 0;             // original command+0x04: 3,4,5,6
    std::uint32_t selector = 0;         // +0x08
    std::int32_t track_index = 0;       // +0x10
    std::array<float,3> values{};       // +0x14, +0x18, +0x1C
    TossVec3 origin{};                  // +0x20
    std::array<float,4> quaternion{};  // +0x2C
    float secondary = 0.f;             // +0x50
    float influence = 0.f;             // +0x54
    float weight = 1.f;                // +0x58
    float limit = 0.f;                 // +0x5C
    std::uint32_t optional_payload = 0;// +0x60, used only by type 5
};
struct OriginalPoseDriverTrack {
    std::uint32_t selector = 0;       // original track+0x00
    std::uint32_t type = 0;           // +0x04
    std::uint32_t optional_payload = 0;// +0x08
    TossVec3 origin{};                // +0x0C
    std::array<float,4> quaternion{}; // +0x18, intentionally zero at reset
    float cap = 1.f;                  // +0x44
    float secondary = 1.f;            // +0x48
    std::array<float,3> values{};     // +0x4C, +0x50, +0x54
    float influence = 0.f;           // +0x5C
    float type3_height = 0.f;        // +0x68, written only on type 3
    float type3_reset_marker = 0.f;  // +0xD0, written only on type 3
};
class IOriginalPoseQuaternionMixer {
public:
    virtual ~IOriginalPoseQuaternionMixer() = default;
    // The original calls 0x2226D210; no guess about its floating-point
    // implementation is embedded in this portable first-pass module.
    virtual std::optional<std::array<float,4>> mix(
        const std::array<float,4>& previous,
        const std::array<float,4>& incoming, float weight) = 0;
};
using OriginalPoseCommandGroup = std::vector<OriginalPoseDriverCommand>;
enum class OriginalPoseFirstPassCode : std::uint8_t {
    Applied, InvalidTrack, InvalidCommand, InvalidFloat,
    MissingQuaternionMixer, QuaternionMixRejected, HostRejected
};
struct OriginalPoseFirstPassResult {
    OriginalPoseFirstPassCode code=OriginalPoseFirstPassCode::HostRejected;
    std::vector<OriginalPoseDriverTrack> tracks{};
    std::size_t commands_seen=0, commands_applied=0, commands_ignored=0;
    bool successful() const noexcept {return code==OriginalPoseFirstPassCode::Applied;}
};
// Reconstructs original reset stride 0xE4, group stride 0x14 and command
// stride 0x7C as *value-owned* snapshots rather than native x86 pointers.
// Results are transactional: rejected input returns no partially updated state.
OriginalPoseFirstPassResult apply_original_pose_driver_first_pass(
    std::size_t track_count, const std::vector<OriginalPoseCommandGroup>& groups,
    float type3_global_height, IOriginalPoseQuaternionMixer* mixer=nullptr,
    const std::vector<OriginalPoseDriverTrack>& prior_tracks={}) noexcept;
// Parse ONE 0x7C-byte command record from an owner-provided byte snapshot;
// no original code/data is shipped with this parser.
std::optional<OriginalPoseDriverCommand> parse_original_pose_command(
    const std::vector<std::uint8_t>& bytes,std::size_t offset=0) noexcept;
} // namespace sin_compat::jessica
