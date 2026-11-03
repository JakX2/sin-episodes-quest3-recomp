#include "sin_compat/source_provider_c_api.h"
#include <cassert>
int main(){sin101_context* ptr=nullptr;assert(sin101_abi_version()==SIN101_ABI_VERSION);assert(sin101_create(0,nullptr,&ptr)==SIN101_BAD_VERSION);assert(ptr==nullptr);}
