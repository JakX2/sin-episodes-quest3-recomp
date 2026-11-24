#pragma once
#include <cstddef>
#include <cstdint>

namespace sin_port::vcd {
// Portable VCD lexer for owner-provided scene text. The original server
// wrapper delegates to engine ParseFile; this is not that engine's binary.
struct Tokenizer {
    const char* data=nullptr;
    std::size_t bytes=0,pos=0;
    char current[1024]{};
    bool failed=false,quoted=false;
    bool begin(const char* text,std::size_t size) noexcept {
        data=text;bytes=size;pos=0;current[0]=0;failed=false;quoted=false;
        return text!=nullptr;
    }
    char at(std::size_t i) const noexcept {return data&&i<bytes?data[i]:0;}
    static bool horizontal(char c) noexcept {return c==' '||c=='\t'||c=='\r'||c=='\v'||c=='\f';}
    static bool whitespace(char c) noexcept {return horizontal(c)||c=='\n';}
    // 221a2ae0 tests the raw cursor for LF, semicolon, hash or //.
    // The portable lexer positions it past horizontal whitespace only.
    bool same_line() const noexcept {
        const char c=at(pos);
        return !failed&&c&&c!='\n'&&c!=';'&&c!='#'&&!(c=='/'&&at(pos+1)=='/');
    }
    bool next(unsigned /*crossline*/,unsigned& more) noexcept {
        more=0;current[0]=0;quoted=false;
        if(failed||!data)return false;
        for(;;){
            while(whitespace(at(pos)))++pos;
            if(at(pos)=='/'&&at(pos+1)=='/'){
                while(at(pos)&&at(pos)!='\n')++pos;
                continue;
            }
            break;
        }
        char c=at(pos);
        if(!c)return true;
        std::size_t n=0;
        const auto append=[&](char ch){if(n+1>=sizeof(current))return false;current[n++]=ch;return true;};
        if(c=='"'){
            quoted=true;++pos;
            while((c=at(pos))&&c!='"'){
                if(!append(c)){failed=true;current[0]=0;return false;}++pos;
            }
            if(c!='"'){failed=true;current[0]=0;return false;}
            ++pos;
        }else if(c=='{'||c=='}'){
            current[n++]=c;++pos;
        }else{
            while((c=at(pos))&&!whitespace(c)&&c!='{'&&c!='}'&&c!='"'){
                if(!append(c)){failed=true;current[0]=0;return false;}++pos;
            }
        }
        current[n]=0;
        while(horizontal(at(pos)))++pos;
        more=1;return true;
    }
};
}
