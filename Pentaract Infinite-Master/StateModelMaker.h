#pragma once
#include "StateManager.h"
#include "Model5D.h"
#include "Shader.h"
#include "ui.h"
#include "QuadRenderer.h"

class StateModelMaker : public State
{
public:
    static StateModelMaker* instance();
    bool receivesMouseWithUi() override { return true; }
    void init(StateManager&) override;
    void close(StateManager&) override;
    void update(StateManager&,double) override;
    void render(StateManager&) override;
    void mouseInput(StateManager&,double,double) override;
    void mouseButtonInput(StateManager&,int,int,int) override;
    void scrollInput(StateManager&,double,double) override;
    void keyInput(StateManager&,int,int,int,int) override;
    void windowResize(StateManager&,int,int) override;
private:
    Model5D::Model model;
    std::vector<Model5D::Cell> cells;
    Model5D::Mesh section;
    Model5D::Point camera{2,-5,-8,0,0};
    int xAxis=1,zAxis=2,firstAxis=-1,selected=-1,pointPage=0,pointRows=6;
    double yaw=0.55,pitch=-0.12,distance=4;
    bool looking=false,tab=false,dirty=true,refresh=true;
    double mouseX=0,mouseY=0;
    int width=1280,height=720,panelWidth=300;
    std::string status;
    Shader shader;
    unsigned vao=0,vbo=0;
    QuadRenderer background;
    ui::page page;
    ui::text title,positionText,statusText,selectedText,planeText;
    ui::text dimensionsLabel,dimensionsValue;
    ui::button fewerDimensions,moreDimensions;
    ui::text_input name;
    std::array<ui::text,Model5D::maxDimensions> coordinateLabels;
    std::array<ui::text_input,Model5D::maxDimensions> coordinates;
    ui::button saveButton,loadButton,backButton,addButton,applyButton,deleteButton,teleportButton,previousButton,nextButton;
    ui::button previousModelButton,nextModelButton;
    ui::checkbox displayLinks,displayPoints;
    size_t currentShape=0;
    ui::text shapeLabel;
    ui::button previousShape,nextShape,newSolid,newVoid;
    ui::checkbox voidShape;
    std::array<ui::button,6> pointButtons;
    struct Vertex {glm::vec3 position;glm::vec4 color;};
    glm::mat4 matrix() const;
    glm::dvec3 forward() const;
    Model5D::Point placement() const;
    std::vector<int> hidden() const;
    void setDimensions(int dimensions);
    glm::vec3 visible(Model5D::Point) const;
    glm::dvec2 screen(Model5D::Point) const;
    void rebuild();
    void select(int);
    void buildUi(StateManager&);
    void add(Model5D::Point);
    void browseModel(int direction);
    void saveModel();
    void loadModel(bool showError=true);
    void restoreModel();
    bool rememberModel() const;
    void fileError(const char* operation,const std::exception& error,bool showDialog=true);
    void frameModel();
    void draw(const std::vector<Vertex>&,unsigned);
    std::filesystem::path path() const;
};
