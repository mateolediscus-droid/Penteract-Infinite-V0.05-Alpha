#pragma once
#include "Block.h"
#include "AxisGroups.h"
#include "WaterSettings.h"
#include <charconv>
#include <array>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace WorldCommands {
inline const std::string axisNames="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
inline std::string axisName(size_t axis){return AxisNames::name(axis);}
inline double number(const std::string& token) {
    size_t used=0;double value=std::stod(token,&used);
    if(used!=token.size() || !std::isfinite(value))throw std::runtime_error("Invalid number: "+token);
    return value;
}
template<size_t N> struct Context {
    std::array<double,N> eye;
    double& speed;
    int renderType;
    std::function<void(const std::array<int32_t,N>&,uint16_t)> edit;
    std::function<void(const std::array<double,N>&)> teleport;
    std::function<void(int,int)> align;
    std::function<void()> save;
};
template<size_t N> std::string execute(std::string text,Context<N> context) {
    std::istringstream input(text);std::vector<std::string> args;
    for(std::string arg;input>>arg;)args.push_back(arg);
    if(args.empty())return {};
    if(args[0].starts_with('/'))args[0].erase(0,1);
    size_t cursor=1;
    auto take=[&]() {if(cursor>=args.size())throw std::runtime_error("Missing argument");return args[cursor++];};
    auto end=[&]() {if(cursor!=args.size())throw std::runtime_error("Too many arguments");};
    auto position=[&]() {
        std::array<double,N> p{};
        for(size_t axis=0;axis<N;++axis) {
            auto value=take();
            p[axis]=value[0]=='~'?context.eye[axis]+(value.size()==1?0:number(value.substr(1))):number(value);
            if(!std::isfinite(p[axis]) || p[axis]<INT32_MIN+1024. || p[axis]>INT32_MAX-1024.)
                throw std::runtime_error("Coordinate outside world range");
        }
        return p;
    };
    auto cell=[](const std::array<double,N>& p) {
        std::array<int32_t,N> result{};for(size_t i=0;i<N;++i)result[i]=int32_t(std::floor(p[i]));return result;
    };
    auto block=[&]() {
        auto token=take();size_t used=0;auto value=std::stoll(token,&used);
        if(used!=token.size() || value<0 || value>=ExtendedBlock::COUNT || value==Block::EXTENDED_MARKER ||
            (value!=Block::AIR && !ExtendedBlock::isRenderable(uint16_t(value))))
            throw std::runtime_error("Invalid block ID");
        return uint16_t(value);
    };
    if(args[0]=="help") {
        end();return "help | clear | save | speed [units/sec]\nWaterMaxUpdate [count|unlimited] (0 pauses water)\nsetBlock <position> <id>\nfill <position A> <position B> <id>\ntp <position>\nalign B C / align AA AB / align 1 2\n"
            "Coordinates ("+std::to_string(N)+"): A through "+axisName(N-1)+"; A is vertical. Relative: ~, ~2, ~-2.";
    }
    if(args[0]=="clear"){end();return {};}
    if(args[0]=="WaterMaxUpdate" || args[0]=="watermaxupdate") {
        if(cursor<args.size()) {
            auto token=take();end();uint64_t value=WaterSettings::unlimited;
            if(token!="unlimited") {
                auto result=std::from_chars(token.data(),token.data()+token.size(),value);
                if(result.ec!=std::errc{} || result.ptr!=token.data()+token.size())
                    throw std::runtime_error("WaterMaxUpdate expects a non-negative integer or unlimited.");
            }
            WaterSettings::set(value);
        }
        return "WaterMaxUpdate: "+(WaterSettings::maxUpdates==WaterSettings::unlimited?std::string("unlimited"):std::to_string(WaterSettings::maxUpdates))+" block updates / 150 ms (30 directions each).";
    }
    if(args[0]=="save"){end();context.save();return "Saved.";}
    if(args[0]=="speed") {
        if(cursor<args.size()) {
            double speed=number(take());end();
            if(speed<0 || speed>INT32_MAX)throw std::runtime_error("Invalid movement speed");
            context.speed=speed;
        }
        return "Speed: "+std::to_string(context.speed)+" units/sec.";
    }
    if(args[0]=="tp") {auto p=position();end();context.teleport(p);return "Teleported.";}
    if(args[0]=="setBlock") {auto p=cell(position());auto id=block();end();context.edit(p,id);return "Block set.";}
    if(args[0]=="fill") {
        auto a=cell(position()),b=cell(position());auto id=block();end();
        for(size_t i=0;i<N;++i)if(a[i]>b[i])std::swap(a[i],b[i]);
        auto p=a;uint64_t count=0;
        // Direct inclusive Cartesian traversal, matching the existing fill command.
        for(;;) {
            context.edit(p,id);++count;
            size_t axis=0;
            for(;axis<N;++axis) {if(p[axis]<b[axis]){++p[axis];break;}p[axis]=a[axis];}
            if(axis==N)break;
        }
        return "Filled "+std::to_string(count)+" blocks.";
    }
    if(args[0]=="align") {
        std::vector<std::string> axes;
        while(cursor<args.size())axes.push_back(take());
        if(context.renderType!=2 && axes.size()==1 && axes[0].size()==2 && std::isalpha((unsigned char)axes[0][0]) && std::isalpha((unsigned char)axes[0][1]))
            axes={axes[0].substr(0,1),axes[0].substr(1,1)};
        if(axes.empty() || axes.size()>2 || (context.renderType!=2 && axes.size()!=2))
            throw std::runtime_error("align requires two axes (one is allowed in 2D rendering)");
        auto parse=[&](const std::string& axis) {
            int result=-1;
            if(std::isalpha((unsigned char)axis[0]))result=AxisNames::parse(axis);
            else {auto value=number(axis);if(value==std::floor(value) && value>=1 && value<N)result=int(value);}
            if(result<1 || result>=N)throw std::runtime_error("Invalid horizontal axis");
            return result;
        };
        int x=parse(axes[0]),z=axes.size()==2?parse(axes[1]):(x==1?2:1);
        if(x==z)throw std::runtime_error("Choose different axes");
        context.align(x,z);return "Aligned.";
    }
    throw std::runtime_error("Unknown command. Use help.");
}
}
