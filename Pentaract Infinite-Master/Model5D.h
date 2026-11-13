#pragma once
#include <array>
#include <vector>
#include <set>
#include <filesystem>
#include <glm/glm.hpp>

namespace Model5D
{
    std::filesystem::path storageDirectory();
    std::filesystem::path modelPath(std::string name);
    std::vector<std::filesystem::path> savedModels();
    inline constexpr int maxDimensions=6;
    using Point = std::array<double,maxDimensions>;
    using Edge = std::pair<size_t,size_t>;
    struct Cell { std::array<size_t,6> vertices{}; int count=0; unsigned boundary=63; size_t hull=0; };
    struct Shape { bool subtractive=false; bool operator==(const Shape&) const = default; };
    struct Mesh { std::vector<glm::dvec3> triangles, lines; };
    struct Model
    {
        int dimensions=5;
        std::vector<Point> points;
        std::set<Edge> edges;
        std::vector<Shape> shapes{{false}};
        std::vector<size_t> pointShapes;
        size_t shapeOf(size_t index) const { return pointShapes.at(index); }
        size_t addShape(bool subtractive=false);
        size_t add(Point point,size_t shape=0);
        void erase(size_t index);
        void toggle(size_t first, size_t second);
        bool linked(size_t first, size_t second) const;
        std::vector<Cell> cells() const;
        void save(const std::filesystem::path& path) const;
        static Model load(const std::filesystem::path& path);
    };
    Mesh slice(const Model& model, const std::vector<Cell>& cells, int xAxis, int zAxis, const Point& origin);
    Mesh convexSection(const Mesh& mesh);
}
