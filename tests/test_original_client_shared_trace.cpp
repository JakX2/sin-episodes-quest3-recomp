#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "sin_compat/original_client_shared_trace.hpp"

namespace native=sin_compat::original_client_shared_trace;
struct Host final:native::ITraceHost {
    std::vector<native::MessageEvent> messages;
    std::vector<native::StatusEvent> statuses;
    int format_calls{};
    int preview_number_calls{};
    std::string formatted{"formatted text"};
    std::int32_t preview_number{-7};
    std::string last_format;
    std::vector<std::uint32_t> last_args;
    std::string q_vsnprintf(const std::string& format,
                            const std::vector<std::uint32_t>& words) override {
        ++format_calls;last_format=format;last_args=words;
        return formatted;
    }
    void msg(const native::MessageEvent& value) override {
        messages.push_back(value);
    }
    void status_callback(const native::StatusEvent& value) override {
        statuses.push_back(value);
    }
    std::int32_t original_preview_global_number() override {
        ++preview_number_calls;return preview_number;
    }
};
static native::TraceObject state() {
    native::TraceObject s{};
    s.descriptor_18=native::Descriptor{13u,std::string{"player_health"},0x0500u};
    s.group_string_1c="CPlayer";
    s.preview_label_38="prediction";
    return s;
}
static void verify_authentic_import_and_function_addresses() {
    static_assert(native::kDifferenceVA==0x24197910u);
    static_assert(native::kStatusVA==0x241979C0u);
    static_assert(native::kPreviewVA==0x24197FE0u);
    static_assert(native::kOriginalQvsnprintfIAT==0x2427625Cu);
    static_assert(native::kOriginalTier0MsgIAT==0x242761ECu);
    static_assert(native::kOriginalTypeNamesVA==0x242FDAE0u);
    static_assert(native::kOriginalPreviewGlobalVA==0x242FA458u);
    static_assert(native::kOriginalStackProbeVA==0x242493E0u);
    static_assert(native::kOriginalFormatBufferBytes==4096u);
    static_assert(native::kOriginalTypeNames.size()==26u);
    assert(std::string(native::original_type_name(13u))=="FIELD_EHANDLE");
    assert(std::string(native::original_type_name(4u))=="FIELD_QUATERNION");
    assert(std::string(native::original_type_name(25u))==
           "FIELD_INTERVALFIELD_MODELINDEX");
    bool refused=false;
    try {native::original_type_name(26u);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
}
static void difference_counter_unconditional_and_pending_gate() {
    auto s=state();Host host;
    native::difference(s,"%i",{42u},host);
    assert(s.difference_count_28==1u);
    assert(host.messages.empty() && host.format_calls==0);
    s.difference_pending_24=true;
    s.suppress_difference_if_callback_16=true;
    s.callback_pointer_30_nonnull=true;
    native::difference(s,"%i",{77u},host);
    assert(s.difference_count_28==2u);
    assert(s.difference_pending_24 && host.format_calls==0);
    s.suppress_difference_if_callback_16=false;
    native::difference(s,"%i",{77u},host);
    assert(s.difference_count_28==3u);
    assert(!s.difference_pending_24);
    assert(host.format_calls==1 && host.last_format=="%i");
    assert(host.last_args==std::vector<std::uint32_t>{77u});
    assert(host.messages.size()==1u);
    const auto& message=host.messages[0];
    assert(message.kind==native::MessageKind::DifferenceLine);
    assert(message.original_format=="%03i %s::%s - %s");
    assert(message.counter_or_preview_number==3u);
    assert(message.group_or_label=="CPlayer");
    assert(message.descriptor_name=="player_health");
    assert(message.message=="formatted text");
}
static void first_difference_newline_and_name_fallback() {
    auto s=state();Host host;
    s.difference_pending_24=true;
    s.descriptor_18.reset();
    native::difference(s,"hello",{},host);
    assert(host.format_calls==1 && host.messages.size()==2u);
    assert(host.messages[0].kind==native::MessageKind::FirstDifferenceNewline);
    assert(host.messages[0].original_format=="\n");
    assert(host.messages[1].descriptor_name=="empty");
    assert(host.messages[1].counter_or_preview_number==1u);
    assert(!s.difference_pending_24);
    host.messages.clear();
    s.descriptor_18=native::Descriptor{1u,std::nullopt,0u};
    s.difference_pending_24=true;
    native::difference(s,"test",{},host);
    assert(host.messages.size()==1u);
    assert(host.messages[0].descriptor_name=="NULL");
    assert(s.difference_count_28==2u);
}
static void native_32_bit_counter_wrap_and_4096_buffer_limit() {
    auto s=state();Host host;
    s.difference_count_28=std::numeric_limits<std::uint32_t>::max();
    s.difference_pending_24=true;
    host.formatted=std::string(5000u,'z');
    native::difference(s,"very long",{},host);
    assert(s.difference_count_28==0u);
    assert(host.messages.size()==1u); // No first-difference newline on wrap to 0.
    assert(host.messages[0].message.size()==4095u);
}
static void status_callback_gates_and_descriptor_flags() {
    auto s=state();Host host;
    s.status_pending_25=true;
    native::status(s,0u,"%s",{0x1234u},host);
    assert(host.format_calls==0 && host.statuses.empty());
    assert(s.status_pending_25); // No callback pointer: flag untouched.
    s.callback_pointer_30_nonnull=true;
    native::status(s,2u,"%s",{0x1234u},host);
    assert(host.format_calls==1);
    assert(host.last_args==std::vector<std::uint32_t>{0x1234u});
    assert(host.statuses.size()==1u && !s.status_pending_25);
    const auto& e=host.statuses[0];
    assert(e.group=="CPlayer" && e.field_name=="player_health");
    assert(e.original_field_type=="FIELD_EHANDLE");
    assert(e.original_formatted_message=="formatted text");
    assert(e.descriptor_flag_08 && e.descriptor_flag_10);
    assert(e.operation_not_equal_one && e.operation_equals_two);
    s.status_pending_25=true;
    s.descriptor_18->flags_12=0u;
    native::status(s,1u,"none",{},host);
    const auto& normal=host.statuses.back();
    assert(!normal.descriptor_flag_08 && !normal.descriptor_flag_10);
    assert(!normal.operation_not_equal_one && !normal.operation_equals_two);
    assert(s.status_pending_25==false);
}
static void status_preserves_fallback_name_and_host_null_refusal() {
    auto s=state();Host host;
    s.status_pending_25=true;s.callback_pointer_30_nonnull=true;
    s.descriptor_18->name_pointer_04.reset();
    native::status(s,0u,"%d",{},host);
    assert(host.statuses.size()==1u);
    assert(host.statuses[0].field_name=="NULL");
    assert(host.statuses[0].operation_not_equal_one);
    s.status_pending_25=true;s.descriptor_18.reset();
    bool refused=false;
    try {native::status(s,0u,"x",{},host);}
    catch(const std::runtime_error&){refused=true;}
    assert(refused && s.status_pending_25);
    s.descriptor_18=native::Descriptor{26u,std::string{"bad"},0u};
    refused=false;
    try {native::status(s,0u,"x",{},host);}
    catch(const std::out_of_range&){refused=true;}
    assert(refused);
}
static void preview_is_independent_of_all_other_gates() {
    auto s=state();Host host;
    s.difference_pending_24=false;s.status_pending_25=false;
    s.callback_pointer_30_nonnull=false;
    native::preview(s,"vec (%f)",{0x3f800000u},host);
    assert(host.format_calls==1 && host.preview_number_calls==1);
    assert(host.last_format=="vec (%f)");
    assert(host.messages.size()==1u);
    const auto& m=host.messages[0];
    assert(m.kind==native::MessageKind::PreviewLine);
    assert(m.original_format=="%i %s %s : %s\n");
    assert(m.counter_or_preview_number==static_cast<std::uint32_t>(-7));
    assert(m.group_or_label=="prediction");
    assert(m.descriptor_name=="player_health");
    assert(m.message=="formatted text");
    s.descriptor_18.reset();
    bool refused=false;
    try {native::preview(s,"value",{},host);}
    catch(const std::runtime_error&){refused=true;}
    assert(refused);
}
int main(){
    verify_authentic_import_and_function_addresses();
    difference_counter_unconditional_and_pending_gate();
    first_difference_newline_and_name_fallback();
    native_32_bit_counter_wrap_and_4096_buffer_limit();
    status_callback_gates_and_descriptor_flags();
    status_preserves_fallback_name_and_host_null_refusal();
    preview_is_independent_of_all_other_gates();
}
