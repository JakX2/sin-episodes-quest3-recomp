#pragma once
#include <cmath>
namespace sin_port::cone_distance_math {
// 2226c080 / 2226be40; 53-bit original x87 precision, explicit f32 stores.
inline bool cone_line_distance(const float* point,const float* start,const float* end,bool horizontal,float& out) noexcept{
    float dx=static_cast<float>(static_cast<double>(end[0])-start[0]);
    const double dy=static_cast<double>(end[1])-start[1],dz=static_cast<double>(end[2])-start[2];
    const float y=static_cast<float>(dy),z=static_cast<float>(dz);
    const double py=horizontal?dy:y,pz=horizontal?0.:dz;
    const double denom=static_cast<double>(dx)*dx+py*py+pz*pz;
    double k=0;
    if(denom>=static_cast<double>(.00001f)){
        // 2226bdab..2226bdd5 / 2226c003..2226c01f: subtract separately
        // accumulated dot products, rather than pre-subtracting the coordinates.
        const double point_dot=horizontal?
            (static_cast<double>(point[1])*py+static_cast<double>(point[0])*dx):
            ((static_cast<double>(point[1])*py+static_cast<double>(point[2])*pz)+static_cast<double>(point[0])*dx);
        const double start_dot=horizontal?
            (static_cast<double>(start[0])*dx+static_cast<double>(start[1])*py):
            ((static_cast<double>(start[0])*dx+static_cast<double>(start[1])*py)+static_cast<double>(start[2])*pz);
        const double numerator=point_dot-start_dot;
        k=numerator/denom;
    }
    if(horizontal)k=static_cast<float>(k); // 2226c09e
    const float xdiff=static_cast<float>(static_cast<double>(point[0])-(start[0]+k*dx));
    const double ydiff=static_cast<double>(point[1])-(start[1]+k*y);
    const double zdiff=horizontal?0.:static_cast<double>(point[2])-static_cast<float>(start[2]+k*z);
    // 3D accumulates Z^2 + Y^2 first; the horizontal helper starts with X^2.
    const float sum=static_cast<float>(horizontal?
        static_cast<double>(xdiff)*xdiff+ydiff*ydiff:
        (zdiff*zdiff+ydiff*ydiff)+static_cast<double>(xdiff)*xdiff);
    if(!std::isfinite(sum)||sum<0)return false;
    out=std::sqrt(sum);return true;
}
}
