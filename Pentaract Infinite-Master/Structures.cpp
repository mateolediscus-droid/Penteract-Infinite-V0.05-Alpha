#include "Structures.h"
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include "tuple_hash.h"

namespace Structures
{
    namespace
    {
        std::array<int, 5> array(m5::ivec5 p) { return {p.a, p.b, p.c, p.d, p.e}; }
        m5::ivec5 position(const nlohmann::json& j)
        {
            auto p = j.get<std::array<int, 5>>();
            return {p[0], p[1], p[2], p[3], p[4]};
        }
        uint64_t mix(uint64_t x)
        {
            x += 0x9e3779b97f4a7c15ull;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
            return x ^ (x >> 31);
        }
        uint64_t nameHash(const std::string& name)
        {
            uint64_t h = 14695981039346656037ull;
            for (unsigned char c : name) h = (h ^ c) * 1099511628211ull;
            return h;
        }
        void validateRule(const Rule& rule)
        {
            if (!std::isfinite(rule.frequency) || rule.frequency < 0 || rule.frequency > 1 ||
                rule.spacing < 1 || rule.spacing > 1048576 || rule.minHeight > rule.maxHeight ||
                rule.foundationDepth < 0 || rule.foundationDepth > 8)
                throw std::runtime_error("Invalid structure frequency, spacing or height range: " + rule.file);
            for (const auto& biome : rule.biome)
                if (biome != "default" && biome != "test") throw std::runtime_error("Unknown biome: " + biome);
        }
        bool simpleName(const std::string& name)
        {
            return !name.empty() && name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-") == std::string::npos;
        }
    }

    int floorDiv(int value, int divisor)
    {
        int q = value / divisor;
        return q - (value % divisor < 0 ? 1 : 0);
    }

    std::filesystem::path libraryDirectory(int dimensions)
    {
        if (dimensions < 2 || dimensions > 5) throw std::runtime_error("Invalid structure dimension");
        return std::filesystem::path("Structure") / (std::to_string(dimensions) + "D");
    }

    void requireDimensions(const Template& structure, int dimensions)
    {
        if (dimensions < 2 || dimensions > 5 || structure.dimensions != dimensions)
            throw std::runtime_error("Structure dimension does not match world dimension");
        for (int axis = dimensions; axis < 5; ++axis)
        {
            if (structure.size[axis] != 1) throw std::runtime_error("Structure extends along an inactive axis");
            for (const auto& block : structure.blocks)
                if (block.pos[axis] != 0) throw std::runtime_error("Block on an inactive structure axis");
        }
    }

    Template capture(const std::string& name, m5::ivec5 min, m5::ivec5 size,
        const std::vector<CapturedChunk>& chunks, int dimensions)
    {
        Template result{name, size, {}, dimensions};
        requireDimensions(result, dimensions);
        const m5::ivec5 extent{Chunk::HEIGHT, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE, Chunk::SIZE};
        for (const auto& captured : chunks)
        {
            auto corner = captured.pos * extent;
            m5::ivec5 low, high;
            for (int i = 0; i < 5; ++i)
            {
                low[i] = std::max(0, min[i] - corner[i]);
                high[i] = std::min(extent[i], min[i] + size[i] - corner[i]);
            }
            for (int a = low.a; a < high.a; ++a)
            for (int b = low.b; b < high.b; ++b)
            for (int c = low.c; c < high.c; ++c)
            for (int d = low.d; d < high.d; ++d)
            for (int e = low.e; e < high.e; ++e)
            {
                m5::ivec5 local{a,b,c,d,e};
                auto id = captured.chunk->getBlockFull(m5::uvec5(local));
                if (id != ::Block::AIR && id != ExtendedBlock::STRUCTURE)
                    result.blocks.push_back({corner + local - min, id});
            }
        }
        return result;
    }

    nlohmann::json toJson(const Template& structure)
    {
        nlohmann::json blocks = nlohmann::json::array();
        for (const auto& block : structure.blocks)
            blocks.push_back({{"pos", array(block.pos)}, {"id", block.id}});
        return {{"version", 1}, {"dimensions", structure.dimensions}, {"name", structure.name}, {"size", array(structure.size)}, {"blocks", blocks}};
    }

    Template fromJson(const nlohmann::json& j)
    {
        if (j.at("version").get<int>() != 1) throw std::runtime_error("Unsupported structure version");
        Template result;
        result.dimensions = j.value("dimensions", 5);
        result.name = j.at("name").get<std::string>();
        result.size = position(j.at("size"));
        for (int i = 0; i < 5; ++i)
            if (result.size[i] < 1 || result.size[i] > 1048576) throw std::runtime_error("Invalid structure size");
        for (const auto& b : j.at("blocks"))
        {
            auto pos = position(b.at("pos"));
            int id = b.at("id").get<int>();
            for (int i = 0; i < 5; ++i)
                if (pos[i] < 0 || pos[i] >= result.size[i]) throw std::runtime_error("Block outside structure bounds");
            if (id == ::Block::AIR || id == ExtendedBlock::STRUCTURE) continue;
            if (id < 0 || id > 65535 || !ExtendedBlock::isRenderable((uint16_t)id))
                throw std::runtime_error("Unknown structure block ID");
            result.blocks.push_back({pos, (uint16_t)id});
        }
        requireDimensions(result, result.dimensions);
        return result;
    }

    namespace
    {
        void put(std::ostream& out, uint64_t value, int bytes)
        {
            for (int i=0;i<bytes;++i) out.put((char)(value >> (i*8)));
        }
        uint64_t get(std::istream& in, int bytes)
        {
            uint64_t value=0;
            for (int i=0;i<bytes;++i) { int c=in.get(); if(c==EOF) throw std::runtime_error("Truncated structure binary"); value |= (uint64_t)(uint8_t)c << (i*8); }
            return value;
        }
        void validateBlock(const Template& s, const Block& b)
        {
            for(int i=0;i<5;++i) if(b.pos[i]<0 || b.pos[i]>=s.size[i]) throw std::runtime_error("Block outside structure bounds");
            if(b.id==::Block::AIR || b.id==ExtendedBlock::STRUCTURE || !ExtendedBlock::isRenderable(b.id)) throw std::runtime_error("Invalid structure block ID");
        }
    }

    void writeBinary(const std::filesystem::path& path, const Template& s)
    {
        requireDimensions(s, s.dimensions);
        int width=1;
        for(int i=0;i<5;++i)
        {
            if(s.size[i]<1 || s.size[i]>1048576) throw std::runtime_error("Invalid structure size");
            if(s.size[i]>256) width=std::max(width,2);
            if(s.size[i]>65536) width=4;
        }
        if(s.name.size()>65535) throw std::runtime_error("Structure name too long");
        for(const auto& b:s.blocks) validateBlock(s,b);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path,std::ios::binary);
        out.write("PSTR5D\r\n",8); put(out,2,2); put(out,width,1); put(out,s.dimensions,1);
        for(int i=0;i<5;++i) put(out,s.size[i],4);
        put(out,s.blocks.size(),8); put(out,s.name.size(),2); out.write(s.name.data(),s.name.size());
        for(const auto& b:s.blocks) { for(int i=0;i<5;++i) put(out,b.pos[i],width); put(out,b.id,2); }
        out.flush();
        if(!out) throw std::runtime_error("Cannot write " + path.string());
    }

    Template readBinary(const std::filesystem::path& path)
    {
        std::ifstream in(path,std::ios::binary);
        char magic[8]{}; in.read(magic,8);
        if(!in || std::string_view(magic,8)!="PSTR5D\r\n") throw std::runtime_error("Invalid structure binary: " + path.string());
        if(get(in,2)!=2) throw std::runtime_error("Unsupported structure binary version");
        int width=(int)get(in,1);
        int dimensions=(int)get(in,1);
        if((width!=1 && width!=2 && width!=4) || dimensions<2 || dimensions>5) throw std::runtime_error("Invalid structure encoding");
        Template s;
        s.dimensions=dimensions;
        for(int i=0;i<5;++i) { auto n=get(in,4); if(n<1 || n>1048576) throw std::runtime_error("Invalid structure size"); s.size[i]=(int)n; }
        auto count=get(in,8), nameLength=get(in,2);
        auto length=std::filesystem::file_size(path);
        const uint64_t header=42+nameLength, record=5*width+2;
        if(length<header || (length-header)%record || count!=(length-header)/record || count>s.blocks.max_size()) throw std::runtime_error("Invalid structure binary length");
        s.name.resize((size_t)nameLength); in.read(s.name.data(),nameLength);
        s.blocks.reserve((size_t)count);
        for(uint64_t n=0;n<count;++n)
        {
            Block b;
            for(int i=0;i<5;++i) { auto value=get(in,width); if(value>INT_MAX) throw std::runtime_error("Invalid coordinate"); b.pos[i]=(int)value; }
            b.id=(uint16_t)get(in,2); validateBlock(s,b); s.blocks.push_back(b);
        }
        requireDimensions(s, dimensions);
        return s;
    }

    void writeHistory(const std::filesystem::path& path, nlohmann::json history)
    {
        size_t version=0;
        for(auto& catalog:history.at("catalogs"))
        {
            size_t rule=0;
            for(auto& entry:catalog.at("structures"))
            {
                auto file="structure-catalogs/v"+std::to_string(version)+"-r"+std::to_string(rule++)+".bin";
                writeBinary(path.parent_path()/file,fromJson(entry.at("template")));
                entry.erase("template"); entry["templateBinary"]=file;
            }
            ++version;
        }
        writeJson(path,history);
    }

    nlohmann::json readHistory(const std::filesystem::path& path)
    {
        auto history=nlohmann::json::parse(std::ifstream(path));
        if(history.contains("catalogs")) for(auto& catalog:history.at("catalogs")) for(auto& entry:catalog.at("structures"))
            if(entry.contains("templateBinary"))
            {
                auto file=std::filesystem::path(entry.at("templateBinary").get<std::string>());
                if(file.is_absolute() || file.lexically_normal()!=file || file.string().find("..")!=std::string::npos) throw std::runtime_error("Invalid catalog binary path");
                entry["template"]=toJson(readBinary(path.parent_path()/file)); entry.erase("templateBinary");
            }
        return history;
    }

    void writeJson(const std::filesystem::path& path, const nlohmann::json& json)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path);
        if (!file || !(file << json.dump(2) << '\n')) throw std::runtime_error("Cannot write " + path.string());
    }

    nlohmann::json toJson(const Catalog& catalog)
    {
        nlohmann::json entries = nlohmann::json::array();
        for (const auto& rule : catalog)
            entries.push_back({{"file", rule.file}, {"frequency", rule.frequency},
                {"height", {rule.minHeight, rule.maxHeight}}, {"spacing", rule.spacing},
                {"template", toJson(rule.structure)}, {"requireClear", rule.requireClear},
                {"foundationDepth", rule.foundationDepth}, {"biome", rule.biome}, {"avoidOverlap", rule.avoidOverlap},
                {"blockExclusion", rule.blockExclusion}});
        return {{"version", 1}, {"structures", entries}};
    }

    static Rule ruleFromJson(const nlohmann::json& entry)
    {
            Rule rule;
            rule.file = entry.at("file").get<std::string>();
            rule.frequency = entry.value("frequency", 0.1);
            rule.spacing = entry.value("spacing", 32);
            rule.requireClear = entry.value("requireClear", true);
            rule.foundationDepth = entry.value("foundationDepth", 0);
            rule.avoidOverlap = entry.value("avoidOverlap", false);
            rule.blockExclusion = entry.value("blockExclusion", false);
            rule.biome = entry.value("biome", std::vector<std::string>{"default", "test"});
            auto height = entry.at("height").get<std::array<int, 2>>();
            rule.minHeight = height[0]; rule.maxHeight = height[1];
            validateRule(rule);
            return rule;
    }

    Catalog catalogFromJson(const nlohmann::json& json)
    {
        Catalog result;
        if (json.at("version").get<int>() != 1) throw std::runtime_error("Unsupported generation version");
        for (const auto& entry : json.at("structures"))
        {
            if (!entry.value("enabled", true)) continue;
            Rule rule=ruleFromJson(entry);
            rule.structure = fromJson(entry.at("template"));
            if (!rule.structure.blocks.empty() && rule.frequency > 0) result.push_back(std::move(rule));
        }
        return result;
    }

    Catalog loadLibrary(const std::filesystem::path& root, std::vector<std::string>* warnings, int dimensions)
    {
        auto path = root / "generation.json";
        if (!std::filesystem::exists(path)) return {};
        auto config = nlohmann::json::parse(std::ifstream(path));
        Catalog result;
        for (auto entry : config.at("structures"))
        {
            if (!entry.value("enabled", true)) continue;
            try
            {
            auto file = std::filesystem::path(entry.at("file").get<std::string>());
            if (file.has_parent_path() || file.extension() != ".bin")
                throw std::runtime_error("Structure file must be a .bin filename inside Structure");
            auto model = readBinary(root / file);
            requireDimensions(model, dimensions);
            entry["requireClear"] = entry.value("requireClear", false);
            entry["foundationDepth"] = entry.value("foundationDepth", 8);
            entry["avoidOverlap"] = entry.value("avoidOverlap", true);
            entry["blockExclusion"] = true;
            double frequency = entry.value("frequency", 0.1);
            if (frequency > 1)
            {
                entry["frequency"] = std::min(frequency / 100.0, 1.0);
                if (warnings) warnings->push_back(file.string() + ": frequency interpreted as percent (maximum 100%)");
            }
            Rule rule=ruleFromJson(entry);
            rule.structure=std::move(model);
            if (!rule.structure.blocks.empty() && rule.frequency>0) result.push_back(std::move(rule));
            }
            catch (const std::exception& error)
            {
                if (warnings) warnings->push_back(entry.value("file", "?") + ": " + error.what());
                else throw;
            }
        }
        return result;
    }

    void saveTemplate(const std::filesystem::path& root, const Template& structure)
    {
        if (!simpleName(structure.name)) throw std::runtime_error("Name: letters, digits, _ or -");
        if (structure.name == "generation") throw std::runtime_error("Choose a name other than generation");
        if (structure.blocks.empty()) throw std::runtime_error("No blocks in the selected region");
        auto configPath = root / "generation.json";
        nlohmann::json config = {{"version", 1}, {"structures", nlohmann::json::array()}};
        if (std::filesystem::exists(configPath)) config = nlohmann::json::parse(std::ifstream(configPath));
        std::string filename = structure.name + ".bin";
        bool found = false;
        for (const auto& entry : config.at("structures")) found |= entry.at("file") == filename;
        if (!found) config["structures"].push_back({{"file", filename}, {"frequency", 0.25},
            {"height", {0, 127}}, {"spacing", 8}, {"enabled", true}, {"requireClear", false},
            {"foundationDepth", 8}, {"biome", {"default", "test"}}, {"avoidOverlap", true}});
        writeBinary(root / filename, structure);
        writeJson(configPath, config);
    }

    void apply(Chunk& chunk, m5::ivec5 chunkPos, uint64_t seed,
        const Catalog& catalog, const TerrainQuery& terrain, const BiomeQuery& biome, int dimensions)
    {
        for (const auto& rule : catalog) requireDimensions(rule.structure, dimensions);
        for (int axis = dimensions; axis < 5; ++axis) if (chunkPos[axis] != 0) return;
        const m5::ivec5 extent{Chunk::HEIGHT, Chunk::SIZE, dimensions >= 3 ? Chunk::SIZE : 1,
            dimensions >= 4 ? Chunk::SIZE : 1, dimensions >= 5 ? Chunk::SIZE : 1};
        const auto min = chunkPos * extent;
        const auto max = min + extent - 1;
        m5::ivec5 halo{0};
        for (const auto& rule : catalog)
            if (rule.avoidOverlap && rule.frequency > 0 && !rule.biome.empty())
                for (int axis = 0; axis < 5; ++axis)
                    halo[axis] = std::max(halo[axis], rule.structure.size[axis] + (axis == 0 ? rule.foundationDepth : 0));
        struct Candidate
        {
            const Rule* rule;
            m5::ivec5 origin, low, high;
            uint64_t priority;
            std::vector<Block> foundation;
        };
        std::vector<Candidate> candidates;
        using OccupiedKey = std::tuple<int,int,int,int,int>;
        std::unordered_map<const Rule*, std::unordered_set<OccupiedKey>> occupied;
        auto occupiedKey = [](m5::ivec5 p) { return OccupiedKey{p.a,p.b,p.c,p.d,p.e}; };
        for (const auto& rule : catalog)
            if (rule.avoidOverlap)
                for (const auto& block : rule.structure.blocks) occupied[&rule].insert(occupiedKey(block.pos));
        auto write = [&](m5::ivec5 worldPos, uint16_t id, bool active)
        {
            auto p = worldPos - min;
            for (int axis = 0; axis < 5; ++axis) if (p[axis] < 0 || p[axis] >= extent[axis]) return;
            chunk.setBlock(m5::uvec5(p), id);
            if (active && (id == ExtendedBlock::SAND || id == ExtendedBlock::ANTI_SAND ||
                ExtendedBlock::isLever(id) || ExtendedBlock::isCable(id) || ExtendedBlock::isRepeater(id) ||
                ExtendedBlock::isRedirector(id) || ExtendedBlock::isCrystalDoor(id)))
                chunk.generatedActiveBlocks.push_back(m5::uvec5(p));
        };
        for (const auto& rule : catalog)
        {
            const auto searchMin = rule.avoidOverlap ? min - halo : min;
            const auto searchMax = rule.avoidOverlap ? max + halo : max;
            const auto& shape = rule.structure;
            if (shape.blocks.empty() || rule.biome.empty()) continue;
            int baseA = shape.size.a;
            for (const auto& block : shape.blocks) baseA = std::min(baseA, block.pos.a);
            std::vector<m5::ivec5> footprint;
            for (const auto& block : shape.blocks) if (block.pos.a == baseA) footprint.push_back(block.pos);
            int minGround = std::max(rule.minHeight, searchMin.a - shape.size.a);
            int maxGround = std::min(rule.maxHeight, searchMax.a - 1 + rule.foundationDepth);
            if (minGround > maxGround || rule.frequency <= 0) continue;
            m5::ivec5 low, high;
            low.a = floorDiv(minGround, 32); high.a = floorDiv(maxGround, 32);
            for (int axis = 1; axis < 5; ++axis)
            {
                if (axis >= dimensions) { low[axis] = high[axis] = 0; continue; }
                low[axis] = floorDiv(searchMin[axis] - shape.size[axis] + 1, rule.spacing);
                high[axis] = floorDiv(searchMax[axis], rule.spacing);
            }
            uint64_t salt = mix(seed ^ nameHash(rule.file));
            for (int a = low.a; a <= high.a; ++a)
            for (int b = low.b; b <= high.b; ++b)
            for (int c = low.c; c <= high.c; ++c)
            for (int d = low.d; d <= high.d; ++d)
            for (int e = low.e; e <= high.e; ++e)
            {
                uint64_t h = salt;
                for (int coordinate : {a,b,c,d,e}) h = mix(h ^ (uint64_t)(int64_t)coordinate);
                if ((h >> 11) * (1.0 / 9007199254740992.0) >= rule.frequency) continue;
                m5::ivec5 origin{0,b*rule.spacing,c*rule.spacing,d*rule.spacing,e*rule.spacing};
                for (int axis = 1; axis < dimensions; ++axis)
                {
                    h = mix(h);
                    origin[axis] += (int)(h % rule.spacing);
                }
                bool intersects = true;
                for (int axis = 1; axis < 5; ++axis)
                    intersects &= origin[axis] <= searchMax[axis] && origin[axis] + shape.size[axis] > searchMin[axis];
                if (!intersects) continue;
                bool allowed = true;
                for (auto p : footprint)
                {
                    if (std::find(rule.biome.begin(), rule.biome.end(), "default") != rule.biome.end() &&
                        std::find(rule.biome.begin(), rule.biome.end(), "test") != rule.biome.end()) break;
                    auto name = biome ? biome(origin + p) : std::string("default");
                    if (std::find(rule.biome.begin(), rule.biome.end(), name) == rule.biome.end()) { allowed = false; break; }
                }
                if (!allowed) continue;

                // Search the entire candidate band, not just this chunk's intersection.
                // Every chunk must choose exactly the same floor for a spanning structure.
                int floorMin = std::max(rule.minHeight, a*32);
                int floorMax = std::min(rule.maxHeight, a*32 + 31);
                bool placed = false;
                for (int ground = floorMax; ground >= floorMin && !placed; --ground)
                {
                    origin.a = ground + 1 - baseA;
                    bool fits = true;
                    bool supported = false;
                    for (const auto& block : shape.blocks)
                    {
                        if (!rule.requireClear && block.pos.a != baseA) continue;
                        auto p = origin + block.pos;
                        bool air = terrain(p) == ::Block::AIR;
                        if (rule.requireClear && !air) { fits = false; break; }
                        if (block.pos.a == baseA)
                        {
                            --p.a;
                            supported |= air && ExtendedBlock::isSolid(terrain(p));
                            if (supported && !rule.requireClear) break;
                        }
                    }
                    if (!fits || !supported) continue;
                    std::vector<Block> foundation;
                    if (rule.foundationDepth > 0)
                    {
                        // Only the globally lowest occupied layer defines the footprint.
                        for (auto offset : footprint)
                        {
                            auto p = origin + offset;
                            --p.a;
                            int gap = 0;
                            while (gap < rule.foundationDepth && terrain(p) == ::Block::AIR) { --p.a; ++gap; }
                            auto material = terrain(p);
                            if (!ExtendedBlock::isSolid(material)) { fits = false; break; }
                            for (int depth = 1; depth <= gap; ++depth)
                            {
                                auto q = p; q.a += depth;
                                auto fill = material == ::Block::GRASS && depth < gap ? (uint16_t)::Block::DIRT : material;
                                foundation.push_back({q, fill});
                            }
                        }
                    }
                    if (!fits) continue;
                    placed = true;
                    if (rule.avoidOverlap)
                    {
                        auto low = origin;
                        low.a -= rule.foundationDepth;
                        candidates.push_back({&rule, origin, low, origin + shape.size - 1, h, std::move(foundation)});
                        continue;
                    }
                    for (const auto& block : foundation)
                        write(block.pos, block.id, false);
                    for (const auto& block : shape.blocks)
                        write(origin + block.pos, block.id, true);
                }
            }
        }
        auto rank = [&](const Candidate& c) {
            return std::tuple{c.priority, c.rule->file, c.origin.a, c.origin.b, c.origin.c, c.origin.d, c.origin.e,
                c.rule - catalog.data()};
        };
        for (const auto& candidate : candidates)
        {
            bool visible = true;
            for (int axis = 0; axis < 5; ++axis)
                visible &= candidate.low[axis] <= max[axis] && candidate.high[axis] >= min[axis];
            if (!visible) continue;
            bool blocked = false;
            // Local priority minima are independent of chunk borders and worker order.
            // Include every conflicting candidate, even if it loses another conflict.
            for (const auto& other : candidates)
            {
                if (&other == &candidate || !(rank(other) < rank(candidate))) continue;
                bool overlaps = true;
                for (int axis = 0; axis < 5; ++axis)
                    overlaps &= candidate.low[axis] <= other.high[axis] && other.low[axis] <= candidate.high[axis];
                if (overlaps && candidate.rule->blockExclusion)
                {
                    overlaps = false;
                    const Candidate* smaller = &candidate;
                    const Candidate* larger = &other;
                    if (smaller->rule->structure.blocks.size() > larger->rule->structure.blocks.size()) std::swap(smaller, larger);
                    const auto& cells = occupied.at(larger->rule);
                    for (const auto& block : smaller->rule->structure.blocks)
                    {
                        if (cells.contains(occupiedKey(smaller->origin + block.pos - larger->origin)))
                        { overlaps = true; break; }
                    }
                }
                if (overlaps) { blocked = true; break; }
            }
            if (blocked) continue;
            for (const auto& block : candidate.foundation) write(block.pos, block.id, false);
            for (const auto& block : candidate.rule->structure.blocks) write(candidate.origin + block.pos, block.id, true);
        }
    }

}
