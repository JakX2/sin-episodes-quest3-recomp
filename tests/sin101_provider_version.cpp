#include "sin_compat/source_provider_c_api.h"
#include <cstdio>
// Do not rely on assert() in Release CI: NDEBUG may disable it.
int main() {
    sin101_context* ctx=nullptr;
    if(sin101_abi_version()!=SIN101_ABI_VERSION) {
        std::fputs("Phase101 version mismatch\n",stderr);return 1;
    }
    const auto status=sin101_create(0,nullptr,&ctx);
    if(status!=SIN101_BAD_VERSION || ctx!=nullptr) {
        if(ctx)sin101_destroy(ctx);
        std::fputs("Phase101 invalid ABI was accepted\n",stderr);return 1;
    }
    return 0;
}
