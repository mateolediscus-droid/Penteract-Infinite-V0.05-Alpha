#pragma once
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>

// Owner-thread deadline queue. Sleeping/frozen blocks have no recurring checks.
template<class Position> class TemporalEvents {
    double now=0;
    std::multimap<double,Position> queue;
    std::map<Position,typename std::multimap<double,Position>::iterator> entries;
public:
    TemporalEvents()=default;
    TemporalEvents(const TemporalEvents&)=delete;
    TemporalEvents& operator=(const TemporalEvents&)=delete;
    TemporalEvents(TemporalEvents&&)=default;
    TemporalEvents& operator=(TemporalEvents&&)=default;
    template<class Delay> void schedule(const Position& p,Delay&& delay) {
        if(entries.contains(p))return;
        double duration=delay(p);
        entries.emplace(p,queue.emplace(std::isfinite(duration)?now+std::max(0.,duration):INFINITY,p));
    }
    void erase(const Position& p) {
        auto it=entries.find(p);if(it==entries.end())return;
        queue.erase(it->second);entries.erase(it);
    }
    void clear(){entries.clear();queue.clear();}
    template<class Overdue> std::vector<Position> advance(double dt,Overdue&& overdue) {
        now+=dt;std::vector<Position> ready;
        while(!queue.empty() && queue.begin()->first<=now+1e-9) {
            auto it=queue.begin();auto p=it->second;
            overdue(p,std::max(0.,now-it->first));ready.push_back(p);
            entries.erase(p);queue.erase(it);
        }
        return ready;
    }
    template<class Visit> void remaining(Visit&& visit)const {
        for(const auto& [deadline,p]:queue)visit(p,std::max(0.,deadline-now));
    }
};
