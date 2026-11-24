#pragma once
#include "native_radio_message.h"
namespace sin_port::native_weapon_tables {
inline constexpr char radio_entity_export[]="SiN_GetRadioEntityApi";
inline constexpr unsigned radio_entity_bytes=0x414;
enum class RadioFieldKind:unsigned {Key,Input,Output};
struct RadioField {
    const char* name;
    RadioFieldKind kind;
    unsigned field_type,offset;
};
struct RadioEntitySchema {
    const char* classname;
    const char* cpp_class;
    unsigned object_bytes,field_count;
    const RadioField* fields;
};
struct RadioEntityBindings {
    RadioBindings message;
    // These are existing engine-owned base class operations, not
    // original-address calls. The host must construct a real compatible
    // entity and register its datamap/virtual functions before exposure.
    int (__cdecl *base_construct)(void*,void* entity,unsigned bytes) noexcept;
    int (__cdecl *class_init)(void*,void* entity,const char* classname) noexcept;
    // Spawn calls virtual +68 (precache); the adapter performs this
    // leaf itself, followed by these two existing base helpers.
    int (__cdecl *spawn_helper)(void*,void* entity,unsigned original_leaf,unsigned a,unsigned b) noexcept;
    int (__cdecl *base_activate)(void*,void* entity) noexcept;
    int (__cdecl *destroy_outputs)(void*,void* entity,unsigned offset) noexcept;
    int (__cdecl *base_destroy)(void*,void* entity) noexcept;
};
struct RadioEntityApi {
    std::uint32_t version,size;
    const RadioEntitySchema* (__cdecl *schema)() noexcept;
    int (__cdecl *construct)(HologramControl*,const RadioEntityBindings*) noexcept;
    // Typed datamap assignments only. Existing Source key parser and
    // output parser remain host-owned. Priority stores original raw int.
    int (__cdecl *key)(HologramControl*,const char* name,const RadioValue*) noexcept;
    int (__cdecl *input)(HologramControl*,RadioDispatcher*,const RadioEntityBindings*,const char* name,const RadioValue*) noexcept;
    int (__cdecl *precache)(HologramControl*,const RadioEntityBindings*) noexcept;
    int (__cdecl *spawn)(HologramControl*,const RadioEntityBindings*) noexcept;
    int (__cdecl *activate)(HologramControl*,const RadioEntityBindings*) noexcept;
    // Release original output containers and base object, not storage.
    // Host invalidates the serial handle and releases allocation.
    int (__cdecl *destroy)(HologramControl*,const RadioEntityBindings*) noexcept;
};
using GetRadioEntityApi=const RadioEntityApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
