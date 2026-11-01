#pragma once
// Independent, portable parser for the user's original StatDefinitions schema.
// No proprietary stats data or dynamic-difficulty algorithm is embedded.
#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace sin_compat {
struct StatDefinition {
    std::string name;
    std::string short_text;
    std::string long_text;
    std::string percentage_of; // empty if none
};
struct StatPercentage {
    enum class Status {
        Found, UnknownStat, NotPercentageStat, MissingNumerator,
        MissingDenominator, ZeroDenominator, InvalidValue
    };
    Status status = Status::UnknownStat;
    double percent = 0.0;
    explicit operator bool() const noexcept { return status == Status::Found; }
};
class StatSchema final {
public:
    // Strict schema only. A failed load discards all previous definitions.
    bool load(std::string_view keyvalues, std::string* error = nullptr);
    const StatDefinition* find(std::string_view name) const noexcept;
    const std::vector<StatDefinition>& definitions() const noexcept { return definitions_; }
    StatPercentage percentage(const std::string& stat,
                              const std::map<std::string, double>& values) const;
private:
    std::vector<StatDefinition> definitions_;
    std::map<std::string, std::size_t> index_;
};
} // namespace sin_compat
