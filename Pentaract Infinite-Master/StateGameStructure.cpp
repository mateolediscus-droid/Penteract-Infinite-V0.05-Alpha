#include "StateGame.h"
#include "BackgroundWork.h"
#include <charconv>
#include <fstream>

namespace
{
    World::ChunkKey selectionKey(m5::ivec5 p) { return {p.a,p.b,p.c,p.d,p.e}; }
    std::array<int,5> coordinates(m5::ivec5 p) { return {p.a,p.b,p.c,p.d,p.e}; }
    m5::ivec5 position(const nlohmann::json& j)
    {
        auto p = j.get<std::array<int,5>>();
        return {p[0],p[1],p[2],p[3],p[4]};
    }
}

void StateGame::openStructureEditor(StateManager& s, m5::ivec5 pos)
{
    structureBlockPos = pos;
    structureSelection = structureSelections.try_emplace(selectionKey(pos)).first->second;
    structureEditorOpen = true;
    structurePreview = true;
    console.open = false;
    keys = {};
    vel = {};
    structurePage.clear();
    auto place = [this](ui::element& element, int x, int y)
    {
        element.setAlignX(ui::ALIGN_CENTER_X);
        element.setAlignY(ui::ALIGN_CENTER_Y);
        element.setOffsetX(x);
        element.setOffsetY(y);
        structurePage.addElem(&element);
    };
    structureTitle.setText("Structure Block - " + std::to_string(world.getDimensions()) + "D");
    structureTitle.setColor({0.2f, 1.0f, 0.35f, 1.0f});
    place(structureTitle, 0, -228);
    structureNameLabel.setText("Name");
    structureNameLabel.setSize(1);
    place(structureNameLabel, -164, -197);
    structureNameInput.setText(structureSelection.name);
    structureNameInput.setSize(280, 32);
    place(structureNameInput, 0, -166);
    structureOffsetLabel.setText("Offset");
    structureSizeLabel.setText("Size");
    structureOffsetLabel.setSize(1);
    structureSizeLabel.setSize(1);
    place(structureOffsetLabel, -30, -125);
    place(structureSizeLabel, 114, -125);
    const char* axes[] = {"A / Y", "B / X", "C / Z", "D / W", "E / V"};
    for (int axis = 0; axis < world.getDimensions(); ++axis)
    {
        int y = -93 + axis * 40;
        structureAxisLabels[axis].setText(axes[axis]);
        structureAxisLabels[axis].setSize(1);
        place(structureAxisLabels[axis], -150, y);
        structureOffsetInputs[axis].setText(std::to_string(structureSelection.offset[axis]));
        structureOffsetInputs[axis].setSize(112, 32);
        place(structureOffsetInputs[axis], -30, y);
        structureSizeInputs[axis].setText(std::to_string(structureSelection.size[axis]));
        structureSizeInputs[axis].setSize(112, 32);
        place(structureSizeInputs[axis], 114, y);
    }
    structureBoundsCheckbox.setText("Bounds");
    structureBoundsCheckbox.setChecked(structureSelection.visible);
    structureBoundsCheckbox.setAction([this](bool visible) { structureSelection.visible = visible; });
    place(structureBoundsCheckbox, -113, 112);
    structureStatus.setText("");
    structureStatus.setSize(1);
    structureStatus.setWrapWidth(430);
    place(structureStatus, 0, 155);
    structureSaveButton.setText("Save");
    structureSaveButton.setSize(110, 36);
    structureSaveButton.setAction([this] { startStructureCapture(); });
    place(structureSaveButton, -135, 205);
    structureLoadButton.setText("Load");
    structureLoadButton.setSize(110, 36);
    structureLoadButton.setAction([this] { startStructurePlacement(); });
    place(structureLoadButton, 0, 205);
    structureCloseButton.setText("Close");
    structureCloseButton.setSize(110, 36);
    structureCloseButton.setAction([this, &s] { closeStructureEditor(s); });
    place(structureCloseButton, 135, 205);
    structurePreviousButton.setText("<");
    structurePreviousButton.setSize(32, 32);
    structurePreviousButton.setAction([this] { chooseStructureFile(-1); });
    place(structurePreviousButton, -164, -166);
    structureNextButton.setText(">");
    structureNextButton.setSize(32, 32);
    structureNextButton.setAction([this] { chooseStructureFile(1); });
    place(structureNextButton, 164, -166);
    enableCursor(s.getWindow());
    s.setUiPage(&structurePage);
}

void StateGame::chooseStructureFile(int direction)
{
    if (structureCapturing || structureSaveResult.valid() || structurePlacement) return;
    try
    {
        std::vector<std::string> names;
        auto library = Structures::libraryDirectory(world.getDimensions());
        if (std::filesystem::exists(library))
            for (const auto& entry : std::filesystem::directory_iterator(library))
                if (entry.is_regular_file() && entry.path().extension() == ".bin")
                {
                    try
                    {
                        Structures::requireDimensions(Structures::readBinary(entry.path()), world.getDimensions());
                        names.push_back(entry.path().stem().string());
                    }
                    catch (const std::exception&) {}
                }
        std::sort(names.begin(), names.end());
        if (names.empty()) { structureStatus.setText("No saved structures"); return; }
        auto it = std::find(names.begin(), names.end(), structureNameInput.getText());
        int index = it == names.end() ? (direction > 0 ? 0 : (int)names.size() - 1) :
            ((int)(it - names.begin()) + direction + (int)names.size()) % (int)names.size();
        structureNameInput.setText(names[index]);
        readStructureInputs(false);
        structureStatus.setText(std::to_string(index + 1) + " / " + std::to_string(names.size()) + ": " + names[index] + ".bin");
    }
    catch (const std::exception& error) { structureStatus.setText(error.what()); }
}

bool StateGame::readStructureInputs(bool reportError)
{
    auto next = structureSelection;
    next.name = structureNameInput.getText();
    for (int i = 0; i < 5; ++i)
    {
        if (i >= world.getDimensions()) { next.offset[i] = 0; next.size[i] = 1; continue; }
        auto parse = [](const std::string& text, int& value)
        {
            auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            return result.ec == std::errc{} && result.ptr == text.data() + text.size();
        };
        if (!parse(structureOffsetInputs[i].getText(), next.offset[i]) ||
            !parse(structureSizeInputs[i].getText(), next.size[i]) || next.size[i] <= 0 ||
            next.size[i] > 1048576 || std::abs((int64_t)next.offset[i]) > 1048576)
        {
            if (reportError) structureStatus.setText("Invalid offset or size");
            return false;
        }
    }
    structureSelection = next;
    structureSelections[selectionKey(structureBlockPos)] = next;
    world.setStructureRegion(structureBlockPos, structureBlockPos + next.offset, next.size);
    return true;
}

void StateGame::closeStructureEditor(StateManager& s)
{
    if (structureCapturing || structureSaveResult.valid() || structurePlacement) return;
    readStructureInputs(false);
    structureEditorOpen = false;
    keys = {};
    s.setUiPage(nullptr);
    disableCursor(s.getWindow());
}

void StateGame::startStructureCapture()
{
    if (structureCapturing || structureSaveResult.valid() || structurePlacement || !readStructureInputs(true)) return;
    structureCaptureSelection = structureSelection;
    structureCaptureMin = structureBlockPos + structureSelection.offset;
    auto end = structureCaptureMin + structureSelection.size - 1;
    m5::ivec5 low, high;
    for (int i = 0; i < 5; ++i)
    {
        int extent = i == 0 ? Chunk::HEIGHT : Chunk::SIZE;
        low[i] = Structures::floorDiv(structureCaptureMin[i], extent);
        high[i] = Structures::floorDiv(end[i], extent);
    }
    structureCapturePositions.clear();
    structureCaptureChunks.clear();
    for (int a = low.a; a <= high.a; ++a)
    for (int b = low.b; b <= high.b; ++b)
    for (int c = low.c; c <= high.c; ++c)
    for (int d = low.d; d <= high.d; ++d)
    for (int e = low.e; e <= high.e; ++e)
        structureCapturePositions.push_back({a,b,c,d,e});
    structureCaptureIndex = 0;
    structureCapturing = true;
    structureStatus.setText("Loading region...");
}

void StateGame::updateStructureEditor()
{
    if (!structureCapturing && !structureSaveResult.valid() && !structurePlacement) readStructureInputs(false);
    if (structurePlacement)
    {
        auto min = structureBlockPos + structureSelection.offset;
        m5::ivec5 low, high;
        for (int i = 0; i < 5; ++i)
        {
            int extent = i == 0 ? Chunk::HEIGHT : Chunk::SIZE;
            low[i] = Structures::floorDiv(min[i], extent);
            high[i] = Structures::floorDiv(min[i] + structurePlacement->size[i] - 1, extent);
        }
        bool ready = true;
        for (int a = low.a; a <= high.a; ++a)
        for (int b = low.b; b <= high.b; ++b)
        for (int c = low.c; c <= high.c; ++c)
        for (int d = low.d; d <= high.d; ++d)
        for (int e = low.e; e <= high.e; ++e)
        {
            m5::ivec5 pos{a,b,c,d,e};
            world.requestChunk(pos, true);
            ready &= world.findChunk(pos) != nullptr;
        }
        if (!ready) return;
        size_t end = std::min(structurePlacementIndex + 512, structurePlacement->blocks.size());
        for (; structurePlacementIndex < end; ++structurePlacementIndex)
        {
            const auto& block = structurePlacement->blocks[structurePlacementIndex];
            auto pos = min + block.pos;
            if (pos != structureBlockPos) world.editTemporalBlock(pos, block.id, block.id != Block::AIR);
        }
        updateRendererData();
        if (structurePlacementIndex == structurePlacement->blocks.size())
        {
            structureStatus.setText("Loaded " + std::to_string(structurePlacementIndex) + " blocks");
            structurePlacement.reset();
        }
        return;
    }
    if (structureCapturing)
    {
        bool allLoaded = true;
        for (auto pos : structureCapturePositions)
        {
            world.requestChunk(pos, true);
            allLoaded &= world.findChunk(pos) != nullptr;
        }
        if (!allLoaded) return;
        for (size_t i = structureCaptureIndex; i < std::min(structureCaptureIndex + 24, structureCapturePositions.size()); ++i)
            world.requestChunk(structureCapturePositions[i]);
        for (int copied = 0; copied < 4 && structureCaptureIndex < structureCapturePositions.size(); ++copied)
        {
            auto pos = structureCapturePositions[structureCaptureIndex];
            auto chunk = world.findChunk(pos);
            if (!chunk && world.canGenerateChunk(pos)) break;
            if (chunk)
            {
                auto snapshot = std::make_shared<Chunk>();
                snapshot->blocks = chunk->blocks;
                snapshot->extendedBlocks = chunk->extendedBlocks;
                structureCaptureChunks.push_back({pos, std::move(snapshot)});
            }
            ++structureCaptureIndex;
        }
        if (structureCaptureIndex == structureCapturePositions.size())
        {
            structureCapturing = false;
            structureStatus.setText("Saving...");
            structureSaveResult = BackgroundWork::storage().addTaskFuture(
                [selection = structureCaptureSelection, min = structureCaptureMin, chunks = std::move(structureCaptureChunks), dimensions = world.getDimensions()]
                {
                    auto result = Structures::capture(selection.name, min, selection.size, chunks, dimensions);
                    Structures::saveTemplate(Structures::libraryDirectory(dimensions), result);
                    return result.blocks.size();
                });
        }
    }
    if (structureSaveResult.valid() && structureSaveResult.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        try
        {
            auto count = structureSaveResult.get();
            reloadStructures();
            structureStatus.setText("Saved " + std::to_string(count) + " blocks: " + structureCaptureSelection.name + ".bin");
        }
        catch (const std::exception& error) { structureStatus.setText(error.what()); }
    }
}

void StateGame::startStructurePlacement()
{
    if (structureCapturing || structureSaveResult.valid() || structurePlacement || !readStructureInputs(true)) return;
    try
    {
        auto name = std::filesystem::path(structureSelection.name + ".bin");
        if (name.has_parent_path()) throw std::runtime_error("Choose a structure name, not a path");
        auto shape = Structures::readBinary(Structures::libraryDirectory(world.getDimensions()) / name);
        Structures::requireDimensions(shape, world.getDimensions());
        for (int i = 0; i < 5; ++i) structureSizeInputs[i].setText(std::to_string(shape.size[i]));
        readStructureInputs(true);
        structurePlacement = std::move(shape);
        structurePlacementIndex = 0;
        structureStatus.setText("Loading structure region...");
    }
    catch (const std::exception& error) { structureStatus.setText(error.what()); }
}

void StateGame::saveStructureSelections()
{
    nlohmann::json entries = nlohmann::json::array();
    for (const auto& [key, selection] : structureSelections)
    {
        m5::ivec5 pos{std::get<0>(key),std::get<1>(key),std::get<2>(key),std::get<3>(key),std::get<4>(key)};
        entries.push_back({{"pos", coordinates(pos)}, {"name", selection.name},
            {"offset", coordinates(selection.offset)}, {"size", coordinates(selection.size)}, {"visible", selection.visible}});
    }
    Structures::writeJson(worldPath / "structure-blocks.json", {{"version", 1}, {"blocks", entries}});
}

void StateGame::loadStructureSelections()
{
    structureSelections.clear();
    auto path = worldPath / "structure-blocks.json";
    if (!std::filesystem::exists(path)) return;
    try
    {
        auto j = nlohmann::json::parse(std::ifstream(path));
        for (const auto& entry : j.at("blocks"))
        {
            structureSelections[selectionKey(position(entry.at("pos")))] = {
                entry.at("name").get<std::string>(), position(entry.at("offset")), position(entry.at("size")), entry.value("visible", true)};
            auto p = position(entry.at("pos"));
            auto& selection = structureSelections.at(selectionKey(p));
            world.setStructureRegion(p, p + selection.offset, selection.size);
        }
    }
    catch (const std::exception& error) { print(error.what()); }
}

void StateGame::reloadStructures()
{
    structureRefresh = {};
    try
    {
        std::vector<std::string> warnings;
        auto library = Structures::libraryDirectory(world.getDimensions());
        auto catalog = std::make_shared<Structures::Catalog>(Structures::loadLibrary(library, &warnings, world.getDimensions()));
        world.setStructures(catalog);
        print("Structures: " + std::to_string(catalog->size()) + " active rules for new chunks.");
        for (const auto& warning : warnings) print("Structures: " + warning);
        nlohmann::json diagnostic = {{"active_rules", catalog->size()}, {"warnings", warnings}};
        Structures::writeJson(library / "generation-status.txt", diagnostic);
    }
    catch (const std::exception& error) { print(std::string("Structures: ") + error.what()); }
}

void StateGame::pollStructureLibrary(double dt)
{
    if (structureRefresh.valid() && structureRefresh.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        try {
            auto result = structureRefresh.get();
            if (result.catalog) {
                world.setStructures(result.catalog);
                print("Structures: " + std::to_string(result.catalog->size()) + " active rules for new chunks.");
                for (const auto& warning : result.warnings) print("Structures: " + warning);
            }
            structureLibraryStamp = std::move(result.stamp);
        } catch (const std::exception& error) { print(std::string("Structures: ") + error.what()); }
    }
    structureReloadTimer += dt;
    if (structureReloadTimer < 2.0 || structureRefresh.valid()) return;
    structureReloadTimer = 0;
    auto dimensions = world.getDimensions();
    auto library = Structures::libraryDirectory(dimensions);
    // Capture values only: a completed job may outlive the current world.
    structureRefresh = BackgroundWork::storage().addTaskFuture(
        [library, dimensions, previous = structureLibraryStamp]() {
            StructureRefresh result;
            std::vector<std::string> entries;
            if (std::filesystem::exists(library))
                for (const auto& entry : std::filesystem::directory_iterator(library))
                    if (entry.is_regular_file() && (entry.path().extension() == ".bin" || entry.path().filename() == "generation.json"))
                        entries.push_back(entry.path().string() + std::to_string(entry.last_write_time().time_since_epoch().count()));
            std::sort(entries.begin(), entries.end());
            for (const auto& entry : entries) result.stamp += entry + '\n';
            if (result.stamp != previous) {
                result.catalog = std::make_shared<Structures::Catalog>(Structures::loadLibrary(library, &result.warnings, dimensions));
                Structures::writeJson(library / "generation-status.txt",
                    {{"active_rules", result.catalog->size()}, {"warnings", result.warnings}});
            }
            return result;
        });
}
