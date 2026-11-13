    struct structures {

    struct Block { Position position; BlockState state; };
    struct Template
    {
        std::string name;
        Position size=[] {Position p{};p.fill(1);return p;}();
        std::vector<Block> blocks;
    };

    static inline std::filesystem::path library(const std::filesystem::path& root)
    {
        return root / (std::to_string(N)+"D");
    }

    static inline Position translated(Position a, const Position& b)
    {
        for (int axis = 0; axis <N; ++axis)
        {
            int64_t n = int64_t(a[axis]) + b[axis];
            if (n < INT32_MIN || n > INT32_MAX) throw std::out_of_range("ND structure coordinate overflow");
            a[axis] = int32_t(n);
        }
        return a;
    }

    static inline void validate(const Template& t)
    {
        if (t.name.size() > UINT16_MAX) throw std::invalid_argument("Structure name too long");
        for (int size : t.size) if (size <= 0) throw std::invalid_argument("Invalid structure extent");
        std::set<Position> occupied;
        for (const auto& block : t.blocks)
        {
            WorldND::validate(block.state);
            for (int axis = 0; axis <N; ++axis)
                if (block.position[axis] < 0 || block.position[axis] >= t.size[axis])
                    throw std::invalid_argument("Block outside ND structure bounds");
            if (!occupied.insert(block.position).second) throw std::invalid_argument("Duplicate structure block");
        }
    }

    struct binary
    {
        // Explicit little endian records, never a raw C++ struct (padding/ABI).
        static inline void put(std::ostream& out, uint64_t n, int bytes)
        {
            for (int i = 0; i < bytes; ++i) out.put(char(n >> (i * 8)));
            if (!out) throw std::runtime_error("Cannot write ND structure");
        }
        static inline uint64_t get(std::istream& in, int bytes)
        {
            uint64_t n = 0;
            for (int i = 0; i < bytes; ++i)
            {
                int c = in.get();
                if (c == EOF) throw std::runtime_error("Truncated ND structure");
                n |= uint64_t(uint8_t(c)) << (i * 8);
            }
            return n;
        }
    };

    static inline void write(std::ostream& out, const Template& t)
    {
        validate(t);
        out.write(structureMagic().data(), 8);
        binary::put(out, saveVersion, 2);
        binary::put(out, N, N<=15?1:4);
        binary::put(out, 0, 1);
        for (int size : t.size) binary::put(out, uint32_t(size), 4);
        binary::put(out, t.blocks.size(), 8);
        binary::put(out, t.name.size(), 2);
        out.write(t.name.data(), t.name.size());
        for (const auto& block : t.blocks)
        {
            for (int coordinate : block.position) binary::put(out, uint32_t(coordinate), 4);
            binary::put(out, block.state.id, 2);
            for(auto word:block.state.faces.words)binary::put(out,word,4);
        }
        if (!out) throw std::runtime_error("Cannot write ND structure");
    }

    static inline Template read(std::istream& in)
    {
        char magic[8]{};
        in.read(magic, 8);
        if (std::string(magic, 8) != structureMagic() || binary::get(in, 2) != saveVersion ||
            binary::get(in, N<=15?1:4) != N || binary::get(in, 1) != 0)
            throw std::runtime_error("Not a supported ND structure");
        Template t;
        for (int& size : t.size)
        {
            auto n = binary::get(in, 4);
            if (n == 0 || n > INT32_MAX) throw std::runtime_error("Invalid ND extent");
            size = int(n);
        }
        uint64_t count = binary::get(in, 8);
        auto length = binary::get(in, 2);
        t.name.resize(size_t(length));
        in.read(t.name.data(), std::streamsize(length));
        if (!in) throw std::runtime_error("Truncated ND structure name");
        // Read incrementally: a corrupt count cannot trigger a giant allocation.
        for (uint64_t i = 0; i < count; ++i)
        {
            Block block{};
            for (int& coordinate : block.position)
            {
                auto n = binary::get(in, 4);
                if (n > INT32_MAX) throw std::runtime_error("Invalid relative ND coordinate");
                coordinate = int(n);
            }
            block.state.id = uint16_t(binary::get(in, 2));
            for(auto& word:block.state.faces.words)word=uint32_t(binary::get(in,4));
            t.blocks.push_back(block);
        }
        if (in.peek() != EOF) throw std::runtime_error("Trailing ND structure data");
        validate(t);
        return t;
    }

    static inline Template capture(std::string name, const Position& min, const Position& size, const Logic& logic)
    {
        Template t{std::move(name), size, {}};
        validate(t);
        Position maxOffset = size;
        for (auto& v : maxOffset) --v;
        translated(min, maxOffset);
        Position cursor{};
        do
        {
            // Missing chunks are an error, never silently captured as air.
            auto state = logic.state(translated(min, cursor));
            if (state.id != ::Block::AIR) t.blocks.push_back({cursor, state});
            int axis = 0;
            for (; axis <N; ++axis)
            {
                if (++cursor[axis] < size[axis]) break;
                cursor[axis] = 0;
            }
            if (axis ==N) break;
        } while (true);
        return t;
    }

    // The caller owns this index for a placement batch/world. Reserve occupied
    // voxels, not the bounding box; different U slices never alias each other.
    class PlacementIndex
    {
    public:
        bool reserve(const Template& t, const Position& origin)
        {
            validate(t);
            std::vector<Position> positions;
            for (const auto& block : t.blocks)
            {
                if (block.state.id == ::Block::AIR) continue;
                auto p = translated(origin, block.position);
                if (occupied.contains(p)) return false;
                positions.push_back(p);
            }
            occupied.insert(positions.begin(), positions.end());
            return true;
        }
        size_t size() const { return occupied.size(); }
    private:
        std::set<Position> occupied;
    };

    static inline void place(const Template& t, const Position& origin, Logic& logic)
    {
        validate(t);
        // Validate all coordinates before changing the destination world.
        for (const auto& block : t.blocks) translated(origin, block.position);
        for (const auto& block : t.blocks) logic.set(translated(origin, block.position), block.state);
    }
    };
