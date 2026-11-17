// Independent PE32 DLL lifecycle fixture. No original DLL forwarding.
#include "lifecycle_contract.hpp"
#include <cstring>
namespace {
using namespace sin_synthetic_lifecycle;
class Runtime final : public IRuntime {
public:
    bool __thiscall initialize(const HostCallbacks* callbacks) noexcept override {
        if (active_) return false; // never replace an active callback owner
        if (!callbacks || callbacks->size != sizeof(HostCallbacks) ||
            !callbacks->notify || !callbacks->context) return false;
        // Copy the callback bundle before notification: host owns its storage.
        callbacks_ = *callbacks;
        active_ = true;
        ++generation_;
        callbacks_.notify(callbacks_.context, kAttach);
        return true;
    }
    bool __thiscall initialized() noexcept override { return active_; }
    std::uint32_t __thiscall generation() noexcept override { return generation_; }
    void __thiscall shutdown() noexcept override {
        if (!active_) return;
        // Clear the state before calling back to tolerate a reentrant query.
        HostCallbacks prior = callbacks_;
        callbacks_ = {};
        active_ = false;
        prior.notify(prior.context, kDetach);
    }
    ~Runtime() { shutdown(); } // DLL detach cleans up only if host still owns callbacks.
private:
    HostCallbacks callbacks_{};
    std::uint32_t generation_ = 0;
    bool active_ = false;
};
Runtime runtime;
}
extern "C" __declspec(dllexport) void* __cdecl CreateInterface(
    const char* name, int* status) noexcept {
    const bool found = name && std::strcmp(name, sin_synthetic_lifecycle::kName) == 0;
    if (status) *status = found ? 0 : 1;
    return found ? static_cast<sin_synthetic_lifecycle::IRuntime*>(&runtime) : nullptr;
}
