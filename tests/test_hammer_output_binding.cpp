#include "sin_compat/hammer_output_binding.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace sin_compat;

struct FakeHost final : IEntityOutputHost {
    std::map<std::string, std::vector<EntityRef>> names;
    std::vector<std::string> delivered;
    std::vector<std::pair<EntityRef, EntityRef>> contexts;
    bool is_live(EntityRef e) const override { return e.index > 0 && e.generation == 1; }
    std::vector<EntityRef> resolve_targets(const std::string& target,
                                           EntityRef activator, EntityRef caller) override {
        if (target == "!activator") return {activator};
        if (target == "!caller") return {caller};
        if (target == "effects_*") return {EntityRef{4, 1}, EntityRef{5, 1}};
        const auto it = names.find(target);
        return it == names.end() ? std::vector<EntityRef>{} : it->second;
    }
    void accept_input(EntityRef entity, const std::string& input,
                      const std::string& parameter, EntityRef activator,
                      EntityRef caller) override {
        delivered.push_back(std::to_string(entity.index) + ":" + input + ":" + parameter);
        contexts.emplace_back(activator, caller);
    }
};

int main() {
    const auto a = parse_hammer_output_value(" beam_1 , Break , , 0.10 , -1 ");
    assert(a && a.connection.target == "beam_1" && a.connection.input == "Break");
    assert(a.connection.parameter.empty() && a.connection.delay_seconds == 0.10);
    assert(a.connection.max_fires == -1);
    const auto b = parse_hammer_output_value("door\x1bOpen\x1bparam,with,commas\x1b" "0.25\x1b" "1");
    assert(b && b.connection.target == "door" && b.connection.parameter == "param,with,commas");
    assert(b.connection.delay_seconds == 0.25 && b.connection.max_fires == 1);
    assert(parse_hammer_output_value("!activator,Enable,,0,1"));
    assert(parse_hammer_output_value("effects_*,StopSound,,0,1"));
    assert(parse_hammer_output_value("door,Use,,0,0")); // disabled link, not malformed
    assert(!parse_hammer_output_value("door,Use,,0"));
    assert(!parse_hammer_output_value("door,Use,,0,-1,extra"));
    assert(!parse_hammer_output_value("door,Use,parameter,with,comma,0,-1"));
    assert(parse_hammer_output_value("door,Use,parameter,with,comma,0,-1").error ==
           HammerOutputError::WrongFieldCount);
    assert(parse_hammer_output_value("door,,unused,0,-1").error == HammerOutputError::EmptyInput);
    assert(parse_hammer_output_value(",Open,,0,-1").error == HammerOutputError::EmptyTarget);
    for (const auto* s : {"door,Open,,-1,-1", "door,Open,,nan,-1",
                          "door,Open,,inf,-1", "door,Open,,1e309,-1",
                          "door,Open,,0abc,-1", "door,Open,,,-1"}) {
        assert(parse_hammer_output_value(s).error == HammerOutputError::InvalidDelay);
    }
    for (const auto* s : {"door,Open,,0,-2", "door,Open,,0,9999999999999999999",
                          "door,Open,,0,", "door,Open,,0,1.5"}) {
        assert(parse_hammer_output_value(s).error == HammerOutputError::InvalidMaxFires);
    }
    assert(parse_hammer_output_value(std::string(16385, 'x')).error == HammerOutputError::TooLong);

    // Independently authored miniature version of SiN's *kind* of staged
    // destruction relay, not copied source-map names, geometry, or scripts.
    FakeHost host;
    const EntityRef relay{1, 1}, player{2, 1};
    host.names["beam_1"] = {{3, 1}};
    host.names["beam_2"] = {{6, 1}};
    EntityIoRuntime runtime(host);
    for (const auto* line : {"beam_1,Break,,0.10,-1", "beam_2,Break,,0.15,-1",
                             "beam_1,EnableMotion,,0.23,-1", "effects_*,StopSound,,0.25,1",
                             "!activator,Notify,,0.30,1"}) {
        const auto parsed = parse_hammer_output_value(line);
        assert(parsed && runtime.connect(relay, "OnTrigger", parsed.connection));
    }
    assert(runtime.fire(relay, "OnTrigger", player) == 5);
    assert(runtime.advance_to(0.09).inputs_delivered == 0);
    assert(runtime.advance_to(0.10).inputs_delivered == 1);
    assert(runtime.advance_to(0.15).inputs_delivered == 1);
    assert(runtime.advance_to(0.23).inputs_delivered == 1);
    assert(runtime.advance_to(0.25).inputs_delivered == 2);
    assert(runtime.advance_to(0.30).inputs_delivered == 1);
    assert((host.delivered == std::vector<std::string>{
        "3:Break:", "6:Break:", "3:EnableMotion:", "4:StopSound:",
        "5:StopSound:", "2:Notify:"}));
    assert(host.contexts.back().first == player && host.contexts.back().second == relay);
    // One-shot links are consumed at fire() time, repeating links remain.
    assert(runtime.fire(relay, "OnTrigger", player) == 3);
    assert(runtime.advance_to(0.60).inputs_delivered == 3);
}
