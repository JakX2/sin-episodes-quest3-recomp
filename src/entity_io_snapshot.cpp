#include "sin_compat/entity_io_runtime.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <utility>

namespace sin_compat {
namespace {

constexpr std::size_t kMaxSnapshotBytes = 16U * 1024U * 1024U;
constexpr std::size_t kMaxStringBytes = 4096U;
constexpr std::uint32_t kMaxOutputGroups = 65536U;
constexpr std::uint32_t kMaxPendingEvents = 65536U;
constexpr std::uint32_t kMaxConnections = 262144U;
constexpr std::uint32_t kMaxConnectionsPerOutput = 4096U;

bool add_size(std::size_t& total, std::size_t amount) {
    if (amount > kMaxSnapshotBytes - total) return false;
    total += amount;
    return true;
}

bool add_string_size(std::size_t& total, const std::string& value) {
    return value.size() <= kMaxStringBytes &&
           add_size(total, 4U) && add_size(total, value.size());
}

bool valid_connection(const EntityIoSavedConnection& connection) {
    if (connection.target.empty() || connection.input.empty() ||
        !std::isfinite(connection.delay_seconds) ||
        connection.delay_seconds < 0.0 || connection.max_fires < -1) {
        return false;
    }
    if (connection.max_fires == -1) return connection.remaining == -1;
    if (connection.max_fires == 0) return connection.remaining == 0;
    return connection.remaining >= 0 &&
           connection.remaining <= connection.max_fires;
}

bool valid_event(const EntityIoSavedEvent& event) {
    return !event.target.empty() && !event.input.empty() &&
           std::isfinite(event.remaining_seconds) &&
           event.remaining_seconds >= 0.0;
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
}

void append_u64(std::vector<std::uint8_t>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8)
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
}

void append_i32(std::vector<std::uint8_t>& out, std::int32_t value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    append_u32(out, bits);
}

void append_f64(std::vector<std::uint8_t>& out, double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    append_u64(out, bits);
}

void append_string(std::vector<std::uint8_t>& out, const std::string& value) {
    append_u32(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

class Reader final {
public:
    Reader(const std::uint8_t* bytes, std::size_t size)
        : bytes_(bytes), size_(size) {}

    bool read_u32(std::uint32_t& value) {
        if (remaining() < 4U) return false;
        value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8)
            value |= static_cast<std::uint32_t>(bytes_[position_++]) << shift;
        return true;
    }

    bool read_u64(std::uint64_t& value) {
        if (remaining() < 8U) return false;
        value = 0;
        for (unsigned shift = 0; shift < 64; shift += 8)
            value |= static_cast<std::uint64_t>(bytes_[position_++]) << shift;
        return true;
    }

    bool read_i32(std::int32_t& value) {
        std::uint32_t bits = 0;
        if (!read_u32(bits)) return false;
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool read_f64(double& value) {
        std::uint64_t bits = 0;
        if (!read_u64(bits)) return false;
        std::memcpy(&value, &bits, sizeof(value));
        return true;
    }

    bool read_string(std::string& value) {
        std::uint32_t length = 0;
        if (!read_u32(length) || length > kMaxStringBytes ||
            remaining() < static_cast<std::size_t>(length)) return false;
        value.assign(reinterpret_cast<const char*>(bytes_ + position_), length);
        position_ += length;
        return true;
    }

    bool read_magic() {
        static constexpr std::uint8_t magic[4] = {'S', 'I', 'O', 'Q'};
        if (remaining() < sizeof(magic)) return false;
        for (std::uint8_t expected : magic) {
            if (bytes_[position_++] != expected) return false;
        }
        return true;
    }

    std::size_t remaining() const { return size_ - position_; }
    bool at_end() const { return position_ == size_; }

private:
    const std::uint8_t* bytes_;
    std::size_t size_;
    std::size_t position_ = 0;
};

} // namespace

bool validate_entity_io_snapshot(const EntityIoSnapshot& snapshot) {
    static_assert(sizeof(double) == 8, "snapshot format requires binary64");
    static_assert(std::numeric_limits<double>::is_iec559,
                  "snapshot format requires IEEE-754 binary64");

    if (snapshot.version != kEntityIoSnapshotVersion ||
        snapshot.outputs.size() > kMaxOutputGroups ||
        snapshot.pending_events.size() > kMaxPendingEvents) return false;

    std::size_t encoded_size = 24U;
    std::size_t connection_count = 0;
    std::set<std::pair<std::uint64_t, std::string>> output_keys;

    for (const auto& output : snapshot.outputs) {
        if (output.source_id == 0 || output.output.empty() ||
            output.connections.empty() ||
            output.connections.size() > kMaxConnectionsPerOutput ||
            !output_keys.emplace(output.source_id, output.output).second) {
            return false;
        }
        if (!add_size(encoded_size, 8U) ||
            !add_string_size(encoded_size, output.output) ||
            !add_size(encoded_size, 4U)) return false;

        connection_count += output.connections.size();
        if (connection_count > kMaxConnections) return false;

        for (const auto& connection : output.connections) {
            if (!valid_connection(connection) ||
                !add_string_size(encoded_size, connection.target) ||
                !add_string_size(encoded_size, connection.input) ||
                !add_string_size(encoded_size, connection.parameter) ||
                !add_size(encoded_size, 16U)) return false;
        }
    }

    std::set<std::uint64_t> ordinals;
    for (const auto& event : snapshot.pending_events) {
        if (!valid_event(event) || event.ordinal >= snapshot.next_ordinal ||
            !ordinals.insert(event.ordinal).second ||
            !add_size(encoded_size, 32U) ||
            !add_string_size(encoded_size, event.target) ||
            !add_string_size(encoded_size, event.input) ||
            !add_string_size(encoded_size, event.parameter)) return false;
    }

    return encoded_size <= kMaxSnapshotBytes;
}

bool encode_entity_io_snapshot(const EntityIoSnapshot& snapshot,
                               std::vector<std::uint8_t>& bytes) {
    if (!validate_entity_io_snapshot(snapshot)) return false;

    std::vector<std::uint8_t> encoded;
    encoded.reserve(24U + snapshot.outputs.size() * 16U +
                    snapshot.pending_events.size() * 64U);
    encoded.insert(encoded.end(), {'S', 'I', 'O', 'Q'});
    append_u32(encoded, snapshot.version);
    append_u64(encoded, snapshot.next_ordinal);
    append_u32(encoded, static_cast<std::uint32_t>(snapshot.outputs.size()));
    append_u32(encoded, static_cast<std::uint32_t>(snapshot.pending_events.size()));

    for (const auto& output : snapshot.outputs) {
        append_u64(encoded, output.source_id);
        append_string(encoded, output.output);
        append_u32(encoded, static_cast<std::uint32_t>(output.connections.size()));
        for (const auto& connection : output.connections) {
            append_string(encoded, connection.target);
            append_string(encoded, connection.input);
            append_string(encoded, connection.parameter);
            append_f64(encoded, connection.delay_seconds);
            append_i32(encoded, connection.max_fires);
            append_i32(encoded, connection.remaining);
        }
    }

    for (const auto& event : snapshot.pending_events) {
        append_u64(encoded, event.caller_id);
        append_u64(encoded, event.activator_id);
        append_string(encoded, event.target);
        append_string(encoded, event.input);
        append_string(encoded, event.parameter);
        append_f64(encoded, event.remaining_seconds);
        append_u64(encoded, event.ordinal);
    }

    if (encoded.size() > kMaxSnapshotBytes) return false;
    bytes.swap(encoded);
    return true;
}

bool decode_entity_io_snapshot(const std::uint8_t* bytes, std::size_t size,
                               EntityIoSnapshot& snapshot) {
    if (!bytes || size < 24U || size > kMaxSnapshotBytes) return false;
    Reader reader(bytes, size);
    if (!reader.read_magic()) return false;

    EntityIoSnapshot decoded;
    std::uint32_t output_count = 0;
    std::uint32_t event_count = 0;
    if (!reader.read_u32(decoded.version) ||
        !reader.read_u64(decoded.next_ordinal) ||
        !reader.read_u32(output_count) ||
        !reader.read_u32(event_count) ||
        output_count > kMaxOutputGroups ||
        event_count > kMaxPendingEvents) return false;

    decoded.outputs.reserve(output_count);
    for (std::uint32_t i = 0; i < output_count; ++i) {
        EntityIoSavedOutput output;
        std::uint32_t connection_count = 0;
        if (!reader.read_u64(output.source_id) ||
            !reader.read_string(output.output) ||
            !reader.read_u32(connection_count) ||
            connection_count == 0 ||
            connection_count > kMaxConnectionsPerOutput) return false;

        output.connections.reserve(connection_count);
        for (std::uint32_t j = 0; j < connection_count; ++j) {
            EntityIoSavedConnection connection;
            std::int32_t max_fires = 0;
            std::int32_t remaining = 0;
            if (!reader.read_string(connection.target) ||
                !reader.read_string(connection.input) ||
                !reader.read_string(connection.parameter) ||
                !reader.read_f64(connection.delay_seconds) ||
                !reader.read_i32(max_fires) ||
                !reader.read_i32(remaining)) return false;
            connection.max_fires = max_fires;
            connection.remaining = remaining;
            output.connections.push_back(std::move(connection));
        }
        decoded.outputs.push_back(std::move(output));
    }

    decoded.pending_events.reserve(event_count);
    for (std::uint32_t i = 0; i < event_count; ++i) {
        EntityIoSavedEvent event;
        if (!reader.read_u64(event.caller_id) ||
            !reader.read_u64(event.activator_id) ||
            !reader.read_string(event.target) ||
            !reader.read_string(event.input) ||
            !reader.read_string(event.parameter) ||
            !reader.read_f64(event.remaining_seconds) ||
            !reader.read_u64(event.ordinal)) return false;
        decoded.pending_events.push_back(std::move(event));
    }

    if (!reader.at_end() || !validate_entity_io_snapshot(decoded)) return false;
    snapshot = std::move(decoded);
    return true;
}

} // namespace sin_compat
