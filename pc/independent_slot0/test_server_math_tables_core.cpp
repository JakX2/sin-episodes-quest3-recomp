#include "server_math_tables_core.h"
#include <cstdio>
#include <limits>
using namespace sin_independent;
static bool Near(double a,double b,double tolerance=2e-6) {
    return std::abs(a-b)<=tolerance;
}
static int fail(int code,const char* why) {
    std::fprintf(stderr,"FAIL server math %d: %s\n",code,why);
    return code;
}
int main() {
    ServerMathTables t;
    if(t.initialized || t.Initialize(0,2.2f,0,2) ||
       t.Initialize(2.2f,std::numeric_limits<float>::quiet_NaN(),0,2) ||
       t.initialized)return fail(1,"invalid parameters");
    if(!t.InitializeRetailDefaults() || !t.initialized)return fail(2,"retail defaults");
    for(unsigned i=0;i<256;++i) {
        if(!Near(t.sine[i],std::sin(i*6.2831853071795864769/256.0)) ||
           !Near(t.textureToLinear[i],std::pow(i/255.0,2.2)) ||
           !Near(t.gammaToLinear[i],std::pow(i/255.0,2.2)) ||
           !Near(t.linearToGamma[i],std::pow(i/255.0,1.0/2.2)) ||
           t.exponent[i]!=static_cast<float>(std::ldexp(1.0,static_cast<int>(i)-128)/255.0))
            return fail(3,"sine, color and exponent tables");
        if(std::abs(static_cast<int>(t.textureGamma[i])-static_cast<int>(i))>1)
            return fail(4,"identity texture gamma within one quantization step");
    }
    for(unsigned i=1;i<1024;++i) {
        if(t.linearToTexture[i]<t.linearToTexture[i-1] ||
           t.linearToScreen[i]<t.linearToScreen[i-1])
            return fail(5,"monotonic 1024-entry maps");
    }
    if(t.linearToTexture[0] || t.linearToScreen[0] ||
       t.linearToTexture[1023]!=255 || t.linearToScreen[1023]!=255)
        return fail(6,"byte map endpoints");
    for(unsigned i=0;i<4096;++i) {
        const double value=std::pow(i/1024.0,1.0/2.2)*0.5;
        if(!Near(t.linearToVertex[i],std::min(value,1.0)) ||
           std::abs(static_cast<double>(t.linearToLightmap[i])-
                    std::min(value*255,255.0))>0.501)
            return fail(7,"lighting maps");
    }
    if(!Near(t.linearToVertex[1024],0.5) ||
       t.linearToLightmap[1024]!=128 ||
       !Near(t.sine[64],1) || !Near(t.sine[192],-1))
        return fail(8,"known quadrants and lightmap tie");
    const auto before=t.linearToVertex;
    if(t.Initialize(2.2f,2.2f,-1,2) || t.linearToVertex!=before || !t.initialized)
        return fail(9,"invalid reinit must preserve table");
    if(!t.Initialize(2.2f,2.2f,2,4) || !Near(t.linearToVertex[1024],0.25) ||
       t.linearToLightmap[1024]!=64 || t.linearToScreen[512]!=255)
        return fail(10,"overbright four and brightness scale");
    if(!t.Initialize(4,2.2f,0,1) ||
       !Near(t.linearToVertex[512],std::pow(0.5,0.25)) ||
       std::abs(t.linearToScreen[512]-static_cast<int>(255*std::pow(512/1023.0,1.0/3)))>1)
        return fail(11,"screen gamma cap distinct from vertex gamma");
    std::puts("PASS independent server scalar math tables: all indices, endpoints, gamma cap, overbright, finite input");
    return 0;
}
