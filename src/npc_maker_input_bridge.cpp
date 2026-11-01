#include "sin_compat/npc_maker_input_bridge.hpp"
#include <charconv>
#include <cmath>
#include <locale>
#include <sstream>
#include <string>
#include <utility>
namespace sin_compat {
namespace {
std::string trimmed(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}
bool decimal_int(const std::string& param, int& value) {
    const auto s = trimmed(param);
    if (s.empty()) return false;
    // from_chars is locale independent and rejects truncated/overflowing ints.
    const auto result = std::from_chars(s.data(), s.data() + s.size(), value, 10);
    return result.ec == std::errc{} && result.ptr == s.data() + s.size();
}
bool decimal_float(const std::string& param, double& value) {
    const auto s = trimmed(param);
    if (s.empty() || s.find('\0') != std::string::npos) return false;
    std::istringstream stream(s);
    stream.imbue(std::locale::classic());
    stream >> value;
    if (stream.fail() || !std::isfinite(value)) return false;
    char trailing = 0;
    return !(stream >> trailing);
}
} // namespace
NpcMakerInputDispatch dispatch_npc_maker_input(
    NpcMakerRuntime& runtime, EntityRef maker,
    const std::string& input, const std::string& param) {
    if (input == "Enable") return {true, runtime.enable(maker), {}};
    if (input == "Disable") return {true, runtime.disable(maker), {}};
    if (input == "Toggle") return {true, runtime.toggle(maker), {}};
    if (input == "Spawn") {
        auto action = runtime.spawn_now(maker);
        return {true, action.result, std::move(action.events)};
    }
    const bool is_quota_delta = input == "AddMaxChildren";
    const bool is_quota = input == "SetMaxChildren";
    const bool is_live = input == "SetMaxLiveChildren";
    const bool is_frequency = input == "SetSpawnFrequency";
    if (!is_quota_delta && !is_quota && !is_live && !is_frequency)
        return {};
    if (is_frequency) {
        double frequency = 0;
        if (!decimal_float(param, frequency)) return {true, NpcMakerResult::InvalidSetting, {}};
        return {true, runtime.set_spawn_frequency(maker, frequency), {}};
    }
    int number = 0;
    if (!decimal_int(param, number)) return {true, NpcMakerResult::InvalidSetting, {}};
    if (is_quota_delta) return {true, runtime.add_max_children(maker, number), {}};
    if (is_quota) return {true, runtime.set_max_children(maker, number), {}};
    return {true, runtime.set_max_live_children(maker, number), {}};
}
} // namespace sin_compat
