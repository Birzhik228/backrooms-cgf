#include "world.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
bool inside(float value,float low,float high){return value>=low-.001f && value<=high+.001f;}
std::uint64_t digest(const std::vector<br::Vertex>& vertices) {
    std::uint64_t result=1469598103934665603ULL;
    for(const auto& v:vertices)for(float f:{v.x,v.y,v.z,v.nx,v.ny,v.nz,v.u,v.v,v.material}) {
        std::uint32_t bits;std::memcpy(&bits,&f,sizeof(bits));result=(result^bits)*1099511628211ULL;
    }
    return result;
}
std::vector<br::Vertex> corner(const std::vector<br::Vertex>& vertices,float x,float z) {
    std::vector<br::Vertex> result;
    for(const auto& v:vertices)
        if(inside(v.x,x+5,x+7) && inside(v.z,z+5,z+7) && v.y>.001f && v.y<.15f)
            result.push_back(v);
    return result;
}
void validFaces(const std::vector<br::Vertex>& vertices) {
    check(vertices.size()%3==0,"complete triangles");
    for(std::size_t i=0;i<vertices.size();i+=3) {
        const auto& a=vertices[i];const auto& b=vertices[i+1];const auto& c=vertices[i+2];
        const double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
        check((uy*vz-uz*vy)*a.nx+(uz*vx-ux*vz)*a.ny+(ux*vy-uy*vx)*a.nz>=-1e-7,"detail winding matches its normal");
        for(const auto* v:{&a,&b,&c}) {
            const double length=v->nx*v->nx+v->ny*v->ny+v->nz*v->nz;
            check(std::isfinite(length) && std::abs(length-1)<.0001,"unit detail normals");
        }
    }
}
}
int main(){try {
    int corners=0,toys=0;std::size_t maximum=0;
    for(std::uint64_t seed:{0ULL,1ULL,2ULL,3ULL,7ULL,42ULL,101ULL,12071998ULL}) {
        br::World world(seed,br::Level::Fun);world.update(3.75,3.75,0);
        const auto& chunk=world.chunks().at({0,0});maximum=std::max(maximum,chunk.vertices.size());
        validFaces(chunk.vertices);validFaces(chunk.distantVertices);
        for(int z=0;z<br::CHUNK_CELLS;++z)for(int x=0;x<br::CHUNK_CELLS;++x) {
            const auto hash=br::hashCell(x,z,seed^0x46554e);
            if((hash>>12)%5==0)continue; // Existing southeast tables are solid furniture.
            const auto near=corner(chunk.vertices,float(x*br::CELL),float(z*br::CELL));
            if(near.empty())continue;
            const auto far=corner(chunk.distantVertices,float(x*br::CELL),float(z*br::CELL));
            ++corners;
            check(near.size()<=420,"small corner detail budget");
            check(near.size()==far.size() && digest(near)==digest(far),"floor landmarks retain identity across LOD changes");
            for(int dz=0;dz<5;++dz)for(int dx=0;dx<5;++dx)
                check(!world.blocked(x*br::CELL+5.25+dx*.33,z*br::CELL+5.25+dz*.33,.15),"low party debris remains walk-over");
        }
        // Metal winding keys only occur in the new tabletop toys at this height.
        for(const auto& v:chunk.vertices)if(v.material==7 && v.y>.85f && v.y<1.03f)++toys;
        const auto original=digest(chunk.vertices),distant=digest(chunk.distantVertices);
        world.update(6000,-6000,0);world.update(3.75,3.75,0);
        check(digest(world.chunks().at({0,0}).vertices)==original &&
              digest(world.chunks().at({0,0}).distantVertices)==distant,"decor and toys regenerate identically after eviction");
    }
    check(corners>=15 && toys>0,"seed sweep exercises floor details and winding toys");
    std::cout<<"Fun details passed: "<<corners<<" sparse corners, LOD/streaming, walk-over geometry, winding; max "<<maximum<<" vertices/chunk.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
