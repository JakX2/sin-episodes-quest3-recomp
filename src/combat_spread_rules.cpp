#include "sin_compat/combat_spread_rules.hpp"
#include <cmath>
#include <locale>
#include <sstream>
#include <utility>
namespace sin_compat {
namespace {
class Lexer {
public:
    explicit Lexer(std::string_view raw) : raw_(raw) {}
    bool next(std::string& token, bool& is_brace, std::string& error) {
        token.clear(); is_brace=false;
        for (;;) {
            while (pos_<raw_.size() && (raw_[pos_]==' ' || raw_[pos_]=='\n' ||
                   raw_[pos_]=='\r' || raw_[pos_]=='\t')) ++pos_;
            if (pos_+1<raw_.size() && raw_[pos_]=='/' && raw_[pos_+1]=='/') {
                pos_+=2;
                while (pos_<raw_.size() && raw_[pos_]!='\n') ++pos_;
                continue;
            }
            break;
        }
        if (pos_==raw_.size()) return false;
        char c=raw_[pos_++];
        if (c=='{' || c=='}') {
            token.assign(1,c); is_brace=true; return true;
        }
        if (c=='"') {
            while (pos_<raw_.size()) {
                c=raw_[pos_++];
                if (c=='"') return true;
                if (c=='\\') {
                    if (pos_>=raw_.size()) {error="unterminated escape";return false;}
                    c=raw_[pos_++];
                    if (c!='"' && c!='\\') {error="invalid escape sequence";return false;}
                }
                token.push_back(c);
                if (token.size()>16384) {error="token length limit";return false;}
            }
            error="unterminated quoted token"; return false;
        }
        if (c=='/') {error="unexpected slash";return false;}
        token.push_back(c);
        while (pos_<raw_.size()) {
            c=raw_[pos_];
            if (c==' ' || c=='\n' || c=='\r' || c=='\t' || c=='{' || c=='}') break;
            if (c=='/' && pos_+1<raw_.size() && raw_[pos_+1]=='/') break;
            token.push_back(c); ++pos_;
            if (token.size()>16384) {error="token length limit";return false;}
        }
        return true;
    }
private:
    std::string_view raw_;
    std::size_t pos_=0;
};
struct ParsedNode {
    bool scalar=false;
    std::string value;
    std::map<std::string, ParsedNode> members;
};
class Parser {
public:
    explicit Parser(std::string_view text):lex_(text){}
    bool parse(ParsedNode& root,std::string& error) {
        if (!fetch(error)) return fail(error,"empty rules document");
        if (brace_ || token_!="AttackSpreadRules") return fail(error,"expected AttackSpreadRules root");
        if (!fetch(error)) return fail(error,"missing root block");
        if (!brace_ || token_!="{") return fail(error,"expected root brace");
        if (!parse_object(root,0,error)) return false;
        if (fetch(error)) return fail(error,"trailing tokens after root");
        return error.empty();
    }
    static bool fail(std::string& error,const char* message) {error=message;return false;}
private:
    bool fetch(std::string& error) {return lex_.next(token_,brace_,error);}
    bool parse_object(ParsedNode& obj,unsigned depth,std::string& error) {
        if (depth>16) return fail(error,"nesting depth exceeded");
        for (;;) {
            if (!fetch(error)) return fail(error,"unclosed object");
            if (brace_ && token_=="}") return true;
            if (brace_ || token_.empty()) return fail(error,"expected nonempty key");
            const auto key=token_;
            if (obj.members.find(key)!=obj.members.end()) return fail(error,"duplicate key");
            if (!fetch(error)) return fail(error,"value missing");
            ParsedNode child;
            if (brace_) {
                if (token_!="{") return fail(error,"unexpected closing brace");
                if (!parse_object(child,depth+1,error)) return false;
            } else {
                child.scalar=true;
                child.value=token_;
            }
            obj.members.emplace(key,std::move(child));
        }
    }
    Lexer lex_;
    std::string token_;
    bool brace_=false;
};
} // namespace

const CombatSpreadRules::Node* CombatSpreadRules::child(const Node& node,
                                                         const std::string& name) {
    auto it=node.members.find(name);
    return it==node.members.end()?nullptr:&it->second;
}
const CombatSpreadRules::Node* CombatSpreadRules::named_or_wildcard(
    const Node& node,const std::string& name) {
    const Node* exact=child(node,name);
    return exact ? exact : child(node,"*");
}
bool CombatSpreadRules::load(std::string_view text,std::string* error) {
    loaded_=false;
    root_=Node{};
    std::string failure;
    if (text.size()>262144) failure="rules file size limit";
    ParsedNode parsed;
    if (failure.empty()) Parser(text).parse(parsed,failure);
    if (!failure.empty()) {
        if (error) *error=failure;
        return false;
    }
    auto copy=[&](auto&& self,const ParsedNode& from,Node& to)->void {
        to.scalar=from.scalar;
        to.value=from.value;
        for (const auto& pair:from.members)
            self(self,pair.second,to.members[pair.first]);
    };
    copy(copy,parsed,root_);
    const Node* classes=child(root_,"class_names");
    if (!classes || classes->scalar) {
        root_=Node{};
        if (error) *error="missing class_names map";
        return false;
    }
    loaded_=true;
    if (error) error->clear();
    return true;
}
SpreadLookup CombatSpreadRules::lookup(const std::string& attacker,
                                      const std::string& weapon,
                                      const std::string& target) const {
    if (!loaded_) return {};
    const Node* classes=child(root_,"class_names");
    if (!classes) return {};
    const Node* actor=named_or_wildcard(*classes,attacker);
    if (!actor || !actor->scalar) return {SpreadLookup::Status::NoActorRule,0};
    const Node* arule=child(root_,actor->value);
    if (!arule || arule->scalar) return {};
    const Node* weapons=child(*arule,"weapon_class_names");
    if (!weapons || weapons->scalar) return {};
    const Node* mapped_weapon=named_or_wildcard(*weapons,weapon);
    if (!mapped_weapon || !mapped_weapon->scalar) return {};
    const Node* wrule=child(*arule,mapped_weapon->value);
    if (!wrule || wrule->scalar) return {};
    const Node* targets=child(*wrule,"target_class_names");
    if (!targets || targets->scalar) return {};
    const Node* mapped_target=named_or_wildcard(*targets,target);
    if (!mapped_target || !mapped_target->scalar) return {};
    const Node* value=child(*wrule,mapped_target->value);
    if (!value || !value->scalar) return {};
    double spread=0.0;
    std::istringstream input(value->value);
    input.imbue(std::locale::classic());
    if (!(input>>spread)) return {SpreadLookup::Status::InvalidValue,0};
    input>>std::ws;
    if (!input.eof() || !std::isfinite(spread) || spread<0.0 || spread>1.0)
        return {SpreadLookup::Status::InvalidValue,0};
    return {SpreadLookup::Status::Found,spread};
}
} // namespace sin_compat
