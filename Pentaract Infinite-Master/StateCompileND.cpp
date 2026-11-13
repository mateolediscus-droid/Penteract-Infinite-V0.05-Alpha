#include "StateCompileND.h"
#include "StateManager.h"
#include "json.hpp"
#include "NDSourceRoot.h"
#include <Windows.h>
#include <fstream>
#include <stdexcept>
#include <cmath>

namespace {
std::wstring quote(const std::filesystem::path& p){return L"\""+p.wstring()+L"\"";}
std::wstring environment(const wchar_t* name){
    DWORD n=GetEnvironmentVariableW(name,nullptr,0);
    if(!n)return {};
    std::wstring value(n,L'\0');GetEnvironmentVariableW(name,value.data(),n);value.resize(n-1);return value;
}
void restore(const wchar_t* name,const std::wstring& value){SetEnvironmentVariableW(name,value.empty()?nullptr:value.c_str());}
}

void StateCompileND::open(StateManager& s,const std::filesystem::path& path,int n,bool creating){
    static StateCompileND state;
    state.world=std::filesystem::absolute(path);state.root=std::filesystem::current_path();state.dimension=n;state.fromCreation=creating;
    s.pushState(&state);
}
void StateCompileND::stop(){
    // Closing the job also stops MSBuild/compiler descendants on cancellation.
    if(job){CloseHandle(job);job=nullptr;}
    if(process){CloseHandle(process);process=nullptr;}
}
void StateCompileND::close(StateManager& s){stop();progress.cleanup();s.setUiPage(nullptr);}
void StateCompileND::init(StateManager& s){
    stop();started=false;elapsed=0;nextPoll=0;startTime=GetTickCount64();
    progress=QuadRenderer(Shader::get("quad"));progress.init();
    page.clear();status.setText("Preparing "+std::to_string(dimension)+"D...");
    for(auto text:{&status,&phase,&timer}){
        text->setAlignX(ui::ALIGN_CENTER_X);text->setAlignY(ui::ALIGN_CENTER_Y);text->setSize(2);text->setColor({1,1,1,1});
    }
    status.setOffsetY(-100);status.setWrapWidth(0);
    phase.setOffsetY(-50);phase.setWrapWidth(760);phase.setText("Preparing compiler...");
    timer.setOffsetY(45);timer.setText("0 s");
    cancel.setText("Cancel");cancel.setSize(110,50);cancel.setAlignX(ui::ALIGN_CENTER_X);cancel.setAlignY(ui::ALIGN_CENTER_Y);cancel.setOffsetY(110);
    cancel.setAction([this,&s]{
        bool creating=fromCreation;stop();s.popState();
        if(creating)s.popState(); // Skip the suspended generation state on the way back.
    });
    page.addElem(&status);page.addElem(&phase);page.addElem(&timer);page.addElem(&cancel);s.setUiPage(&page);
}
void StateCompileND::render(StateManager& s){
    glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glDisable(GL_DEPTH_TEST);
    if(!process)return;
    int w,h;s.getSize(&w,&h);int width=std::min(500,std::max(100,w-80));
    progress.setMode(QuadRenderer::MODE_FILL);progress.setColor(.15f,.15f,.15f);progress.setPos((w-width)/2,h/2,width,8);progress.render();
    int offset=int((.5-.5*std::cos(elapsed*3.))*(width-60));
    progress.setColor(.15f,.8f,.35f);progress.setPos((w-width)/2+offset,h/2,60,8);progress.render();
}
void StateCompileND::update(StateManager& s,double dt){
    try {
        if(!started){
            started=true;
            if(!std::filesystem::is_regular_file(root/"nd-build.json"))throw std::runtime_error("Missing nd-build.json compiler configuration.");
            auto source=ndSourceRoot(root);
            auto script=source/"tools"/"build-runtime-nd.ps1";
            if(!std::filesystem::is_regular_file(script))throw std::runtime_error("ND build sources are missing (nd-build.json).");
            auto requests=root/"nd-build-requests";std::filesystem::create_directories(requests);
            result=requests/(std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+".json");
            auto powershell=std::filesystem::path(environment(L"SystemRoot"))/"System32/WindowsPowerShell/v1.0/powershell.exe";
            std::wstring command=quote(powershell)+L" -NoProfile -ExecutionPolicy Bypass -File "+quote(script)+
                L" -Dimension "+std::to_wstring(dimension)+L" -Result "+quote(result);
            job=CreateJobObjectW(nullptr,nullptr);
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if(!job || !SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("Cannot create compiler job.");
            STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
            if(!CreateProcessW(powershell.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,source.c_str(),&startup,&child))
                throw std::runtime_error("Cannot start PowerShell compiler.");
            process=child.hProcess;
            if(!AssignProcessToJobObject(job,child.hProcess)){TerminateProcess(child.hProcess,1);CloseHandle(child.hThread);throw std::runtime_error("Cannot supervise compiler process.");}
            ResumeThread(child.hThread);CloseHandle(child.hThread);
        }
        if(!process)return;
        elapsed=double(GetTickCount64()-startTime)/1000.;
        if(WaitForSingleObject(process,0)==WAIT_TIMEOUT){
            status.setText("Building "+std::to_string(dimension)+"D");
            timer.setText(std::to_string(int(elapsed))+" s");
            if(elapsed>=nextPoll){
                nextPoll=elapsed+.5;
                // The writer may briefly hold the file during atomic replacement.
                try {nlohmann::json p;std::ifstream(result.wstring()+L".progress.json")>>p;phase.setText(p.at("phase").get<std::string>());}
                catch(const std::exception&){}
            }
            return;
        }
        DWORD code=1;GetExitCodeProcess(process,&code);stop();
        nlohmann::json report=nlohmann::json::object();
        if(std::filesystem::is_regular_file(result))std::ifstream(result)>>report;
        if(code || !report.value("success",false))throw std::runtime_error(report.value("error",std::string("Compilation failed. See nd-cache build logs.")));
        std::filesystem::path exe=report.at("executable").get<std::string>();
        if(!std::filesystem::is_regular_file(exe))throw std::runtime_error("Compiled executable not found.");
        // Child uses the installed game's assets/worlds, not the compiler cache directory.
        auto oldRoot=environment(L"PENTERACT_ND_ROOT"),oldWorld=environment(L"PENTERACT_ND_WORLD");
        SetEnvironmentVariableW(L"PENTERACT_ND_ROOT",root.c_str());SetEnvironmentVariableW(L"PENTERACT_ND_WORLD",world.c_str());
        STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
        auto command=quote(exe);
        BOOL launched=CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,root.c_str(),&startup,&child);
        restore(L"PENTERACT_ND_ROOT",oldRoot);restore(L"PENTERACT_ND_WORLD",oldWorld);
        if(!launched)throw std::runtime_error("Cannot launch compiled engine.");
        CloseHandle(child.hThread);CloseHandle(child.hProcess);s.quit();
    }catch(const std::exception& error){
        stop();status.setText("Compilation stopped");phase.setText(error.what());phase.setSize(1);cancel.setText("Back");
    }
}
