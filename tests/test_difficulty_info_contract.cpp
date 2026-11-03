#include "sin_compat/difficulty_info_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
using namespace sin_compat::difficulty;

struct Token { char kind; std::uint32_t bits; };
static std::uint32_t encode(float x) { std::uint32_t v; std::memcpy(&v,&x,4); return v; }
static float decode(std::uint32_t x) { float v; std::memcpy(&v,&x,4); return v; }

struct Fixture : DDInfoReader, DDInfoWriter {
    std::vector<Token> tokens{};
    std::size_t cursor=0, limit=static_cast<std::size_t>(-1);
    bool read_bit(bool& v) override {
        if (cursor>=tokens.size() || cursor>=limit || tokens[cursor].kind!='b') return false;
        v=tokens[cursor++].bits!=0; return true;
    }
    bool read_i16(std::int16_t& v) override {
        if (cursor>=tokens.size() || cursor>=limit || tokens[cursor].kind!='s') return false;
        v=static_cast<std::int16_t>(static_cast<std::uint16_t>(tokens[cursor++].bits));
        return true;
    }
    bool read_f32(float& v) override {
        if (cursor>=tokens.size() || cursor>=limit || tokens[cursor].kind!='f') return false;
        v=decode(tokens[cursor++].bits); return true;
    }
    bool write_bit(bool v) override {
        if (tokens.size()>=limit) return false;
        tokens.push_back({'b',v?1u:0u}); return true;
    }
    bool write_i16(std::int16_t v) override {
        if (tokens.size()>=limit) return false;
        tokens.push_back({'s',static_cast<std::uint16_t>(v)}); return true;
    }
    bool write_f32(float v) override {
        if (tokens.size()>=limit) return false;
        tokens.push_back({'f',encode(v)}); return true;
    }
};

int main() {
    static_assert(ddinfo_field_bit_count==2+32+16+1+6);
    DDInfoFields original{};
    original.first_flags={{true,false}};
    original.computed_float=3.125f;
    original.integer=-12345;
    original.shared_flag=true;
    original.advisor_flags={{false,true,false,false,true,false}};
    Fixture f;
    assert(write_ddinfo_fields(f,original));
    assert(f.tokens.size()==11);
    const char kinds[]={'b','b','f','s','b','b','b','b','b','b','b'};
    for (std::size_t n=0;n<f.tokens.size();++n) assert(f.tokens[n].kind==kinds[n]);
    DDInfoFields copy{};
    assert(read_ddinfo_fields(f,copy));
    assert(f.cursor==11 && copy.first_flags==original.first_flags &&
           copy.computed_float==original.computed_float &&
           copy.integer==original.integer && copy.shared_flag==original.shared_flag &&
           copy.advisor_flags==original.advisor_flags);
    auto effective=client_effective_ddinfo_flags(copy);
    assert(!effective[0] && effective[1]);
    assert(effective[2] && effective[3] && effective[4] && !effective[5]);
    assert(!copy.advisor_flags[2] && !copy.advisor_flags[3]);
    Fixture again;
    assert(write_ddinfo_fields(again,copy));
    for (std::size_t n=0;n<again.tokens.size();++n) {
        assert(again.tokens[n].kind==f.tokens[n].kind);
        assert(again.tokens[n].bits==f.tokens[n].bits);
    }
    for (std::size_t n=0;n<11;++n) {
        Fixture truncated;
        truncated.tokens=f.tokens; truncated.limit=n;
        DDInfoFields prior{}; prior.integer=1234;
        assert(!read_ddinfo_fields(truncated,prior));
        assert(prior.integer==1234);
    }
    Fixture wrong;
    wrong.tokens=f.tokens; wrong.tokens[3].kind='f';
    DDInfoFields ignored{};
    assert(!read_ddinfo_fields(wrong,ignored));
    for (std::size_t n=0;n<11;++n) {
        Fixture failure; failure.limit=n;
        assert(!write_ddinfo_fields(failure,original));
        assert(failure.tokens.size()==n);
    }
    copy.shared_flag=false;
    effective=client_effective_ddinfo_flags(copy);
    assert(!effective[2] && !effective[3]);
}
