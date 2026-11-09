#pragma once

// Independently authored reconstruction of the verified offset-composition
// portion of SiN's original client.dll C_BaseViewModel::CalcViewModelView.
// The engine host supplies its own origin and view-angle basis vectors.

#include <type_traits>

namespace sin_compat::client {

// The original m_viewmodelOriginOffset receive field is a 12-byte float triplet.
struct OriginalViewModelVector3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(OriginalViewModelVector3f) == 12,
              "SiN viewmodel origin offset is three 32-bit floats");
static_assert(std::is_standard_layout<OriginalViewModelVector3f>::value,
              "viewmodel vector must remain standard-layout");
static_assert(std::is_trivially_copyable<OriginalViewModelVector3f>::value,
              "viewmodel vector must remain trivially copyable");

struct OriginalViewModelOriginInputs {
    OriginalViewModelVector3f input_origin{};
    OriginalViewModelVector3f view_right{};
    OriginalViewModelVector3f view_forward{};
    OriginalViewModelVector3f view_up{};
    OriginalViewModelVector3f origin_offset{};
};

// Reconstructs only the verified original composition:
// origin + right*offset.x + forward*offset.y + up*offset.z.
// Basis generation, network decoding, and the remaining camera/viewmodel
// conditions belong to the native client host and are intentionally excluded.
OriginalViewModelVector3f compose_original_viewmodel_origin(
    const OriginalViewModelOriginInputs& inputs) noexcept;

} // namespace sin_compat::client
