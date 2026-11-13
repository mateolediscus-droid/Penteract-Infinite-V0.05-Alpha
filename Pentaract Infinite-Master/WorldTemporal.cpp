#include "World.h"
#include <Windows.h>
#include <bit>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace {
    using Key = World::ChunkKey;
    m5::ivec5 position(const Key& k) { return {std::get<0>(k),std::get<1>(k),std::get<2>(k),std::get<3>(k),std::get<4>(k)}; }
    m5::ivec5 chunkPosition(m5::ivec5 p) {
        for(int i=0;i<5;++i) {int size=i?Chunk::SIZE:Chunk::HEIGHT;int q=p[i]/size;if(p[i]%size<0)--q;p[i]=q;}
        return p;
    }
    m5::uvec5 localPosition(m5::ivec5 p,m5::ivec5 c) {
        return m5::uvec5(p-c*m5::ivec5{Chunk::HEIGHT,Chunk::SIZE,Chunk::SIZE,Chunk::SIZE,Chunk::SIZE});
    }
    void put(std::ostream& out,uint64_t n,int bytes) {for(int i=0;i<bytes;++i)out.put(char(n>>(8*i)));if(!out)throw std::runtime_error("Temporal save write failed");}
    uint64_t get(std::istream& in,int bytes) {uint64_t n=0;for(int i=0;i<bytes;++i){int c=in.get();if(c==EOF)throw std::runtime_error("Truncated temporal save");n|=uint64_t(uint8_t(c))<<(8*i);}return n;}
    void putKey(std::ostream& out,const Key& k) {auto p=position(k);for(int i=0;i<5;++i)put(out,uint32_t(p[i]),4);}
    Key getKey(std::istream& in) {m5::ivec5 p;for(int i=0;i<5;++i)p[i]=int32_t(uint32_t(get(in,4)));return {p.a,p.b,p.c,p.d,p.e};}
    double getTime(std::istream& in) {double t=std::bit_cast<double>(get(in,8));if(!std::isfinite(t)||t<0)throw std::runtime_error("Invalid temporal clock");return t;}
}

double World::temporalRate(int birthPercent,int sectorPercent)
{
    double b=birthPercent/100.,s=sectorPercent/100.;
    return (b*s+(1-b)*(1-s))/(b*b+(1-b)*(1-b));
}

void World::selectTemporalSimulation(int sector)
{
    if(sector==temporalCurrent)return;
    std::swap(simulation,temporalSectors[temporalCurrent].simulation);
    std::swap(simulation,temporalSectors[sector].simulation);
    temporalCurrent=sector;
}

World::TemporalCell World::temporalCell(m5::ivec5 p) const
{
    auto k=blockKey(p);
    if(auto it=temporalSectors[temporalCurrent].cells.find(k);it!=temporalSectors[temporalCurrent].cells.end())return it->second;
    if(auto it=temporalBase.find(k);it!=temporalBase.end())return it->second;
    auto c=chunkPosition(p);auto chunk=findChunk(c);
    return {chunk?chunk->getBlockFull(localPosition(p,c)):uint16_t(Block::AIR),0,50};
}

World::TemporalCell World::identifyTemporal(m5::ivec5 p)
{
    auto cell=temporalCell(p);if(cell.identity || cell.id==Block::AIR)return cell;
    auto k=blockKey(p);cell.identity=temporalNextIdentity++;
    temporalAffected[key(chunkPosition(p))].insert(k);
    // Unmodified terrain has one shared identity, allocated only when needed.
    if(auto it=temporalSectors[temporalCurrent].cells.find(k);it!=temporalSectors[temporalCurrent].cells.end()) {
        it->second=cell;temporalSectors[temporalCurrent].locations[cell.identity]=k;
    } else temporalBase[k]=cell;
    return cell;
}

void World::recordTemporal(m5::ivec5 p,uint16_t oldValue,uint16_t value)
{
    auto k=blockKey(p);auto old=identifyTemporal(p);
    temporalBase.try_emplace(k,TemporalCell{oldValue,0,50});
    temporalAffected[key(chunkPosition(p))].insert(k);
    auto& sector=temporalSectors[temporalCurrent];
    if(old.identity)sector.locations.erase(old.identity);
    TemporalCell next=old;next.id=value;
    if(value==Block::AIR) {next.identity=0;next.birth=50;simulation.clocks.erase(k);}
    sector.cells[k]=next;
    if(next.identity)sector.locations[next.identity]=k;
}

void World::applyTemporalChunk(Chunk& chunk,m5::ivec5 pos)
{
    if(!temporalEnabled)return;
    auto it=temporalAffected.find(key(pos));if(it==temporalAffected.end())return;
    const auto& cells=temporalSectors[temporalView].cells;
    for(const auto& k:it->second) {
        auto found=cells.find(k);uint16_t id=found!=cells.end()?found->second.id:temporalBase.at(k).id;
        auto local=localPosition(position(k),pos);
        if(chunk.getBlockFull(local)!=id)chunk.setBlock(local,id);
    }
    savedChunks.insert(key(pos));
}

void World::refreshTemporalView()
{
    for(auto& [k,chunk]:chunks)applyTemporalChunk(*chunk,position(k));
}

bool World::changeTemporalSector(int delta)
{
    if(!temporalEnabled)return false;
    int next=std::clamp(temporalView+delta,0,10);if(next==temporalView)return false;
    temporalView=next;selectTemporalSimulation(next);refreshTemporalView();return true;
}

void World::editTemporalBlock(m5::ivec5 p,uint16_t value,bool placement)
{
    if(!containsPosition(p))return;
    if(!temporalEnabled) {setBlock(p,value);return;}
    // Ensure the terrain reference exists before broadcasting the edit.
    getBlock(p);
    auto original=identifyTemporal(p);int source=temporalView;
    if(placement && value!=Block::AIR) {
        TemporalCell created{value,temporalNextIdentity++,uint8_t(source*10)};
        for(int sector=0;sector<11;++sector) {
            selectTemporalSimulation(sector);setBlock(p,value);
            auto& state=temporalSectors[sector];auto k=blockKey(p);
            if(state.cells[k].identity)state.locations.erase(state.cells[k].identity);
            state.cells[k]=created;state.locations[created.identity]=k;simulation.clocks.erase(k);
        }
    } else if(original.id!=Block::AIR) {
        bool frozen=temporalRate(original.birth,source*10)==0;
        for(int sector=0;sector<11;++sector) {
            if(frozen ? sector!=source : temporalRate(original.birth,sector*10)==0)continue;
            selectTemporalSimulation(sector);auto& state=temporalSectors[sector];
            auto at=state.locations.find(original.identity);
            auto q=at==state.locations.end()?p:position(at->second);
            if(temporalCell(q).identity==original.identity)setBlock(q,value);
        }
    }
    selectTemporalSimulation(source);
}

bool World::temporalDue(m5::ivec5 p,double period)
{
    if(!temporalEnabled || temporalStep==0)return true;
    double rate=temporalRate(temporalCell(p).birth,temporalCurrent*10);
    if(rate==0)return false;
    auto& clock=simulation.clocks[blockKey(p)];clock+=temporalStep*rate;
    if(clock+1e-10<period)return false;
    clock=std::max(0.,clock-period);return true;
}

void World::moveTemporal(m5::ivec5 from,m5::ivec5 to,uint16_t value)
{
    if(!temporalEnabled) {setBlock(to,value);setBlock(from,Block::AIR);return;}
    auto cell=identifyTemporal(from);auto old=blockKey(from),next=blockKey(to);
    double clock=simulation.clocks.contains(old)?simulation.clocks.at(old):0;
    setBlock(from,Block::AIR);setBlock(to,value);
    auto& sector=temporalSectors[temporalCurrent];sector.cells[next]=cell;sector.locations[cell.identity]=next;
    simulation.clocks[next]=clock;
}

bool World::advanceTemporal(double dt)
{
    if(!temporalEnabled)return false;
    temporalAccumulator+=std::max(0.,dt);bool changed=false;
    // All sectors share terrain residency; only sparse active simulations are advanced.
    while(temporalAccumulator+1e-10>=.05) {
        temporalAccumulator=std::max(0.,temporalAccumulator-.05);temporalStep=.05;
        for(int sector=0;sector<11;++sector) {
            selectTemporalSimulation(sector);
            bool moved=updateFallingSand(),signal=updateRedstoneCables();
            if(sector==temporalView)changed|=moved||signal;
            bool pending=!simulation.activeCableBlocks.empty() || !simulation.activeCrystalDoorBlocks.empty() ||
                !simulation.activeRepeaterBlocks.empty() || !simulation.activeRedirectorBlocks.empty();
            if(signal || pending)simulation.idle=0;
            else if(simulation.redstoneTicksActive && (simulation.idle+=.05)>=30.)stopRedstoneTicks();
        }
    }
    temporalStep=0;selectTemporalSimulation(temporalView);return changed;
}

void World::saveTemporal(const std::filesystem::path& file) const
{
    if(!temporalEnabled)return;
    auto temporary=file;temporary+=".tmp";
    std::ofstream out(temporary,std::ios::binary);out.write("PTIME2D\n",8);
    put(out,2,2);put(out,dimensions,1);put(out,temporalView,1);put(out,11,1);put(out,temporalNextIdentity,8);
    put(out,std::bit_cast<uint64_t>(temporalAccumulator),8);
    auto cells=[&](const auto& records) {
        put(out,records.size(),8);
        for(const auto& [k,c]:records){putKey(out,k);put(out,c.id,2);put(out,c.identity,8);put(out,c.birth,1);}
    };
    auto sim=[&](const Simulation& s) {
        auto set=[&](const auto& records){put(out,records.size(),8);for(const auto& k:records)putKey(out,k);};
        auto map=[&](const auto& records){put(out,records.size(),8);for(const auto& [k,v]:records){putKey(out,k);put(out,v,1);}};
        set(s.activeSandBlocks);set(s.activeAntiSandBlocks);set(s.activeCableBlocks);
        set(s.activeCrystalDoorBlocks);set(s.activeRepeaterBlocks);set(s.activeRedirectorBlocks);
        map(s.repeaterInputFaces);map(s.repeaterOutputPowers);map(s.repeaterCooldownTicks);map(s.redirectorOutputPowers);
        put(out,s.redstoneTicksActive,1);put(out,std::bit_cast<uint64_t>(s.idle),8);put(out,s.clocks.size(),8);
        for(const auto& [k,t]:s.clocks){putKey(out,k);put(out,std::bit_cast<uint64_t>(t),8);}
    };
    cells(temporalBase);
    for(int i=0;i<11;++i){cells(temporalSectors[i].cells);sim(i==temporalCurrent?simulation:temporalSectors[i].simulation);}
    out.flush();if(!out)throw std::runtime_error("Cannot save temporal sectors");out.close();
    if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot replace temporal save");
}

void World::enableTemporal(const std::filesystem::path& file)
{
    if(dimensions<2 || dimensions>5 || temporalEnabled)return;
    if(!std::filesystem::exists(file)) {
        for(int i=0;i<11;++i)if(i!=temporalCurrent)temporalSectors[i].simulation=simulation;
        temporalEnabled=true;return;
    }
    std::ifstream in(file,std::ios::binary);char magic[8];in.read(magic,8);
    if(std::string(magic,8)!="PTIME2D\n")throw std::runtime_error("Invalid temporal save");
    auto version=get(in,2);
    if(version!=1 && version!=2)throw std::runtime_error("Unsupported temporal save version");
    int savedDimensions=version==1?5:int(get(in,1));
    if(savedDimensions!=dimensions)throw std::runtime_error("Temporal save spatial dimension mismatch");
    int view=int(get(in,1));if(view>10 || get(in,1)!=11)throw std::runtime_error("Invalid temporal sector");
    auto next=get(in,8);double accumulator=getTime(in);
    std::map<ChunkKey,TemporalCell> base;std::array<TemporalSector,11> sectors;
    auto count=[&](){uint64_t n=get(in,8);if(n>std::filesystem::file_size(file)/20)throw std::runtime_error("Invalid temporal record count");return n;};
    auto cells=[&](auto& records) {
        for(uint64_t n=count();n;--n) {
            auto k=getKey(in);TemporalCell c{uint16_t(get(in,2)),get(in,8),uint8_t(get(in,1))};
            if(!containsPosition(position(k)) || c.birth>100 || c.identity>=next || (c.id && !ExtendedBlock::isRenderable(c.id)))throw std::runtime_error("Invalid temporal block");
            if(!records.emplace(k,c).second)throw std::runtime_error("Duplicate temporal position");
        }
    };
    auto sim=[&](Simulation& s) {
        auto set=[&](auto& records){for(uint64_t n=count();n;--n)records.insert(getKey(in));};
        auto map=[&](auto& records){for(uint64_t n=count();n;--n){auto k=getKey(in);records[k]=uint8_t(get(in,1));}};
        set(s.activeSandBlocks);set(s.activeAntiSandBlocks);set(s.activeCableBlocks);
        set(s.activeCrystalDoorBlocks);set(s.activeRepeaterBlocks);set(s.activeRedirectorBlocks);
        map(s.repeaterInputFaces);map(s.repeaterOutputPowers);map(s.repeaterCooldownTicks);map(s.redirectorOutputPowers);
        s.redstoneTicksActive=get(in,1)!=0;s.idle=getTime(in);
        for(uint64_t n=count();n;--n){auto k=getKey(in);s.clocks[k]=getTime(in);}
    };
    cells(base);
    for(auto& sector:sectors){cells(sector.cells);sim(sector.simulation);for(auto& [k,c]:sector.cells)if(c.identity)sector.locations[c.identity]=k;}
    if(in.peek()!=EOF)throw std::runtime_error("Trailing temporal save data");
    temporalBase=std::move(base);temporalSectors=std::move(sectors);temporalNextIdentity=next;
    temporalView=temporalCurrent=view;simulation=std::move(temporalSectors[view].simulation);
    temporalSectors[view].simulation={};temporalAccumulator=accumulator;
    for(auto& [k,c]:temporalBase)temporalAffected[key(chunkPosition(position(k)))].insert(k);
    temporalEnabled=true;refreshTemporalView();
}
