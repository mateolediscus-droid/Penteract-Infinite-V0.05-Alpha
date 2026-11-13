#pragma once
#include "Chunk.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Structures
{
    struct Block
    {
        m5::ivec5 pos;
        uint16_t id;
    };
    struct Template
    {
        std::string name;
        m5::ivec5 size{ 1 };
        std::vector<Block> blocks;
        int dimensions = 5;
    };
    struct Rule
    {
        std::string file;
        double frequency = 0.1;
        int minHeight = 0;
        int maxHeight = 127;
        int spacing = 32;
        Template structure;
        bool requireClear = true;
        int foundationDepth = 0;
        bool avoidOverlap = false;
        bool blockExclusion = false;
        std::vector<std::string> biome{"default", "test"};
    };
    using Catalog = std::vector<Rule>;
    using TerrainQuery = std::function<uint16_t(const m5::ivec5&)>;
    using BiomeQuery = std::function<std::string(const m5::ivec5&)>;
    struct CapturedChunk
    {
        m5::ivec5 pos;
        std::shared_ptr<const Chunk> chunk;
    };
    Template capture(const std::string& name, m5::ivec5 min, m5::ivec5 size,
        const std::vector<CapturedChunk>& chunks, int dimensions = 5);

    std::filesystem::path libraryDirectory(int dimensions);
    void requireDimensions(const Template& structure, int dimensions);

    int floorDiv(int value, int divisor);
    nlohmann::json toJson(const Template& structure);
    Template fromJson(const nlohmann::json& json);
    Template readBinary(const std::filesystem::path& path);
    void writeBinary(const std::filesystem::path& path, const Template& structure);
    void writeHistory(const std::filesystem::path& path, nlohmann::json history);
    nlohmann::json readHistory(const std::filesystem::path& path);
    nlohmann::json toJson(const Catalog& catalog);
    Catalog catalogFromJson(const nlohmann::json& json);
    Catalog loadLibrary(const std::filesystem::path& root, std::vector<std::string>* warnings = nullptr, int dimensions = 5);
    void saveTemplate(const std::filesystem::path& root, const Template& structure);
    void writeJson(const std::filesystem::path& path, const nlohmann::json& json);
    void apply(Chunk& chunk, m5::ivec5 chunkPos, uint64_t seed,
        const Catalog& catalog, const TerrainQuery& terrain, const BiomeQuery& biome = {}, int dimensions = 5);
}
