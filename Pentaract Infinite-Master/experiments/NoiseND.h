#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <vector>

// Finite spectral field: full-rank random bases avoid a shared projection kernel.
template<size_t N> class NoiseND
{
    static_assert(N > 0);
public:
    using Point = std::array<double, N>;
    struct Projection {
        Point anchor{};
        std::vector<double> phases;
        const void* source=nullptr;
    };
private:
    static constexpr size_t groups = (64 + N - 1) / N;
    struct Wave { Point frequency{}; double phase = 0, weight = 1; };
    std::vector<Wave> waves;
    double normalization = 1;
    struct Changes { std::array<size_t,8> axes{};std::array<double,8> deltas{};size_t count=0; };
    Changes prepare(const Point& p,Projection& cache) const {
        Changes changes;
        bool rebase=cache.source!=waves.data();
        if(!rebase)for(size_t axis=0;axis<N;++axis)if(p[axis]!=cache.anchor[axis]) {
            if(changes.count==changes.axes.size()){rebase=true;break;}
            changes.axes[changes.count]=axis;changes.deltas[changes.count++]=p[axis]-cache.anchor[axis];
        }
        if(rebase) {
            cache.anchor=p;cache.phases.resize(waves.size());cache.source=waves.data();
            for(size_t j=0;j<waves.size();++j) {
                double phase=waves[j].phase;
                for(size_t axis=0;axis<N;++axis)phase+=p[axis]*waves[j].frequency[axis];
                cache.phases[j]=phase;
            }
            changes.count=0;
        }
        return changes;
    }
    double projectedPhase(size_t wave,const Projection& cache,const Changes& changes) const {
        double phase=cache.phases[wave];
        // Always relative to a fixed anchor, never to the previous sample: no accumulated drift.
        for(size_t i=0;i<changes.count;++i)phase+=changes.deltas[i]*waves[wave].frequency[changes.axes[i]];
        return phase;
    }
    struct Random {
        uint64_t state;
        double uniform() {
            auto z = (state += 0x9E3779B97F4A7C15ull);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            return (double((z ^ (z >> 31)) >> 11) + .5) / 9007199254740992.;
        }
        double normal() { return std::sqrt(-2 * std::log(uniform())) * std::cos(2 * std::numbers::pi * uniform()); }
    };
public:
    explicit NoiseND(uint64_t seed) : waves(groups * N) {
        Random random{seed}; double variance = 0;
        for (size_t group = 0; group < groups; ++group) {
            // Both matrices grow quadratically; keep them off caller/worker stacks.
            std::vector<Point> basis(N);
            double band = std::pow(2., int(group % 3));
            double radius = std::sqrt(double(N)) * (.7 + .6 * random.uniform()) * band;
            for (size_t row = 0; row < N; ++row) {
                double length;
                do {
                    for (auto& x : basis[row]) x = random.normal();
                    for (int pass = 0; pass < 2; ++pass)
                        for (size_t previous = 0; previous < row; ++previous) {
                            double dot = 0;
                            for (size_t i = 0; i < N; ++i) dot += basis[row][i] * basis[previous][i];
                            for (size_t i = 0; i < N; ++i) basis[row][i] -= dot * basis[previous][i];
                        }
                    length = 0; for (auto x : basis[row]) length += x * x;
                    length = std::sqrt(length);
                } while (length < 1e-10);
                auto& wave = waves[group * N + row];
                for (size_t i = 0; i < N; ++i) {
                    basis[row][i] /= length;
                    wave.frequency[i] = basis[row][i] * radius;
                }
                wave.phase = random.uniform() * 2 * std::numbers::pi;
                wave.weight = 1 / band;
                variance += wave.weight * wave.weight;
            }
        }
        normalization = .5 * std::sqrt(2 / variance);
    }
    double evaluate(const Point& p) const {
        double result = 0;
        for (const auto& wave : waves) {
            double phase = wave.phase;
            for (size_t i = 0; i < N; ++i) phase += p[i] * wave.frequency[i];
            result += wave.weight * std::cos(phase);
        }
        return result * normalization;
    }
    double evaluate(const Point& p,Projection& cache) const {
        if constexpr(N<=64)return evaluate(p);
        auto changes=prepare(p,cache);double result=0;
        for(size_t j=0;j<waves.size();++j)result+=waves[j].weight*std::cos(projectedPhase(j,cache,changes));
        return result*normalization;
    }
    template<size_t Count> std::array<double,Count> line(const Point& origin,const Point& step,Projection& cache) const {
        if constexpr(N<=64)return line<Count>(origin,step);
        auto changes=prepare(origin,cache);
        std::vector<size_t> axes;for(size_t axis=0;axis<N;++axis)if(step[axis]!=0)axes.push_back(axis);
        std::array<double,Count> result{};
        for(size_t i=0;i<waves.size();++i) {
            const auto& wave=waves[i];double phase=projectedPhase(i,cache,changes),delta=0;
            for(auto axis:axes)delta+=step[axis]*wave.frequency[axis];
            double c=std::cos(phase),s=std::sin(phase);
            const double dc=std::cos(delta),ds=std::sin(delta);
            for(size_t j=0;j<Count;++j) {
                if(j&&j%32==0){c=std::cos(phase+delta*j);s=std::sin(phase+delta*j);}
                result[j]+=wave.weight*c;
                double next=c*dc-s*ds;s=s*dc+c*ds;c=next;
            }
        }
        for(auto& x:result)x*=normalization;
        return result;
    }
    template<size_t Count> std::array<double, Count> line(const Point& origin, const Point& step) const {
        std::array<double, Count> result{};
        for (const auto& wave : waves) {
            double phase = wave.phase, delta = 0;
            for (size_t i = 0; i < N; ++i) {
                phase += origin[i] * wave.frequency[i];
                delta += step[i] * wave.frequency[i];
            }
            double c = std::cos(phase), s = std::sin(phase);
            const double dc = std::cos(delta), ds = std::sin(delta);
            for (size_t j = 0; j < Count; ++j) {
                if (j && j % 32 == 0) { c = std::cos(phase + delta * j); s = std::sin(phase + delta * j); }
                result[j] += wave.weight * c;
                double next = c * dc - s * ds;
                s = s * dc + c * ds; c = next;
            }
        }
        for (auto& x : result) x *= normalization;
        return result;
    }
};
