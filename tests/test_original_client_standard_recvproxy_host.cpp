#include "sin_compat/original_client_standard_recvproxy_host.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace recv=sin_compat::original_client_standard_recvproxy_host;

int main() {
    static_assert(recv::kOriginalStringProxyVA==0x24126CA0u);
    static_assert(recv::kOriginalDataTableProxyVA==0x24126CE0u);
    static_assert(recv::kOriginalStringProxyBytes==59u);
    static_assert(recv::kOriginalDataTableProxyBytes==11u);
    static_assert(recv::kOriginalRecvPropStringBufferOffset==12u);
    static_assert(recv::kOriginalReceivedValuePointerOffset==4u);
    static_assert(!recv::CanPublishRetailProxyPointers());

    // Original x86 writes the received chars including the first NUL,
    // then forces final source-size-1 NUL, preserving intervening bytes.
    {
        std::vector<std::uint8_t> dst(8u,0x7bu);
        const std::string source("ABC\0",4u);
        assert(recv::ApplyString(std::string_view(source.data(),source.size()),
                                 dst,8)==recv::Status::Applied);
        assert(dst[0]=='A' && dst[1]=='B' && dst[2]=='C');
        assert(dst[3]==0u);
        assert(dst[4]==0x7bu && dst[5]==0x7bu && dst[6]==0x7bu);
        assert(dst[7]==0u);
    }
    // Source string has no NUL within the original destination capacity:
    // original copies capacity bytes then forcibly truncates the last one.
    {
        std::vector<std::uint8_t> dst(6u,0x99u);
        assert(recv::ApplyString("LONGER",dst,4)==recv::Status::Applied);
        assert(dst[0]=='L' && dst[1]=='O' && dst[2]=='N' && dst[3]==0u);
        assert(dst[4]==0x99u && dst[5]==0x99u);
    }
    // Capacity 1: the original string proxy always ends with a NUL.
    {
        std::vector<std::uint8_t> dst(1u,0x7eu);
        assert(recv::ApplyString("X",dst,1)==recv::Status::Applied);
        assert(dst[0]==0u);
    }
    // Native signed <=0 RecvProp buffer size causes a no-op, not an
    // integer underflow or an invented arbitrary default buffer size.
    {
        std::vector<std::uint8_t> dst{1u,2u,3u};
        const auto saved=dst;
        assert(recv::ApplyString({},dst,0)==recv::Status::SourceNoOp);
        assert(dst==saved);
        assert(recv::ApplyString({},dst,-1)==recv::Status::SourceNoOp);
        assert(dst==saved);
    }
    // Invalid source lengths and undersized destination buffers refuse
    // TRANSACTIONALLY; no source access beyond known owned bytes.
    {
        std::vector<std::uint8_t> dst(5u,0x36u);
        const auto saved=dst;
        assert(recv::ApplyString("ABC",dst,4)==recv::Status::Refused);
        assert(dst==saved);
        assert(recv::ApplyString("ABCDEF",dst,6)==recv::Status::Refused);
        assert(dst==saved);
        assert(recv::ApplyString({},dst,1)==recv::Status::Refused);
        assert(dst==saved);
        assert(recv::ApplyString("A",dst,2147483647)==recv::Status::Refused);
        assert(dst==saved);
    }
    {
        std::vector<std::uint8_t> dst(6u,0x44u);
        const std::string withNul("Q\0stale",7u);
        assert(recv::ApplyString(
            std::string_view(withNul.data(),withNul.size()),dst,6)
            ==recv::Status::Applied);
        assert(dst[0]=='Q' && dst[1]==0u);
        assert(dst[2]==0x44u && dst[3]==0x44u && dst[4]==0x44u);
        assert(dst[5]==0u);
    }

    // Original datatable proxy writes the third stack argument DWORD
    // through the second argument pointer, without consulting first.
    // These are host TOKENS only, not original 2006 PE32 pointers.
    {
        std::uint32_t token=0xCAFE1297u;
        recv::ApplyDataTableToken(0x00112233u,token);
        assert(token==0x00112233u);
        recv::ApplyDataTableToken(0u,token);
        assert(token==0u);
        recv::ApplyDataTableToken(0xFEEDBABEu,token);
        assert(token==0xFEEDBABEu);
    }
    assert(!recv::CanPublishRetailProxyPointers());
    std::puts("PASS Phase1707 original 2006 bounded string and datatable proxy "
              "host behavior; no retail native VClient011.slot5 callbacks");
    return 0;
}
