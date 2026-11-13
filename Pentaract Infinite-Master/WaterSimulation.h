#pragma once
#include "Block.h"
#include "WaterSettings.h"
#include <optional>
#include <array>
#include <map>
#include <random>
#include <algorithm>
#include <climits>
#include <vector>

template<int N> class WaterSimulation {
public:
    using Position=std::array<int32_t,N>;
    void changed(const Position& p,uint16_t id) {
        if(id!=ExtendedBlock::WATER){sources.erase(p);return;}
        if(sources.try_emplace(p,0.).second){running=true;idle=0;sweepLeft=0;}
    }
    bool active()const{return running;}
    template<class Read,class Write,class Rate>
    void advance(double dt,Read read,Write write,Rate rate,uint64_t* sharedBudget=nullptr) {
        if(!running)return;
        dt=std::clamp(dt,0.,.25);elapsed+=dt;
        while(running && elapsed+1e-9>=.15) {
        constexpr double step=.15;
        elapsed=std::max(0.,elapsed-step);
        uint64_t localBudget=WaterSettings::maxUpdates;
        auto& budget=sharedBudget?*sharedBudget:localBudget;
        if(!budget)continue;
        bool placed=false,completedSweep=false;
        // New sources join only after this tick, never recursively during it.
        std::vector<Position> additions;
        if(!sweepLeft)sweepLeft=sources.size();
        auto it=nextSource?sources.lower_bound(*nextSource):sources.begin();
        size_t remaining=sources.size();
        while(remaining-- && budget && !sources.empty()) {
            if(it==sources.end())it=sources.begin();
            const auto& p=it->first;
            // Reading/removing an obsolete source also costs one update.
            --budget;
            if(sweepLeft && --sweepLeft==0)completedSweep=true;
            if(read(p)!=ExtendedBlock::WATER){it=sources.erase(it);continue;}
            it->second+=step*rate(p);
            while(it->second+1e-9>=.15) {
            it->second=std::max(0.,it->second-.15);
            for(int attempt=0;attempt<30;++attempt) {
                int side=direction(random);if(side)++side; // Skip only +A (face 1).
                int axis=side/2,delta=side%2?1:-1;
                if((delta<0&&p[axis]==INT32_MIN)||(delta>0&&p[axis]==INT32_MAX))continue;
                auto to=p;to[axis]+=delta;
                if(read(to)!=Block::AIR)continue;
                write(p,to);additions.push_back(std::move(to));placed=true;
            }
            if(it->second+1e-9>=.15){if(!budget){it->second=std::min(it->second,.15);break;}--budget;}
            }
            ++it;
        }
        if(!sources.empty()){if(it==sources.end())it=sources.begin();nextSource=it->first;}
        else nextSource.reset();
        for(const auto& p:additions)sources.try_emplace(p,0.);
        if(placed)sweepLeft=0;
        idle=placed?0:idle+step;
        // Never declare equilibrium before the deferred sources have had a turn.
        if(sources.empty()||(completedSweep && idle+1e-9>=1.5))running=false;
        }
    }
private:
    std::map<Position,double> sources;
    std::optional<Position> nextSource;
    size_t sweepLeft=0;
    std::mt19937 random{std::random_device{}()};
    std::uniform_int_distribution<int> direction{0,2*N-2};
    double elapsed=0,idle=0;
    bool running=false;
};
