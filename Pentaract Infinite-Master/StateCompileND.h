#pragma once
#include "State.h"
#include "ui.h"
#include "QuadRenderer.h"
#include <filesystem>

class StateCompileND : public State {
    ui::page page;
    ui::text status;
    ui::text phase, timer;
    QuadRenderer progress;
    ui::button cancel;
    std::filesystem::path world, root, result;
    int dimension=0;
    void* process=nullptr;
    void* job=nullptr;
    bool started=false;
    bool fromCreation=false;
    uint64_t startTime=0;
    double nextPoll=0;
    double elapsed=0;
    void stop();
public:
    static void open(StateManager&,const std::filesystem::path&,int,bool fromCreation=false);
    void init(StateManager&) override;
    void update(StateManager&,double) override;
    void close(StateManager&) override;
    void render(StateManager&) override;
};
