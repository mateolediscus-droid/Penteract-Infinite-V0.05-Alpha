#include "Model5D.h"
#include "libs/qhull/libqhull_r/qhull_ra.h"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <functional>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace Model5D
{
    namespace
    {
        constexpr size_t maxPoints = 512;
        using Mask = std::bitset<maxPoints>;
        double dot(const Point& a, const Point& b)
        {
            return std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
        }

        void hull(const Model& model, const std::vector<size_t>& group, std::vector<Cell>& cells)
        {
            // Work in the affine span, so flat 4D faces do not require jittering 5D points.
            const auto origin = model.points[group.front()];
            std::vector<Point> offsets;
            double scale = 0;
            for(auto index : group)
            {
                Point p{};
                for(int a = 0; a < model.dimensions; ++a) p[a] = model.points[index][a] - origin[a];
                scale = std::max(scale, std::sqrt(dot(p, p)));
                offsets.push_back(p);
            }
            if(scale == 0) return;
            for(auto& p : offsets) for(auto& x : p) x /= scale;
            std::vector<Point> basis;
            for(int axis = 0; axis < model.dimensions; ++axis)
            {
                Point best{};
                double length = 0;
                for(auto p : offsets)
                {
                    for(int pass = 0; pass < 2; ++pass) for(const auto& b : basis)
                    {
                        double projection = dot(p, b);
                        for(int a = 0; a < maxDimensions; ++a) p[a] -= projection * b[a];
                    }
                    if(dot(p, p) > length) { best = p; length = dot(p, p); }
                }
                if(length < 1e-20) break;
                for(auto& x : best) x /= std::sqrt(length);
                basis.push_back(best);
            }
            int dimension = (int)basis.size();
            if(dimension < model.dimensions-1) return;
            std::vector<double> coordinates;
            for(const auto& p : offsets) for(const auto& b : basis) coordinates.push_back(dot(p, b));

            auto state = std::make_unique<qhT>();
            qhT* qh = state.get();
            std::unique_ptr<FILE, decltype(&fclose)> errors(tmpfile(), fclose);
            qh_zero(qh, errors ? errors.get() : stderr);
            char flags[] = "qhull Qt Qx";
            int code = qh_new_qhull(qh, dimension, (int)group.size(), coordinates.data(), False,
                flags, nullptr, errors ? errors.get() : stderr);
            std::vector<Cell> output;
            if(code == qh_ERRnone)
            {
                facetT* facet;
                vertexT* vertex;
                vertexT** vertexp;
                FORALLfacets
                {
                    Cell cell;cell.hull=cells.size();
                    FOREACHvertex_(facet->vertices)
                    {
                        if(cell.count < 6) cell.vertices[cell.count++] = group[qh_pointid(qh, vertex->point)];
                    }
                    if(cell.count != dimension) continue;
                    if(dimension == model.dimensions-1)
                    {
                        // Fill a lower-rank face by coning its boundary to an existing point.
                        if(std::find(cell.vertices.begin(), cell.vertices.begin() + cell.count, group[0]) != cell.vertices.begin() + cell.count) continue;
                        cell.vertices[cell.count++] = group[0];
                    }
                    std::sort(cell.vertices.begin(), cell.vertices.begin() + cell.count);
                    output.push_back(cell);
                }
            }
            qh_freeqhull(qh, !qh_ALL);
            int remaining, bytes;
            qh_memfreeshort(qh, &remaining, &bytes);
            if(code != qh_ERRnone) throw std::runtime_error("Hull calculation failed (Qhull " + std::to_string(code) + ")");
            cells.insert(cells.end(), output.begin(), output.end());
        }
    }

    Mesh convexSection(const Mesh& mesh)
    {
        Model model;model.dimensions=3;
        for(auto p:mesh.triangles)
        {
            Point point{p.y,p.x,p.z};
            bool duplicate=false;
            for(auto old:model.points)if(std::abs(old[0]-point[0])+std::abs(old[1]-point[1])+std::abs(old[2]-point[2])<1e-8){duplicate=true;break;}
            if(!duplicate)model.points.push_back(point);
        }
        if(model.points.size()<3)return {};
        std::vector<size_t> indices(model.points.size());std::iota(indices.begin(),indices.end(),0);
        std::vector<Cell> boundary;hull(model,indices,boundary);
        return slice(model,boundary,1,2,{});
    }
    std::vector<Cell> Model::cells() const
    {
        if(dimensions<3||dimensions>maxDimensions)throw std::runtime_error("ND must be between 3 and 6");
        if(points.size() > maxPoints) throw std::runtime_error("Point limit: 512");
        std::vector<Mask> neighbors(points.size());
        for(auto [a, b] : edges)
        {
            if(a >= points.size() || b >= points.size() || a == b) throw std::runtime_error("Invalid link");
            if(shapeOf(a)!=shapeOf(b))continue;
            neighbors[a].set(b); neighbors[b].set(a);
        }
        std::vector<Cell> result;
        std::vector<size_t> group;
        // The union of all simplices in a complete group is exactly its convex hull.
        // Pivoted maximal-clique enumeration avoids enumerating every 5/6-point subset.
        std::function<void(Mask, Mask)> visit = [&](Mask candidates, Mask excluded)
        {
            if(group.size() + candidates.count() < dimensions) return;
            if(candidates.none())
            {
                if(excluded.none()) hull(*this, group, result);
                return;
            }
            size_t pivot = points.size(), best = 0;
            auto choices = candidates | excluded;
            for(size_t i = 0; i < points.size(); ++i) if(choices[i])
            {
                size_t score = (candidates & neighbors[i]).count();
                if(pivot == points.size() || score > best) { pivot = i; best = score; }
            }
            auto branches = candidates & ~neighbors[pivot];
            for(size_t i = 0; i < points.size(); ++i) if(branches[i])
            {
                group.push_back(i);
                visit(candidates & neighbors[i], excluded & neighbors[i]);
                group.pop_back();
                candidates.reset(i); excluded.set(i);
            }
        };
        Mask candidates;
        for(size_t i = 0; i < points.size(); ++i) candidates.set(i);
        visit(candidates, {});
        std::sort(result.begin(), result.end(), [](const Cell& a, const Cell& b) { return a.vertices < b.vertices; });
        result.erase(std::unique(result.begin(), result.end(), [](const Cell& a, const Cell& b) { return a.vertices == b.vertices; }), result.end());
        return result;
    }
}
