#include "Model5D.h"
#include <algorithm>
#include <fstream>
#include <cmath>
#include <bit>
#include <stdexcept>
#include <cctype>
#include <map>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Model5D
{
    namespace
    {
        constexpr double epsilon=1e-8;
        bool finite(Point p) { for(auto x:p) if(!std::isfinite(x) || std::abs(x)>1e8) return false; return true; }
        void put(std::ostream& out,uint64_t n,int size) { for(int i=0;i<size;++i) out.put((char)(n>>(i*8))); }
        uint64_t get(std::istream& in,int size) { uint64_t n=0; for(int i=0;i<size;++i) { int c=in.get(); if(c==EOF) throw std::runtime_error("Truncated model"); n|=(uint64_t)(uint8_t)c<<(i*8); } return n; }
        bool polygon(std::vector<glm::dvec3> vertices,Mesh& mesh)
        {
            if(vertices.size()<3) return true;
            auto base=vertices[0]; glm::dvec3 normal(0), u(0);
            for(size_t i=1;i<vertices.size();++i) for(size_t j=i+1;j<vertices.size();++j)
            {
                auto n=glm::cross(vertices[i]-base,vertices[j]-base);
                if(glm::dot(n,n)>glm::dot(normal,normal)) { normal=n; u=vertices[i]-base; }
            }
            if(glm::length(normal)<epsilon) return true;
            normal=glm::normalize(normal); u=glm::normalize(u); auto v=glm::cross(normal,u);
            for(auto p:vertices) if(std::abs(glm::dot(p-base,normal))>epsilon) return false;
            struct Flat { glm::dvec2 p; glm::dvec3 world; };
            std::vector<Flat> flat;
            for(auto p:vertices) flat.push_back({{glm::dot(p-base,u),glm::dot(p-base,v)},p});
            std::sort(flat.begin(),flat.end(),[](auto a,auto b){return a.p.x!=b.p.x?a.p.x<b.p.x:a.p.y<b.p.y;});
            auto turn=[](Flat a,Flat b,Flat c){auto p=b.p-a.p,q=c.p-a.p; return p.x*q.y-p.y*q.x;};
            std::vector<Flat> hull;
            for(auto p:flat) { while(hull.size()>1 && turn(hull[hull.size()-2],hull.back(),p)<=epsilon) hull.pop_back(); hull.push_back(p); }
            auto lower=hull.size();
            for(int i=(int)flat.size()-2;i>=0;--i) { while(hull.size()>lower && turn(hull[hull.size()-2],hull.back(),flat[i])<=epsilon) hull.pop_back(); hull.push_back(flat[i]); }
            if(hull.size()>1) hull.pop_back();
            for(size_t i=1;i+1<hull.size();++i) { mesh.triangles.push_back(hull[0].world); mesh.triangles.push_back(hull[i].world); mesh.triangles.push_back(hull[i+1].world); }
            for(size_t i=0;i<hull.size();++i) { mesh.lines.push_back(hull[i].world); mesh.lines.push_back(hull[(i+1)%hull.size()].world); }
            return true;
        }
    }

    bool Model::linked(size_t a,size_t b) const { if(a>b) std::swap(a,b); return edges.contains({a,b}); }
    size_t Model::addShape(bool subtractive) { if(shapes.size()>=512)throw std::runtime_error("Shape limit: 512");shapes.push_back({subtractive});return shapes.size()-1; }
    size_t Model::add(Point p,size_t shape)
    {
        if(shape>=shapes.size())throw std::runtime_error("Invalid shape");
        if(!finite(p)) throw std::runtime_error("Invalid coordinates");
        size_t index=points.size();
        for(size_t i=0;i<index;++i) if(shapeOf(i)==shape)edges.insert({i,index});
        points.push_back(p);pointShapes.push_back(shape); return index;
    }
    void Model::erase(size_t index)
    {
        if(index>=points.size()) return;
        pointShapes.erase(pointShapes.begin()+index);
        points.erase(points.begin()+index); std::set<Edge> next;
        for(auto [a,b]:edges) if(a!=index && b!=index) next.insert({a-(a>index),b-(b>index)});
        edges=std::move(next);
    }
    void Model::toggle(size_t a,size_t b)
    {
        if(a==b || a>=points.size() || b>=points.size()) return;
        if(shapeOf(a)!=shapeOf(b))return;
        if(a>b) std::swap(a,b);
        if(!edges.erase({a,b})) edges.insert({a,b});
    }
    static Mesh sliceRaw(const Model& model,const std::vector<Cell>& cells,int xAxis,int zAxis,const Point& origin)
    {
        if(model.dimensions<3||model.dimensions>maxDimensions||xAxis<1||xAxis>=model.dimensions||zAxis<1||zAxis>=model.dimensions||xAxis==zAxis)throw std::invalid_argument("Invalid slice axes");
        std::array<int,3> hidden{-1,-1,-1}; for(int a=1,i=0;a<model.dimensions;++a) if(a!=xAxis && a!=zAxis) hidden[i++]=a;
        Mesh mesh;
        for(const auto& cell:cells)
        {
            struct Vertex { glm::dvec3 p; std::array<double,6> weights; };
            std::vector<Vertex> vertices;
            auto add=[&](std::array<double,6> weights)
            {
                Point p{};
                for(int i=0;i<cell.count;++i) { if(weights[i]<-epsilon) return; for(int a=0;a<maxDimensions;++a) p[a]+=weights[i]*model.points[cell.vertices[i]][a]; }
                for(int a:hidden)if(a>=0&&std::abs(p[a]-origin[a])>epsilon)return;
                glm::dvec3 projected(p[xAxis],p[0],p[zAxis]);
                for(auto& old:vertices) if(glm::length(old.p-projected)<epsilon) return;
                vertices.push_back({projected,weights});
            };
            auto hiddenPoint=[&](int i){auto p=model.points[cell.vertices[i]];glm::dvec3 result(0);for(int a=0;a<3;++a)if(hidden[a]>=0)result[a]=p[hidden[a]]-origin[hidden[a]];return result;};
            for(int i=0;i<cell.count;++i)
            {
                std::array<double,6> weights{}; weights[i]=1; add(weights);
                for(int j=i+1;j<cell.count;++j)
                {
                    auto a=hiddenPoint(i),b=hiddenPoint(j),delta=b-a; double norm=glm::dot(delta,delta);
                    if(norm>epsilon*epsilon) { double t=-glm::dot(a,delta)/norm; weights={}; weights[i]=1-t;weights[j]=t;add(weights); }
                    for(int k=j+1;k<cell.count;++k)
                    {
                        auto c=hiddenPoint(k);
                        auto u=b-a,v=c-a;
                        glm::dmat2 gram(glm::dvec2(glm::dot(u,u),glm::dot(u,v)),glm::dvec2(glm::dot(u,v),glm::dot(v,v)));
                        if(std::abs(glm::determinant(gram))>=epsilon*epsilon)
                        {
                            auto w=glm::inverse(gram)*glm::dvec2(-glm::dot(u,a),-glm::dot(v,a));
                            weights={};weights[i]=1-w.x-w.y;weights[j]=w.x;weights[k]=w.y;add(weights);
                        }
                        // Three hidden coordinates require intersections inside tetrahedra too.
                        if(model.dimensions==6)for(int l=k+1;l<cell.count;++l)
                        {
                            glm::dmat4 m(glm::dvec4(a,1),glm::dvec4(b,1),glm::dvec4(c,1),glm::dvec4(hiddenPoint(l),1));
                            if(std::abs(glm::determinant(m))<epsilon)continue;
                            auto w=glm::inverse(m)*glm::dvec4(0,0,0,1);
                            weights={};weights[i]=w.x;weights[j]=w.y;weights[k]=w.z;weights[l]=w.w;add(weights);
                        }
                    }
                }
            }
            std::vector<glm::dvec3> face;for(auto v:vertices)face.push_back(v.p);
            if(!polygon(face,mesh))for(int opposite=0;opposite<cell.count;++opposite)
            {face.clear();for(auto v:vertices)if(std::abs(v.weights[opposite])<epsilon)face.push_back(v.p);polygon(face,mesh);}
        }
        return mesh;
    }
    namespace
    {
        using Polygon=std::vector<glm::dvec3>;
        struct Plane { glm::dvec3 normal;double offset; };
        struct Section { Mesh mesh;std::vector<Plane> planes;bool subtractive; };
        Polygon clip(const Polygon& input,const Plane& plane,bool inside)
        {
            Polygon out;
            if(input.empty())return out;
            auto previous=input.back();double a=glm::dot(previous,plane.normal)-plane.offset;
            if(!inside)a=-a;
            const double threshold=inside?epsilon:-epsilon;
            for(auto current:input)
            {
                double b=glm::dot(current,plane.normal)-plane.offset;if(!inside)b=-b;
                if((a<=threshold)!=(b<=threshold))out.push_back(previous+(current-previous)*(a/(a-b)));
                if(b<=threshold)out.push_back(current);
                previous=current;a=b;
            }
            return out;
        }
        std::vector<Polygon> outside(const Polygon& input,const Section& section)
        {
            if(section.planes.empty())return {input};
            std::vector<Polygon> result;auto remaining=input;
            // Disjoint fragments outside a convex half-space intersection.
            for(const auto& plane:section.planes)
            {
                auto fragment=clip(remaining,plane,false);
                if(fragment.size()>=3)result.push_back(std::move(fragment));
                remaining=clip(remaining,plane,true);if(remaining.size()<3)break;
            }
            return result;
        }
    }
    Mesh slice(const Model& model,const std::vector<Cell>& cells,int xAxis,int zAxis,const Point& origin)
    {
        if(model.shapes.size()==1&&!model.shapes.front().subtractive)return sliceRaw(model,cells,xAxis,zAxis,origin);
        std::map<size_t,std::vector<Cell>> hulls;
        for(const auto& cell:cells)hulls[cell.hull].push_back(cell);
        std::vector<Section> sections;
        for(const auto& [id,boundary]:hulls)
        {
            // Re-hull the convex 3D section: a slice on triangulation seams can
            // otherwise produce duplicate facets, especially in 5D/6D.
            Section section;section.mesh=convexSection(sliceRaw(model,boundary,xAxis,zAxis,origin));
            section.subtractive=model.shapes[model.shapeOf(boundary.front().vertices[0])].subtractive;
            const auto& triangles=section.mesh.triangles;
            if(triangles.empty())continue;
            glm::dvec3 center(0);for(auto p:triangles)center+=p;center/=(double)triangles.size();
            bool volume=true;
            for(size_t i=0;i<triangles.size();i+=3)
            {
                auto normal=glm::cross(triangles[i+1]-triangles[i],triangles[i+2]-triangles[i]);
                if(glm::length(normal)<epsilon)continue;
                normal=glm::normalize(normal);double offset=glm::dot(normal,triangles[i]);
                double distance=glm::dot(normal,center)-offset;
                if(std::abs(distance)<epsilon){volume=false;break;}
                if(distance>0){normal=-normal;offset=-offset;}
                bool duplicate=false;for(auto p:section.planes)if(glm::length(p.normal-normal)<epsilon&&std::abs(p.offset-offset)<epsilon){duplicate=true;break;}
                if(!duplicate)section.planes.push_back({normal,offset});
            }
            if(!volume)section.planes.clear();
            sections.push_back(std::move(section));
        }
        Mesh result;
        auto emit=[&](const Polygon& p){polygon(p,result);};
        auto subtract=[&](std::vector<Polygon> fragments,size_t self)
        {
            for(size_t j=0;j<sections.size();++j)if(j!=self&&sections[j].subtractive)
            {
                std::vector<Polygon> next;
                for(const auto& p:fragments)
                {
                    bool sharedWall=false;
                    if(sections[self].subtractive&&j>self)for(auto a:sections[self].planes)for(auto b:sections[j].planes)
                        if(glm::length(a.normal-b.normal)<epsilon&&std::abs(a.offset-b.offset)<epsilon&&
                            std::all_of(p.begin(),p.end(),[&](auto v){return std::abs(glm::dot(v,a.normal)-a.offset)<epsilon;}))sharedWall=true;
                    auto pieces=sharedWall?std::vector<Polygon>{p}:outside(p,sections[j]);
                    next.insert(next.end(),pieces.begin(),pieces.end());
                }
                fragments=std::move(next);
            }
            for(const auto& p:fragments)emit(p);
        };
        for(size_t i=0;i<sections.size();++i)
        {
            const auto& section=sections[i];const auto& triangles=section.mesh.triangles;
            for(size_t t=0;t<triangles.size();t+=3)
            {
                Polygon face{triangles[t],triangles[t+1],triangles[t+2]};
                if(!section.subtractive)subtract({face},i);
                else if(!section.planes.empty())for(const auto& solid:sections)if(!solid.subtractive&&!solid.planes.empty())
                {
                    auto wall=face;for(auto plane:solid.planes)wall=clip(wall,plane,true);
                    bool coincident=false;
                    for(auto plane:solid.planes)if(std::all_of(wall.begin(),wall.end(),[&](auto p){return std::abs(glm::dot(p,plane.normal)-plane.offset)<epsilon;})){coincident=true;break;}
                    if(wall.size()>=3&&!coincident)subtract({wall},i);
                }
            }
        }
        return result;
    }
    std::filesystem::path storageDirectory()
    {
#ifdef _WIN32
        wchar_t executable[32768]{};
        auto length=GetModuleFileNameW(nullptr,executable,32768);
        if(!length || length>=32768)throw std::runtime_error("Cannot locate game folder");
        return std::filesystem::path(executable).parent_path()/"Models";
#else
        return std::filesystem::absolute("Models");
#endif
    }
    std::filesystem::path modelPath(std::string name)
    {
        auto first=name.find_first_not_of(" \t");
        name=first==std::string::npos?"model":name.substr(first,name.find_last_not_of(" \t")-first+1);
        std::string ending=name.size()>=4?name.substr(name.size()-4):"";
        for(auto& c:ending)c=(char)std::tolower((unsigned char)c);
        if(ending==".5dm")name.resize(name.size()-4);
        if(name.empty() || name.back()=='.' || name.back()==' ' || name.find_first_of("<>:\"/\\|?*")!=std::string::npos ||
            std::any_of(name.begin(),name.end(),[](unsigned char c){return c<32;}))
            throw std::runtime_error("Invalid filename");
        auto filename=std::filesystem::path(std::u8string(name.begin(),name.end()));
        filename+=".5dm";
        return storageDirectory()/filename;
    }
    std::vector<std::filesystem::path> savedModels()
    {
        std::vector<std::filesystem::path> result;
        auto directory=storageDirectory();
        if(std::filesystem::exists(directory))for(const auto& entry:std::filesystem::directory_iterator(directory))
        {
            auto extension=entry.path().extension().string();
            for(auto& c:extension)c=(char)std::tolower((unsigned char)c);
            if(entry.is_regular_file()&&extension==".5dm")result.push_back(entry.path());
        }
        std::sort(result.begin(),result.end());return result;
    }
    void Model::save(const std::filesystem::path& path) const
    {
        if(dimensions<3||dimensions>maxDimensions)throw std::runtime_error("ND must be between 3 and 6");
        if(points.size()>512)throw std::runtime_error("Point limit: 512");
        if(shapes.empty()||shapes.size()>512||pointShapes.size()!=points.size())throw std::runtime_error("Invalid shapes");
        for(auto shape:pointShapes)if(shape>=shapes.size())throw std::runtime_error("Invalid point shape");
        for(auto p:points)if(!finite(p))throw std::runtime_error("Invalid point");
        for(auto [a,b]:edges)if(a>=b||b>=points.size())throw std::runtime_error("Invalid link");
        if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
        auto temporary=path;temporary+=".tmp";
        std::ofstream out(temporary,std::ios::binary);
        if(!out)throw std::runtime_error("Cannot create model file");
        out.write("MODEL5D5",8);put(out,points.size(),4);put(out,edges.size(),8);put(out,dimensions,4);
        Point anchor=points.empty()?Point{}:points.front();
        for(auto x:anchor) put(out,std::bit_cast<uint64_t>(x),8);
        for(auto p:points) for(int a=0;a<maxDimensions;++a) put(out,std::bit_cast<uint64_t>(p[a]-anchor[a]),8);
        for(auto [a,b]:edges) {put(out,a,4);put(out,b,4);}
        put(out,shapes.size(),4);for(auto shape:shapes)put(out,shape.subtractive?1:0,1);
        for(auto shape:pointShapes)put(out,shape,4);
        out.close();
        if(!out) throw std::runtime_error("Cannot finish writing model");
        auto verified=load(temporary);
        if(verified.dimensions!=dimensions||verified.points.size()!=points.size()||verified.edges!=edges||verified.shapes!=shapes||verified.pointShapes!=pointShapes)throw std::runtime_error("Model verification failed");
#ifdef _WIN32
        if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace saved model (Windows " + std::to_string(GetLastError()) + ")");
#else
        std::filesystem::rename(temporary,path);
#endif
    }
    Model Model::load(const std::filesystem::path& path)
    {
        std::ifstream in(path,std::ios::binary);
        if(!in)throw std::runtime_error(std::filesystem::exists(path)?"Cannot open model file":"Model file not found");
        char magic[8]{};in.read(magic,8);
        bool grouped=std::string_view(magic,8)=="MODEL5D5";
        bool sixAxes=grouped||std::string_view(magic,8)=="MODEL5D4";
        bool dimensional=sixAxes||std::string_view(magic,8)=="MODEL5D3";
        bool relative=dimensional||std::string_view(magic,8)=="MODEL5D2";
        if(!in || (!relative && std::string_view(magic,8)!="MODEL5D1")) throw std::runtime_error("Invalid model file");
        auto count=get(in,4),links=get(in,8);
        int storedAxes=sixAxes?6:5;
        if(count>512 || links>count*(count?count-1:0)/2 || (!grouped&&std::filesystem::file_size(path)!=(sixAxes?72:dimensional?64:relative?60:20)+count*storedAxes*8+links*8)) throw std::runtime_error("Invalid model size");
        int dimensions=dimensional?(int)get(in,4):5;
        if(dimensions<3||dimensions>storedAxes)throw std::runtime_error("Invalid model dimensions");
        Point anchor{};
        if(relative)for(int a=0;a<storedAxes;++a)anchor[a]=std::bit_cast<double>(get(in,8));
        if(!finite(anchor))throw std::runtime_error("Invalid model origin");
        Model result;result.dimensions=dimensions;
        for(size_t i=0;i<count;++i) {Point p{};for(int a=0;a<storedAxes;++a)p[a]=anchor[a]+std::bit_cast<double>(get(in,8));if(!finite(p))throw std::runtime_error("Invalid point");result.points.push_back(p);}
        for(size_t i=0;i<links;++i) {auto a=get(in,4),b=get(in,4);if(a>=b || b>=count || !result.edges.insert({a,b}).second)throw std::runtime_error("Invalid link");}
        result.pointShapes.resize(count,0);
        if(grouped)
        {
            auto groups=get(in,4);if(!groups||groups>512)throw std::runtime_error("Invalid shape count");
            result.shapes.clear();for(size_t i=0;i<groups;++i){auto type=get(in,1);if(type>1)throw std::runtime_error("Invalid shape type");result.shapes.push_back({type!=0});}
            for(auto& shape:result.pointShapes){shape=get(in,4);if(shape>=groups)throw std::runtime_error("Invalid point shape");}
            for(auto [a,b]:result.edges)if(result.shapeOf(a)!=result.shapeOf(b))throw std::runtime_error("Link between shapes");
            if(in.peek()!=EOF)throw std::runtime_error("Invalid model size");
        }
        return result;
    }
}
