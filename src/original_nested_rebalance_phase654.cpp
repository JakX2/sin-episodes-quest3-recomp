#include "sin_compat/original_nested_rebalance_phase654.hpp"
#include <limits>
#include <initializer_list>

namespace sin_compat::original_nested_rebalance_phase654 {
namespace {
struct BusyGuard {
  bool& flag;
  explicit BusyGuard(bool& f) noexcept : flag(f) { flag = true; }
  ~BusyGuard() noexcept { flag = false; }
};
bool add(std::uint32_t base, std::uint32_t displacement,
         std::uint32_t& address) noexcept {
  if (base == 0 || base > std::numeric_limits<std::uint32_t>::max() - displacement)
    return false;
  address = base + displacement;
  return true;
}
}

bool Rebalance::init_sentinel(Result& result) {
  std::uint8_t bit = 0;
  result.stage = Stage::ReadSentinelFlag;
  if (!host_.read_u8(kSentinelFlagVA, bit)) return false;
  if (bit & 1u) return true;
  std::uint32_t flags = 0;
  result.stage = Stage::InitSentinel;
  if (!host_.read_u32(kSentinelFlagVA, flags) ||
      !host_.write_u32(kSentinelFlagVA, flags | 1u)) return false;
  ++result.writes_committed;
  for (std::uint32_t offset : {0u, 2u, 4u}) {
    if (!host_.write_u16(kSentinelVA + offset, kNil)) return false;
    ++result.writes_committed;
  }
  if (!host_.write_u16(kSentinelVA + 6u, kBlack)) return false;
  ++result.writes_committed;
  result.sentinel_initialized = true;
  return true;
}

bool Rebalance::field(std::uint32_t map, std::uint16_t node,
                      std::uint32_t offset, std::uint32_t& address,
                      Result& result) {
  if (!init_sentinel(result)) return false;
  if (node == kNil) return add(kSentinelVA, offset, address);
  std::uint32_t slot_array_address = 0, base = 0;
  result.stage = Stage::ReadNodeArray;
  if (!add(map, 4u, slot_array_address) ||
      !host_.read_u32(slot_array_address, base)) {
    result.code = Code::AddressOverflow;
    return false;
  }
  if (!add(base, static_cast<std::uint32_t>(node) * 12u + offset, address)) {
    result.code = Code::AddressOverflow;
    return false;
  }
  return true;
}

bool Rebalance::read(std::uint32_t map, std::uint16_t node,
                     std::uint32_t offset, std::uint16_t& value, Result& result) {
  std::uint32_t address = 0;
  if (!field(map, node, offset, address, result)) return false;
  result.stage = Stage::ReadNode;
  return host_.read_u16(address, value);
}

bool Rebalance::write(std::uint32_t map, std::uint16_t node,
                      std::uint32_t offset, std::uint16_t value, Result& result) {
  std::uint32_t address = 0;
  if (!field(map, node, offset, address, result)) return false;
  result.stage = Stage::WriteNode;
  if (!host_.write_u16(address, value)) return false;
  ++result.writes_committed;
  return true;
}

bool Rebalance::read_root(std::uint32_t map, std::uint16_t& root, Result& result) {
  std::uint32_t address = 0;
  result.stage = Stage::ReadRoot;
  if (!add(map, 0x10u, address)) {
    result.code = Code::AddressOverflow;
    return false;
  }
  return host_.read_u16(address, root);
}

bool Rebalance::write_root(std::uint32_t map, std::uint16_t root, Result& result) {
  std::uint32_t address = 0;
  result.stage = Stage::WriteRoot;
  if (!add(map, 0x10u, address)) {
    result.code = Code::AddressOverflow;
    return false;
  }
  if (!host_.write_u16(address, root)) return false;
  ++result.writes_committed;
  return true;
}

bool Rebalance::rotate(std::uint32_t map, std::uint16_t pivot, bool left,
                       Result& result) {
  const std::uint32_t toward = left ? 2u : 0u;
  const std::uint32_t away = left ? 0u : 2u;
  result.stage = left ? Stage::RotateLeft : Stage::RotateRight;
  if (pivot == kNil) {
    result.code = Code::InvalidRotation;
    return false;
  }
  std::uint16_t opposite = kNil, subchild = kNil, parent = kNil, root = kNil;
  if (!read(map, pivot, toward, opposite, result)) return false;
  if (opposite == kNil) {
    result.code = Code::InvalidRotation;
    return false;
  }
  if (!read(map, opposite, away, subchild, result) ||
      !read(map, pivot, 4u, parent, result) ||
      !read_root(map, root, result)) return false;
  if (!write(map, pivot, toward, subchild, result)) return false;
  if (subchild != kNil && !write(map, subchild, 4u, pivot, result)) return false;
  if (!write(map, opposite, 4u, parent, result)) return false;
  if (pivot == root) {
    if (!write_root(map, opposite, result)) return false;
  } else {
    if (parent == kNil) {
      result.code = Code::InvalidRotation;
      return false;
    }
    std::uint16_t left_child = kNil;
    if (!read(map, parent, 0u, left_child, result)) return false;
    if (!write(map, parent, left_child == pivot ? 0u : 2u, opposite, result))
      return false;
  }
  if (!write(map, opposite, away, pivot, result) ||
      !write(map, pivot, 4u, opposite, result)) return false;
  if (left) ++result.left_rotations;
  else ++result.right_rotations;
  return true;
}

Result Rebalance::rotate_left(std::uint32_t map, std::uint16_t pivot) noexcept {
  Result result{};
  if (busy_) { result.code = Code::Reentrant; return result; }
  if (!map) { result.code = Code::InvalidMap; return result; }
  BusyGuard guard(busy_);
  try {
    if (rotate(map, pivot, true, result)) result.code = Code::Completed;
  } catch (...) { result.code = Code::HostRejected; }
  return result;
}

Result Rebalance::rotate_right(std::uint32_t map, std::uint16_t pivot) noexcept {
  Result result{};
  if (busy_) { result.code = Code::Reentrant; return result; }
  if (!map) { result.code = Code::InvalidMap; return result; }
  BusyGuard guard(busy_);
  try {
    if (rotate(map, pivot, false, result)) result.code = Code::Completed;
  } catch (...) { result.code = Code::HostRejected; }
  return result;
}

Result Rebalance::fixup(std::uint32_t map, std::uint16_t inserted) noexcept {
  Result result{};
  if (busy_) { result.code = Code::Reentrant; return result; }
  if (!map) { result.code = Code::InvalidMap; return result; }
  if (inserted == kNil) { result.code = Code::InvalidNode; return result; }
  BusyGuard guard(busy_);
  try {
    std::uint16_t current = inserted, root = kNil;
    if (!read_root(map, root, result)) return result;
    if (root == kNil) { result.code = Code::InvalidNode; return result; }
    while (current != root) {
      if (++result.iterations > 65536u) {
        result.code = Code::LoopGuard;
        return result;
      }
      std::uint16_t parent = kNil, grandparent = kNil, parent_color = kBlack;
      if (!read(map, current, 4u, parent, result) ||
          !read(map, parent, 6u, parent_color, result)) return result;
      if (parent_color != kRed) break;
      if (!read(map, parent, 4u, grandparent, result)) return result;
      if (grandparent == kNil) {
        result.code = Code::InvalidNode;
        return result;
      }
      std::uint16_t left_child = kNil;
      if (!read(map, grandparent, 0u, left_child, result)) return result;
      const bool parent_on_left = left_child == parent;
      std::uint16_t uncle = kNil, uncle_color = kBlack;
      if (!read(map, grandparent, parent_on_left ? 2u : 0u, uncle, result) ||
          !read(map, uncle, 6u, uncle_color, result)) return result;
      if (uncle_color == kRed) {
        if (!write(map, parent, 6u, kBlack, result) ||
            !write(map, uncle, 6u, kBlack, result) ||
            !write(map, grandparent, 6u, kRed, result)) return result;
        current = grandparent;
      } else {
        std::uint16_t child = kNil;
        if (!read(map, parent, parent_on_left ? 2u : 0u, child, result))
          return result;
        if (child == current) {
          // Native saves the old parent as the loop cursor before rotation.
          const std::uint16_t old_parent = parent;
          if (!rotate(map, parent, parent_on_left, result)) return result;
          parent = current;
          current = old_parent;
          if (!read(map, parent, 4u, grandparent, result)) return result;
        }
        if (!write(map, parent, 6u, kBlack, result) ||
            !write(map, grandparent, 6u, kRed, result) ||
            !rotate(map, grandparent, !parent_on_left, result)) return result;
      }
      if (!read_root(map, root, result)) return result;
    }
    if (!read_root(map, root, result)) return result;
    // The native root-only fast path does NOT initialize the lazy sentinel.
    // It writes directly via DWORD[map+4] + uint16(root)*12 + 6.
    std::uint32_t slot_array_address = 0, base = 0, color_address = 0;
    result.stage = Stage::ReadNodeArray;
    if (!add(map, 4u, slot_array_address) ||
        !host_.read_u32(slot_array_address, base) ||
        !add(base, static_cast<std::uint32_t>(root) * 12u + 6u, color_address)) {
      result.code = Code::AddressOverflow;
      return result;
    }
    result.stage = Stage::WriteNode;
    if (!host_.write_u16(color_address, kBlack)) return result;
    ++result.writes_committed;
    result.root_blacked = true;
    result.code = Code::Completed;
  } catch (...) { result.code = Code::HostRejected; }
  return result;
}
}  // namespace sin_compat::original_nested_rebalance_phase654
