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
inline constexpr char server_activity_choice_export[]="SiN_GetServerActivityChoiceApi";
struct ServerActivityGenerationNode {
    std::uint32_t* word;
    int parent; // root -1; otherwise earlier node index
};
struct ServerActivityCompositeView {
    // Already resolved global sequence order, including group sequences.
    const ServerActivityDescriptor* descriptors;
    unsigned count;
    const ServerActivityGenerationNode* groups;
    unsigned group_count;
};
struct ServerActivityChoiceResult {
    ServerActivityMetadataResult refresh;
    unsigned scanned,lazy_registrations;
    int selected,failed_index;
};
struct ServerActivityChoiceApi {
    std::uint32_t version,size;
    // Original 2211c110: signed min of root and immediate group words.
    int (__cdecl *generation)(const ServerActivityCompositeView*,std::uint32_t*) noexcept;
    // Original 220eb1c0/220eb2c0: refresh on mismatch, then lazy scan.
    // Publish to every resolved descendant only after successful refresh.
    int (__cdecl *choose)(ServerActivityDictionary*,ServerEventDictionary*,const ServerActivityCompositeView*,
        int activity,int current,bool looping,const ServerSequenceChoiceBindings*,ServerActivityChoiceResult*) noexcept;
};
using GetServerActivityChoiceApi=const ServerActivityChoiceApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
}
