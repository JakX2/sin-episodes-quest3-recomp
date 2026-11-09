#include "sin_compat/original_viewmodel_origin_offset.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cmath>
#include <cstdio>

using namespace sin_compat::client;

namespace {

bool near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.00001f;
}

void assert_vector(OriginalViewModelVector3f actual,
                   OriginalViewModelVector3f expected) {
    assert(near(actual.x, expected.x));
    assert(near(actual.y, expected.y));
    assert(near(actual.z, expected.z));
}

void test_identity_basis_adds_each_component_on_its_axis() {
    const OriginalViewModelOriginInputs inputs{
        {10.0f, 20.0f, 30.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {2.0f, 3.0f, 4.0f}};
    assert_vector(compose_original_viewmodel_origin(inputs), {12.0f, 23.0f, 34.0f});
}

void test_rotated_view_basis_uses_right_forward_up_not_world_axes() {
    const OriginalViewModelOriginInputs inputs{
        {100.0f, -20.0f, 5.0f},
        {0.0f, 1.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {2.0f, 3.0f, 4.0f}};
    assert_vector(compose_original_viewmodel_origin(inputs), {97.0f, -18.0f, 9.0f});
}

void test_host_basis_is_not_normalized_or_reinterpreted() {
    const OriginalViewModelOriginInputs inputs{
        {1.0f, 2.0f, 3.0f},
        {2.0f, 0.0f, 0.0f},
        {0.0f, 3.0f, 0.0f},
        {0.0f, 0.0f, 4.0f},
        {1.5f, -2.0f, 0.25f}};
    assert_vector(compose_original_viewmodel_origin(inputs), {4.0f, -4.0f, 4.0f});
}

void test_zero_offset_preserves_input_origin() {
    const OriginalViewModelOriginInputs inputs{
        {-7.0f, 11.0f, 2.5f},
        {0.0f, 1.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 0.0f}};
    assert_vector(compose_original_viewmodel_origin(inputs), inputs.input_origin);
}

void test_negative_and_mixed_offsets_keep_signed_components() {
    const OriginalViewModelOriginInputs inputs{
        {4.0f, 5.0f, 6.0f},
        {1.0f, 2.0f, 0.0f},
        {0.0f, 1.0f, -1.0f},
        {-1.0f, 0.0f, 2.0f},
        {-2.0f, 3.0f, -0.5f}};
    assert_vector(compose_original_viewmodel_origin(inputs), {2.5f, 4.0f, 2.0f});
}

} // namespace

int main() {
    test_identity_basis_adds_each_component_on_its_axis();
    test_rotated_view_basis_uses_right_forward_up_not_world_axes();
    test_host_basis_is_not_normalized_or_reinterpreted();
    test_zero_offset_preserves_input_origin();
    test_negative_and_mixed_offsets_keep_signed_components();
    std::puts("PASS: original SiN client viewmodel origin-offset composition (portable math only)");
    return 0;
}
