#pragma once
#include <algorithm>
#include <cmath>

// Sample once per rendered frame, using wall-clock time rather than simulation dt.
struct FrameMeter {
    double fps=0, peakMs=0;
    double previous=-1, elapsed=0, peak=0;
    unsigned frames=0;
    void sample(double now) {
        if(!std::isfinite(now))return;
        if(previous<0) {previous=now;return;}
        double dt=now-previous;previous=now;
        if(dt<=0)return;
        elapsed+=dt;peak=std::max(peak,dt);++frames;
        if(elapsed>=.5) {
            fps=frames/elapsed;peakMs=peak*1000;
            elapsed=peak=0;frames=0;
        }
    }
};
