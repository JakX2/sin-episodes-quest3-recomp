#include "sin_compat/vcd_tokenizer.hpp"
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
using sin_port::vcd::Tokenizer;
static void synthetic(){
    Tokenizer t;unsigned more=0;
    const char sample[]="// comment\r\nactor \"Jessica\"\r\n{ time 0 1\r\nparam \"scenes\\voice.wav\" // end\r\nparam \"\"\r\n}";
    assert(t.begin(sample,sizeof(sample)-1));
    auto expect=[&](const char* value,bool same,bool quoted=false){assert(t.next(0,more)&&more&&!std::strcmp(t.current,value)&&t.same_line()==same&&t.quoted==quoted);};
    expect("actor",true);expect("Jessica",false,true);expect("{",true);expect("time",true);expect("0",true);expect("1",false);
    expect("param",true);expect("scenes\\voice.wav",false,true);expect("param",true);expect("",false,true);expect("}",false);
    assert(t.next(1,more)&&!more&&!t.failed);
    const std::string longtoken(1024,'a');assert(t.begin(longtoken.data(),longtoken.size()));assert(!t.next(1,more)&&t.failed&&!more);
    assert(t.begin("\"broken",7));assert(!t.next(1,more)&&t.failed);
    assert(t.begin("x;",2)&&t.next(1,more)&&more&&!std::strcmp(t.current,"x;"));
}
int main(int argc,char** argv){
    synthetic();
    if(argc!=2)return 0;
    unsigned files=0,events=0,actors=0,channels=0;std::uint64_t tokens=0,empty=0;
    for(const auto& entry:std::filesystem::recursive_directory_iterator(argv[1])){
        if(!entry.is_regular_file()||entry.path().extension()!=".vcd")continue;
        std::ifstream in(entry.path(),std::ios::binary);assert(in);
        std::string bytes((std::istreambuf_iterator<char>(in)),{});
        Tokenizer t;assert(t.begin(bytes.data(),bytes.size()));unsigned more=0;int depth=0;
        for(;;){assert(t.next(1,more));if(!more)break;++tokens;
            if(t.quoted){if(!*t.current)++empty;continue;}
            if(!std::strcmp(t.current,"{"))++depth;
            else if(!std::strcmp(t.current,"}")){--depth;assert(depth>=0);}
            else if(!std::strcmp(t.current,"event"))++events;
            else if(!std::strcmp(t.current,"actor"))++actors;
            else if(!std::strcmp(t.current,"channel"))++channels;
        }
        assert(!depth&&!t.failed);++files;
    }
    std::cout<<"files="<<files<<" tokens="<<tokens<<" empty_quotes="<<empty<<" events="<<events<<" actors="<<actors<<" channels="<<channels<<'\n';
    assert(files&&events);
}
