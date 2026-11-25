#pragma once
#include "native_server_activity_dictionary.h"
#include "native_server_sequence_choice.h"
namespace sin_port::native_weapon_tables {
inline constexpr char server_activity_metadata_export[]="SiN_GetServerActivityMetadataApi";
struct ServerActivityDescriptor {
    void* storage;
    unsigned bytes,offset;
};
struct ServerActivityMetadataView {
    const ServerActivityDescriptor* descriptors;
    unsigned count;
    // One resolved model generation (+c4). Composite studio-header
    // traversal is deliberately outside this single-model API.
    std::uint32_t* generation;
};
struct ServerActivityMetadataResult {
    unsigned refreshed,visited,activity_writes,events_existing,events_created;
    int failed_index;
};
struct ServerActivityMetadataApi {
    std::uint32_t version,size;
    // 220eaf70: force activity-name registration even if already marked.
    int (__cdecl *register_activity)(ServerActivityDictionary*,const ServerActivityDescriptor*,ServerSequenceChoiceEntry*) noexcept;
    // 220eb170: lazily resolve only if descriptor+0d bit 0x10 clear.
    int (__cdecl *get)(ServerActivityDictionary*,const ServerActivityDescriptor*,ServerSequenceChoiceEntry*) noexcept;
    // 220eaff0 / 220eb040: on generation mismatch, activity then
    // named event registration per descriptor; publish generation last.
    int (__cdecl *refresh)(ServerActivityDictionary*,ServerEventDictionary*,const ServerActivityMetadataView*,ServerActivityMetadataResult*) noexcept;
};
using GetServerActivityMetadataApi=const ServerActivityMetadataApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
