#pragma once
#include "StateManager.h"
#include "ui.h"
#include "QuadRenderer.h"
#include "TextureProfile.h"
#include <memory>

class StateTextureEditor : public State {
public:
    static StateTextureEditor* instance();
    bool receivesMouseWithUi() override{return true;}
    void init(StateManager&) override;
    void close(StateManager&) override;
    void render(StateManager&) override;
    void mouseInput(StateManager&,double,double) override;
    void mouseButtonInput(StateManager&,int,int,int) override;
    void scrollInput(StateManager&,double,double) override;
    void windowResize(StateManager&,int,int) override;
private:
    friend struct TextureEditorTestAccess;
    ui::page page;
    std::vector<std::shared_ptr<ui::element>> controls;
    std::string displayedWeight;
    std::array<std::string,6> displayed{};
    std::array<ui::button*,8> colorButtons{};
    std::array<ui::text_input*,6> fields{};
    std::array<ui::checkbox*,10> faces{};
    std::array<ui::text_input*,3> biasFields{};
    std::array<std::string,3> displayedBias{};
    std::array<ui::text_input*,2> groupFields{};
    std::array<std::string,2> displayedGroup{};
    ui::checkbox* transparencyToggle=nullptr;
    ui::checkbox* highlightToggle=nullptr;
    ui::text* groupStatus=nullptr;
    TextureProfiles::GroupShape groupDraft;
    std::array<int,4> groupContext{-1,-1,-1,-1};
    int groupIndex=-1;
    void selectGroup(int);
    void storeGroup();
    void deleteGroup();
    void updateGroupStatus();
    int groupFace=-1,groupSlice=-1;
    bool displayedTransparency=false;
    ui::text_input *wCoordinate=nullptr,*vCoordinate=nullptr;
    std::array<ui::text_input*,4> boundsFields{};
    ui::text* regionLabel=nullptr;
    int regionIndex=-1;
    static constexpr int previewTop=366;
    ui::checkbox *allSlices=nullptr,*erase=nullptr,*smooth=nullptr;
    ui::text *caption=nullptr,*status=nullptr,*distribution=nullptr;
    ui::button* planeButton=nullptr;
    ui::button* stateButton=nullptr;
    ui::text* editorTitle=nullptr;
    int logicalState=0;
    uint16_t profileId() const{return blocks[selected]+logicalState;}
    void nextState();
    std::vector<uint16_t> blocks;
    std::map<uint16_t,TextureProfiles::Profile> drafts;
    std::map<uint16_t,TextureProfiles::Profile> undo;
    int selected=0,color=0,slice=8,plane=0,width=1280,height=720,panel=340;
    int xAxis=1,zAxis=2;
    int paintedFace=-1;
    float yaw=.65f,pitch=.45f,distance=3.2f,w=.5f,v=.5f;
    double mx=0,my=0;
    bool orbit=false,painting=false;
    Shader shader;GLuint vao=0,vbo=0;QuadRenderer quad;
    TextureProfiles::Profile& profile();
    void buildUi(StateManager&);
    void refreshFields();
    bool commitFields(bool force=false);
    void changed();
    bool paint();
    void chooseBlock(int);
    int referenceFace()const;
    void editColorCount(bool adding);
    int activeRegion(int face);
    void changeRegion(int direction);
    void editRegion(bool adding);
    bool commitRegionBounds();
    void moveSection(float,float);
    void setSectionCoordinates();
    glm::mat4 matrix()const;
    template<class T> T* add(int x,int y){auto p=std::make_shared<T>();auto raw=p.get();raw->setOffsetX(x);raw->setOffsetY(y);page.addElem(raw);controls.push_back(std::move(p));return raw;}
};
