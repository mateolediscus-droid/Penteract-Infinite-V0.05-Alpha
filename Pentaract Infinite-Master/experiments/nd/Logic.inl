
    struct BlockState
    {
        uint16_t id = Block::AIR;
        FaceMask faces{};
        bool operator==(const BlockState&) const = default;
    };

    static inline void validate(BlockState state)
    {
        if (state.id >= Block::COUNT && !ExtendedBlock::isExtended(state.id))
            throw std::invalid_argument("Unknown ND block ID");
        if (!state.faces.valid() || (state.faces && !ExtendedBlock::isRedirector(state.id)))
            throw std::invalid_argument("Invalid ND face metadata");
    }

    class Logic
    {
    public:
        explicit Logic(Space& space, std::function<FaceMask(const Position&)> generatedFaces = {})
            : space(space), generatedFaces(std::move(generatedFaces)) {}
        std::function<double(const Position&)> temporalDelay;
        std::function<void(const Position&,double)> temporalOverdue;
        template<class Visit> void remaining(Visit&& visit)const {events.remaining(visit);}
        void resetSchedule(){events.clear();prepared=false;}
        void reschedule(const Position& p){events.erase(p);prepared=false;}
        template<class Archive> void archive(Archive& a) {
            a(circuits,known,faces,outputs,elapsed,idleTicks,running);
            a(repeaters);
        }
        void swapSimulation(Logic& other) {
            using std::swap;
            swap(circuits,other.circuits);swap(known,other.known);swap(faces,other.faces);
            swap(repeaters,other.repeaters);swap(outputs,other.outputs);
            swap(elapsed,other.elapsed);swap(idleTicks,other.idleTicks);swap(running,other.running);
            swap(events,other.events);swap(prepared,other.prepared);
        }
        const auto& faceEdits() const { return faces; }
        void restoreFaces(const Position& p, FaceMask mask) { faces[p] = mask; }
        // Circuit records survive terrain eviction; no surrounding chunks are required.
        void chunkUnloaded(const Position&) {}

        void retainChunks(const std::set<Position>&) {}

        void restoreCircuit(const Position& p, uint16_t id) { track(p,id); }
        void resumeCircuits() { wake(); }

        std::optional<uint16_t> block(const Position& p) const
        {
            if (auto id=space.loadedBlock(p)) return id;
            auto it=known.find(p);
            return it==known.end()?std::nullopt:std::optional<uint16_t>{it->second};
        }

        BlockState state(const Position& p) const
        {
            auto id = block(p);
            if (!id) throw std::runtime_error("ND block is not loaded");
            auto it = faces.find(p);
            auto mask=it!=faces.end()?it->second:
                (ExtendedBlock::isRedirector(*id) && generatedFaces?generatedFaces(p):FaceMask{});
            mask|=ExtendedBlock::redirectorMask(*id);
            return {*id,mask};
        }

        void set(const Position& p, BlockState value)
        {
            validate(value);
            if (ExtendedBlock::isRedirector(value.id))
            {
                value.faces |= ExtendedBlock::redirectorMask(value.id);
                value.id = ExtendedBlock::REDIRECTOR_0;
            }
            auto old = block(p);
            if (old && state(p) == value) return;
            if(old && !space.find(address(p).chunk)) space.restore(p,value.id);
            else space.set(p,value.id);
            if (ExtendedBlock::isRedirector(value.id)) faces[p] = value.faces; else faces.erase(p);
            if (!old || !ExtendedBlock::isRepeater(*old) || !ExtendedBlock::isRepeater(value.id)) repeaters.erase(p);
            outputs.erase(p);
            events.erase(p);
            track(p, value.id);
            if ((old && circuit(*old)) || circuit(value.id)) wake();
        }

        // Call on the owner thread after publishing a generated/loaded chunk.
        // Terrain does not contain circuits; only extended records need inspection.
        void chunkLoaded(const Position& key)
        {
            auto chunk = space.find(key);
            if (!chunk) return;
            std::vector<Position> candidates;
            for(const auto& [packed,id]:chunk->extendedBlocks())
                if(circuit(id))candidates.push_back(position(key,{packed&127,(packed>>7)&7,(packed>>10)&7,(packed>>13)&7,(packed>>16)&7}));
            loadedCandidates(candidates);
        }
        void loadedCandidates(const std::vector<Position>& candidates)
        {
            bool discovered=false;
            for(const auto& p:candidates) {
                auto id=space.loadedBlock(p);if(!id)continue;
                const bool fresh=circuit(*id) && !circuits.contains(p);
                discovered|=fresh;track(p,*id);
                if(fresh)space.restore(p,*id);
                if(ExtendedBlock::isRedirector(*id) && !faces.contains(p))
                    faces[p]=(generatedFaces?generatedFaces(p):FaceMask{}),faces[p]|=ExtendedBlock::redirectorMask(*id);
            }
            if(discovered)wake();
        }

        bool interact(const Position& p, int side)
        {
            auto value = state(p);
            if (ExtendedBlock::isLever(value.id)) value.id = ExtendedBlock::toggledLever(value.id);
            else if (ExtendedBlock::isRepeater(value.id)) value.id = ExtendedBlock::nextRepeaterMode(value.id);
            else if (ExtendedBlock::isRedirector(value.id) && side >= 0 && side < (2*N)) value.faces.toggle(side);
            else return false;
            set(p, value);
            return true;
        }

        uint8_t output(const Position& p, int side) const
        {
            if (side < 0 || side >= (2*N)) return 0;
            auto id = block(p);
            if (!id) return 0;
            if (*id == ExtendedBlock::LEVER_ON) return 15;
            if (ExtendedBlock::isCable(*id)) return ExtendedBlock::cablePower(*id);
            auto power = outputs.find(p);
            if (power == outputs.end()) return 0;
            if (ExtendedBlock::isRepeater(*id))
            {
                auto r = repeaters.find(p);
                return r != repeaters.end() && r->second.input != side ? power->second : 0;
            }
            if (ExtendedBlock::isRedirector(*id) && state(p).faces.test(side)) return power->second;
            return 0;
        }

        bool active() const { return running; }

        bool advance(double seconds)
        {
            if (!std::isfinite(seconds) || seconds < 0) throw std::invalid_argument("Invalid circuit elapsed time");
            if (!running) { elapsed = 0; return false; }
            if(temporalDelay) {
                if(!prepared) {for(const auto& p:circuits)events.schedule(p,temporalDelay);prepared=true;}
                auto ready=events.advance(seconds,temporalOverdue);
                bool changed=false;
                if(!ready.empty())changed=tick(&ready);
                else if(++idleTicks>=600)running=false;
                for(const auto& p:ready)if(circuits.contains(p))events.schedule(p,temporalDelay);
                if(!running)resetSchedule();
                return changed;
            }
            elapsed += seconds;
            bool changed = false;
            const double interval=.25;
            while (elapsed + 1e-9 >= interval && running)
            {
                elapsed = std::max(0.,elapsed-interval);
                changed |= tick();
            }
            return changed;
        }

    private:
        struct Repeater { int input = -1; unsigned cooldown = 0; template<class Archive> void archive(Archive& a) {a(input,cooldown);} };
        Space& space;
        std::function<FaceMask(const Position&)> generatedFaces;
        std::set<Position> circuits;
        std::map<Position,uint16_t> known;
        std::map<Position, FaceMask> faces;
        std::map<Position, Repeater> repeaters;
        std::map<Position, uint8_t> outputs;
        double elapsed = 0;
        unsigned idleTicks = 0;
        bool running = false;
        bool prepared=false;
        TemporalEvents<Position> events;

        static bool circuit(uint16_t id)
        {
            return ExtendedBlock::isCable(id) || ExtendedBlock::isLever(id) ||
                ExtendedBlock::isRepeater(id) || ExtendedBlock::isRedirector(id) || ExtendedBlock::isCrystalDoor(id);
        }

        void track(const Position& p, uint16_t id)
        {
            if (circuit(id)) {circuits.insert(p);known[p]=id;}
            else {circuits.erase(p);known.erase(p);events.erase(p);}
        }

        bool adjacent() const
        {
            for (const auto& p : circuits)
            for (int side = 0; side < (2*N); ++side)
                if (auto n = neighbor(p, side); n && circuits.contains(*n)) return true;
            return false;
        }

        void wake()
        {
            prepared=false;
            running = adjacent();
            idleTicks = 0;
            if (!running) elapsed = 0;
        }

        uint8_t input(const Position& p, int side) const
        {
            auto n = neighbor(p, side);
            return n ? output(*n, side ^ 1) : 0;
        }

        bool powered(const Position& p) const
        {
            for (int side = 0; side < (2*N); ++side) if (input(p, side)) return true;
            return false;
        }

        bool tick(const std::vector<Position>* ready=nullptr)
        {
            if (!ready && !adjacent()) { running = false; return false; }
            // All inputs read the previous tick, independently of map iteration order.
            auto nextRepeaters = repeaters;
            auto nextOutputs=ready?outputs:std::map<Position,uint8_t>{};
            std::map<Position, uint16_t> changes;
            bool cooling = false;
            const auto all=ready?std::vector<Position>{}:std::vector<Position>(circuits.begin(),circuits.end());
            for (const auto& p : ready?*ready:all)
            {
                auto id = block(p);
                if (!id) continue;
                nextOutputs.erase(p);
                uint8_t power = 0;
                if (ExtendedBlock::isRepeater(*id))
                {
                    auto& r = nextRepeaters[p];
                    if (r.cooldown) --r.cooldown;
                    else if (r.input >= 0)
                    {
                        if (input(p, r.input)) power = ExtendedBlock::repeaterMode(*id);
                        else { r.input = -1; r.cooldown = 20; }
                    }
                    else
                    {
                        uint8_t strongest = 0;
                        for (int side = 0; side < (2*N); ++side)
                            if (auto candidate = input(p, side); candidate > strongest)
                            { strongest = candidate; r.input = side; }
                        if (strongest) power = ExtendedBlock::repeaterMode(*id);
                    }
                    cooling |= r.cooldown != 0;
                    if (power) nextOutputs[p] = power;
                }
                else if (ExtendedBlock::isRedirector(*id))
                {
                    auto mask = state(p).faces;
                    for (int side = 0; side < (2*N); ++side)
                        if (!mask.test(side)) power = std::max(power, input(p, side));
                    if (power) nextOutputs[p] = power;
                }
                else if (ExtendedBlock::isCable(*id))
                {
                    for (int side = 0; side < (2*N); ++side)
                    {
                        auto n = neighbor(p, side);
                        auto incoming = input(p, side);
                        if (incoming && n)
                        {
                            auto source = block(*n);
                            if (source && ExtendedBlock::isCable(*source)) --incoming;
                            power = std::max(power, incoming);
                        }
                    }
                    if (ExtendedBlock::cablePower(*id) != power) changes[p] = ExtendedBlock::cableId(power);
                }
                else if (ExtendedBlock::isCrystalDoor(*id))
                {
                    bool open = powered(p);
                    // Match the existing vertical two-block door behavior.
                    for (int side : {0, 1})
                        if (auto n = neighbor(p, side); n)
                            if (auto other = block(*n); other && ExtendedBlock::isCrystalDoor(*other)) open |= powered(*n);
                    uint16_t next = open ? ExtendedBlock::CRYSTAL_DOOR_OPEN : ExtendedBlock::CRYSTAL_DOOR;
                    if (next != *id) changes[p] = next;
                }
            }
            bool changed = !changes.empty() || outputs != nextOutputs;
            repeaters = std::move(nextRepeaters);
            outputs = std::move(nextOutputs);
            for (const auto& [p, id] : changes) {space.restore(p, id);known[p]=id;}
            idleTicks = changed || cooling ? 0 : idleTicks + 1;
            if (idleTicks >= (temporalDelay?600u:120u)) running = false;
            return changed;
        }
    };
