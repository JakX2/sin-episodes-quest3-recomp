#pragma once
// Phase 493: bounded, path-free owner soundscape diagnostics.
// The trace contains only closed, independently authored enum names
// and script read counters. No proprietary game bytes, pointers, paths,
// engine internals or user-machine absolute paths are ever serialized.
#include "client_soundscape_retail_gate.h"
#include <cstddef>
#include <cstdio>

namespace sin_independent::soundscape_diagnostic {
inline bool Format(const soundscape_retail_gate::Report& report,
                   char* output,std::size_t capacity) noexcept {
    if(!output||capacity==0)return false;
    output[0]='\0';
    const auto& details=report.diagnostic;
    if(details.script_reads_succeeded>details.script_reads_attempted ||
       details.script_reads_attempted>
           soundscape_manifest::kMaxFileEntries)
        return false;
    const int written=std::snprintf(
        output,capacity,"%s:%s:%s:%s:%u:%u",
        soundscape_engine_fs::StageName(details.stage),
        soundscape_engine_fs::StatusName(report.bridge_result),
        soundscape_manifest::StatusName(details.manifest_status),
        soundscape_catalog::StatusName(details.catalog_status),
        static_cast<unsigned>(details.script_reads_attempted),
        static_cast<unsigned>(details.script_reads_succeeded));
    if(written<0||static_cast<std::size_t>(written)>=capacity) {
        output[0]='\0';
        return false;
    }
    return true;
}
} // namespace sin_independent::soundscape_diagnostic
