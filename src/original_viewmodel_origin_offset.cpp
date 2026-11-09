#include "sin_compat/original_viewmodel_origin_offset.hpp"

namespace sin_compat::client {
namespace {

OriginalViewModelVector3f scale(OriginalViewModelVector3f vector,
                                float amount) noexcept {
    return {vector.x * amount, vector.y * amount, vector.z * amount};
}

OriginalViewModelVector3f add(OriginalViewModelVector3f left,
                              OriginalViewModelVector3f right) noexcept {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

} // namespace

OriginalViewModelVector3f compose_original_viewmodel_origin(
    const OriginalViewModelOriginInputs& inputs) noexcept {
    const auto right_offset = scale(inputs.view_right, inputs.origin_offset.x);
    const auto forward_offset = scale(inputs.view_forward, inputs.origin_offset.y);
    const auto up_offset = scale(inputs.view_up, inputs.origin_offset.z);

    return add(add(add(inputs.input_origin, right_offset), forward_offset),
               up_offset);
}

} // namespace sin_compat::client
