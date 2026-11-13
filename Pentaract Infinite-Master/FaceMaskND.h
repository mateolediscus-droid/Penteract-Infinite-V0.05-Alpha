#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

// Face indices span words; no shift ever uses the number of spatial dimensions.
template<int N> struct FaceMaskND {
    static constexpr int wordCount=(2*N+31)/32;
    std::array<uint32_t,wordCount> words{};
    FaceMaskND()=default;
    FaceMaskND(uint32_t low){words[0]=low;}
    explicit operator bool()const {for(auto w:words)if(w)return true;return false;}
    bool operator==(const FaceMaskND&)const=default;
    bool valid()const {
        if constexpr((2*N)%32==0)return true;
        else return (words.back()>>((2*N)%32))==0;
    }
    bool test(int face)const {return face>=0 && face<2*N && (words[face/32]&(1u<<(face%32)))!=0;}
    void toggle(int face){
        if(face<0 || face>=2*N)throw std::out_of_range("ND block face");
        words[face/32]^=1u<<(face%32);
    }
    FaceMaskND& operator|=(uint32_t low){words[0]|=low;return *this;}
    template<class Archive> void archive(Archive& a){a(words);}
};
