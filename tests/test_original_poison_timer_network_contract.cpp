#include "sin_compat/original_poison_timer_network_contract.hpp"
#include "sin_compat/original_client_poison_timer_hud.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace sin_compat::original_poison_timer_network;
namespace {
void write_f32(std::vector<std::uint8_t>& bytes,
               std::size_t original_offset,float value) {
    assert(original_offset+sizeof(float)<=bytes.size());
    std::memcpy(bytes.data()+original_offset,&value,sizeof(value));
}
float read_f32(const std::vector<std::uint8_t>& bytes,
               std::size_t original_offset) {
    assert(original_offset+sizeof(float)<=bytes.size());
    float value=0;
    std::memcpy(&value,bytes.data()+original_offset,sizeof(value));
    return value;
}
void test_original_x86_client_server_nested_property_evidence() {
    constexpr auto client=original_client_field();
    constexpr auto server=original_server_field();
    static_assert(client.local_offset==0x11C8U,"Original client nested table");
    static_assert(server.local_offset==0x137CU,"Original server nested table");
    static_assert(client.nested_offset==server.nested_offset,"Same nested field");
    static_assert(client.absolute_player_offset==0x11ECU,"HUD player field");
    static_assert(server.absolute_player_offset==0x13A0U,"Server send field");
    static_assert(client.absolute_player_offset!=server.absolute_player_offset,
                  "Must not cast server entity memory to client structure");
    assert(client.network_table=="DT_HL2Local");
    assert(server.network_table=="DT_HL2Local");
    assert(client.parent_property=="m_HL2Local");
    assert(client.field_property=="m_poisonTimer");
}
void test_client_and_server_offsets_in_separate_synthetic_player_images() {
    std::vector<std::uint8_t> client(0x1400U,0);
    std::vector<std::uint8_t> server(0x1500U,0);
    write_f32(client,kOriginalClientPlayerPoisonTimerOffset,0.75f);
    write_f32(server,kOriginalServerPlayerPoisonTimerOffset,0.25f);
    assert(read_f32(client,kOriginalClientPlayerPoisonTimerOffset)==0.75f);
    assert(read_f32(server,kOriginalServerPlayerPoisonTimerOffset)==0.25f);
    assert(read_f32(server,kOriginalClientPlayerPoisonTimerOffset)==0.0f);

    // The original CHudPoisonTimer consumes its own client object field.
    sin_compat::original_client_poison_hud::State hud{};
    hud.paint_enabled=1;
    hud.positive_resource=11;
    hud.nonpositive_resource=22;
    update_display_from_player(hud,true,
        read_f32(client,kOriginalClientPlayerPoisonTimerOffset));
    assert(std::fabs(hud.display_fraction-0.25f)<1e-6f);
    assert(choose_paint_resource(hud).selected_resource==11U);
    write_f32(client,kOriginalClientPlayerPoisonTimerOffset,1.25f);
    update_display_from_player(hud,true,
        read_f32(client,kOriginalClientPlayerPoisonTimerOffset));
    assert(std::fabs(hud.display_fraction-(-0.25f))<1e-6f);
    assert(choose_paint_resource(hud).selected_resource==22U);
}
}
int main() {
    test_original_x86_client_server_nested_property_evidence();
    test_client_and_server_offsets_in_separate_synthetic_player_images();
}
