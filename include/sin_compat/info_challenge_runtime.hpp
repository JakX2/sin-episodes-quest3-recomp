#pragma once
// Independently authored entity-query seam from original 2006 FGD + DLL.
// This is neither the original difficulty formula nor Source engine ABI.
#include "sin_compat/weapon_entity_io.hpp"
#include <map>
#include <string>
#include <cstddef>
namespace sin_compat {
enum class InfoChallengeQueryStatus {
    Emitted, InvalidEntity, UnknownInput, ProviderUnavailable, InvalidValue
};
struct InfoChallengeOutput {
    InfoChallengeQueryStatus status = InfoChallengeQueryStatus::UnknownInput;
    const char* output_name = nullptr;
    float value = 0.0f;
    explicit operator bool() const noexcept {
        return status == InfoChallengeQueryStatus::Emitted;
    }
};
class IInfoChallengeHost {
public:
    virtual ~IInfoChallengeHost() = default;
    virtual bool is_live(EntityRef info_entity) const = 0;
    // Original GetInitial uses a global value at +0x8; current is returned
    // from a separate routine at 0x222d3730. Host supplies both numbers.
    virtual bool initial_challenge(float& out) const = 0;
    virtual bool current_challenge(float& out) const = 0;
};
class InfoChallengeRuntime final {
public:
    explicit InfoChallengeRuntime(IInfoChallengeHost& host) : host_(host) {}
    // Original difference handler uses a cached entity-local +0x3dc value.
    // The host supplies this cache; its initialization was not verified.
    bool register_entity(EntityRef entity, float cached_initial_baseline);
    void remove_entity(EntityRef entity);
    InfoChallengeOutput handle_input(EntityRef entity, const std::string& input) const;
    std::size_t entity_count() const noexcept { return baselines_.size(); }
private:
    struct RefLess { bool operator()(EntityRef a, EntityRef b) const noexcept {
        return a.index != b.index ? a.index < b.index : a.generation < b.generation;
    }};
    IInfoChallengeHost& host_;
    std::map<EntityRef,float,RefLess> baselines_;
};
} // namespace sin_compat
