#pragma once
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <limits>
#include <cstdint>

namespace AxisNames {
inline std::string name(size_t axis){
    std::string result;
    for(;;){result.push_back(char('A'+axis%26));if(axis<26)break;axis=axis/26-1;}
    std::reverse(result.begin(),result.end());return result;
}
inline int parse(const std::string& text){
    if(text.empty())return -1;
    size_t value=0;
    for(unsigned char c:text){
        if(c>='a' && c<='z')c-=32;
        if(c<'A' || c>'Z')return -1;
        if(value>(size_t(INT32_MAX)+1-(c-'A'+1))/26)return -1;
        value=value*26+c-'A'+1;
    }
    return int(value-1);
}
}
struct AxisGroups {
    std::vector<std::vector<int>> groups;
    explicit AxisGroups(int n=6){reset(n);}
    static int shortcutStep(uint32_t code){
        switch(code){case 0xe9:return 1;case '&':return -1;case '"':return 10;case '\'':return -10;default:return 0;}
    }
    int shifted(int current,int step)const{
        const int64_t count=static_cast<int64_t>(groups.size());
        if(!count)return current;
        auto next=(int64_t(current)+step)%count;
        return int(next<0?next+count:next);
    }
    void reset(int n){groups.clear();for(int a=0;a<n;++a){if(a%9==0)groups.emplace_back();groups.back().push_back(a);}}
    void assign(std::vector<std::vector<int>> value,int n){
        std::vector<bool> seen(n,false);int count=0;
        for(const auto& group:value){
            if(group.empty() || group.size()>9)throw std::runtime_error("Each group needs 1 to 9 axes.");
            for(int axis:group){if(axis<0 || axis>=n || seen[axis])throw std::runtime_error("Each axis must occur exactly once.");seen[axis]=true;++count;}
        }
        if(count!=n)throw std::runtime_error("Missing axes.");groups=std::move(value);
    }
    bool contains(int group,int axis)const{const auto& g=groups.at(group);return std::find(g.begin(),g.end(),axis)!=g.end();}
    int hiddenAt(int group,int ordinal,std::array<int,2> plane,bool side)const{
        for(int axis:groups.at(group)){
            if(!axis || axis==plane[0] || (!side && axis==plane[1]))continue;
            if(ordinal--==0)return (side?1:2)+axis-1-int(plane[0]<axis)-int(!side && plane[1]<axis);
        }
        return -1;
    }
    std::vector<int> hidden(int group,std::array<int,2> plane,bool side)const{
        std::vector<int> result;
        for(int axis:groups.at(group)){
            if(!axis || axis==plane[0] || (!side && axis==plane[1]))continue;
            int slot=(side?1:2)+axis-1-int(plane[0]<axis)-int(!side && plane[1]<axis);
            result.push_back(slot);
        }
        return result;
    }
    std::array<int,2> plane(int group,int n)const{
        std::vector<int> horizontal;for(int axis:groups.at(group))if(axis)horizontal.push_back(axis);
        if(horizontal.empty())return {1,2};
        if(horizontal.size()==1)return {horizontal[0],horizontal[0]==1?2:1};
        return {horizontal[0],horizontal[1]};
    }
};
