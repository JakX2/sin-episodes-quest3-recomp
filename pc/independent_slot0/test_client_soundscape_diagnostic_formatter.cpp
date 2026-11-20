// Phase 493: emitted owner-retail soundscape diagnostics must be bounded
// and contain no private resource paths, original script text or pointers.
// This exercises the EXACT formatter compiled into independent client.dll.
#include "client_soundscape_diagnostic_formatter.h"
#include <cstdio>
#include <cstring>

namespace d=sin_independent::soundscape_diagnostic;
namespace r=sin_independent::soundscape_retail_gate;
namespace fs=sin_independent::soundscape_engine_fs;
namespace m=sin_independent::soundscape_manifest;
namespace c=sin_independent::soundscape_catalog;
static int Fail(int n) {
    std::fprintf(stderr,"owned soundscape telemetry fixture failed %d\n",n);
    return n;
}
static bool Safe(const char* value) {
    if(!value||std::strlen(value)>180)return false;
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);
        *p;++p) {
        const unsigned char ch=*p;
        if(!((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||
             ch==':'||ch=='-'))
            return false;
    }
    return true;
}
int main() {
    char output[256]={};
    r::Report report{};
    report.status=r::Status::CatalogVerified;
    report.bridge_result=fs::Status::OwnedCatalogReady;
    report.diagnostic.stage=fs::Stage::Complete;
    report.diagnostic.script_reads_attempted=7;
    report.diagnostic.script_reads_succeeded=7;
    if(!d::Format(report,output,sizeof(output)) ||
       std::strcmp(output,
           "complete:owned-engine-soundscape-catalog-ready-no-game-init:"
           "owned-soundscape-manifest-ready:"
           "independent-soundscape-catalog-ready:7:7")!=0 ||
       !Safe(output))return Fail(1);

    report.status=r::Status::FilesystemOrParseRefused;
    report.bridge_result=fs::Status::InvalidEngineAbi;
    report.diagnostic={};
    if(!d::Format(report,output,sizeof(output)) ||
       std::strcmp(output,
           "resolve-abi:engine-filesystem-abi-refused:"
           "owned-soundscape-manifest-ready:"
           "independent-soundscape-catalog-ready:0:0")!=0 ||
       !Safe(output))return Fail(2);

    report.bridge_result=fs::Status::ManifestOrDependencyRefused;
    report.diagnostic.stage=fs::Stage::ReadScript;
    report.diagnostic.manifest_status=m::Status::MissingFile;
    report.diagnostic.script_reads_attempted=7;
    report.diagnostic.script_reads_succeeded=6;
    if(!d::Format(report,output,sizeof(output)) ||
       std::strcmp(output,
           "read-script:soundscape-manifest-or-file-refused:"
           "soundscape-file-unavailable:"
           "independent-soundscape-catalog-ready:7:6")!=0 ||
       !Safe(output))return Fail(3);

    report.bridge_result=fs::Status::CatalogParseRefused;
    report.diagnostic.stage=fs::Stage::BuildCatalog;
    report.diagnostic.manifest_status=m::Status::Ready;
    report.diagnostic.catalog_status=c::Status::MalformedDefinition;
    report.diagnostic.script_reads_succeeded=7;
    if(!d::Format(report,output,sizeof(output)) ||
       std::strcmp(output,
           "build-catalog:soundscape-definition-refused:"
           "owned-soundscape-manifest-ready:"
           "malformed-soundscape-keyvalues:7:7")!=0 ||
       !Safe(output))return Fail(4);

    // Truncated formatting cannot masquerade as a complete valid
    // phase string, even when its input Report came from a good read.
    char tiny[5]={'x','x','x','x','\0'};
    if(d::Format(report,tiny,sizeof(tiny)) ||
       tiny[0]!='\0' ||
       d::Format(report,nullptr,100) ||
       d::Format(report,tiny,0))
        return Fail(5);

    report.diagnostic.script_reads_attempted=65;
    if(d::Format(report,output,sizeof(output)) ||
       output[0]!='\0')return Fail(6);
    report.diagnostic.script_reads_attempted=1;
    report.diagnostic.script_reads_succeeded=2;
    if(d::Format(report,output,sizeof(output)) ||
       output[0]!='\0')return Fail(7);

    // The report has no user-supplied string fields; output cannot
    // include filesystem paths, raw 2006 KeyValues, or memory addresses.
    std::puts("PASS bounded owner soundscape formatter: success, ABI, missing seventh file, catalog parse, invalid counters and truncated-output refusal");
    return 0;
}
