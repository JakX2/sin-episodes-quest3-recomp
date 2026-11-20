#pragma once
// Phase 482: independent bounded KeyValues-like soundscape manifest loader.
// Original SHA-authenticated C_SoundscapeSystem::Init VA 0x240F4160
// loads "scripts/soundscapes_manifest.txt" with path id "GAME",
// iterates top-level "file" entries and ignores/logs unknown names.
// The actual retail manifest root is misspelled "soundscaples_manifest".
// Preserve the supplied root verbatim; do not require a corrected name.
//
// This is NOT proprietary KeyValues ABI, IFileSystem or sound playback.
// Invalid paths and incomplete snapshots are refused conservatively,
// rather than inheriting the original game's permissive failure flow.
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sin_independent::soundscape_manifest {
inline constexpr std::string_view kManifestPath="scripts/soundscapes_manifest.txt";
inline constexpr std::string_view kOriginalPathId="GAME";
inline constexpr std::string_view kObservedRetailRoot="soundscaples_manifest";
inline constexpr std::size_t kMaxManifestBytes=256u*1024u;
inline constexpr std::size_t kMaxTokenBytes=4096u;
inline constexpr std::size_t kMaxFileEntries=64u;
inline constexpr std::size_t kMaxSingleSoundscapeBytes=512u*1024u;
inline constexpr std::size_t kMaxTotalSoundscapeBytes=4u*1024u*1024u;

enum class Status {
    Ready, Malformed, ManifestTooLarge, UnsafeFilePath,
    TooManyEntries, NoFileEntries, MissingFile,
    FileTooLarge, TotalSizeExceeded, AllocationRefused
};
inline const char* StatusName(Status status) noexcept {
    switch(status){
        case Status::Ready:return "owned-soundscape-manifest-ready";
        case Status::Malformed:return "invalid-soundscape-manifest";
        case Status::ManifestTooLarge:return "soundscape-manifest-too-large";
        case Status::UnsafeFilePath:return "unsafe-soundscape-file-name";
        case Status::TooManyEntries:return "soundscape-manifest-entry-cap";
        case Status::NoFileEntries:return "empty-soundscape-manifest";
        case Status::MissingFile:return "soundscape-file-unavailable";
        case Status::FileTooLarge:return "soundscape-file-too-large";
        case Status::TotalSizeExceeded:return "soundscape-total-size-limit";
        case Status::AllocationRefused:return "soundscape-allocation-refused";
    }
    return "unknown";
}
struct Manifest final {
    std::string root;
    std::vector<std::string> file_paths; // retain order and duplicates
    std::size_t unknown_keys=0;
};
struct File final {
    std::string path;
    std::string text;
};
struct Snapshot final {
    Manifest manifest{};
    std::vector<File> files{};
    std::size_t total_bytes=0;
};

namespace detail {
enum class Kind { Word, Open, Close, End, Error };
struct Token final {Kind kind=Kind::End;std::string text{};};
class Lexer final {
public:
    explicit Lexer(std::string_view text) noexcept:src_(text){
        if(src_.size()>=3 &&
           static_cast<unsigned char>(src_[0])==0xef &&
           static_cast<unsigned char>(src_[1])==0xbb &&
           static_cast<unsigned char>(src_[2])==0xbf)at_=3;
    }
    Token Next() {
        while(at_<src_.size()){
            char c=src_[at_];
            if(c==' '||c=='\r'||c=='\n'||c=='\t'||c=='\v'||c=='\f'){
                ++at_;continue;
            }
            if(c=='/'&&at_+1<src_.size()&&src_[at_+1]=='/'){
                at_+=2;
                while(at_<src_.size()&&src_[at_]!='\n')++at_;
                continue;
            }
            break;
        }
        if(at_==src_.size())return {Kind::End,{}};
        char c=src_[at_++];
        if(c=='{')return {Kind::Open,{}};
        if(c=='}')return {Kind::Close,{}};
        if(c=='\0'||c==';')return {Kind::Error,{}};
        std::string value;
        if(c=='"'){
            while(at_<src_.size()){
                c=src_[at_++];
                if(c=='"')return {Kind::Word,std::move(value)};
                if(c=='\n'||c=='\r'||c=='\0')
                    return {Kind::Error,{}};
                if(c=='\\'&&at_<src_.size()&&
                   (src_[at_]=='"'||src_[at_]=='\\'))
                    c=src_[at_++];
                value+=c;
                if(value.size()>kMaxTokenBytes)return {Kind::Error,{}};
            }
            return {Kind::Error,{}};
        }
        value+=c;
        while(at_<src_.size()){
            c=src_[at_];
            if(c==' '||c=='\n'||c=='\r'||c=='\t'||
               c=='{'||c=='}'||c=='\v'||c=='\f')break;
            if(c=='/'&&at_+1<src_.size()&&src_[at_+1]=='/')break;
            if(c=='"'||c=='\0'||c==';')return {Kind::Error,{}};
            value+=c;
            ++at_;
            if(value.size()>kMaxTokenBytes)return {Kind::Error,{}};
        }
        return {Kind::Word,std::move(value)};
    }
private:
    std::string_view src_;
    std::size_t at_=0;
};
inline bool EqualsAsciiIgnoreCase(std::string_view left,
                                  std::string_view right) noexcept {
    if(left.size()!=right.size())return false;
    for(std::size_t i=0;i<left.size();++i){
        const unsigned char a=static_cast<unsigned char>(left[i]);
        const unsigned char b=static_cast<unsigned char>(right[i]);
        const unsigned char x=a>='A'&&a<='Z'?a+('a'-'A'):a;
        const unsigned char y=b>='A'&&b<='Z'?b+('a'-'A'):b;
        if(x!=y)return false;
    }
    return true;
}
inline bool SafePath(std::string_view path) noexcept {
    if(path.size()<12u||path.size()>240u||
       path.substr(0,8)!="scripts/" ||
       path.substr(path.size()-4)!=".txt")return false;
    std::size_t from=0;
    while(from<path.size()){
        const std::size_t slash=path.find('/',from);
        const std::size_t end=slash==std::string_view::npos?path.size():slash;
        const auto segment=path.substr(from,end-from);
        if(segment.empty()||segment=="."||segment=="..")return false;
        for(char c:segment){
            if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
                 (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))
                return false;
        }
        if(slash==std::string_view::npos)break;
        from=slash+1;
    }
    return true;
}
inline bool SkipObject(Lexer& lexer,unsigned depth) {
    if(depth>12u)return false;
    while(true){
        const Token key=lexer.Next();
        if(key.kind==Kind::Close)return true;
        if(key.kind!=Kind::Word)return false;
        const Token value=lexer.Next();
        if(value.kind==Kind::Word)continue;
        if(value.kind==Kind::Open&&SkipObject(lexer,depth+1))continue;
        return false;
    }
}
} // namespace detail

inline Status Parse(std::string_view text,Manifest& destination) noexcept {
    if(text.size()>kMaxManifestBytes)return Status::ManifestTooLarge;
    try {
        detail::Lexer lexer(text);
        auto root=lexer.Next();
        if(root.kind!=detail::Kind::Word||root.text.empty())
            return Status::Malformed;
        if(lexer.Next().kind!=detail::Kind::Open)return Status::Malformed;
        Manifest pending{};
        pending.root=std::move(root.text);
        while(true){
            const auto key=lexer.Next();
            if(key.kind==detail::Kind::Close)break;
            if(key.kind!=detail::Kind::Word)return Status::Malformed;
            const auto value=lexer.Next();
            if(value.kind==detail::Kind::Open){
                if(detail::EqualsAsciiIgnoreCase(key.text,"file") ||
                   !detail::SkipObject(lexer,1))return Status::Malformed;
                ++pending.unknown_keys;
                continue;
            }
            if(value.kind!=detail::Kind::Word)return Status::Malformed;
            if(!detail::EqualsAsciiIgnoreCase(key.text,"file")){
                ++pending.unknown_keys;
                continue;
            }
            if(!detail::SafePath(value.text))return Status::UnsafeFilePath;
            if(pending.file_paths.size()>=kMaxFileEntries)
                return Status::TooManyEntries;
            pending.file_paths.push_back(value.text);
        }
        if(lexer.Next().kind!=detail::Kind::End)return Status::Malformed;
        if(pending.file_paths.empty())return Status::NoFileEntries;
        destination=std::move(pending);
        return Status::Ready;
    }catch(...){return Status::AllocationRefused;}
}

// Caller-injected independently owned resource provider contract:
// bool read(string_view canonical_game_relative_path,string& text).
// Publish no partial state if ANY dependency is absent/oversized.
class OwnedSnapshot final {
public:
    const Snapshot& value() const noexcept{return value_;}
    void Clear() noexcept{value_={};}
    const std::string* Find(std::string_view path) const noexcept {
        for(const auto& file:value_.files)
            if(file.path==path)return &file.text;
        return nullptr;
    }
    template<class Read>
    Status Load(std::string_view manifestText,Read&& reader) noexcept {
        Manifest parsed{};
        const Status parsedStatus=Parse(manifestText,parsed);
        if(parsedStatus!=Status::Ready)return parsedStatus;
        try {
            Snapshot pending{};
            pending.manifest=std::move(parsed);
            for(const auto& path:pending.manifest.file_paths){
                std::string body;
                if(!reader(std::string_view(path),body))
                    return Status::MissingFile;
                if(body.size()>kMaxSingleSoundscapeBytes)
                    return Status::FileTooLarge;
                if(body.size()>kMaxTotalSoundscapeBytes-pending.total_bytes)
                    return Status::TotalSizeExceeded;
                pending.total_bytes+=body.size();
                pending.files.push_back({path,std::move(body)});
            }
            value_=std::move(pending);
            return Status::Ready;
        }catch(...){return Status::AllocationRefused;}
    }
private:
    Snapshot value_{};
};
} // namespace sin_independent::soundscape_manifest
