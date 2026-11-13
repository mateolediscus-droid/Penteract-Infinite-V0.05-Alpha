#pragma once
#include "TemporalBinary.h"
#include "WaterSimulation.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <windows.h>

// One resident terrain, eleven sparse timelines. All hooks run on the owner thread.
template<int N,class Space,class Logic,class Falling>
class TemporalND {
public:
    using Position=std::array<int32_t,N>;
    using BlockState=decltype(std::declval<Logic>().state(Position{}));
private:
    struct Cell {
        uint16_t id=Block::AIR;
        uint64_t identity=0;
        uint8_t birth=50;
        template<class A> void archive(A& a) {a(id,identity,birth);}
    };
    struct Sector {
        std::map<Position,Cell> cells;
        std::map<uint64_t,Position> locations;
        std::map<Position,double> fallClock,logicClock;
        std::unique_ptr<Logic> logic;
        Falling falling;
        WaterSimulation<N> water;
    };
    Space& space;
    Logic& logic;
    Falling& falling;
    std::array<Sector,11> sectors;
    std::map<Position,Cell> base;
    std::map<Position,std::set<Position>> affected;
    uint64_t nextIdentity=1;
    int view=5,current=5;
    double accumulator=0;
    bool visibleChanged=false;
    double waterBudgetClock=0;
    uint64_t waterBudget=WaterSettings::maxUpdates,waterRevision=WaterSettings::revision;
    int waterFirst=0;

    static Position chunkKey(Position p) {p[0]=int32_t(std::floor(double(p[0])/32.));return p;}
    static bool isFalling(uint16_t id) {return id==ExtendedBlock::SAND || id==ExtendedBlock::ANTI_SAND;}
    Cell cell(const Position& p)const {
        const auto& cells=sectors[current].cells;
        if(auto it=cells.find(p);it!=cells.end())return it->second;
        if(auto it=base.find(p);it!=base.end())return it->second;
        auto raw=space.loadedRaw(p);
        if(!raw) {auto it=space.modifications().find(p);if(it!=space.modifications().end())raw=it->second;}
        return {raw.value_or(Block::AIR),0,50};
    }
    Cell identify(const Position& p) {
        auto value=cell(p);
        if(value.id!=Block::AIR && !value.identity) {
            value.identity=nextIdentity++;
            if(auto it=sectors[current].cells.find(p);it!=sectors[current].cells.end()) {
                it->second=value;sectors[current].locations[value.identity]=p;
            } else base[p]=value;
        }
        return value;
    }
    void write(const Position& p,Cell value) {
        if(!base.contains(p))base.emplace(p,cell(p));
        auto& sector=sectors[current];auto old=cell(p);
        if(old.identity)sector.locations.erase(old.identity);
        sector.cells[p]=value;
        if(value.identity)sector.locations[value.identity]=p;
        affected[chunkKey(p)].insert(p);
        if(current==view) {space.restoreRaw(p,value.id);visibleChanged=true;}
    }
    void record(const Position& p,uint16_t id) {
        auto value=identify(p);
        if(value.id==id) {
            // A face-mask edit keeps its block ID but still needs a mesh upload.
            if(current==view) {space.restoreRaw(p,id);visibleChanged=true;}
            return;
        }
        if(id==Block::AIR)value={};
        else if(!value.identity)value={id,nextIdentity++,50};
        value.id=id;write(p,value);falling.changed(space,p);
    }
    void select(int index) {
        if(index==current)return;
        logic.swapSimulation(*sectors[current].logic);
        std::swap(falling,sectors[current].falling);
        current=index;
        logic.swapSimulation(*sectors[current].logic);
        std::swap(falling,sectors[current].falling);
        configureFalling();
    }
    double delay(const Position& p,double interval,std::map<Position,double>& clocks) {
        const auto value=identify(p);
        double speed=rate(value.birth,current*10);
        if(speed==0)return INFINITY;
        double elapsed=0;
        if(auto it=clocks.find(p);it!=clocks.end()){elapsed=it->second;clocks.erase(it);}
        return std::max(0.,interval-elapsed)/speed;
    }
    void configureFalling() {
        falling.temporalDelay=[this](const Position& p){return delay(p,.5,sectors[current].fallClock);};
        falling.temporalOverdue=[this](const Position& p,double overdue){sectors[current].fallClock[p]=overdue*rate(cell(p).birth,current*10);};
    }
    void captureClocks() {
        auto capture=[this](auto& clocks,double interval,const Position& p,double remaining) {
            if(std::isfinite(remaining))clocks[p]=std::clamp(interval-remaining*rate(cell(p).birth,current*10),0.,interval);
        };
        falling.remaining([&](const Position& p,double t){capture(sectors[current].fallClock,.5,p,t);});
        logic.remaining([&](const Position& p,double t){capture(sectors[current].logicClock,.25,p,t);});
    }
    bool frozen(const Cell& c)const {return rate(c.birth,current*10)==0;}
    void materialize() {
        for(const auto& [p,original]:base) {
            auto id=cell(p).id;auto previous=space.loadedRaw(p);
            if(!previous) {
                auto it=space.modifications().find(p);
                previous=it==space.modifications().end()?original.id:it->second;
            }
            if(*previous!=id){space.restoreRaw(p,id);visibleChanged=true;}
        }
    }
    void apply(const Position& p,BlockState value) {
        logic.set(p,value);falling.changed(space,p);
        sectors[current].water.changed(p,value.id);
    }
public:
    TemporalND(Space& s,Logic& l,Falling& f):space(s),logic(l),falling(f) {
        for(auto& sector:sectors)sector.logic=std::make_unique<Logic>(space);
        // Existing saved circuits belong to every sector at initial enablement.
        for(const auto& [p,id]:space.modifications()) {
            for(auto& sector:sectors) {
                sector.logic->restoreCircuit(p,id);
                if(id==ExtendedBlock::WATER)sector.water.changed(p,id);
                if(auto it=logic.faceEdits().find(p);it!=logic.faceEdits().end())sector.logic->restoreFaces(p,it->second);
            }
        }
        for(auto& sector:sectors)sector.logic->resumeCircuits();
        space.temporalRead=[this](const Position& p,std::optional<uint16_t> raw)->std::optional<uint16_t> {
            if(current==view && raw)return raw;
            if(sectors[current].cells.contains(p) || base.contains(p))return cell(p).id;
            return raw;
        };
        space.temporalWrite=[this](const Position& p,uint16_t id){record(p,id);};
        logic.temporalDelay=[this](const Position& p){return delay(p,.25,sectors[current].logicClock);};
        logic.temporalOverdue=[this](const Position& p,double overdue){sectors[current].logicClock[p]=overdue*rate(cell(p).birth,current*10);};
        configureFalling();
    }
    ~TemporalND() {space.temporalRead={};space.temporalWrite={};logic.temporalDelay={};logic.temporalOverdue={};falling.temporalDelay={};falling.temporalOverdue={};}
    static double rate(int birth,int sector) {
        const double b=birth/100.,s=sector/100.;
        return (b*s+(1-b)*(1-s))/(b*b+(1-b)*(1-b));
    }
    int percent()const {return view*10;}
    void changeSector(int delta) {
        int next=std::clamp(view+delta,0,10);
        if(next==view)return;
        int previous=view;
        select(next);view=next;materialize();
        const auto& oldFaces=sectors[previous].logic->faceEdits();
        const auto& newFaces=logic.faceEdits();
        auto mark=[&](const auto& a,const auto& b){for(const auto& [p,mask]:a) {
            auto it=b.find(p);
            if((it==b.end()?0:it->second)!=mask && space.loadedRaw(p))space.restoreRaw(p,cell(p).id);
        }};
        mark(oldFaces,newFaces);mark(newFaces,oldFaces);
    }
    void edit(const Position& p,BlockState value,bool placement=true) {
        select(view);
        if(placement && value.id!=Block::AIR) {
            const Cell fresh{value.id,nextIdentity++,uint8_t(view*10)};
            for(int i=0;i<11;++i) {
                select(i);identify(p);apply(p,value);write(p,fresh);
                sectors[i].fallClock.erase(p);sectors[i].logicClock.erase(p);
                falling.reschedule(space,p);logic.reschedule(p);
            }
        } else {
            auto original=identify(p);bool isolated=frozen(original);
            for(int i=0;i<11;++i) {
                select(i);
                if(isolated ? i!=view : frozen(original))continue;
                auto where=p;
                if(auto it=sectors[i].locations.find(original.identity);it!=sectors[i].locations.end())where=it->second;
                if(cell(where).identity!=original.identity)continue;
                apply(where,value);
                if(value.id==Block::AIR) {sectors[i].fallClock.erase(where);sectors[i].logicClock.erase(where);}
            }
        }
        select(view);
    }
    bool interact(const Position& p,int side) {
        auto value=logic.state(p);
        if(ExtendedBlock::isLever(value.id))value.id=ExtendedBlock::toggledLever(value.id);
        else if(ExtendedBlock::isRepeater(value.id))value.id=ExtendedBlock::nextRepeaterMode(value.id);
        else if(ExtendedBlock::isRedirector(value.id) && side>=0 && side<2*N) {
            if constexpr(requires {value.faces.toggle(side);})value.faces.toggle(side);
            else value.faces^=uint32_t(1u<<side);
        }
        else return false;
        edit(p,value,false);return true;
    }
    void chunkLoaded(const Position& key) {
        auto chunk=space.find(key);if(!chunk)return;
        std::set<Position> candidates;
        for(const auto& [packed,id]:chunk->extendedBlocks()) {
            if(id!=ExtendedBlock::WATER && !isFalling(id) && !ExtendedBlock::isLever(id) && !ExtendedBlock::isCable(id) &&
                !ExtendedBlock::isRepeater(id) && !ExtendedBlock::isRedirector(id) && !ExtendedBlock::isCrystalDoor(id))continue;
            auto p=key;p[0]=key[0]*32+int32_t(packed&127);
            if(isFalling(id) && !base.contains(p)) {
                auto support=p;
                int64_t a=int64_t(p[0])+(id==ExtendedBlock::SAND?-1:1);
                if(a>=INT32_MIN && a<=INT32_MAX) {
                    support[0]=int32_t(a);
                    auto below=space.loadedRaw(support);
                    if(!base.contains(support) && below && *below!=Block::AIR)continue;
                }
            }
            candidates.insert(p);
        }
        if(auto it=affected.find(key);it!=affected.end())candidates.insert(it->second.begin(),it->second.end());
        std::vector<Position> positions(candidates.begin(),candidates.end());
        for(int i=0;i<11;++i) {
            if(positions.empty() && !(i==current?falling:sectors[i].falling).waitingFor(key))continue;
            select(i);logic.loadedCandidates(positions);falling.loadedCandidates(space,key,positions);
            for(const auto& p:positions)if(cell(p).id==ExtendedBlock::WATER)sectors[i].water.changed(p,ExtendedBlock::WATER);
        }
        select(view);
    }
    void retainChunks(const std::set<Position>& wanted) {
        for(int i=0;i<11;++i){select(i);falling.retainChunks(wanted);}
        select(view);
    }
    bool advance(double dt) {
        accumulator+=std::clamp(dt,0.,.25);
        while(accumulator+1e-9>=.05) {
            accumulator=std::max(0.,accumulator-.05);
            waterBudgetClock+=.05;
            if(waterBudgetClock+1e-9>=.15 || waterRevision!=WaterSettings::revision) {
                if(waterBudgetClock+1e-9>=.15)waterBudgetClock=std::max(0.,waterBudgetClock-.15);
                waterBudget=WaterSettings::maxUpdates;waterRevision=WaterSettings::revision;
                waterFirst=(waterFirst+1)%11;
            }
            for(int index=0;index<11;++index) {
                int i=(waterFirst+index)%11;
                if(!(i==current?logic:*sectors[i].logic).active() &&
                    !(i==current?falling:sectors[i].falling).activeCount() && !sectors[i].water.active())continue;
                select(i);logic.advance(.05);
                if constexpr(requires {space.simulationBlock(Position{});}) {
                    sectors[i].water.advance(.05,[this](const Position& p){return space.simulationBlock(p);},
                        [this](const Position& from,const Position& to){
                            auto birth=identify(from).birth;
                            // A flow is local to its timeline, not a new player placement.
                            record(to,ExtendedBlock::WATER);
                            auto value=identify(to);value.birth=birth;write(to,value);
                        },[this](const Position& p){return rate(cell(p).birth,current*10);},&waterBudget);
                }
                for(const auto& [from,to]:falling.moves(space)) {
                    if(space.loadedBlock(to)!=Block::AIR)continue;
                    auto value=identify(from);if(!isFalling(value.id))continue;
                    double residue=sectors[i].fallClock[from];
                    apply(from,{Block::AIR});apply(to,{value.id});write(to,value);
                    sectors[i].fallClock.erase(from);sectors[i].fallClock[to]=residue;
                    falling.reschedule(space,to);
                }
            }
        }
        select(view);return std::exchange(visibleChanged,false);
    }
    void save(const std::filesystem::path& path) {
        select(view);
        struct RestoreView {TemporalND& time;~RestoreView(){time.select(time.view);}} restoreView{*this};
        auto temp=path;temp+=".tmp";
        std::ofstream out(temp,std::ios::binary|std::ios::trunc);
        out.write("PTIMEND\n",8);temporalBinary::Writer a(out);
        if constexpr(N>255)a(uint16_t(3),uint32_t(N));
        else a(uint16_t(2),uint8_t(N));
        a(uint8_t(view),nextIdentity,accumulator,base);
        for(int i=0;i<11;++i) {
            select(i);auto& sector=sectors[i];
            captureClocks();
            a(sector.cells,sector.fallClock,sector.logicClock);
            logic.archive(a);falling.archive(a);
        }
        select(view);out.flush();if(!out)throw std::runtime_error("Cannot flush temporal save");out.close();
        if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace temporal save");
    }
    void load(const std::filesystem::path& path) {
        if(!std::filesystem::exists(path))return;
        std::ifstream in(path,std::ios::binary);char magic[8]{};in.read(magic,8);
        if(std::string(magic,8)!="PTIMEND\n")throw std::runtime_error("Invalid ND temporal save");
        temporalBinary::Reader a(in,std::filesystem::file_size(path)-8);
        uint16_t version;uint32_t dimension;uint8_t selected;a(version);
        if(version==2){uint8_t narrowDimension;a(narrowDimension);dimension=narrowDimension;}
        else if(version==3)a(dimension);
        else throw std::runtime_error("Incompatible ND temporal save");
        a(selected);
        if(dimension!=N || selected>10)throw std::runtime_error("Incompatible ND temporal save");
        a(nextIdentity,accumulator,base);
        if(!nextIdentity || !std::isfinite(accumulator) || accumulator<0 || accumulator>=.05)throw std::runtime_error("Invalid temporal clock");
        auto check=[&](const Position& p,const Cell& value) {
            if(value.birth>100 || value.identity>=nextIdentity || (value.id>=Block::COUNT && !ExtendedBlock::isExtended(value.id)))throw std::runtime_error("Invalid temporal block");
            affected[chunkKey(p)].insert(p);
        };
        affected.clear();for(const auto& [p,value]:base)check(p,value);
        for(int i=0;i<11;++i) {
            select(i);auto& sector=sectors[i];
            a(sector.cells,sector.fallClock,sector.logicClock);logic.archive(a);falling.archive(a);
            logic.resetSchedule();falling.resetSchedule();
            sector.locations.clear();
            for(const auto& [p,value]:sector.cells) {check(p,value);if(value.identity)sector.locations[value.identity]=p;}
            sector.water={};
            for(const auto& [p,value]:base)if(cell(p).id==ExtendedBlock::WATER)sector.water.changed(p,ExtendedBlock::WATER);
            for(const auto& [p,value]:sector.cells)if(value.id==ExtendedBlock::WATER)sector.water.changed(p,value.id);
            for(const auto* clocks:{&sector.fallClock,&sector.logicClock})for(const auto& [p,t]:*clocks)
                if(!std::isfinite(t) || t<0 || t>.5)throw std::runtime_error("Invalid local temporal clock");
        }
        a.finish();select(selected);view=selected;materialize();
    }
};
