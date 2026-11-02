#pragma once

// Portable SiN Episodes phonebook data/runtime adapter.
//
// Reads the user's original scripts/phonebook.txt KeyValues data at runtime,
// retains the exact phone number as text (including leading zeros), then
// requests the named sound cue through a host callback. This is independently
// written C++17; it is NOT the original phone entity or Source engine ABI.

#include <cstddef>
#include <map>
#include <string>
#include <string_view>

namespace sin_compat {

enum class PhoneDialStatus {
    CueRequested,
    NotLoaded,
    InvalidNumber,
    UnknownNumber,
};

struct PhoneDialResult {
    PhoneDialStatus status = PhoneDialStatus::NotLoaded;
    std::string sound_cue;
    explicit operator bool() const noexcept {
        return status == PhoneDialStatus::CueRequested;
    }
};

class IPhoneAudioHost {
public:
    virtual ~IPhoneAudioHost() = default;
    virtual void request_phone_sound(const std::string& sound_cue) = 0;
};

class PhoneBookRuntime final {
public:
    // Validates and atomically replaces the entire original PhoneBook mapping.
    // A failed load clears prior entries; game code must not keep stale cues.
    bool load(std::string_view keyvalues, std::string* error = nullptr);

    // Does not strip zeros, parse the number as an integer, or silently supply
    // the engine's unspecified "bad number" sound.
    PhoneDialResult lookup(std::string_view dialed_digits) const;
    PhoneDialResult dial(std::string_view dialed_digits, IPhoneAudioHost& host) const;

    bool loaded() const noexcept { return loaded_; }
    std::size_t entry_count() const noexcept { return cues_.size(); }

private:
    bool loaded_ = false;
    std::map<std::string, std::string> cues_;
};

} // namespace sin_compat
