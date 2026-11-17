// Portable test of independent original-derived handle resolution. Native
// Windows x86 CI also verifies the original record stride and field offsets.
#include "server_entity_handle_core.h"
#include <array>
#include <cstdio>

class CBaseEntity { public: int marker = 0; };

static int fail(int code, const char* reason) {
    std::fprintf(stderr, "FAIL entity handle core %d: %s\n", code, reason);
    return code;
}
int main() {
    using namespace sin_independent;
    static_assert(kEntityIndexMask == kEntityHandleCapacity - 1, "index range");
    static_assert(SplitEntityHandle(0xABCDE123u).entry == 0x123u, "12-bit index");
    static_assert(SplitEntityHandle(0xABCDE123u).serial == 0xABCDEu, "20-bit serial");
    CBaseEntity first{1}, replacement{2}, last{3}, world{4};
    std::array<EntityHandleEntry,kEntityHandleCapacity> entries{};
    EntityHandleView view(entries.data(), entries.size());
    constexpr std::uint32_t index = 0x123u;
    constexpr std::uint32_t oldHandle = 0xABCDE123u;
    constexpr std::uint32_t newHandle = 0xABCDF123u;
    entries[index].entity = &first;
    entries[index].serial = 0xABCDEu;
    if (view.Resolve(oldHandle) != &first) return fail(1,"known handle");
    if (view.Resolve(newHandle) != nullptr) return fail(2,"wrong serial");

    // Reuse a freed entry; an old serial must never identify the new entity.
    entries[index].entity = nullptr;
    if (view.Resolve(oldHandle) != nullptr) return fail(3,"unoccupied entry");
    entries[index].entity = &replacement;
    entries[index].serial = 0xABCDFu;
    if (view.Resolve(oldHandle) != nullptr || view.Resolve(newHandle) != &replacement)
        return fail(4,"stale handle after entry reuse");

    // Highest entry, highest valid serial, and the invalid sentinel.
    entries[0xFFF].entity = &last;
    entries[0xFFF].serial = 0xFFFFEu;
    if (view.Resolve(0xFFFFEFFFu) != &last) return fail(5,"last entry");
    entries[0xFFF].serial = 0xFFFFFu;
    if (view.Resolve(kInvalidEntityHandle) != nullptr)
        return fail(6,"sentinel always invalid, even if table matches");
    entries[0].entity = &world;
    entries[0].serial = 0;
    if (view.Resolve(0) != &world) return fail(7,"zero is not invalid sentinel");

    EntityHandleView shortView(entries.data(), 2);
    if (shortView.Resolve(newHandle) != nullptr) return fail(8,"bounded short view");
    EntityHandleView emptyView(nullptr, kEntityHandleCapacity);
    if (emptyView.Resolve(0) != nullptr) return fail(9,"null storage");

    std::puts("PASS entity handle resolution: index, serial, invalid, empty, reuse, bounds");
    return 0;
}
