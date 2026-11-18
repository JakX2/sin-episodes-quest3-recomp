#pragma once
// Scalar independent reconstruction of the original server startup tables.
// Original MathLib_Init RVA 0x26DCC0, sine RVA 0x268910, gamma RVA 0x26CDA0,
// exponent RVA 0x26CD30. No original global addresses or SIMD callbacks.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace sin_independent {
struct ServerMathTables {
    std::array<float,256> sine{},textureToLinear{},linearToGamma{},gammaToLinear{},exponent{};
    std::array<std::uint8_t,256> textureGamma{};
    std::array<int,1024> linearToTexture{},linearToScreen{};
    std::array<float,4096> linearToVertex{};
    std::array<std::uint8_t,4096> linearToLightmap{};
    bool initialized=false;

    // Independent bounded inputs. Retail startup uses exactly 2.2,2.2,0,2.
    // The scalar backend does not claim bit-exact legacy x87/SIMD parity.
    bool Initialize(float gamma,float texture,float brightness,int overbright) noexcept {
        if(!std::isfinite(gamma) || gamma<=0 || gamma>16 ||
           !std::isfinite(texture) || texture<=0 || texture>16 ||
           !std::isfinite(brightness) || brightness<0 || brightness>16)
            return false;
        const double g=1.0/std::min<double>(gamma,3.0);
        const double threshold=brightness<=0?0.125:
            brightness>1?0.05:0.125-0.075*brightness*brightness;
        const double factor=overbright==2?0.5:overbright==4?0.25:1.0;
        const auto truncByte=[](double x) noexcept {
            return static_cast<std::uint8_t>(std::clamp(x,0.0,255.0));
        };
        for(std::size_t i=0;i<256;++i) {
            const double x=static_cast<double>(i)/255.0;
            sine[i]=static_cast<float>(std::sin(i*(6.2831853071795864769/256.0)));
            textureGamma[i]=truncByte(255.0*std::pow(x,texture*g));
            textureToLinear[i]=static_cast<float>(std::pow(x,texture));
            linearToGamma[i]=static_cast<float>(std::pow(x,1.0/2.2));
            gammaToLinear[i]=static_cast<float>(std::pow(x,2.2));
            exponent[i]=static_cast<float>(std::ldexp(1.0,static_cast<int>(i)-128)/255.0);
        }
        for(std::size_t i=0;i<1024;++i) {
            const double x=static_cast<double>(i)/1023.0;
            linearToTexture[i]=truncByte(255.0*std::pow(x,1.0/texture));
            const double lit=x*(brightness>1?brightness:1);
            const double adjusted=lit<=threshold?0.125*lit/threshold:
                0.125+0.875*(lit-threshold)/(1.0-threshold);
            linearToScreen[i]=truncByte(255.0*std::pow(adjusted,g));
        }
        for(std::size_t i=0;i<4096;++i) {
            const double lit=std::pow(static_cast<double>(i)/1024.0,1.0/gamma)*factor;
            linearToVertex[i]=static_cast<float>(std::min(lit,1.0));
            // Model default nearest-even x87 rounding independently of
            // the host's floating-point control word.
            const double value=std::clamp(lit*255.0,0.0,255.0);
            const int lower=static_cast<int>(value);
            const double frac=value-lower;
            linearToLightmap[i]=static_cast<std::uint8_t>(
                lower+(frac>0.5 || (frac==0.5 && (lower&1))));
        }
        initialized=true;
        return true;
    }
    bool InitializeRetailDefaults() noexcept {return Initialize(2.2f,2.2f,0.0f,2);}
};
} // namespace sin_independent
