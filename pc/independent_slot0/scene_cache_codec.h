#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
#include <utility>

namespace sin_port::scene_cache {
// Original binary scene.cache: repository system 2, scene schema 3.
// Names, not runtime sound-emitter IDs, are serialized.
inline constexpr std::uint32_t system_version=2,scene_version=3;
inline constexpr std::size_t max_file_bytes=64u*1024u*1024u,max_name=511,max_records=65535;
struct Record {
    std::string path;
    std::uint32_t saved_stamp=0,duration_bits=0;
    std::vector<std::string> sounds;
    float duration() const noexcept {
        static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE float required");
        float f;std::memcpy(&f,&duration_bits,4);return f;
    }
};
struct Repository {std::uint32_t checksum=0;std::vector<Record> records;};
enum class Result {Ok,Malformed,UnsupportedVersion,ChecksumMismatch,Limit,AllocationFailure};
namespace detail {
struct Reader {
    const unsigned char* p;std::size_t n,pos=0;
    bool u32(std::uint32_t& v) noexcept {
        if(n-pos<4)return false;
        v=std::uint32_t(p[pos])|(std::uint32_t(p[pos+1])<<8)|(std::uint32_t(p[pos+2])<<16)|(std::uint32_t(p[pos+3])<<24);pos+=4;return true;
    }
    bool u16(unsigned& v) noexcept {
        if(n-pos<2)return false;
        v=unsigned(p[pos])|(unsigned(p[pos+1])<<8);pos+=2;return true;
    }
    bool name(std::string& v) {
        const auto* end=static_cast<const unsigned char*>(std::memchr(p+pos,0,n-pos));
        if(!end||std::size_t(end-(p+pos))>max_name)return false;
        v.assign(reinterpret_cast<const char*>(p+pos),std::size_t(end-(p+pos)));pos=std::size_t(end-p)+1;return true;
    }
};
inline void u32(std::vector<unsigned char>& b,std::uint32_t x){for(unsigned i=0;i<4;++i)b.push_back(static_cast<unsigned char>(x>>(i*8)));}
inline void u16(std::vector<unsigned char>& b,unsigned x){b.push_back(static_cast<unsigned char>(x));b.push_back(static_cast<unsigned char>(x>>8));}
inline bool valid_name(const std::string& x){return x.size()<=max_name&&x.find('\0')==std::string::npos;}
inline void name(std::vector<unsigned char>& b,const std::string& x){b.insert(b.end(),x.begin(),x.end());b.push_back(0);}
}
// Host validation is intentionally stricter than the original CUtlBuffer
// reader: reject short/trailing data, overlong names and negative counts.
// The caller's output is not modified on failure.
inline Result decode(const void* bytes,std::size_t size,std::uint32_t expected_checksum,Repository& out) noexcept {
    if(!bytes||size<16)return Result::Malformed;
    if(size>max_file_bytes)return Result::Limit;
    try{
        detail::Reader r{static_cast<const unsigned char*>(bytes),size};
        std::uint32_t sys=0,ver=0,count=0;Repository tmp;
        if(!r.u32(sys)||!r.u32(ver)||!r.u32(tmp.checksum)||!r.u32(count))return Result::Malformed;
        if(sys!=system_version||ver!=scene_version)return Result::UnsupportedVersion;
        if(expected_checksum&&expected_checksum!=tmp.checksum)return Result::ChecksumMismatch;
        if(count>max_records)return Result::Limit;
        if(count>(size-16)/15)return Result::Malformed; // length + minimum payload
        tmp.records.reserve(count);
        for(std::uint32_t i=0;i<count;++i){
            std::uint32_t extent=0;
            if(!r.u32(extent)||extent<11||extent>r.n-r.pos)return Result::Malformed;
            detail::Reader record{r.p+r.pos,extent};r.pos+=extent;
            Record item;unsigned sounds=0;
            if(!record.name(item.path)||!record.u32(item.saved_stamp)||!record.u32(item.duration_bits)||!record.u16(sounds)||
               sounds>record.n-record.pos)return Result::Malformed;
            item.sounds.reserve(sounds);
            for(unsigned j=0;j<sounds;++j){std::string name;if(!record.name(name))return Result::Malformed;item.sounds.push_back(std::move(name));}
            if(record.pos!=record.n)return Result::Malformed;
            tmp.records.push_back(std::move(item));
        }
        if(r.pos!=r.n)return Result::Malformed;
        out=std::move(tmp);return Result::Ok;
    }catch(...){return Result::AllocationFailure;}
}
inline Result encode(const Repository& in,std::vector<unsigned char>& out) noexcept {
    if(in.records.size()>max_records)return Result::Limit;
    try{
        std::vector<unsigned char> b;
        detail::u32(b,system_version);detail::u32(b,scene_version);detail::u32(b,in.checksum);detail::u32(b,static_cast<std::uint32_t>(in.records.size()));
        for(const auto& item:in.records){
            if(!detail::valid_name(item.path)||item.sounds.size()>65535)return Result::Limit;
            std::size_t length=item.path.size()+11;
            for(const auto& x:item.sounds){if(!detail::valid_name(x))return Result::Limit;length+=x.size()+1;}
            if(length>max_file_bytes||b.size()>max_file_bytes-length||max_file_bytes-b.size()-length<4)return Result::Limit;
            detail::u32(b,static_cast<std::uint32_t>(length));detail::name(b,item.path);
            detail::u32(b,item.saved_stamp);detail::u32(b,item.duration_bits);detail::u16(b,static_cast<unsigned>(item.sounds.size()));
            for(const auto& x:item.sounds)detail::name(b,x);
        }
        out=std::move(b);return Result::Ok;
    }catch(...){return Result::AllocationFailure;}
}
}
