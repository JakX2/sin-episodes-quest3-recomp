#include "../pc/independent_slot0/scene_cache_codec.h"
#include <cassert>
#include <iostream>
using namespace sin_port::scene_cache;
int main(){
    // Independent byte fixture from original field order; not generated
    // by encode, and not copied from a proprietary scene.cache file.
    const std::vector<unsigned char> fixture={
        2,0,0,0,3,0,0,0,0x78,0x56,0x34,0x12,2,0,0,0,
        17,0,0,0,'a',0,9,0,0,0,0,0,0x20,0x40,2,0,'x',0,'y','z',0,
        12,0,0,0,'b',0,0,0,0,0,0,0,0,0,0,0};
    Repository x;
    assert(decode(fixture.data(),fixture.size(),0x12345678,x)==Result::Ok);
    assert(x.records.size()==2&&x.records[0].path=="a"&&x.records[0].saved_stamp==9&&x.records[0].duration()==2.5f);
    assert((x.records[0].sounds==std::vector<std::string>{"x","yz"}));
    assert(x.records[1].path=="b"&&x.records[1].sounds.empty());
    std::vector<unsigned char> bytes;
    assert(encode(x,bytes)==Result::Ok&&bytes==fixture);
    assert(decode(bytes.data(),bytes.size(),0,x)==Result::Ok);
    assert(decode(bytes.data(),bytes.size(),1,x)==Result::ChecksumMismatch);
    for(std::size_t n=0;n<fixture.size();++n)assert(decode(fixture.data(),n,0,x)!=Result::Ok);
    assert(x.records.size()==2&&x.records[0].duration()==2.5f);
    auto bad=fixture;bad[4]=2;assert(decode(bad.data(),bad.size(),0,x)==Result::UnsupportedVersion);
    bad=fixture;bad[12]=0xff;bad[13]=0xff;bad[14]=0xff;bad[15]=0xff;
    assert(decode(bad.data(),bad.size(),0,x)==Result::Limit);
    bad=fixture;bad[16]=16;assert(decode(bad.data(),bad.size(),0,x)==Result::Malformed);
    bad=fixture;bad[30]=255;assert(decode(bad.data(),bad.size(),0,x)==Result::Malformed);
    bad=fixture;bad.push_back(0);assert(decode(bad.data(),bad.size(),0,x)==Result::Malformed);
    x.records[0].sounds={"same","same",""};x.records[0].duration_bits=0x7fc12345;
    assert(encode(x,bytes)==Result::Ok&&decode(bytes.data(),bytes.size(),0,x)==Result::Ok);
    assert(x.records[0].duration_bits==0x7fc12345&&x.records[0].sounds.size()==3&&x.records[0].sounds[0]==x.records[0].sounds[1]);
    auto previous=bytes;x.records[0].path=std::string(512,'a');
    assert(encode(x,bytes)==Result::Limit&&bytes==previous);
    x.records[0].path=std::string("a\0b",3);assert(encode(x,bytes)==Result::Limit&&bytes==previous);
    Repository empty;assert(encode(empty,bytes)==Result::Ok&&bytes.size()==16&&decode(bytes.data(),bytes.size(),0,x)==Result::Ok&&x.records.empty());
    std::cout<<"PASS scene.cache binary v2/3 framing, duration, sound names, truncation and bounds\n";
}
