#pragma once
#include <cstdint>
#include <cstring>
namespace sin_port::native_weapon_tables {
inline constexpr char client_flex_rule_export[]="SiN_GetClientFlexRuleApi";
enum class ClientFlexOpcode : std::uint32_t {
    constant=1, controller=2, descriptor=3, add=4, subtract=5, multiply=6, divide=7
};
// Retail op record: opcode +0, raw float bits or signed index +4.
struct ClientFlexOp {std::uint32_t opcode,arg;};
struct ClientFlexRule {unsigned target,count;const ClientFlexOp* ops;};
struct ClientFlexRuleApi {
    std::uint32_t version,size;
    int (__cdecl *evaluate)(bool,const ClientFlexRule*,unsigned,const int*,unsigned,
        const float*,float*,unsigned) noexcept;
};
using GetClientFlexRuleApi=const ClientFlexRuleApi* (__cdecl *)(std::uint32_t,std::uint32_t) noexcept;
namespace flex_rule_detail {
inline int evaluate(const ClientFlexRule* rules,unsigned rule_count,const int* map,unsigned map_count,
    const float* global,float* output,unsigned output_count) noexcept {
    float result[128]{};
    for(unsigned r=0;r<rule_count;++r){
        const auto& rule=rules[r];
        if(rule.target>=output_count||!rule.count||!rule.ops)return 0;
        float stack[32];unsigned depth=0;
        for(unsigned i=0;i<rule.count;++i){
            const auto& op=rule.ops[i];
            if(op.opcode>=1&&op.opcode<=3){
                if(depth==32)return 0;
                float value=0;
                if(op.opcode==1)std::memcpy(&value,&op.arg,4);
                else if(op.opcode==2){
                    if(op.arg>=map_count||map[op.arg]<0||map[op.arg]>=128)return 0;
                    value=global[map[op.arg]];
                }else{
                    if(op.arg>=output_count)return 0;
                    value=result[op.arg];
                }
                stack[depth++]=value;
            }else if(op.opcode>=4&&op.opcode<=7){
                if(depth<2)return 0;
                const float b=stack[--depth],a=stack[depth-1];
                switch(op.opcode){
                case 4:stack[depth-1]=static_cast<float>(static_cast<double>(a)+b);break;
                case 5:stack[depth-1]=static_cast<float>(static_cast<double>(a)-b);break;
                case 6:stack[depth-1]=static_cast<float>(static_cast<double>(a)*b);break;
                case 7:stack[depth-1]=b>0.0001?static_cast<float>(static_cast<double>(a)/b):0.f;break;
                }
            } // original switch skips unsupported opcodes
        }
        if(!depth)return 0;
        result[rule.target]=stack[0]; // original reads stack base, not stack top
    }
    if(output_count)std::memcpy(output,result,output_count*sizeof(float));
    return 1;
}
}
}
