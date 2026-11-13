#include "HyperplaneAlignment.h"
#pragma once
#include "SliceMeshRenderer.h"
#include "SideViewND.h"
#include "FrameMeter.h"
#include "TextureAxisWindow.h"
#include "TextureProfileAxes.h"
#include "GameConsole.h"
#include "WorldCommands.h"
#include "RayAxisGroup.h"
#include "NDFrameData.h"
#include "CollisionPrefetch.h"
#include "WorldRenderResolution.h"
#include "TemporalND.h"
#include "WaterSimulation.h"
#include "StateManager.h"
#include "GPUBuffer.h"
#include "TextureBuffer.h"
#include "ChunkRenderData.h"

#include "experiments/nd/WorldND.h"
#include "WorldRenderType.h"
#include "StateTitleScreen.h"
#include "QuadRendererBasic.h"
#include <Windows.h>
#include <format>
#include <fstream>
#include <iterator>

template<int N> class StateGameND : public State
{
    using Engine=WorldND<N>;
    using structures=typename Engine::structures;
    using Space=typename Engine::Space;
    using Logic=typename Engine::Logic;
    using Generation=typename Engine::Generation;
    using BlockState=typename Engine::BlockState;
    using FaceMask=typename Engine::FaceMask;
    static constexpr int faceStride=N+FaceMask::wordCount+1;
    static constexpr int blockRecordBytes=4*N+2+4*FaceMask::wordCount;
    static constexpr int worldHeaderBytes=N<=15?18:22;
    static inline constexpr auto address=&Engine::address;
    static inline constexpr auto position=&Engine::position;
    static inline constexpr auto neighbor=&Engine::neighbor;
    static inline constexpr auto validate=&Engine::validate;
    static inline constexpr auto collisionChunks=&Engine::collisionChunks;
    static inline constexpr auto abovePlacedBlock=&Engine::abovePlacedBlock;
    static inline constexpr auto bodyOverlaps=&Engine::bodyOverlaps;
    static inline constexpr auto extent=Engine::extent;
    static std::string dimName(){return std::to_string(N)+"D";}
    static std::string worldMagic(){return N<=15?(N<10?"PWRL":"PWR")+dimName()+"\r\n":"PWRLND\r\n";}
public:
    // Shared by gameplay and the native integration test.
    typename Engine::Space& world() { return *space; }
    typename Engine::Logic& circuits() { return *logic; }
    const typename Engine::Vector& playerPosition() const { return eye; }

private:
    friend struct TemporalNDTestAccess;
    friend struct WorldHighTestAccess;
    friend struct World6DTestAccess;
    friend struct World7DTestAccess;
    friend struct World8DTestAccess;
    friend struct World9DTestAccess;
    friend struct World10DTestAccess;
    friend struct World11DTestAccess;
    friend struct World12DTestAccess;
    friend struct World13DTestAccess;
    friend struct World14DTestAccess;
    friend struct WorldNDTestAccess;
    friend struct World15DTestAccess;
    TextureAxisWindow<N> textureWindow;
    WorldRenderResolution worldResolution;
    ui::text resolutionLabel;
    std::array<ui::checkbox,3> resolutionChoices;
    using Position = typename Engine::Position;
    using Vector = typename Engine::Vector;
    std::filesystem::path path;
    std::shared_ptr<typename Engine::Generation> generation;
    std::unique_ptr<typename Engine::Space> space;
    std::unique_ptr<typename Engine::Logic> logic;
    using Temporal=TemporalND<N,typename Engine::Space,typename Engine::Logic,typename Engine::Falling>;
    Vector eye=[] {Vector p{};p.fill(.5);p[0]=44.5;return p;}();
    Vector forward{}, right{}, up{};
    int renderType=3;
    std::array<bool,18> sideControls{};
    std::optional<Position> sidePlacement;
    double sideWalkTime=0;
    int sideWalkDirection=0;
    double yaw=0, pitch=0, mouseX=0, mouseY=0, verticalSpeed=0, fallTimer=0;
    FrameMeter frameMeter;
    RayAxisGroup<N> rayGroup;
    CollisionPrefetch<N> collisionPrefetch;
    GameConsole console;
    double playerSpeed=7.;
    double streamMs=0, uploadMs=0;
    std::array<int,2> plane{1,2};
    typename Engine::CameraFrame frame;
    bool middleMouse=false, freePlane=false;
    int firstAxis=-1;
    int selectedHidden=2;
    int controlBank=0;
    AxisGroups axisGroups{N};
    bool configuringAxes=false;
    int axisConfigPage=0;
    bool bankKeyHeld=false;
    int bankHidden(int ordinal) const {
        return axisGroups.hiddenAt(controlBank,ordinal,plane,renderType==2);
    }
    void activateControlBank() {
        sideControls={};keys.clear();
        plane=axisGroups.plane(controlBank,N);
        if(renderType==2)SideViewND::align<N>(frame.axes,plane[0]);else frame.align(plane);
        HyperplaneAlignment::centerHorizontal(eye,N);target={};sidePlacement.reset();
        yaw=pitch=0;freePlane=false;firstAxis=-1;
        selectedHidden=bankHidden(0);
        streamValid=false;streamDirty=meshViewDirty=true;camera();
    }
    void constrainGroupFrame(){
        rayGroup.update(axisGroups,controlBank,plane,eye);
        bool valid=rayGroup.active[plane[0]] && (renderType==2 || rayGroup.active[plane[1]]);
        auto slots=axisGroups.hidden(controlBank,plane,renderType==2);slots.push_back(0);if(renderType!=2)slots.push_back(1);
        for(int slot:slots)for(int axis=1;axis<N;++axis)
            if(!rayGroup.active[axis] && std::abs(frame.axes[slot][axis])>1e-10)valid=false;
        if(!valid){
            plane=axisGroups.plane(controlBank,N);
            if(renderType==2)SideViewND::align<N>(frame.axes,plane[0]);else frame.align(plane);
            freePlane=false;selectedHidden=bankHidden(0);
        }
    }
    void switchControlBank(int step=1) {controlBank=axisGroups.shifted(controlBank,step);activateControlBank();}
    void cycleHidden(int direction){
        auto slots=axisGroups.hidden(controlBank,plane,renderType==2);
        if(slots.empty()){selectedHidden=-1;return;}
        auto it=std::find(slots.begin(),slots.end(),selectedHidden);
        int index=it==slots.end()?0:int(it-slots.begin());
        selectedHidden=slots[(index+direction+int(slots.size()))%slots.size()];
    }
    bool flight=false, noclip=false, grounded=false, menu=false, inventory=false, editing=false;
    bool firstMouse=true;
    std::set<int> keys;
    typename Engine::Hit target;
    uint16_t selected=Block::GRASS;
    std::vector<uint16_t> selectable;
    std::set<uint16_t> excluded;
    int inventoryPage=0;

    struct RenderChunk { std::array<uint16_t,32> blocks{}; };
    std::map<Position,RenderChunk> rendered;
    std::set<Position> pendingUploads;
    std::map<Position,std::vector<int>> renderedMasks;
    std::set<Position> viewWanted;
    std::shared_ptr<const std::set<Position>> viewSnapshot;
    uint64_t viewRevision=0;
    std::vector<Position> viewColumns;
    struct ViewPlan {
        std::vector<Position> columns,order;
        std::set<Position> view;
        std::vector<Position> added,removed;
        std::shared_ptr<const std::set<Position>> snapshot;
        uint64_t baseRevision=0;
        int lowA=0,highA=0;
    };
    std::future<ViewPlan> viewTask;
    Vector plannedEye{};
    typename Engine::CameraFrame plannedFrame;
    Vector streamEye{};
    Vector streamPassEye{};
    std::vector<Position> streamCollision;
    bool streamDirty=true;
    typename Engine::CameraFrame streamFrame;
    bool streamValid=false, streamUnderground=false;
    std::set<Position> wanted, registered;
    std::vector<Position> renderOrder;
    size_t requestCursor=0;
    typename Engine::Falling falling;
    WaterSimulation<N> water;
    std::unique_ptr<Temporal> temporal;
    Shader* shader=nullptr;
    Shader* meshShader=nullptr;
    SliceMeshRenderer<N> mesh;
    GPUBuffer handles, textures, tiles, masks;
    GPUBuffer lookup;
    GPUBuffer frameBuffer;
    NDFrameData<N> frameData;
    bool lookupDirty=true;
    bool meshViewDirty=true;
    int maskCount=0;
    QuadRenderer panel;
    ui::page hud, page;
    ui::text coords, selection, title, status, offsetLabel, sizeLabel;
    ui::image selectedIcon;
    std::array<ui::image,8> inventoryIcons;
    ui::button resumeButton, quitButton, saveButton, loadButton, previousButton, nextButton, reloadButton;
    ui::button axesPrevious,axesNext;
    ui::button configAxesButton,applyAxesButton,resetAxesButton;
    ui::text groupColumn,orderColumn,keyColumn;
    std::array<ui::text,8> groupAxisLabels,groupKeyLabels;
    std::array<ui::text_input,N> groupInputs,orderInputs;
    ui::text axesPageLabel;
    int structurePage=0;
    ui::text_input nameInput;
    std::array<ui::text,N> axes;
    std::array<ui::text_input,N> offsets, lengths;
    std::array<ui::checkbox,8> checks;
    struct Region { Position offset=[] {Position p{};p[1]=1;return p;}(), size=[] {Position p{};p.fill(1);return p;}(); std::string name="structure"; };
    std::map<Position,Region> regions;
    Position owner{}, jobOwner{};
    enum class StructureJob { None, Save, Load };
    StructureJob structureJob=StructureJob::None;
    std::optional<typename structures::Template> placing;



    static Position cell(const Vector& v) { Position p{}; for(int i=0;i<N;++i) p[i]=(int)std::floor(v[i]); return p; }
    template<class F> static void each(Position low, const Position& high, F f)
    {
        Position p=low;
        for (;;) { f(p); int i=0; for(;i<N;++i) {if(++p[i]<=high[i]) break; p[i]=low[i];} if(i==N) break; }
    }
    static void replace(const std::filesystem::path& temporary, const std::filesystem::path& destination)
    {
        if (!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace saved file");
    }
    static void jsonFile(const std::filesystem::path& path, const nlohmann::json& json)
    {
        auto temp=path; temp+=".tmp";
        { std::ofstream out(temp); out<<json.dump(2); out.flush(); if(!out) throw std::runtime_error("Cannot save JSON"); }
        replace(temp,path);
    }
    static std::string blockName(uint16_t id)
    {
        return std::string(id<Block::COUNT?Block::names[id]:ExtendedBlock::name(id));
    }
    static void blockIcon(ui::image& image,uint16_t id,float pixels)
    {
        Texture* texture=nullptr;uint16_t atlasId=id;
        if(ExtendedBlock::isExtended(id)) {
            const auto& data=ExtendedBlock::data(id);
            texture=Texture::get(std::string(data.iconTextureName));atlasId=data.iconBlock;
        }
        bool custom=texture!=nullptr;
        if(!texture)texture=Texture::get("blockIcons.png");
        image.setTexture(texture);
        image.setClip(custom?0.f:(atlasId-1)/16.f,0,custom?1.f:1.f/16.f,1.f);
        if(texture)image.setScale(pixels/(texture->getSize().a*(custom?1.f:1.f/16.f)),pixels/texture->getSize().b);
    }
public:
#include "StateGameND.inl"
};
extern template class StateGameND<6>;
extern template class StateGameND<7>;
extern template class StateGameND<8>;
extern template class StateGameND<9>;
extern template class StateGameND<10>;
extern template class StateGameND<11>;
extern template class StateGameND<12>;
extern template class StateGameND<13>;
extern template class StateGameND<14>;
extern template class StateGameND<15>;
