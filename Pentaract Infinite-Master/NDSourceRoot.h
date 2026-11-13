#pragma once
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

inline std::filesystem::path ndSourceRoot(const std::filesystem::path& gameRoot) {
    nlohmann::json config;
    std::ifstream(gameRoot/"nd-build.json")>>config;
    std::filesystem::path source=config.at("sourceRoot").get<std::string>();
    source=std::filesystem::weakly_canonical(source.is_absolute()?source:gameRoot/source);
    if(!std::filesystem::is_regular_file(source/"5D-Miner.vcxproj") ||
        !std::filesystem::is_regular_file(source/"tools/build-runtime-nd.ps1"))
        throw std::runtime_error("ND build sources are missing (nd-build.json).");
    return source;
}
