// Pure-original-free synthetic LSB bitstream fixtures for SiN 2006
// Stats user-message bit headers. Does NOT call real game bf_read or decode
// stat values. Builds with native MSVC Win32 and in CTest.
#include "client_stats_bitbuf_cursor.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace m=sin_independent::stats_messages;
namespace bb=sin_independent::stats_bitbuf;
using Status=m::HeaderStatus;

static int Fail(int code) {
    std::fprintf(stderr,"Stats bit-cursor native failure %d\n",code);
    return code;
}
template<std::size_t N>
static void WriteBits(std::array<std::uint8_t,N>& buffer,
                      std::size_t first,unsigned width,
                      std::uint32_t value) {
    for(unsigned i=0;i<width;++i){
        const auto bit=first+i;
        if((value>>i)&1u)
            buffer[bit>>3]|=static_cast<std::uint8_t>(1u << (bit&7u));
        else
            buffer[bit>>3]&=static_cast<std::uint8_t>(~(1u << (bit&7u)));
    }
}
int main(){
    m::OwnedRegistry registered;
    for(std::size_t i=0;i<3;++i)
        if(!registered.RegisterStatsStep(
            sin_independent::stats_system::kInitSteps[4+i]))return Fail(1);
    if(!registered.Complete())return Fail(2);

    // The recovered SHA-pinned original handler asks bf_read for a
    // 16-bit stat ID. That is NOT guaranteed to be byte-aligned within
    // a Source packet. Probe all seven possible nonzero bit offsets,
    // and another that crosses a 32-bit boundary.
    for(std::size_t offset=0;offset<9;++offset){
        std::array<std::uint8_t,8> buf{};
        WriteBits(buf,offset,16,54);
        WriteBits(buf,offset+16,7,0x65);
        bb::Cursor cursor(buf.data(),buf.size(),offset+23,offset);
        bb::ParsedHeader header{};
        if(bb::InspectBitHeader(registered,27,cursor,&header)!=
               Status::KnownHeaderOnly ||
           cursor.position()!=offset+16 || cursor.remaining()!=7 ||
           header.stat_id!=54 || header.message_id!=27 ||
           header.opaque_start_bit!=offset+16 ||
           header.opaque_bits!=7)return Fail(3);
        std::uint32_t tail=0;
        if(!cursor.ReadUBits(7,tail)||tail!=0x65 ||
           cursor.remaining()!=0)return Fail(4);
    }
    // Dynamic original index range occupies IDs 55..65.
    for(const unsigned id:{55u,60u,65u}){
        std::array<std::uint8_t,8> buf{};
        WriteBits(buf,3,16,id);
        bb::Cursor cursor(buf.data(),buf.size(),19,3);
        bb::ParsedHeader out{};
        if(bb::InspectBitHeader(registered,28,cursor,&out)!=
               Status::KnownHeaderOnly ||
           out.stat_id!=static_cast<int>(id) || out.opaque_bits!=0 ||
           cursor.position()!=19)return Fail(5);
    }
    // The previous byte-only inspector would misread this packet,
    // because the 16-bit ID starts at bit offset 5, not at byte zero.
    std::array<std::uint8_t,8> invalid{};
    for(unsigned index:{54u,55u,66u,65535u}){
        invalid.fill(0);
        WriteBits(invalid,5,16,index);
        bb::Cursor left(invalid.data(),invalid.size(),21,5);
        bb::Cursor right=left;
        const auto stat=bb::InspectBitHeader(registered,27,left);
        const auto dynamic=bb::InspectBitHeader(registered,28,right);
        if(index<55){
            if(stat!=Status::KnownHeaderOnly ||
               dynamic!=Status::DynamicIdRefused ||
               right.position()!=5)return Fail(6);
        }else if(index<66){
            if(stat!=Status::StaticIdRefused ||
               left.position()!=5 ||
               dynamic!=Status::KnownHeaderOnly)return Fail(7);
        }else{
            if(stat!=Status::StaticIdRefused ||
               dynamic!=Status::DynamicIdRefused ||
               left.position()!=5 || right.position()!=5)return Fail(8);
        }
    }
    // The original shared protocol declares a one-byte ResetHUD.
    // We consume exactly 8 bits even if the packet cursor is unaligned.
    std::array<std::uint8_t,8> reset{};
    WriteBits(reset,7,8,0xa5);
    bb::Cursor resetCursor(reset.data(),reset.size(),15,7);
    bb::ParsedHeader r{};
    if(bb::InspectBitHeader(registered,5,resetCursor,&r)!=
           Status::KnownHeaderOnly ||
       r.message_id!=5 || r.stat_id!=-1 || r.opaque_bits!=0 ||
       resetCursor.position()!=15)return Fail(9);
    bb::Cursor resetBad(reset.data(),reset.size(),16,7);
    if(bb::InspectBitHeader(registered,5,resetBad)!=
           Status::ResetLengthRefused || resetBad.position()!=7)
        return Fail(10);

    // Standalone Source bit reader: 32-bit fields are LSB-first and may
    // start on a non-dword boundary. The implementation never reads
    // beyond byteLength or valid bitLength.
    std::array<std::uint8_t,10> words{};
    WriteBits(words,27,32,0xDEADBEEFu);
    bb::Cursor raw(words.data(),words.size(),59,27);
    std::uint32_t value=0;
    if(!raw.ReadUBits(32,value) || value!=0xDEADBEEFu ||
       raw.position()!=59 || raw.remaining()!=0)return Fail(11);
    if(raw.ReadUBits(1,value) || !raw.overflow() ||
       raw.position()!=59 || value!=0)return Fail(12);

    // Missing 1 bit of a 16-bit ID refuses without mutating the
    // caller cursor, even though raw bf_read itself sets overflow.
    std::array<std::uint8_t,3> shortData{};
    WriteBits(shortData,2,15,54);
    bb::Cursor truncated(shortData.data(),shortData.size(),17,2);
    bb::ParsedHeader previous{};
    previous.stat_id=123;
    if(bb::InspectBitHeader(registered,27,truncated,&previous)!=
           Status::PayloadTruncated || truncated.position()!=2 ||
       truncated.overflow() || previous.stat_id!=-1)return Fail(13);

    // Input bounds: null, over-wide bit lengths, too-large buffers,
    // invalid initial offset and zero/oversize bit requests.
    bb::Cursor invalidCursor;
    if(invalidCursor.Reset(nullptr,1,8) ||
       invalidCursor.Reset(words.data(),1,9) ||
       invalidCursor.Reset(words.data(),1,8,9) ||
       invalidCursor.Reset(words.data(),bb::kMaxMessageBytes+1,8))
        return Fail(14);
    if(!invalidCursor.Reset(words.data(),1,8) ||
       invalidCursor.ReadUBits(0,value) ||
       invalidCursor.ReadUBits(33,value) ||
       invalidCursor.position()!=0 ||
       invalidCursor.overflow())return Fail(15);

    // A complete second dispatch listener sees the same initial cursor.
    // Each owned copy is independent, like original game's 24-byte
    // bf_read copy-per-callback dispatch, without sharing read offset.
    std::array<std::uint8_t,8> fanout{};
    WriteBits(fanout,1,16,55);
    WriteBits(fanout,17,6,0x3e);
    bb::Cursor owner(fanout.data(),fanout.size(),23,1);
    bb::Cursor a=owner,b=owner;
    bb::ParsedHeader first{},second{};
    if(bb::InspectBitHeader(registered,28,a,&first)!=
           Status::KnownHeaderOnly ||
       bb::InspectBitHeader(registered,28,b,&second)!=
           Status::KnownHeaderOnly ||
       owner.position()!=1 || a.position()!=17 || b.position()!=17 ||
       first.stat_id!=55 || second.stat_id!=55)return Fail(16);
    if(!a.ReadUBits(6,value) || value!=0x3e ||
       b.position()!=17 || owner.position()!=1)return Fail(17);

    registered.Clear();
    bb::Cursor unbound(fanout.data(),fanout.size(),23,1);
    if(bb::InspectBitHeader(registered,28,unbound)!=Status::MissingHook ||
       unbound.position()!=1)return Fail(18);
    if(bb::InspectBitHeader(registered,38,unbound)!=Status::InvalidMessage ||
       bb::InspectBitHeader(registered,-1,unbound)!=Status::InvalidMessage ||
       unbound.position()!=1)return Fail(19);
    std::puts("PASS LSB Source bf_read owned cursor: unaligned IDs, 32-bit crossing, bounds, rollback, isolated listeners");
    return 0;
}
