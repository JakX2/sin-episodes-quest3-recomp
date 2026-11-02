#include "sin_compat/source_entity_lump.hpp"

#include <cctype>
#include <string>
#include <utility>

namespace sin_compat {
namespace {

constexpr std::size_t kMaxLumpBytes = 16 * 1024 * 1024;
constexpr std::size_t kMaxEntities = 16384;
constexpr std::size_t kMaxProperties = 4096;
constexpr std::size_t kMaxTokenBytes = 16384;

std::string lower_ascii(std::string text) {
    for (char& c : text)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return text;
}

class Scanner {
public:
    explicit Scanner(std::string_view source) : source_(source) {}
    std::size_t offset() const { return cursor_; }
    bool end() const { return cursor_ == source_.size(); }
    bool at(char c) const { return !end() && source_[cursor_] == c; }
    void advance() { ++cursor_; }
    void skip() {
        while (!end()) {
            const unsigned char c = static_cast<unsigned char>(source_[cursor_]);
            if (std::isspace(c)) {
                ++cursor_;
            } else if (at('/') && cursor_+1 < source_.size() &&
                       source_[cursor_+1] == '/') {
                cursor_ += 2;
                while (!end() && !at('\n')) ++cursor_;
            } else {
                break;
            }
        }
    }
    bool quoted(std::string& out, std::string& error) {
        if (!at('"')) {
            error = "expected quoted Source entity key or value";
            return false;
        }
        advance();
        out.clear();
        while (!end()) {
            const char c = source_[cursor_++];
            if (c == '"') return true;
            if (c == '\0') {
                error = "embedded NUL in quoted Source keyvalue";
                return false;
            }
            if (c == '\\' && !end() &&
                (at('"') || at('\\'))) {
                out += source_[cursor_++];
            } else {
                out += c;
            }
            if (out.size() > kMaxTokenBytes) {
                error = "entity key/value exceeds size limit";
                return false;
            }
        }
        error = "unterminated quoted Source key/value";
        return false;
    }
private:
    std::string_view source_;
    std::size_t cursor_ = 0;
};

} // namespace

ScenarioLumpParseResult parse_source_entity_text(std::string_view text) {
    ScenarioLumpParseResult result;
    if (text.size() > kMaxLumpBytes) {
        result.error = "entity lump exceeds size limit";
        return result;
    }
    if (text.size() >= 3 &&
        text[0] == '\xEF' && text[1] == '\xBB' && text[2] == '\xBF')
        text.remove_prefix(3);
    while (!text.empty() && text.back() == '\0') text.remove_suffix(1);

    Scanner scan(text);
    while (true) {
        scan.skip();
        if (scan.end()) break;
        if (!scan.at('{')) {
            result.error = "expected '{' at start of entity";
            result.error_offset = scan.offset();
            return result;
        }
        scan.advance();
        ScenarioEntitySpec entity;
        bool closed = false;
        while (true) {
            scan.skip();
            if (scan.end()) break;
            if (scan.at('}')) {
                scan.advance();
                closed = true;
                break;
            }
            if (entity.keyvalues.size() == kMaxProperties) {
                result.error = "too many keyvalues in entity";
                result.error_offset = scan.offset();
                return result;
            }
            std::string key, value;
            if (!scan.quoted(key, result.error)) {
                result.error_offset = scan.offset();
                return result;
            }
            scan.skip();
            if (!scan.quoted(value, result.error)) {
                result.error_offset = scan.offset();
                return result;
            }
            const auto key_lower = lower_ascii(key);
            if (key_lower == "classname" && entity.classname.empty())
                entity.classname = value;
            if (key_lower == "targetname" && entity.targetname.empty())
                entity.targetname = value;
            entity.keyvalues.emplace_back(std::move(key), std::move(value));
        }
        if (!closed) {
            result.error = "unterminated entity block";
            result.error_offset = scan.offset();
            return result;
        }
        if (entity.classname.empty()) {
            result.error = "entity block missing classname";
            result.error_offset = scan.offset();
            return result;
        }
        result.entities.push_back(std::move(entity));
        if (result.entities.size() > kMaxEntities) {
            result.error = "too many entities in lump";
            result.error_offset = scan.offset();
            return result;
        }
    }
    return result;
}

ScenarioLumpLoadReport load_source_entity_text(
        ScenarioEntityWorld& world, std::string_view text) {
    ScenarioLumpLoadReport result;
    auto parsed = parse_source_entity_text(text);
    if (!parsed) {
        result.error = "entity syntax: " + parsed.error;
        return result;
    }
    result.parsed = parsed.entities.size();
    for (const auto& spec : parsed.entities) {
        const auto added = world.spawn(spec);
        if (added) {
            result.created.push_back(added.entity);
            ++result.spawned;
        } else if (added.error == "unsupported entity class; no factory is registered") {
            ++result.unsupported;
        } else {
            for (EntityRef spawned : result.created) world.remove(spawned);
            result.created.clear();
            result.spawned = 0;
            result.error = "invalid supported entity " + spec.classname +
                           ": " + added.error;
            return result;
        }
    }
    return result;
}

} // namespace sin_compat
