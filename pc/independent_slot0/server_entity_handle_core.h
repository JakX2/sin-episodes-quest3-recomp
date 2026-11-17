#pragma once
// Independent entity-handle resolution, matching the verified SiN server
// contact-list lookup semantics. No original module or fixed global address
// is accessed. The view is supplied by the independently owned entity list.
#include <cstddef>
#include <cstdint>

class CBaseEntity;

namespace sin_independent {

constexpr unsigned kEntityIndexBits = 12;
constexpr std::uint32_t kEntityIndexMask = 0xFFFu;
constexpr std::uint32_t kEntitySerialMask = 0xFFFFFu;
constexpr std::uint32_t kInvalidEntityHandle = 0xFFFFFFFFu;
constexpr std::size_t kEntityHandleCapacity = 4096;

struct EntityHandleParts {
    std::uint32_t entry;
    std::uint32_t serial;
};

constexpr EntityHandleParts SplitEntityHandle(std::uint32_t handle) noexcept {
    return {handle & kEntityIndexMask, handle >> kEntityIndexBits};
}

// Independent entry representation. Matches the original x86 record's
// pointer, serial and two link-pointer fields on a 32-bit build. The
// link fields belong to the entity registry, NOT the contact list.
struct EntityHandleEntry {
    CBaseEntity* entity = nullptr;
    std::uint32_t serial = 0;
    EntityHandleEntry* previous = nullptr;
    EntityHandleEntry* next = nullptr;
};

#if defined(_M_IX86)
static_assert(sizeof(EntityHandleEntry) == 16, "original PE32 entry stride");
static_assert(offsetof(EntityHandleEntry, entity) == 0, "entity pointer");
static_assert(offsetof(EntityHandleEntry, serial) == 4, "serial number");
#endif

// The original binary has a full fixed-index global list. The independent
// view additionally validates its supplied size, so incomplete fixtures or
// future smaller independent registries cannot overrun the array.
class EntityHandleView {
public:
    constexpr EntityHandleView(const EntityHandleEntry* entries,
                               std::size_t count) noexcept
        : entries_(entries), count_(count) {}

    CBaseEntity* Resolve(std::uint32_t handle) const noexcept {
        if (handle == kInvalidEntityHandle || !entries_) return nullptr;
        const auto parts = SplitEntityHandle(handle);
        if (parts.entry >= count_) return nullptr;
        const auto& entry = entries_[parts.entry];
        if (entry.serial != parts.serial) return nullptr;
        return entry.entity;
    }

private:
    const EntityHandleEntry* entries_;
    std::size_t count_;
};

} // namespace sin_independent
