
    using Position = std::array<int32_t, N>;
    using Vector = std::array<double, N>;
    static inline constexpr Position extent=[] {Position p{};p.fill(1);p[0]=32;return p;}();

    struct Address
    {
        Position chunk;
        m5::uvec5 local;
    };

    static inline Address address(const Position& p)
    {
        Address result{};result.chunk=p;
        result.local.a=uint32_t(p[0])&31u;
        result.chunk[0]=int32_t((int64_t(p[0])-result.local.a)/32);
        return result;
    }

    static inline Position position(const Position& chunk, const m5::uvec5& local)
    {
        Position p{};
        for (int axis = 0; axis <N; ++axis)
        {
            int64_t coordinate = (int64_t)chunk[axis] * extent[axis] + (axis < 5 ? local[axis] : 0);
            if (coordinate < INT32_MIN || coordinate > INT32_MAX) throw std::out_of_range("ND world coordinate");
            p[axis] = (int32_t)coordinate;
        }
        return p;
    }

    static inline std::optional<Position> neighbor(Position p, int side)
    {
        if (side < 0 || side >= 2*N) throw std::out_of_range("ND block face");
        int axis = side / 2;
        int delta = side % 2 ? 1 : -1;
        if ((delta < 0 && p[axis] == INT32_MIN) || (delta > 0 && p[axis] == INT32_MAX)) return {};
        p[axis] += delta;
        return p;
    }

    struct Hit
    {
        uint16_t block = Block::AIR;
        double distance = 0;
        Position position{};
        int side = -1;
        bool unloaded = false;
    };

    class Space
    {
    public:
        // Each worker receives an immutable position and owns its chunk until publication.
        using Generator = std::function<void(ColumnChunk&, const Position&)>;
        using Resolver = std::function<Generator(const Position&)>;
        explicit Space(Generator generator) : generator(std::move(generator)) {}
        Space(const Space&) = delete;
        Space& operator=(const Space&) = delete;

        size_t loadedCount() const { return chunks.size(); }
        std::vector<Position> takePublished() {return std::exchange(publishedKeys,{});}
        std::set<Position> takeRenderChanges() {return std::exchange(renderChanges,{});}
        size_t cachedCount() const { return cache.size(); }
        void clearGenerationCache() {cache.clear();cacheOrder.clear();}
        void setCacheLimit(size_t limit) {cacheLimit=limit;trimCache();}
        size_t pendingCount() const { return pending.size(); }
        const std::map<Position, uint16_t>& modifications() const { return edits; }
        const auto& loaded() const { return chunks; }
        void setResolver(Resolver value) { resolver=std::move(value); }
        // Owner-thread temporal overlays; generation workers never access these hooks.
        std::function<std::optional<uint16_t>(const Position&, std::optional<uint16_t>)> temporalRead;
        std::function<void(const Position&, uint16_t)> temporalWrite;
        void restore(const Position& p, uint16_t id)
        {
            if(temporalWrite) temporalWrite(p,id); else restoreRaw(p,id);
        }
        void restoreRaw(const Position& p, uint16_t id)
        {
            edits[p] = id;
            saved[address(p).chunk][p] = id;
            if (auto it = chunks.find(address(p).chunk); it != chunks.end()) {
                it->second->setBlock(address(p).local, id);renderChanges.insert(it->first);
            }
        }
        void retain(const std::set<Position>& wanted)
        {
            // Both containers are ordered: merge once instead of N-dimensional
            // tree searches for every resident column.
            auto keep=wanted.begin();
            for (auto it = chunks.begin(); it != chunks.end();)
            {
                while(keep!=wanted.end() && *keep<it->first)++keep;
                if (keep==wanted.end() || it->first!=*keep) {
                    cacheColumn(it->first,*it->second);
                    it = chunks.erase(it);
                } else ++it;
            }
            std::erase_if(queued,[&](const Request& request){
                if(wanted.contains(request.key))return false;
                pending.erase(request.key);return true;
            });
            // Submitted work cannot be cancelled in the pool. Keep it counted until
            // completion instead of accumulating invisible obsolete jobs while turning.
            for(auto& [key,work]:pending)work.wanted=wanted.contains(key);
        }

        const ColumnChunk* find(const Position& key) const
        {
            auto it = chunks.find(key);
            return it == chunks.end() ? nullptr : it->second.get();
        }

        void release(const std::vector<Position>& keys)
        {
            for(const auto& key:keys) {
                if(auto it=chunks.find(key);it!=chunks.end()) {
                    cacheColumn(it->first,*it->second);chunks.erase(it);
                }
                if(auto it=pending.find(key);it!=pending.end())it->second.wanted=false;
            }
            std::erase_if(queued,[&](const Request& request){
                auto it=pending.find(request.key);
                if(it==pending.end() || it->second.wanted)return false;
                pending.erase(it);return true;
            });
        }

        static constexpr size_t batchSize=32;
        size_t submittedBatchCount() const { return submittedBatches; }

        void request(const Position& key)
        {
            if(revive(key))return;
            if (chunks.contains(key))return;
            if(auto it=pending.find(key);it!=pending.end()){it->second.wanted=true;return;}
            if(pending.size() >= 256)return;
            auto generate=resolver?resolver(key):generator;
            queued.push_back({key,std::move(generate)});
            pending.emplace(key,Pending{});
        }

        void flush()
        {
            // Request order already prioritizes the player's collision region.
            for(size_t first=0;first<queued.size();first+=batchSize) {
                size_t last=std::min(first+batchSize,queued.size());
                std::vector<Request> work;
                work.reserve(last-first);
                for(size_t i=first;i<last;++i)work.push_back(std::move(queued[i]));
                std::vector<Position> keys;keys.reserve(work.size());
                for(const auto& item:work)keys.push_back(item.key);
                auto future=BackgroundWork::generation().addTaskFuture([work=std::move(work)] {
                    auto columns=std::make_shared<ColumnChunk[]>(work.size());
                    for(size_t i=0;i<work.size();++i)work[i].generate(columns[i],work[i].key);
                    return columns;
                }).share();
                for(size_t i=0;i<keys.size();++i)pending.at(keys[i])={future,i};
                ++submittedBatches;
            }
            queued.clear();
        }

        bool collect()
        {
            flush();
            bool changed = residencyChanged;residencyChanged=false;
            unsigned budget=128;
            for (auto it = pending.begin(); it != pending.end();)
            {
                if(budget==0)break;
                if (it->second.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) { ++it; continue; }
                auto chunk=it->second.column();
                if(!it->second.wanted){
                    // Obsolete work remains bounded in flight, but never wakes
                    // simulation/rendering just to be immediately unloaded.
                    cacheColumn(it->first,*chunk);
                    it=pending.erase(it);--budget;continue;
                }
                applySaved(*chunk, it->first);
                chunks.try_emplace(it->first, std::move(chunk));
                publishedKeys.push_back(it->first);
                renderChanges.insert(it->first);
                it = pending.erase(it);
                --budget;
                changed = true;
            }
            return changed;
        }

        ColumnChunk& require(const Position& key)
        {
            revive(key);
            if (auto it = chunks.find(key); it != chunks.end()) return *it->second;
            std::shared_ptr<ColumnChunk> chunk;
            if (auto it = pending.find(key); it != pending.end())
            {
                if(!it->second.future.valid())flush();
                chunk=it->second.column();
                pending.erase(it);
            }
            else
            {
                chunk = std::make_shared<ColumnChunk>();
                auto generate=resolver?resolver(key):generator;
                generate(*chunk, key);
            }
            applySaved(*chunk, key);
            publishedKeys.push_back(key);residencyChanged=true;
            renderChanges.insert(key);
            return *chunks.emplace(key, std::move(chunk)).first->second;
        }

        std::optional<uint16_t> loadedBlock(const Position& p) const
        {
            auto value=loadedRaw(p);
            return temporalRead?temporalRead(p,value):value;
        }
        std::optional<uint16_t> loadedRaw(const Position& p) const
        {
            auto a = address(p);
            auto chunk = find(a.chunk);
            if (!chunk) return {};
            return chunk->getBlockFull(a.local);
        }

        uint16_t get(const Position& p)
        {
            auto a = address(p);
            require(a.chunk);
            return *loadedBlock(p);
        }

        uint16_t simulationBlock(const Position& p)
        {
            if(auto id=loadedBlock(p))return *id;
            if(auto it=edits.find(p);it!=edits.end())return it->second;
            const auto a=address(p);
            if(auto it=cache.find(a.chunk);it!=cache.end())return it->second.blocks[a.local.a];
            ColumnChunk column;
            auto generate=resolver?resolver(a.chunk):generator;
            generate(column,a.chunk);
            auto id=column.getBlockFull(a.local);
            cacheColumn(a.chunk,column);
            return id;
        }

        void set(const Position& p, uint16_t id)
        {
            auto a = address(p);
            require(a.chunk);
            restore(p, id);
        }

        Hit trace(Vector origin, Vector direction, double distance, bool generateMissing = false)
        {
            double length2 = 0;
            for (int axis = 0; axis <N; ++axis)
            {
                if (!std::isfinite(origin[axis]) || !std::isfinite(direction[axis])) throw std::invalid_argument("Nonfinite ND ray");
                length2 += direction[axis] * direction[axis];
            }
            if (!std::isfinite(distance) || distance < 0) throw std::invalid_argument("Invalid ND ray length");
            Position cell{};
            Vector next{}, delta{};
            std::array<int, N> step{};
            const double length = std::sqrt(length2);
            if (!std::isfinite(length)) throw std::invalid_argument("Invalid ND ray direction");
            for (int axis = 0; axis <N; ++axis)
            {
                double value = std::floor(origin[axis]);
                if (value < INT32_MIN || value > INT32_MAX) throw std::out_of_range("ND ray origin");
                cell[axis] = (int32_t)value;
                if (length == 0 || direction[axis] == 0) { next[axis] = delta[axis] = INFINITY; continue; }
                direction[axis] /= length;
                step[axis] = direction[axis] > 0 ? 1 : -1;
                delta[axis] = std::abs(1.0 / direction[axis]);
                double fraction = origin[axis] - value;
                next[axis] = (step[axis] > 0 ? 1.0 - fraction : fraction) * delta[axis];
            }
            double traveled = 0;
            int side = -1;
            for (;;)
            {
                auto id = generateMissing ? std::optional<uint16_t>(get(cell)) : loadedBlock(cell);
                if (!id) return {Block::AIR, traveled, cell, side, true};
                if (ExtendedBlock::isSolid(*id) || *id==ExtendedBlock::WATER) return {*id, traveled, cell, side, false};
                auto nearest = std::min_element(next.begin(), next.end());
                if (*nearest > distance || !std::isfinite(*nearest)) return {Block::AIR, distance, cell, -1, false};
                traveled = *nearest;
                // At an edge/corner, advance all tied axes: zero-volume cells are not hit.
                side = -1;
                for (int axis = 0; axis <N; ++axis)
                {
                    if (next[axis] != traveled) continue;
                    auto p = neighbor(cell, axis * 2 + (step[axis] > 0 ? 1 : 0));
                    if (!p) return {Block::AIR, traveled, cell, -1, false};
                    cell = *p;
                    next[axis] += delta[axis];
                    if (side < 0) side = axis * 2 + (step[axis] > 0 ? 0 : 1);
                }
            }
        }

    private:
        Generator generator;
        Resolver resolver;
        struct Request { Position key;Generator generate; };
        struct Pending {
            std::shared_future<std::shared_ptr<ColumnChunk[]>> future;
            size_t index=0;
            bool wanted=true;
            std::shared_ptr<ColumnChunk> column() const {
                const auto& batch=future.get();
                return std::shared_ptr<ColumnChunk>(batch,&batch[index]);
            }
        };
        size_t submittedBatches=0;
        std::vector<Request> queued;
        std::map<Position, std::shared_ptr<ColumnChunk>> chunks;
        // Store only the payload, not an owning pointer into a generation batch.
        // LRU entries reference stable map keys instead of copying N coordinates.
        using CacheOrder=std::list<const Position*>;
        struct Cached {
            std::array<uint16_t,32> blocks;
            uint32_t extendedMask;
            typename CacheOrder::iterator order;
        };
        std::map<Position,Cached> cache;
        CacheOrder cacheOrder;
        size_t cacheLimit=0;
        bool residencyChanged=false;
        std::vector<Position> publishedKeys;
        std::set<Position> renderChanges;
        void trimCache() {
            while(cache.size()>cacheLimit) {
                auto it=cache.find(*cacheOrder.front());
                cacheOrder.pop_front();cache.erase(it);
            }
        }
        void cacheColumn(const Position& key,const ColumnChunk& column) {
            if(!cacheLimit)return;
            auto [it,inserted]=cache.try_emplace(key);
            if(inserted){
                cacheOrder.push_back(&it->first);it->second.order=std::prev(cacheOrder.end());
            }else cacheOrder.splice(cacheOrder.end(),cacheOrder,it->second.order);
            it->second.blocks=column.blocks;it->second.extendedMask=column.extendedMask;
            trimCache();
        }
        bool revive(const Position& key) {
            auto it=cache.find(key);if(it==cache.end())return false;
            auto chunk=std::make_shared<ColumnChunk>();
            chunk->blocks=it->second.blocks;chunk->extendedMask=it->second.extendedMask;
            applySaved(*chunk,key);
            chunks.emplace(key,std::move(chunk));
            publishedKeys.push_back(key);
            renderChanges.insert(key);
            cacheOrder.erase(it->second.order);cache.erase(it);residencyChanged=true;
            return true;
        }
        std::map<Position, Pending> pending;
        std::map<Position, uint16_t> edits;
        std::map<Position, std::map<Position, uint16_t>> saved;
        void applySaved(ColumnChunk& chunk, const Position& key)
        {
            if (auto it = saved.find(key); it != saved.end())
                for (const auto& [p, id] : it->second) chunk.setBlock(address(p).local, id);
        }
    };
