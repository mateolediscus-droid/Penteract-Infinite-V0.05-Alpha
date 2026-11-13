
    struct StructureRule
    {
        typename structures::Template shape;
        std::string file;
        double frequency = .25;
        int spacing = 32, minHeight = 0, maxHeight = 127;
        std::vector<std::string> biomes{"default", "test"};
    };

    class Generation
    {
    public:
        Terrain terrain;
        uint64_t seed;
        std::vector<StructureRule> rules;
        Generation(uint64_t seed, bool caves, bool flat) : terrain(seed,caves,flat), seed(seed) {}

        void load(const std::filesystem::path& root)
        {
            auto path = root / "generation.json";
            rules.clear();
            if (!std::filesystem::exists(path)) return;
            nlohmann::json config; std::ifstream(path) >> config;
            for (const auto& entry : config.at("structures"))
            {
                if (!entry.value("enabled", true)) continue;
                StructureRule r;
                r.file = entry.at("file").get<std::string>();
                if (std::filesystem::path(r.file).filename() != r.file) throw std::runtime_error("Invalid ND structure filename");
                std::ifstream in(root/r.file, std::ios::binary);
                r.shape = structures::read(in);
                r.frequency = entry.value("frequency", .25);
                r.spacing = entry.value("spacing", 32);
                auto range = entry.value("height", std::array<int,2>{0,127});
                r.minHeight=range[0]; r.maxHeight=range[1];
                r.biomes=entry.value("biome",std::vector<std::string>{"default","test"});
                if (!std::isfinite(r.frequency) || r.frequency<0 || r.frequency>1 || r.spacing<1 || r.minHeight>r.maxHeight)
                    throw std::runtime_error("Invalid ND generation rule: " + r.file);
                if (!r.shape.blocks.empty()) rules.push_back(std::move(r));
            }
        }

        void operator()(ColumnChunk& chunk, const Position& key) const
        {
            terrain(chunk,key);
            auto low=position(key,{0,0,0,0,0}), high=low;
            for (int i=0;i<N;++i) high[i]+=extent[i]-1;
            for (const auto& c : candidates(low,high))
            {
                if (!accepted(c)) continue;
                const auto& shape=rules[c.rule].shape;
                int bottom=INT_MAX;
                for (const auto& b:shape.blocks) if (b.state.id) bottom=std::min(bottom,b.position[0]);
                for (const auto& b:shape.blocks)
                {
                    auto p=structures::translated(c.origin,b.position);
                    if (address(p).chunk==key) chunk.setBlock(address(p).local,b.state.id);
                    // Only the global lowest occupied layer supports the structure.
                    if (b.state.id && b.position[0]==bottom)
                    {
                        int h=terrain.height(p);
                        for (int a=std::max(h+1,low[0]);a<std::min(p[0],high[0]+1);++a)
                        {
                            auto foundation=p; foundation[0]=a;
                            if (address(foundation).chunk==key)
                                chunk.setBlock(address(foundation).local,terrain.sandy(p)?ExtendedBlock::SAND:Block::DIRT);
                        }
                    }
                }
            }
        }

        FaceMask faces(const Position& p) const
        {
            for (const auto& c:candidates(p,p)) if (accepted(c))
                for (const auto& b:rules[c.rule].shape.blocks)
                    if (structures::translated(c.origin,b.position)==p) return b.state.faces;
            return 0;
        }

    private:
        struct Candidate
        {
            size_t rule;
            Position cell, origin;
            uint64_t priority;
        };
        static uint64_t mix(uint64_t n)
        {
            n+=0x9E3779B97F4A7C15ull; n=(n^(n>>30))*0xBF58479D1CE4E5B9ull;
            n=(n^(n>>27))*0x94D049BB133111EBull; return n^(n>>31);
        }
        static int floorDiv(int64_t n,int d) { return int(n/d-(n%d<0)); }
        std::optional<Candidate> candidate(size_t index, Position cell) const
        {
            const auto& r=rules[index];
            uint64_t h=mix(seed);
            for (unsigned char ch:r.file) h=mix(h^ch);
            for (int axis=1;axis<N;++axis) h=mix(h^uint32_t(cell[axis]));
            if ((h>>11)*(1.0/9007199254740992.0)>=r.frequency) return {};
            Candidate c{index,cell,{},h};
            for (int axis=1;axis<N;++axis)
            {
                h=mix(h); int64_t n=int64_t(cell[axis])*r.spacing+h%r.spacing;
                if (n<INT32_MIN || n>int64_t(INT32_MAX)-r.shape.size[axis]) return {};
                c.origin[axis]=int(n);
            }
            int ground=terrain.height(c.origin), bottom=INT_MAX;
            auto biome=terrain.sandy(c.origin)?"test":"default";
            if (std::find(r.biomes.begin(),r.biomes.end(),biome)==r.biomes.end()) return {};
            for (const auto& b:r.shape.blocks) if (b.state.id) bottom=std::min(bottom,b.position[0]);
            if (bottom==INT_MAX) return {};
            for (const auto& b:r.shape.blocks) if (b.state.id && b.position[0]==bottom)
                ground=std::max(ground,terrain.height(structures::translated(c.origin,b.position)));
            if (ground<r.minHeight || ground>r.maxHeight) return {};
            c.origin[0]=ground+1-bottom;
            return c;
        }
        std::vector<Candidate> candidates(const Position& low, const Position& high) const
        {
            std::vector<Candidate> out;
            for (size_t i=0;i<rules.size();++i)
            {
                const auto& r=rules[i];
                if (high[0]<0 || low[0]>128+int64_t(r.shape.size[0])) continue;
                Position first{},last{},cell{};
                for (int axis=1;axis<N;++axis)
                {
                    first[axis]=floorDiv(int64_t(low[axis])-r.shape.size[axis]+1,r.spacing);
                    last[axis]=floorDiv(high[axis],r.spacing);
                }
                cell=first;
                for (;;)
                {
                    if (auto c=candidate(i,cell)) out.push_back(*c);
                    int axis=1;
                    for (;axis<N;++axis) { if (++cell[axis]<=last[axis]) break; cell[axis]=first[axis]; }
                    if (axis==N) break;
                }
            }
            return out;
        }
        bool accepted(const Candidate& c) const
        {
            const auto& shape=rules[c.rule].shape;
            auto high=c.origin;
            for (int i=0;i<N;++i) high[i]+=shape.size[i]-1;
            std::set<Position> occupied;
            for (const auto& b:shape.blocks) if (b.state.id) occupied.insert(structures::translated(c.origin,b.position));
            auto rank=[](const Candidate& x) {return std::tuple{x.priority,x.rule,x.cell};};
            for (const auto& other:candidates(c.origin,high))
            {
                if (rank(other)>=rank(c)) continue;
                for (const auto& b:rules[other.rule].shape.blocks)
                    if (b.state.id && occupied.contains(structures::translated(other.origin,b.position))) return false;
            }
            return true;
        }
    };
