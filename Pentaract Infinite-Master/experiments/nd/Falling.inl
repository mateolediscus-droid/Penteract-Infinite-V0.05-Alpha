
    class Falling
    {
        std::set<Position> active;
        std::map<Position,std::set<Position>> waiting;
    public:
        std::function<double(const Position&)> temporalDelay;
        std::function<void(const Position&,double)> temporalOverdue;
        TemporalEvents<Position> events;
        bool prepared=false;
        template<class Visit> void remaining(Visit&& visit)const {events.remaining(visit);}
        void resetSchedule(){events.clear();prepared=false;}
        void reschedule(Space& space,const Position& p){events.erase(p);wake(space,p);}
        template<class Archive> void archive(Archive& a) {a(active,waiting);}
        size_t activeCount() const { return active.size(); }
        bool waitingFor(const Position& key)const {return waiting.contains(key);}
        size_t waitingCount() const { size_t n=0;for(const auto& [key,p]:waiting)n+=p.size();return n; }
        void wake(Space& space,const Position& p)
        {
            if(!space.find(address(p).chunk))return;
            auto id=space.loadedBlock(p);
            if(!id || (*id!=ExtendedBlock::SAND && *id!=ExtendedBlock::ANTI_SAND)) {active.erase(p);events.erase(p);return;}
            auto to=neighbor(p,*id==ExtendedBlock::SAND?0:1);if(!to) return;
            auto support=space.find(address(*to).chunk)?space.loadedBlock(*to):std::nullopt;
            if(!support) {waiting[address(*to).chunk].insert(p);active.erase(p);events.erase(p);}
            else if(*support==Block::AIR) {active.insert(p);if(temporalDelay)events.schedule(p,temporalDelay);}
            else {active.erase(p);events.erase(p);}
        }
        void changed(Space& space,const Position& p)
        {
            wake(space,p);
            for(int side:{0,1}) if(auto n=neighbor(p,side)) wake(space,*n);
        }
        void loaded(Space& space,const Position& key)
        {
            auto chunk=space.find(key);if(!chunk)return;
            for(const auto& [packed,id]:chunk->extendedBlocks())
                if(id==ExtendedBlock::SAND || id==ExtendedBlock::ANTI_SAND) {
                    m5::uvec5 local{packed&127,(packed>>7)&7,(packed>>10)&7,(packed>>13)&7,(packed>>16)&7};
                    int next=int(local.a)+(id==ExtendedBlock::SAND?-1:1);
                    if(next>=0 && next<32) {
                        auto support=local;support.a=next;
                        if(chunk->getBlockFull(support)!=Block::AIR)continue;
                        active.insert(position(key,local));
                    } else wake(space,position(key,local));
                }
            loadedCandidates(space,key,{});
        }
        void loadedCandidates(Space& space,const Position& key,const std::vector<Position>& candidates)
        {
            for(const auto& p:candidates)wake(space,p);
            auto it=waiting.find(key);
            if(it!=waiting.end()) {
                auto pending=std::move(it->second);waiting.erase(it);
                for(const auto& p:pending) wake(space,p);
            }
        }
        void unloaded(const Position& key)
        {
            std::erase_if(active,[&](const Position& p){if(address(p).chunk!=key)return false;events.erase(p);return true;});
            for(auto it=waiting.begin();it!=waiting.end();) {
                std::erase_if(it->second,[&](const Position& p){return address(p).chunk==key;});
                if(it->second.empty())it=waiting.erase(it);else ++it;
            }
        }
        void retainChunks(const std::set<Position>& wanted)
        {
            std::erase_if(active,[&](const Position& p){if(wanted.contains(address(p).chunk))return false;events.erase(p);return true;});
            for(auto it=waiting.begin();it!=waiting.end();) {
                std::erase_if(it->second,[&](const Position& p){return !wanted.contains(address(p).chunk);});
                if(it->second.empty())it=waiting.erase(it);else ++it;
            }
        }
        std::vector<std::pair<Position,Position>> moves(Space& space)
        {
            std::vector<Position> pending;
            if(temporalDelay) {
                if(!prepared){for(const auto& p:active)events.schedule(p,temporalDelay);prepared=true;}
                pending=events.advance(.05,temporalOverdue);
                for(const auto& p:pending)active.erase(p);
            } else {pending.assign(active.begin(),active.end());active.clear();}
            std::vector<std::pair<Position,Position>> result;
            for(const auto& p:pending) {
                if(!space.find(address(p).chunk))continue;
                auto id=space.loadedBlock(p);
                if(!id || (*id!=ExtendedBlock::SAND && *id!=ExtendedBlock::ANTI_SAND))continue;
                auto to=neighbor(p,*id==ExtendedBlock::SAND?0:1);if(!to)continue;
                auto support=space.find(address(*to).chunk)?space.loadedBlock(*to):std::nullopt;
                if(!support)waiting[address(*to).chunk].insert(p);
                else if(*support==Block::AIR)result.emplace_back(p,*to);
            }
            return result;
        }
    };
