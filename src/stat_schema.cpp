#include "sin_compat/stat_schema.hpp"
#include <cmath>
#include <utility>

namespace sin_compat {
namespace {
enum class TokenKind { Value, Open, Close, End, Error };
struct Token { TokenKind kind = TokenKind::End; std::string text; };
class Lexer final {
public:
    explicit Lexer(std::string_view text) : text_(text) {}
    Token next() {
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c == ' ' || c == '\n' || c == '\r' || c == '\t') { ++pos_; continue; }
            if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/') {
                pos_ += 2;
                while (pos_ < text_.size() && text_[pos_] != '\n') ++pos_;
                continue;
            }
            break;
        }
        if (pos_ == text_.size()) return {TokenKind::End, {}};
        const char first = text_[pos_++];
        if (first == '{') return {TokenKind::Open, {}};
        if (first == '}') return {TokenKind::Close, {}};
        if (first != '"') return {TokenKind::Error, "expected quoted KeyValues string"};
        std::string value;
        while (pos_ < text_.size()) {
            char c = text_[pos_++];
            if (c == '"') return {TokenKind::Value, std::move(value)};
            if (c == '\\') {
                if (pos_ == text_.size()) return {TokenKind::Error, "unterminated escape"};
                c = text_[pos_++];
                if (c != '"' && c != '\\') return {TokenKind::Error, "invalid escape"};
            }
            if (c == '\0' || value.size() >= 16384)
                return {TokenKind::Error, "invalid or oversized value"};
            value.push_back(c);
        }
        return {TokenKind::Error, "unterminated quoted string"};
    }
private:
    std::string_view text_;
    std::size_t pos_ = 0;
};
class Parser final {
public:
    explicit Parser(std::string_view text) : lexer_(text) {}
    bool parse(std::vector<StatDefinition>& out, std::string& error) {
        Token root = lexer_.next();
        if (root.kind != TokenKind::Value || root.text != "StatDefinitions")
            return fail(error, "expected StatDefinitions root");
        if (lexer_.next().kind != TokenKind::Open)
            return fail(error, "missing root open brace");
        for (;;) {
            Token entry = lexer_.next();
            if (entry.kind == TokenKind::Close) break;
            if (entry.kind != TokenKind::Value || entry.text != "stat")
                return fail(error, "expected repeated stat block");
            if (lexer_.next().kind != TokenKind::Open)
                return fail(error, "missing stat open brace");
            StatDefinition record;
            unsigned fields = 0;
            for (;;) {
                Token key = lexer_.next();
                if (key.kind == TokenKind::Close) break;
                if (key.kind != TokenKind::Value)
                    return fail(error, "expected stat field");
                Token value = lexer_.next();
                if (value.kind != TokenKind::Value)
                    return fail(error, "expected stat field value");
                unsigned flag = 0;
                std::string* target = nullptr;
                if (key.text == "name") { flag = 1; target = &record.name; }
                else if (key.text == "short_text") { flag = 2; target = &record.short_text; }
                else if (key.text == "long_text") { flag = 4; target = &record.long_text; }
                else if (key.text == "percentage_of") { flag = 8; target = &record.percentage_of; }
                else return fail(error, "unknown stat field");
                if (fields & flag) return fail(error, "duplicate stat field");
                fields |= flag;
                *target = std::move(value.text);
            }
            if ((fields & 7) != 7 || record.name.empty() || record.short_text.empty() ||
                record.long_text.empty() || ((fields & 8) && record.percentage_of.empty()))
                return fail(error, "missing required stat value");
            if (out.size() >= 1024) return fail(error, "stat count limit");
            out.push_back(std::move(record));
        }
        if (lexer_.next().kind != TokenKind::End)
            return fail(error, "trailing content after StatDefinitions");
        return true;
    }
private:
    static bool fail(std::string& error, const char* message) {
        error = message; return false;
    }
    Lexer lexer_;
};
} // namespace

bool StatSchema::load(std::string_view text, std::string* error) {
    definitions_.clear();
    index_.clear();
    std::string issue;
    std::vector<StatDefinition> parsed;
    if (text.size() > 262144) issue = "stat definition size limit";
    else Parser(text).parse(parsed, issue);
    if (issue.empty()) {
        std::map<std::string, std::size_t> by_name;
        for (std::size_t i = 0; i < parsed.size(); ++i) {
            if (!by_name.emplace(parsed[i].name, i).second) {
                issue = "duplicate stat name";
                break;
            }
        }
        if (issue.empty()) {
            for (const auto& d : parsed) {
                if (!d.percentage_of.empty() && by_name.count(d.percentage_of) == 0) {
                    issue = "unknown percentage_of reference";
                    break;
                }
            }
        }
        if (issue.empty()) {
            definitions_ = std::move(parsed);
            index_ = std::move(by_name);
        }
    }
    if (error) *error = issue;
    return issue.empty();
}
const StatDefinition* StatSchema::find(std::string_view name) const noexcept {
    const auto it = index_.find(std::string(name));
    return it == index_.end() ? nullptr : &definitions_[it->second];
}
StatPercentage StatSchema::percentage(const std::string& stat,
                                     const std::map<std::string, double>& values) const {
    const auto* definition = find(stat);
    if (!definition) return {StatPercentage::Status::UnknownStat, 0};
    if (definition->percentage_of.empty())
        return {StatPercentage::Status::NotPercentageStat, 0};
    auto numerator = values.find(stat);
    if (numerator == values.end())
        return {StatPercentage::Status::MissingNumerator, 0};
    auto denominator = values.find(definition->percentage_of);
    if (denominator == values.end())
        return {StatPercentage::Status::MissingDenominator, 0};
    if (!std::isfinite(numerator->second) || !std::isfinite(denominator->second))
        return {StatPercentage::Status::InvalidValue, 0};
    if (denominator->second == 0.0)
        return {StatPercentage::Status::ZeroDenominator, 0};
    // The source explicitly describes a ratio display; it does not specify
    // behavior for missing or zero denominators, which we fail closed.
    const double ratio = (numerator->second / denominator->second) * 100.0;
    if (!std::isfinite(ratio)) return {StatPercentage::Status::InvalidValue, 0};
    return {StatPercentage::Status::Found, ratio};
}
} // namespace sin_compat
