#pragma once
#include "ui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <functional>
#include <sstream>

class GameConsole {
    ui::page page;
    ui::text output,line;
    QuadRenderer background;
    std::vector<std::string> history,log;
    std::string input,draft;
    size_t caret=0;
    int historyAt=-1;
    bool openingCharacter=false;
public:
    bool open=false;
    std::function<std::string(std::string)> execute;
    void init(GLFWwindow* window) {
        open=false;input.clear();draft.clear();history.clear();log.clear();caret=0;historyAt=-1;
        background=QuadRenderer(Shader::get("quad"));background.init();
        page.clear();
        for(auto* text:{&output,&line}) {
            text->setSize(1);text->setAlignX(ui::ALIGN_LEFT);text->setAlignY(ui::ALIGN_TOP);
            text->setOffsetX(12);text->setShadow(false);page.addElem(text);
        }
        page.init(window);
    }
    void cleanup(){background.cleanup();open=false;}
    void show(){open=true;openingCharacter=true;}
    void submit() {
        std::string command=input;input.clear();caret=0;historyAt=-1;
        if(command.find_first_not_of(" \t")==std::string::npos)return;
        history.push_back(command);if(history.size()>64)history.erase(history.begin());
        log.push_back("> "+command);
        try {
            std::istringstream stream(command);std::string name,extra;stream>>name;
            if((name=="clear" || name=="/clear") && !(stream>>extra))log.clear();
            else if(execute){auto result=execute(command);if(!result.empty())log.push_back(result);}
        }catch(const std::exception& e){log.push_back(std::string("Error: ")+e.what());}
        if(log.size()>128)log.erase(log.begin(),log.end()-128);
    }
    void character(uint32_t c) {
        bool skip=openingCharacter && (c=='`' || c=='~' || c==0xB2);
        openingCharacter=false;
        if(!skip && c>=32 && c<127){input.insert(caret,1,char(c));++caret;}
    }
    void key(int key,int action,int mods) {
        if(action==GLFW_RELEASE)return;
        auto left=[&]{if(caret)--caret;if(mods&GLFW_MOD_CONTROL)while(caret && input[caret-1]!=' ')--caret;};
        auto right=[&]{if(caret<input.size())++caret;if(mods&GLFW_MOD_CONTROL)while(caret<input.size() && input[caret]!=' ')++caret;};
        switch(key) {
        case GLFW_KEY_ENTER:case GLFW_KEY_KP_ENTER:if(action==GLFW_PRESS)submit();break;
        case GLFW_KEY_HOME:caret=0;break;
        case GLFW_KEY_END:caret=input.size();break;
        case GLFW_KEY_LEFT:left();break;
        case GLFW_KEY_RIGHT:right();break;
        case GLFW_KEY_BACKSPACE:{auto old=caret;left();input.erase(caret,old-caret);break;}
        case GLFW_KEY_DELETE:{auto old=caret;right();input.erase(old,caret-old);caret=old;break;}
        case GLFW_KEY_UP:
            if(!history.empty()){if(historyAt<0)draft=input;historyAt=std::min(historyAt+1,int(history.size())-1);input=history[history.size()-1-historyAt];caret=input.size();}break;
        case GLFW_KEY_DOWN:
            if(historyAt>=0){--historyAt;input=historyAt<0?draft:history[history.size()-1-historyAt];caret=input.size();}break;
        }
    }
    void render(int width,int height) {
        if(!open)return;
        int panelHeight=std::min(300,std::max(80,height/2));
        int columns=std::max(1,(width-24)/8),rows=std::max(1,(panelHeight-44)/10);
        std::vector<std::string> wrapped;
        for(const auto& entry:log) {
            std::istringstream stream(entry);
            for(std::string row;std::getline(stream,row);) {
                if(row.empty())wrapped.push_back({});
                for(size_t start=0;start<row.size();start+=columns)wrapped.push_back(row.substr(start,columns));
            }
        }
        std::string shown;
        for(size_t i=wrapped.size()>size_t(rows)?wrapped.size()-rows:0;i<wrapped.size();++i)shown+=wrapped[i]+"\n";
        output.setText(shown);output.setOffsetY(12);
        size_t count=size_t(std::max(1,columns-3)),start=caret>=count?caret-count+1:0;
        auto visible=input.substr(start,count);visible.insert(caret-start,"|");
        line.setText("> "+visible);line.setOffsetY(panelHeight-20);
        page.windowResize(width,height);
        background.setMode(QuadRenderer::MODE_FILL);background.setPos(0,0,width,panelHeight);
        background.setColor({.035f,.035f,.035f,.96f});background.render();page.render();
    }
};
