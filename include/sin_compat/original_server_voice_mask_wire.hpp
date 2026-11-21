#pragma once
// Independently authored original 2006 SiN server VoiceMask reference.
// Not original Ritual code or an original engine message implementation.
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace sin_compat::original_server_voice_mask {
constexpr std::size_t kRegisteredVoiceMaskBytes=9;
constexpr std::size_t kRegisteredRequestStateBytes=0;
constexpr std::size_t kAudibleBits=32;
constexpr std::size_t kBlockedBits=32;
constexpr std::size_t kStatusBits=8;
struct Fields { std::uint32_t audible=0, server_banned=0; std::uint8_t status=0; };
inline std::array<std::uint8_t,9> encode(const Fields& f) noexcept {
    std::array<std::uint8_t,9> out{};
    for(std::size_t i=0;i<4;++i){
        out[i]=static_cast<std::uint8_t>(f.audible>>(8*i));
        out[i+4]=static_cast<std::uint8_t>(f.server_banned>>(8*i));
    }
    out[8]=f.status;
    return out;
}
enum class MessageType { RequestState, VoiceMask };
struct Message {MessageType type; std::vector<std::uint8_t> payload;};
// Recovered send ordering, but fields like whether masks changed are supplied
// by the host. Do not mistake this for complete game voice policy.
inline std::vector<Message> messages_for_recipient(
    bool pending_request, bool audible_changed, bool banned_changed,
    const Fields& fields) {
    std::vector<Message> result;
    if(pending_request)result.push_back({MessageType::RequestState,{}});
    if(audible_changed||banned_changed){
        auto bytes=encode(fields);
        result.push_back({MessageType::VoiceMask,{bytes.begin(),bytes.end()}});
    }
    return result;
}
} // namespace sin_compat::original_server_voice_mask