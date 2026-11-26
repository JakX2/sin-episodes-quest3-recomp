#pragma once
#include <cstdint>
namespace sin_port::native_map {
inline constexpr char export_name[]="SiN_GetServerMapEntityApi";
inline constexpr unsigned token_capacity=2048;
enum class Kind:unsigned {End,Word,Malformed};
struct Token {Kind kind;unsigned begin,next;char text[token_capacity];};
struct EntitySlice {unsigned begin,body,end;}; // end exclusive, includes closing brace
struct Pair {char key[token_capacity],value[token_capacity];};
struct Result {unsigned completed,entities,pairs,filtered,created,missing_factory,missing_classname,clears;unsigned offset;};
struct Bindings {
    void* context;
    int (__cdecl *should_create)(void*,const char*,bool*) noexcept; // optional map filter
    int (__cdecl *create)(void*,const char*,void**) noexcept;
    int (__cdecl *clear_factory_global)(void*) noexcept; // unfiltered original path
    int (__cdecl *key_value)(void*,void*,const char*,const char*) noexcept;
    int (__cdecl *missing)(void*,const char*) noexcept;
    int (__cdecl *accept_entity)(void*,void*,const char*,unsigned,const EntitySlice*) noexcept;
};
struct Api {
    std::uint32_t version,size;
    int (__cdecl *token)(const char*,unsigned,unsigned,Token*) noexcept;
    int (__cdecl *slice)(const char*,unsigned,unsigned,EntitySlice*) noexcept;
    int (__cdecl *lookup)(const char*,unsigned,const EntitySlice*,const char*,char*,unsigned,bool*) noexcept;
    int (__cdecl *parse)(const char*,unsigned,const Bindings*,Result*) noexcept;
};
using GetApi=const Api* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
