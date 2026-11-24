#pragma once
#include <cstdint>
#include <cstring>
#include <limits>

namespace sin_port::choreography_index {
// PE32 indexed red-black node prefix: left/right/parent/color. The
// original uses uint32_t for scales and uint16_t for pooled symbols.
template<class I,unsigned Stride> struct Tree {
    static constexpr I nil=(std::numeric_limits<I>::max)();
    unsigned char* rows;
    unsigned count;
    I root;
    I get(I i,unsigned field) const noexcept {
        I v{};std::memcpy(&v,rows+static_cast<unsigned>(i)*Stride+field*sizeof(I),sizeof(v));return v;
    }
    void set(I i,unsigned field,I v) noexcept {
        std::memcpy(rows+static_cast<unsigned>(i)*Stride+field*sizeof(I),&v,sizeof(v));
    }
    bool index(I i) const noexcept {return i!=nil&&static_cast<unsigned>(i)<count;}
    I color(I i) const noexcept {return i==nil?I(1):get(i,3);}
    // Validation before insertion: exact parent links, no red-red, equal
    // black heights, bounded depth. Strict parent links rule out cycles.
    template<class Less>
    int validate(I i,I parent,I lower,I upper,unsigned depth,unsigned& seen,Less less) const noexcept {
        if(i==nil)return 1;
        if(depth>=64||!index(i)||++seen>count||get(i,2)!=parent||get(i,3)>1||
           (lower!=nil&&!less(lower,i))||(upper!=nil&&!less(i,upper)))return -1;
        const I l=get(i,0),r=get(i,1);
        if((l!=nil&&!index(l))||(r!=nil&&!index(r)))return -1;
        if(!color(i)&&(!color(l)||!color(r)))return -1;
        const int a=validate(l,i,lower,i,depth+1,seen,less);
        if(a<0)return -1;
        const int b=validate(r,i,i,upper,depth+1,seen,less);
        return a==b?a+color(i):-1;
    }
    template<class Less> bool valid(unsigned live,Less less) const noexcept {
        if(root==nil)return live==0;
        if(!index(root)||color(root)!=1)return false;
        unsigned seen=0;
        return validate(root,nil,nil,nil,0,seen,less)>0&&seen==live;
    }
    // Rotate toward side (0 = left rotation, 1 = right rotation).
    void rotate(I x,unsigned side) noexcept {
        const I y=get(x,1-side),middle=get(y,side),p=get(x,2);
        set(x,1-side,middle);
        if(middle!=nil)set(middle,2,x);
        set(y,2,p);
        if(p==nil)root=y;
        else set(p,get(p,0)==x?0:1,y);
        set(y,side,x);set(x,2,y);
    }
    // Call only after validation and slot reservation. Link is the
    // original insertion before 22115310 / the 16-bit counterpart.
    void link(I z,I parent,unsigned side) noexcept {
        set(z,0,nil);set(z,1,nil);set(z,2,parent);set(z,3,0);
        if(parent==nil)root=z;else set(parent,side,z);
        while(z!=root){
            I p=get(z,2);
            if(color(p)!=0)break;
            I g=get(p,2);
            const unsigned s=get(g,0)==p?0:1;
            const I uncle=get(g,1-s);
            if(color(uncle)==0){
                set(p,3,1);set(uncle,3,1);set(g,3,0);z=g;
            }else{
                if(get(p,1-s)==z){z=p;rotate(z,s);p=get(z,2);g=get(p,2);}
                set(p,3,1);set(g,3,0);rotate(g,1-s);
            }
        }
        set(root,3,1);
    }
};
}
