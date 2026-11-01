#pragma once
// Generic, independently authored Source-style KeyValues attack-spread lookup.
// The owner's original rules_combat.txt remains external to this library.
#include <map>
#include <string>
#include <string_view>

namespace sin_compat {
struct SpreadLookup {
    enum class Status { Found, NoActorRule, MissingRule, InvalidValue };
    Status status = Status::MissingRule;
    double spread = 0.0; // Original config stores spread factors, NOT degrees.
    explicit operator bool() const noexcept { return status == Status::Found; }
};
class CombatSpreadRules final {
public:
    bool load(std::string_view keyvalues, std::string* error = nullptr);
    SpreadLookup lookup(const std::string& attacker_class,
                        const std::string& weapon_class,
                        const std::string& target_class) const;
    bool loaded() const noexcept { return loaded_; }
private:
    struct Node {
        bool scalar = false;
        std::string value;
        std::map<std::string, Node> members;
    };
    bool loaded_ = false;
    Node root_;
    static const Node* child(const Node&, const std::string&);
    static const Node* named_or_wildcard(const Node&, const std::string&);
};
} // namespace sin_compat
