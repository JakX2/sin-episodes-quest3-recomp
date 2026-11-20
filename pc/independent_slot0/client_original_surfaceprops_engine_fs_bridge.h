#pragma once
// Real-call-capable, opt-in 2006 SiN VFileSystem012 read-only surfaceprops.
//
// Original physics success continuation 0x241927A0 reads the manifest
// with rb/GAME; original per-file helper 0x24191EC0 opens each script
// using rb/nullptr before an unreconstructed VPhysicsSurfaceProps001
// ParseSurfaceData virtual call (slot1). We DO NOT call that method.
// Instead we reuse the now owner-retail-confirmed VFileSystem012
// slots 0,2,3,7 and independently count surface names/base references.
#if !defined(_MSC_VER) || !defined(_M_IX86)
#error Source 2006 original VFileSystem012 surface properties ABI is Win32 x86
#endif
#include "client_engine_filesystem_read.h"
#include "client_original_surfaceprops_manifest_owned.h"
#include <string>
#include <string_view>
#include <utility>

namespace sin_independent::surfaceprops_engine_fs {
namespace owned=sin_independent::original_surfaceprops;
enum class Status {
    SourceShapeVerified, InvalidFileSystem, ManifestOrSourceRefused
};
inline const char* Name(Status v) noexcept {
    switch(v){
        case Status::SourceShapeVerified:
            return "original-surfaceprops-retail-resource-shape-verified";
        case Status::InvalidFileSystem:
            return "original-surfaceprops-engine-filesystem-invalid";
        case Status::ManifestOrSourceRefused:
            return "original-surfaceprops-retail-resource-refused";
    }
    return "unknown";
}
struct Report final {
    Status status=Status::InvalidFileSystem;
    owned::Report owned_result{};
    bool verified() const noexcept {
        return status==Status::SourceShapeVerified&&owned_result.verified();
    }
};
class Bridge final {
public:
    const owned::Snapshot& snapshot() const noexcept {
        return snapshot_.value();
    }
    void Clear() noexcept{snapshot_.Clear();}
    Report ReadOnly(void* filesystem) noexcept {
        Report result{};
        engine_fs::Methods methods{};
        if(!engine_fs::Resolve(filesystem,methods,true))
            return result;
        // An injected READ callback is the only bridge between
        // actual VFileSystem012 handles and the source-free parser.
        // No Source KeyValues, VPhysicsSurfaceProps object or
        // gameplay allocator pointer is invoked/retained.
        owned::OwnedResources next{};
        const auto read=[&](std::string_view path,const char* pathID,
                            std::string& output)->bool {
            const std::string canonical(path);
            return engine_fs::ReadOne(filesystem,methods,canonical.c_str(),
                                      output,"rb",pathID);
        };
        result.owned_result=next.Load(read);
        result.status=result.owned_result.verified()?
            Status::SourceShapeVerified:
            Status::ManifestOrSourceRefused;
        if(result.verified())snapshot_=std::move(next);
        return result;
    }
private:
    owned::OwnedResources snapshot_{};
};
} // namespace sin_independent::surfaceprops_engine_fs
