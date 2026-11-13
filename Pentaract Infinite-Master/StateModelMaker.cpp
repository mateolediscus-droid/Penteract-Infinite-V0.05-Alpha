#include "StateModelMaker.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <format>
#include <cctype>
#include <fstream>
#include <Windows.h>

namespace
{
    std::string modelName(const std::filesystem::path& path)
    {
        auto value=path.stem().u8string();return std::string(value.begin(),value.end());
    }
}

StateModelMaker* StateModelMaker::instance() { static StateModelMaker value; return &value; }
std::vector<int> StateModelMaker::hidden() const {std::vector<int> result;for(int a=1;a<model.dimensions;++a)if(a!=xAxis&&a!=zAxis)result.push_back(a);return result;}
void StateModelMaker::setDimensions(int dimensions)
{
    dimensions=std::clamp(dimensions,3,Model5D::maxDimensions);
    if(dimensions==model.dimensions)return;
    model.dimensions=dimensions;firstAxis=-1;
    if(xAxis>=dimensions||zAxis>=dimensions){xAxis=1;zAxis=2;}
    frameModel();select(selected);rebuild();
}
glm::dvec3 StateModelMaker::forward() const {return {std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch)};}
glm::vec3 StateModelMaker::visible(Model5D::Point p) const {return {(float)p[xAxis],(float)p[0],(float)p[zAxis]};}
glm::mat4 StateModelMaker::matrix() const
{
    auto eye=visible(camera);
    return glm::perspective(glm::radians(65.0f),(float)std::max(1,width-panelWidth)/std::max(1,height),0.03f,2000.0f)*
        glm::lookAt(eye,eye+glm::vec3(forward()),glm::vec3(0,1,0));
}
glm::dvec2 StateModelMaker::screen(Model5D::Point p) const
{
    auto clip=matrix()*glm::vec4(visible(p),1);
    if(clip.w<=0.03) return {-100000,-100000};
    auto n=glm::vec3(clip)/clip.w;
    return {panelWidth+(n.x+1)*0.5*(width-panelWidth),(1-n.y)*0.5*height};
}
Model5D::Point StateModelMaker::placement() const
{
    auto p=camera;auto f=forward();p[xAxis]+=f.x*distance;p[0]+=f.y*distance;p[zAxis]+=f.z*distance;
    for(auto& value:p)value=std::round(value*4)/4;
    for(int a:hidden())p[a]=camera[a];
    for(int a=model.dimensions;a<Model5D::maxDimensions;++a)p[a]=0;
    return p;
}
std::filesystem::path StateModelMaker::path() const
{
    return Model5D::modelPath(name.getText());
}
bool StateModelMaker::rememberModel() const
{
    try
    {
        std::ofstream file(Model5D::storageDirectory()/"last-model.txt",std::ios::binary);
        file<<modelName(path());file.close();return (bool)file;
    }
    catch(...){return false;}
}
void StateModelMaker::fileError(const char* operation,const std::exception& error,bool showDialog)
{
    status=std::string(operation)+" failed: "+error.what();
    statusText.setColor({1,0.35f,0.3f,1});
    if(showDialog)
    {
        int length=MultiByteToWideChar(CP_UTF8,0,status.data(),(int)status.size(),nullptr,0);
        std::wstring message(length,L' ');
        MultiByteToWideChar(CP_UTF8,0,status.data(),(int)status.size(),message.data(),length);
        try {message+=L"\n\n"+path().wstring();}catch(...){}
        MessageBoxW(nullptr,message.c_str(),L"Model Maker",MB_OK|MB_ICONERROR);
    }
}
void StateModelMaker::saveModel()
{
    try
    {
        auto destination=path();model.save(destination);
        name.setText(modelName(destination));
        status=rememberModel()?std::format("Saved: {} points, {} links",model.points.size(),model.edges.size()):"Saved; last selection not recorded";
        statusText.setColor({0.35f,1,0.6f,1});
    }
    catch(const std::exception& e){fileError("Save",e);}
}
void StateModelMaker::loadModel(bool showError)
{
    try
    {
        auto source=path();auto loaded=Model5D::Model::load(source);
        model=std::move(loaded);name.setText(modelName(source));selected=-1;pointPage=0;currentShape=0;
        firstAxis=-1;if(xAxis>=model.dimensions||zAxis>=model.dimensions){xAxis=1;zAxis=2;}
        frameModel();select(-1);rebuild();rememberModel();
        statusText.setColor({0.35f,1,0.6f,1});
    }
    catch(const std::exception& e){fileError("Load",e,showError);}
}
void StateModelMaker::restoreModel()
{
    try
    {
        std::filesystem::create_directories(Model5D::storageDirectory());
        std::ifstream selection(Model5D::storageDirectory()/"last-model.txt",std::ios::binary);
        std::string last;std::getline(selection,last);
        if(!last.empty()) {name.setText(last);loadModel(false);return;}
        auto files=Model5D::savedModels();
        if(!files.empty()){name.setText(modelName(files.front()));loadModel(false);}
    }
    catch(const std::exception& e){fileError("Restore",e,false);}
}
void StateModelMaker::select(int index)
{
    selected=index;
    if(index>=0&&index<model.points.size())currentShape=model.shapeOf(index);
    auto p=index>=0&&index<model.points.size()?model.points[index]:placement();
    for(int i=0;i<Model5D::maxDimensions;++i)coordinates[i].setText(std::format("{:.3f}",p[i]));
    refresh=true;
}
void StateModelMaker::rebuild()
{
    try { cells=model.cells(); status=std::format("{} points / {} links / {} faces",model.points.size(),model.edges.size(),cells.size()); }
    catch(const std::exception& e) {cells.clear();status=e.what();}
    dirty=true;refresh=true;
}
void StateModelMaker::browseModel(int direction)
{
    try
    {
        std::vector<std::string> names;
        for(const auto& file:Model5D::savedModels())names.push_back(modelName(file));
        std::sort(names.begin(),names.end());
        if(names.empty()){status="No saved models";return;}
        auto current=std::find(names.begin(),names.end(),name.getText());
        int index=current==names.end()?(direction>0?-1:0):(int)(current-names.begin());
        index=(index+direction+(int)names.size())%(int)names.size();
        name.setText(names[index]);status="Selected "+names[index];
    }
    catch(const std::exception& e){status=e.what();}
}
void StateModelMaker::frameModel()
{
    if(model.points.empty())return;
    auto minimum=model.points.front(),maximum=minimum;
    for(const auto& p:model.points)for(int a=0;a<Model5D::maxDimensions;++a){minimum[a]=std::min(minimum[a],p[a]);maximum[a]=std::max(maximum[a],p[a]);}
    for(int a=0;a<Model5D::maxDimensions;++a)camera[a]=(minimum[a]+maximum[a])*0.5;
    for(int a:hidden())
    {
        camera[a]=0;
        for(const auto& p:model.points)camera[a]+=p[a]/model.points.size();
    }
    double radius=0;
    for(int a:{0,xAxis,zAxis})radius+=std::pow((maximum[a]-minimum[a])*0.5,2);
    double aspect=(double)std::max(1,width-panelWidth)/std::max(1,height);
    double angle=std::atan(std::tan(glm::radians(32.5))*std::min(1.0,aspect));
    double back=std::max(4.0,std::sqrt(radius)/std::sin(angle)*1.2);
    yaw=0.55;pitch=-0.12;auto f=forward();
    camera[xAxis]-=f.x*back;camera[0]-=f.y*back;camera[zAxis]-=f.z*back;
    dirty=true;
}
void StateModelMaker::add(Model5D::Point p)
{
    try {if(model.points.size()>=512)throw std::runtime_error("Point limit: 512");select((int)model.add(p,currentShape));pointPage=selected/pointRows;rebuild();}
    catch(const std::exception& e){status=e.what();}
}
void StateModelMaker::init(StateManager& s)
{
    s.getSize(&width,&height);glfwSetInputMode(s.getWindow(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    bool firstSession=!vao;
    if(!vao)
    {
        if(!shader.initSources({{GL_VERTEX_SHADER,"#version 450 core\nlayout(location=0) in vec3 p; layout(location=1) in vec4 c; uniform mat4 mvp; out vec4 color; void main(){gl_Position=mvp*vec4(p,1);color=c;gl_PointSize=9;}"},
            {GL_FRAGMENT_SHADER,"#version 450 core\nin vec4 color;out vec4 outputColor;void main(){outputColor=color;}"}}))throw std::runtime_error("Model shader failed");
        glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
        glEnableVertexAttribArray(1);glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,color));glBindVertexArray(0);
        background=QuadRenderer(Shader::get("quad"));background.init();name.setText("model");
        displayLinks.setChecked(true);displayPoints.setChecked(true);
    }
    looking=false;tab=false;glfwGetCursorPos(s.getWindow(),&mouseX,&mouseY);rebuild();select(selected);
    if(firstSession)restoreModel();
    buildUi(s);
}
void StateModelMaker::close(StateManager& s) {looking=false;tab=false;glfwSetInputMode(s.getWindow(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);s.setUiPage(nullptr);}
void StateModelMaker::buildUi(StateManager& s)
{
    refresh=false;page.clear();
    pointRows=std::clamp((height-540)/30,1,6);
    auto place=[&](ui::element& e,int x,int y){e.setAlignX(ui::ALIGN_LEFT);e.setAlignY(ui::ALIGN_TOP);e.setOffsetX(x);e.setOffsetY(y);page.addElem(&e);};
    title.setText("Model Maker");title.setSize(2);place(title,16,18);
    name.setSize(196,28);place(name,16,50);
    auto button=[&](ui::button& b,const char* text,int x,int y,int w,auto action){b.setText(text);b.setSize(w,28);b.setAction(action);place(b,x,y);};
    button(saveButton,"Save",16,88,80,[this]{saveModel();});
    button(loadButton,"Load",104,88,80,[this]{loadModel();});
    button(backButton,"Back",192,88,84,[&s]{s.popState();});
    const char* axes[]={"A / Y","B / X","C / Z","D / W","E / V","F / U"};
    int rowHeight=model.dimensions==6?24:28;
    for(int a=0;a<model.dimensions;++a)
    {
        coordinateLabels[a].setText(axes[a]);coordinateLabels[a].setSize(1);place(coordinateLabels[a],20,131+a*rowHeight);
        coordinates[a].setSize(172,rowHeight-2);place(coordinates[a],104,125+a*rowHeight);
    }
    auto read=[this]{Model5D::Point p=selected>=0?model.points[selected]:Model5D::Point{};for(int a=0;a<model.dimensions;++a){auto text=coordinates[a].getText();size_t end=0;p[a]=std::stod(text,&end);if(end!=text.size()||!std::isfinite(p[a])||std::abs(p[a])>1e8)throw std::runtime_error("Invalid coordinates");}return p;};
    button(addButton,"+ Point",16,272,124,[this,read]{try{add(read());}catch(const std::exception& e){status=e.what();}});
    button(applyButton,"Apply",152,272,124,[this,read]{try{if(selected>=0){model.points[selected]=read();rebuild();}}catch(const std::exception& e){status=e.what();}});
    selectedText.setText(selected<0?"No point selected":std::format("Point {}",selected+1));selectedText.setSize(1);place(selectedText,20,308);
    button(teleportButton,"Teleport",16,328,132,[this]{if(selected>=0){camera=model.points[selected];dirty=true;}});
    button(deleteButton,"Delete",152,328,124,[this]{if(selected>=0){model.erase(selected);select(-1);rebuild();}});
    int pages=std::max(1,((int)model.points.size()+pointRows-1)/pointRows);pointPage=std::clamp(pointPage,0,pages-1);
    for(int i=0;i<pointRows;++i)
    {
        int index=pointPage*pointRows+i;if(index>=model.points.size())break;
        auto group=model.shapeOf(index);
        auto text=std::format("{}P{} / {}{}",index==selected?"> ":"",index+1,model.shapes[group].subtractive?"V":"S",group+1);
        button(pointButtons[i],text.c_str(),16,368+i*30,260,[this,index]{select(index);});
    }
    int footer=374+pointRows*30;
    button(previousButton,"<",16,footer,48,[this]{--pointPage;refresh=true;});
    button(nextButton,">",228,footer,48,[this]{++pointPage;refresh=true;});
    planeText.setSize(1);place(planeText,80,footer+10);
    positionText.setSize(1);positionText.setWrapWidth(268);place(positionText,16,footer+34);
    statusText.setSize(1);statusText.setWrapWidth(268);place(statusText,16,footer+66);
    dimensionsLabel.setText("ND");dimensionsLabel.setSize(2);place(dimensionsLabel,16,height-36);
    button(fewerDimensions,"<",112,height-32,48,[this]{setDimensions(model.dimensions-1);});fewerDimensions.setSize(48,24);
    dimensionsValue.setText(std::to_string(model.dimensions));dimensionsValue.setSize(2);place(dimensionsValue,184,height-36);
    button(moreDimensions,">",228,height-32,48,[this]{setDimensions(model.dimensions+1);});moreDimensions.setSize(48,24);
    button(previousModelButton,"<",216,50,28,[this]{browseModel(-1);});
    button(nextModelButton,">",248,50,28,[this]{browseModel(1);});
    displayLinks.setText("Display links");place(displayLinks,panelWidth+16,16);
    displayPoints.setText("Display points");place(displayPoints,panelWidth+(width>=840?280:16),width>=840?16:48);
    int shapeY=width>=840?52:84;
    shapeLabel.setText(std::format("Current shape: {} / {}",currentShape+1,model.shapes.size()));shapeLabel.setSize(2);place(shapeLabel,panelWidth+16,shapeY);
    auto changeShape=[this](int step){currentShape=(currentShape+model.shapes.size()+step)%model.shapes.size();select(-1);};
    button(previousShape,"<",panelWidth+16,shapeY+24,32,[changeShape]{changeShape(-1);});
    button(nextShape,">",panelWidth+56,shapeY+24,32,[changeShape]{changeShape(1);});
    auto create=[this](bool empty){if(model.shapes.size()>=512){status="Shape limit: 512";return;}currentShape=model.addShape(empty);select(-1);rebuild();};
    button(newSolid,"+ Solid",panelWidth+96,shapeY+24,124,[create]{create(false);});
    button(newVoid,"+ Void",panelWidth+228,shapeY+24,124,[create]{create(true);});
    voidShape.setText("Void");voidShape.setChecked(model.shapes[currentShape].subtractive);place(voidShape,panelWidth+16,shapeY+60);
    voidShape.setAction([this](bool value){model.shapes[currentShape].subtractive=value;rebuild();});
    s.setUiPage(&page);
}
void StateModelMaker::update(StateManager& s,double dt)
{
    if(refresh)buildUi(s);
    auto window=s.getWindow();
    if(looking && glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_RELEASE)
    {looking=false;glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);}
    if(!tab && !dynamic_cast<ui::text_input*>(page.getFocusedElem()))
    {
        auto down=[&](int k){return glfwGetKey(window,k)==GLFW_PRESS?1.0:0.0;};
        double speed=std::min(dt,0.1)*4*(down(GLFW_KEY_LEFT_CONTROL)?3:1);
        auto f=forward();auto right=glm::normalize(glm::cross(f,glm::dvec3(0,1,0)));
        auto move=(down(GLFW_KEY_W)-down(GLFW_KEY_S))*f+(down(GLFW_KEY_D)-down(GLFW_KEY_A))*right;
        camera[xAxis]+=move.x*speed;camera[0]+=(move.y+down(GLFW_KEY_SPACE)-down(GLFW_KEY_LEFT_SHIFT))*speed;camera[zAxis]+=move.z*speed;
        auto h=hidden();double d0=(down(GLFW_KEY_E)-down(GLFW_KEY_Q))*speed,d1=(down(GLFW_KEY_F)-down(GLFW_KEY_R))*speed;
        if(!h.empty()&&d0){camera[h[0]]+=d0;dirty=true;}
        if(h.size()>1&&d1){camera[h[1]]+=d1;dirty=true;}
        double d2=(down(GLFW_KEY_G)-down(GLFW_KEY_T))*speed;
        if(h.size()>2&&d2){camera[h[2]]+=d2;dirty=true;}
    }
    if(dirty){section=Model5D::slice(model,cells,xAxis,zAxis,camera);dirty=false;}
    const char* names[]={"Y","X","Z","W","V","U"};
    planeText.setText(std::format("{}{}  {}/{}",names[xAxis],names[zAxis],pointPage+1,std::max(1,((int)model.points.size()+pointRows-1)/pointRows)));
    auto position=std::format("A {:.2f} B {:.2f} C {:.2f}\n",camera[0],camera[1],camera[2]);
    if(model.dimensions>=4)position+=std::format("D {:.2f} ",camera[3]);
    if(model.dimensions>=5)position+=std::format("E {:.2f} ",camera[4]);
    if(model.dimensions>=6)position+=std::format("F {:.2f}",camera[5]);
    else position+=std::format("Dist {:.2f}",distance);
    positionText.setText(position);
    statusText.setText(status);
}
void StateModelMaker::draw(const std::vector<Vertex>& vertices,unsigned mode)
{
    if(vertices.empty())return;shader.use();shader.setUniform("mvp",matrix());
    glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(mode,0,(int)vertices.size());glBindVertexArray(0);
}
void StateModelMaker::render(StateManager& s)
{
    glViewport(0,0,width,height);glClearColor(0.035f,0.045f,0.052f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(panelWidth,0,std::max(1,width-panelWidth),height);glEnable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glEnable(GL_PROGRAM_POINT_SIZE);
    std::vector<Vertex> grid,triangles,lines,points;
    int cx=(int)std::floor(camera[xAxis]),cz=(int)std::floor(camera[zAxis]);
    for(int i=-16;i<=16;++i)
    {
        glm::vec4 color(0.17f,0.21f,0.23f,1);
        grid.push_back({{cx+i,0,cz-16},color});grid.push_back({{cx+i,0,cz+16},color});
        grid.push_back({{cx-16,0,cz+i},color});grid.push_back({{cx+16,0,cz+i},color});
    }
    draw(grid,GL_LINES);
    for(auto p:section.triangles)triangles.push_back({glm::vec3(p),{0.12f,0.42f,0.95f,1}});
    glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1,1);draw(triangles,GL_TRIANGLES);glDisable(GL_POLYGON_OFFSET_FILL);
    if(displayLinks.getChecked())for(auto p:section.lines)lines.push_back({glm::vec3(p),{0.2f,0.7f,1,1}});
    draw(lines,GL_LINES);lines.clear();
    auto h=hidden();
    for(size_t a=0;a<model.points.size();++a)
    {
        auto p=model.points[a];bool inSlice=true;for(int axis:h)inSlice&=std::abs(p[axis]-camera[axis])<0.02;
        bool empty=model.shapes[model.shapeOf(a)].subtractive;
        if(displayPoints.getChecked())points.push_back({visible(p),a==selected?glm::vec4(1,0.72f,0.2f,1):(empty?glm::vec4(1,0.3f,0.35f,1):(inSlice?glm::vec4(0.3f,1,0.65f,1):glm::vec4(0.72f,0.4f,0.85f,1)))});
        if(!displayLinks.getChecked())continue;
        for(size_t b=a+1;b<model.points.size();++b)
        {
            if(model.shapeOf(a)!=model.shapeOf(b))continue;
            bool linked=model.linked(a,b);auto color=linked?glm::vec4(0.56f,0.61f,0.66f,0.6f):glm::vec4(0.45f,0.25f,0.28f,0.25f);
            auto pa=visible(p),pb=visible(model.points[b]);
            if(linked){lines.push_back({pa,color});lines.push_back({pb,color});}
            else for(int k=0;k<8;k+=2){lines.push_back({glm::mix(pa,pb,k/8.0f),color});lines.push_back({glm::mix(pa,pb,(k+1)/8.0f),color});}
        }
    }
    glDisable(GL_DEPTH_TEST);draw(lines,GL_LINES);draw(points,GL_POINTS);
    if(displayPoints.getChecked())draw({{visible(placement()),{1,0.72f,0.2f,1}}},GL_POINTS);
    glViewport(0,0,width,height);background.setMode(QuadRenderer::MODE_FILL);background.setPos(0,0,panelWidth,height);background.setColor({0.065f,0.075f,0.08f,1});background.render();
}
void StateModelMaker::mouseInput(StateManager&,double x,double y)
{
    if(looking){yaw-=(x-mouseX)*0.004;pitch=std::clamp(pitch-(y-mouseY)*0.004,-1.5,1.5);}mouseX=x;mouseY=y;
}
void StateModelMaker::mouseButtonInput(StateManager& s,int button,int action,int mods)
{
    if(button==GLFW_MOUSE_BUTTON_MIDDLE)
    {
        if(action==GLFW_RELEASE || mouseX>=panelWidth){looking=action==GLFW_PRESS;glfwSetInputMode(s.getWindow(),GLFW_CURSOR,looking?GLFW_CURSOR_DISABLED:GLFW_CURSOR_NORMAL);}return;
    }
    if(action!=GLFW_PRESS || mouseX<panelWidth)return;
    if(button==GLFW_MOUSE_BUTTON_RIGHT){add(placement());return;}
    if(button!=GLFW_MOUSE_BUTTON_LEFT)return;
    glm::dvec2 cursor(mouseX,mouseY);double best=10;int point=-1;
    if(displayPoints.getChecked())for(size_t i=0;i<model.points.size();++i){double d=glm::length(screen(model.points[i])-cursor);if(d<best){best=d;point=(int)i;}}
    if(point>=0){select(point);return;}
    if(!displayLinks.getChecked())return;
    best=7;Model5D::Edge edge{};bool found=false;
    for(size_t a=0;a<model.points.size();++a)for(size_t b=a+1;b<model.points.size();++b)
    {
        if(model.shapeOf(a)!=model.shapeOf(b))continue;
        auto p=screen(model.points[a]),q=screen(model.points[b]),delta=q-p;if(p.x<panelWidth||q.x<panelWidth)continue;
        double length=glm::dot(delta,delta);if(length<1e-8)continue;
        double t=std::clamp(glm::dot(cursor-p,delta)/length,0.0,1.0);double d=glm::length(cursor-(p+t*delta));
        if(d<best){best=d;edge={a,b};found=true;}
    }
    if(found){model.toggle(edge.first,edge.second);rebuild();}
}
void StateModelMaker::scrollInput(StateManager&,double,double y){distance=std::clamp(distance+y*0.25,0.25,128.0);}
void StateModelMaker::keyInput(StateManager& s,int key,int scan,int action,int)
{
    if(key==GLFW_KEY_ESCAPE&&action==GLFW_PRESS){if(looking){looking=false;glfwSetInputMode(s.getWindow(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);}else s.popState();return;}
    if(key==GLFW_KEY_TAB){tab=action!=GLFW_RELEASE;if(tab)buildUi(s);else firstAxis=-1;return;}
    if(!tab||action!=GLFW_PRESS)return;
    const char* label=glfwGetKeyName(key,scan);char c=label&&label[0]&&!label[1]?(char)std::tolower((unsigned char)label[0]):0;
    int axis=c=='x'?1:c=='z'?2:c=='w'?3:c=='v'?4:c=='u'?5:-1;
    if(axis<0||axis>=model.dimensions)return;if(firstAxis<0)firstAxis=axis;
    else if(axis!=firstAxis){xAxis=std::min(axis,firstAxis);zAxis=std::max(axis,firstAxis);yaw=0;pitch=0;dirty=true;firstAxis=-1;}
}
void StateModelMaker::windowResize(StateManager&,int w,int h){width=w;height=h;refresh=true;}
