#include "sin_compat/weapon_scripts.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sin_compat {
namespace {

constexpr std::size_t kMaxScriptSize = 1024 * 1024;
constexpr int kMaxDepth = 32;

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

// Only the immediate children of WeaponData become weapon settings.
// SoundData/TextureData blocks are lexically checked and safely skipped.
class Reader {
public:
    explicit Reader(const std::string& text) : source_(text) {
        if (source_.size() > kMaxScriptSize) throw std::runtime_error("weapon script exceeds 1 MiB");
        if (source_.size() >= 3 && static_cast<unsigned char>(source_[0]) == 0xEF &&
            static_cast<unsigned char>(source_[1]) == 0xBB &&
            static_cast<unsigned char>(source_[2]) == 0xBF) pos_ = 3;
    }

    std::map<std::string, std::string> read_weapon_data() {
        if (lower(next()) != "weapondata") fail("expected WeaponData root");
        expect("{");
        std::map<std::string, std::string> values;
        while (true) {
            const auto key = next();
            if (key.empty()) fail("unexpected end inside WeaponData");
            if (key == "}") break;
            if (key == "{") fail("unexpected opening brace");
            const auto value = next();
            if (value.empty() || value == "}") fail("missing value for " + key);
            if (value == "{") {
                skip_block(1);
            } else {
                const auto name = lower(key);
                if (!values.emplace(name, value).second)
                    fail("duplicate WeaponData key " + key);
            }
        }
        if (!next().empty()) fail("unexpected tokens after WeaponData");
        return values;
    }

private:
    const std::string& source_;
    std::size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& reason) const {
        throw std::runtime_error("KeyValues byte " + std::to_string(pos_) + ": " + reason);
    }

    void skip_space_and_comments() {
        while (pos_ < source_.size()) {
            const unsigned char c = static_cast<unsigned char>(source_[pos_]);
            if (std::isspace(c)) { ++pos_; continue; }
            if (c == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
                pos_ += 2;
                while (pos_ < source_.size() && source_[pos_] != '\n') ++pos_;
                continue;
            }
            break;
        }
    }

    std::string next() {
        skip_space_and_comments();
        if (pos_ >= source_.size()) return {};
        const char c = source_[pos_++];
        if (c == '{' || c == '}') return {c};
        std::string result;
        if (c == '"') {
            bool closed = false;
            while (pos_ < source_.size()) {
                const char ch = source_[pos_++];
                if (ch == '"') { closed = true; break; }
                if (ch == '\\' && pos_ < source_.size()) {
                    const char escaped = source_[pos_];
                    if (escaped == '"' || escaped == '\\') {
                        result.push_back(escaped);
                        ++pos_;
                        continue;
                    }
                }
                result.push_back(ch);
            }
            if (!closed) fail("unterminated quoted string");
            if (result.empty()) fail("empty tokens are not supported");
        } else {
            result.push_back(c);
            while (pos_ < source_.size()) {
                const unsigned char ch = static_cast<unsigned char>(source_[pos_]);
                if (std::isspace(ch) || ch == '{' || ch == '}' ||
                    (ch == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/')) break;
                result.push_back(source_[pos_++]);
            }
        }
        return result;
    }

    void expect(const std::string& value) {
        if (next() != value) fail("expected " + value);
    }

    void skip_block(int depth) {
        if (depth > kMaxDepth) fail("KeyValues nesting too deep");
        while (true) {
            const auto key = next();
            if (key.empty()) fail("unterminated nested block");
            if (key == "}") return;
            if (key == "{") fail("unexpected opening brace in nested block");
            const auto value = next();
            if (value.empty() || value == "}") fail("missing nested value");
            if (value == "{") skip_block(depth + 1);
        }
    }
};

const std::string& required(const std::map<std::string, std::string>& settings,
                            const char* key) {
    const auto it = settings.find(key);
    if (it == settings.end() || it->second.empty())
        throw std::runtime_error(std::string("missing required WeaponData field: ") + key);
    return it->second;
}

int integer_field(const std::map<std::string, std::string>& settings,
                  const char* key, int minval, int maxval) {
    const auto& raw = required(settings, key);
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(raw.c_str(), &end, 10);
    if (errno == ERANGE || end == raw.c_str() || *end != '\0' ||
        parsed < minval || parsed > maxval)
        throw std::runtime_error(std::string("invalid integer field ") + key + ": " + raw);
    return static_cast<int>(parsed);
}

std::array<float, 3> origin_field(const std::map<std::string, std::string>& settings) {
    const auto it = settings.find("viewmodeloriginoffset");
    if (it == settings.end()) return {{0.0f, 0.0f, 0.0f}};
    std::istringstream input(it->second);
    std::array<float, 3> offset{};
    std::string extra;
    if (!(input >> offset[0] >> offset[1] >> offset[2]) ||
        (input >> extra) ||
        !std::isfinite(offset[0]) || !std::isfinite(offset[1]) || !std::isfinite(offset[2]))
        throw std::runtime_error("invalid viewmodelOriginOffset vector");
    return offset;
}

} // namespace

WeaponConfig parse_weapon_script(const std::string& text) {
    const auto fields = Reader(text).read_weapon_data();
    WeaponConfig config;
    config.viewmodel_name = required(fields, "viewmodel");
    config.zoom_fov = integer_field(fields, "zoomfov", 1, 179);
    config.ammo.clip_size = integer_field(fields, "clip_size", -1, 10000);
    config.ammo.clip2_size = integer_field(fields, "clip2_size", -1, 10000);
    config.primary_ammo_name = required(fields, "primary_ammo");
    config.secondary_ammo_name = required(fields, "secondary_ammo");
    config.origin_offset = origin_field(fields);
    return config;
}

const char* weapon_script_filename(WeaponKind kind) {
    switch (kind) {
        case WeaponKind::Magnum: return "weapon_magnum.txt";
        case WeaponKind::AssaultRifle: return "weapon_assault_rifle.txt";
        case WeaponKind::Scattergun: return "weapon_scattergun.txt";
    }
    throw std::invalid_argument("unknown WeaponKind");
}

WeaponConfig load_weapon_script(const std::filesystem::path& game_root, WeaponKind kind) {
    const auto path = game_root / "scripts" / weapon_script_filename(kind);
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open game weapon script: " + path.string());
    std::string text;
    text.reserve(8192);
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount()) {
        const auto count = static_cast<std::size_t>(file.gcount());
        if (text.size() + count > kMaxScriptSize)
            throw std::runtime_error("weapon script exceeds 1 MiB: " + path.string());
        text.append(buffer, count);
    }
    if (file.bad()) throw std::runtime_error("error reading weapon script: " + path.string());
    try {
        return parse_weapon_script(text);
    } catch (const std::exception& e) {
        throw std::runtime_error(path.string() + ": " + e.what());
    }
}

} // namespace sin_compat
