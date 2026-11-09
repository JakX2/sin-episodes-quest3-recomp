#pragma once

// Independently authored, engine-neutral reconstruction of the confirmed
// CRadioMessage fields and input/output dispatch seam. This is not a Source
// game entity class, audio implementation, or replacement game DLL.

#include "sin_compat/entity_io_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sin_compat {

struct RadioMessageState {
    // Original datamap: m_message (+0x3C8), external key "message".
    std::string message;
    // Original datamap: m_speaker (+0x3CC), external key "speaker".
    std::string speaker;
    // Original datamap: m_priority (+0x3C4).
    std::int32_t priority = 0;
};

enum class RadioMessageResult {
    Applied,
    Unchanged,
    MissingLocalPlayer,
    StaleEntity,
    HostRejected,
    OutputInvoked,
    RemoveRequested
};

struct RadioMessageDispatchResult {
    RadioMessageResult result = RadioMessageResult::StaleEntity;
    // Zero means the OnDispatch output was invoked but had no eligible links.
    std::size_t connections_scheduled = 0;
};

class IRadioMessageHost : public IEntityOutputHost {
public:
    ~IRadioMessageHost() override = default;

    // Original InputDispatch resolves the local player before the
    // CBaseEntityOutput::FireOutput call.
    virtual std::optional<EntityRef> resolve_local_player() = 0;

    // The original Remove input's exact removal/lifetime semantics remain
    // subject to further instruction-level verification. Keep removal in
    // the engine adapter instead of deleting the portable object directly.
    virtual bool request_remove(EntityRef entity) = 0;
};

class OriginalRadioMessageRuntime final {
public:
    static constexpr std::string_view kOnDispatchOutput = "OnDispatch";

    OriginalRadioMessageRuntime(EntityRef self, IRadioMessageHost& host,
                                EntityIoRuntime& entity_io)
        : self_(self), host_(host), entity_io_(entity_io) {}

    RadioMessageResult set_message(std::string_view value);
    RadioMessageResult set_speaker(std::string_view value);
    RadioMessageResult set_priority(std::int32_t value);

    RadioMessageDispatchResult dispatch();
    RadioMessageResult remove();

    const RadioMessageState& state() const { return state_; }
    EntityRef self() const { return self_; }

private:
    EntityRef self_;
    IRadioMessageHost& host_;
    EntityIoRuntime& entity_io_;
    RadioMessageState state_;
};

} // namespace sin_compat
