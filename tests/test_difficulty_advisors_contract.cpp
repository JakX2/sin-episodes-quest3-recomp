#include "sin_compat/difficulty_advisors_contract.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
using namespace sin_compat::difficulty;

struct Datum { char kind; std::uint32_t bits; };
static std::uint32_t as_bits(float v) { std::uint32_t b; std::memcpy(&b,&v,4); return b; }
static float as_float(std::uint32_t b) { float v; std::memcpy(&v,&b,4); return v; }
struct Fixture final : TypedReader, TypedWriter {
    std::vector<Datum> fields;
    std::size_t pos=0, limit=static_cast<std::size_t>(-1);
    bool read_f32(float& v) override {
        if (pos>=fields.size() || pos>=limit || fields[pos].kind!='f') return false;
        v=as_float(fields[pos++].bits);return true;
    }
    bool read_i16(std::int16_t& v) override {
        if (pos>=fields.size() || pos>=limit || fields[pos].kind!='s') return false;
        v=static_cast<std::int16_t>(static_cast<std::uint16_t>(fields[pos++].bits));return true;
    }
    bool write_f32(float v) override {
        if (fields.size()>=limit) return false;
        fields.push_back({'f',as_bits(v)});return true;
    }
    bool write_i16(std::int16_t v) override {
        if (fields.size()>=limit) return false;
        fields.push_back({'s',static_cast<std::uint16_t>(v)});return true;
    }
};
int main() {
    static_assert(advisor_scores_field_bits==288 && advisor_channels_field_bits==768);
    static_assert(advisor_count==6 && channel_count==4);
    AdvisorScores scores{};
    AdvisorChannels channels{};
    for (std::size_t a=0;a<advisor_count;++a) {
        scores.entries[a] = {static_cast<float>(a)+0.125f,static_cast<std::int16_t>(static_cast<int>(a)-3)};
        for (std::size_t c=0;c<channel_count;++c)
            channels.entries[a][c]=static_cast<float>(a*100+c)+0.5f;
    }
    Fixture s;
    assert(write_advisor_scores(s,scores));
    assert(s.fields.size()==12);
    for (std::size_t a=0;a<advisor_count;++a) {
        assert(s.fields[2*a].kind=='f' && s.fields[2*a].bits==as_bits(scores.entries[a].value));
        assert(s.fields[2*a+1].kind=='s');
    }
    AdvisorScores decoded_scores{};
    assert(read_advisor_scores(s,decoded_scores));
    assert(s.pos==12);
    for (std::size_t a=0;a<advisor_count;++a) {
        assert(decoded_scores.entries[a].value==scores.entries[a].value);
        assert(decoded_scores.entries[a].code==scores.entries[a].code);
    }
    Fixture ch;
    assert(write_advisor_channels(ch,channels));
    assert(ch.fields.size()==24);
    for (std::size_t a=0;a<advisor_count;++a)
        for (std::size_t c=0;c<channel_count;++c)
            assert(ch.fields[a*4+c].bits==as_bits(channels.entries[a][c]));
    AdvisorChannels decoded_channels{};
    assert(read_advisor_channels(ch,decoded_channels));
    assert(ch.pos==24 && decoded_channels.entries==channels.entries);
    ClientAdvisorPresentation presentation{};
    apply_advisor_scores(presentation,decoded_scores);
    apply_advisor_channels(presentation,decoded_channels);
    for (std::size_t a=0;a<advisor_count;++a) {
        assert(presentation.scores[a]==scores.entries[a].value);
        assert(presentation.codes[a]==scores.entries[a].code);
        for (std::size_t c=0;c<channel_count;++c)
            assert(presentation.channels[c][a]==channels.entries[a][c]);
    }
    // Verified server x86 case matrix for index<4. Last two entries bypass.
    const int expected[5][5] = {
        {2,2,2,3,4}, {0,2,2,3,4}, {0,1,2,3,4},
        {0,1,2,2,4}, {0,1,2,2,2}
    };
    for (std::size_t a=0;a<advisor_count;++a)
        for (int state=0;state<=4;++state)
            for (int raw=0;raw<=4;++raw)
                assert(select_advisor_score_code(a,state,raw)==
                    (a>=4 ? raw : expected[state][raw]));
    assert(select_advisor_score_code(0,-1,-1)==2);
    assert(select_advisor_score_code(0,-1,-2)==-2);
    assert(select_advisor_score_code(0,8,8)==2);
    assert(select_advisor_score_code(4,8,8)==8);
    // Failures must not partially publish decoded data, even though writer
    // rollback is beyond the scope of this transport-agnostic contract.
    for (std::size_t cutoff=0;cutoff<12;++cutoff) {
        Fixture t; t.fields=s.fields; t.limit=cutoff;
        AdvisorScores before{};before.entries[0].value=999.0f;
        assert(!read_advisor_scores(t,before));
        assert(before.entries[0].value==999.0f);
    }
    for (std::size_t cutoff=0;cutoff<24;++cutoff) {
        Fixture t; t.fields=ch.fields; t.limit=cutoff;
        AdvisorChannels before{};before.entries[0][0]=999.0f;
        assert(!read_advisor_channels(t,before));
        assert(before.entries[0][0]==999.0f);
    }
    Fixture wrong; wrong.fields=s.fields;wrong.fields[1].kind='f';
    AdvisorScores invalid{};assert(!read_advisor_scores(wrong,invalid));
    Fixture no;no.limit=11;assert(!write_advisor_scores(no,scores));assert(no.fields.size()==11);
    Fixture no2;no2.limit=23;assert(!write_advisor_channels(no2,channels));assert(no2.fields.size()==23);
}
