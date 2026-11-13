
    class Terrain
    {
        struct NoiseSet {
            uint64_t seed;
            NoiseND<(N-1)> surface, biome;
            NoiseND<N> caves;
            typename NoiseND<(N-1)>::Projection surfaceProjection,biomeProjection;
            typename NoiseND<N>::Projection caveProjection;
            explicit NoiseSet(uint64_t s) : seed(s), surface(s),
                biome(s ^ 0x9E3779B97F4A7C15ull), caves(s ^ 0xD1B54A32D192ED03ull) {}
        };
        uint64_t seed;
        bool caves, flat;
        static constexpr double caveScale = .065;
        NoiseSet& noises() const {
            thread_local NoiseSet fields(seed);
            if (fields.seed != seed) fields = NoiseSet(seed);
            return fields;
        }
        static typename NoiseND<(N-1)>::Point horizontal(const Position& p, double scale) {
            typename NoiseND<(N-1)>::Point result{};
            for (size_t i = 0; i < result.size(); ++i) result[i] = double(p[i + 1]) * scale;
            return result;
        }
        static double caveThreshold(double a) { return .22 + .25 * std::clamp((a - 10) / 10, 0., 1.); }
    public:
        explicit Terrain(uint64_t seed, bool caves = true, bool flat = false) : seed(seed), caves(caves), flat(flat) {}
        int height(const Position& p) const {
            if(flat)return 8;
            auto& fields=noises();
            return std::clamp(int(std::lround(fields.surface.evaluate(horizontal(p,.055),fields.surfaceProjection)*18+30)),4,127);
        }
        bool sandy(const Position& p) const {
            if(flat)return false;
            auto& fields=noises();return fields.biome.evaluate(horizontal(p,.006),fields.biomeProjection)>.2;
        }
        double caveDensity(const Vector& p) const {
            if (!caves || flat || p[0] > 20) return -1;
            typename NoiseND<N>::Point point{};
            for (size_t i = 0; i < point.size(); ++i) point[i] = p[i] * caveScale;
            auto& fields=noises();return fields.caves.evaluate(point,fields.caveProjection)-caveThreshold(p[0]);
        }
        void operator()(ColumnChunk& chunk, const Position& key) const {
            const Position corner = position(key, {0,0,0,0,0});
            if (corner[0] > 127) return;
            const int top = corner[0] < 0 ? -1 : height(corner);
            if (corner[0] > top) return;
            const bool sand = corner[0] >= 0 && sandy(corner);
            const bool carve = caves && !flat && corner[0] <= 20;
            std::array<double, 32> density{};
            if (carve) {
                typename NoiseND<N>::Point origin{}, step{};
                for (size_t i = 0; i < origin.size(); ++i) origin[i] = double(corner[i]) * caveScale;
                step[0] = caveScale;
                auto& fields=noises();density=fields.caves.template line<32>(origin,step,fields.caveProjection);
            }
            for (unsigned a = 0; a < 32 && int64_t(corner[0]) + a <= top; ++a) {
                const int64_t worldA = int64_t(corner[0]) + a;
                if (carve && worldA <= 20 && density[a] > caveThreshold(double(worldA))) continue;
                uint16_t id = worldA < 0 ? Block::STONE :
                    sand && worldA > top - 4 ? ExtendedBlock::SAND :
                    worldA == top ? Block::GRASS : worldA > top - 4 ? Block::DIRT : Block::STONE;
                chunk.setBlock({a,0u,0u,0u,0u}, id);
            }
        }
    };
