#include "world.h"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <cstring>
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::uint64_t digest(const br::Chunk& c) {
    std::uint64_t h=1469598103934665603ULL;
    for(const auto& v:c.vertices) for(float f:{v.x,v.y,v.z,v.nx,v.ny,v.nz,v.material}) {
        std::uint32_t bits;std::memcpy(&bits,&f,4);h=(h^bits)*1099511628211ULL;
    }
    return h;
}
int main() {try {
    std::uint64_t previous=0;
    for(auto seed:{0ULL,42ULL,12071998ULL}) {
        br::World w(seed,br::Level::Fun);w.update(3.75,3.75,1);
        check(w.isFun() && w.chunks().size()==9,"Fun uses the bounded world streamer");
        check(!w.collectKey(3.75,3.75) && w.keyVertices(0,0).empty() && !w.exitCrossed(6.225,-268),"Fun has no stale Level 0 objectives");
        std::set<int> materials;
        for(const auto& entry:w.chunks()) {
            const auto& c=entry.second;
            check(c.doors.empty() && c.hidingSpots.empty(),"Fun does not inherit the exit or vents");
            check(c.vertices.size()<400000,"party mesh per chunk remains bounded");
            check(!c.distantVertices.empty() && c.distantVertices.size()<c.vertices.size(),"distant party geometry reduces triangles");
            std::set<int> distantMaterials;
            for(const auto& v:c.distantVertices) {
                check(std::isfinite(v.x)&&std::isfinite(v.nx)&&std::isfinite(v.ny),"finite distant geometry");
                check(v.x>=-.12f && v.x<=br::CHUNK_SIZE+.12f && v.z>=-.12f && v.z<=br::CHUNK_SIZE+.12f && v.y>=0 && v.y<=br::ROOM_HEIGHT+.001,"distant props preserve room bounds");
                distantMaterials.insert(int(v.material));
            }
            for(int id:{40,41,42,43,44,45,46}) check(distantMaterials.count(id)>0,"distant rooms retain walls, furniture, plates and cakes");
            for(const auto& v:c.vertices) {
                check(std::isfinite(v.x)&&std::isfinite(v.nx)&&std::isfinite(v.ny),"finite geometry and normals");
                check(v.x>=-.12f && v.x<=br::CHUNK_SIZE+.12f && v.z>=-.12f && v.z<=br::CHUNK_SIZE+.12f && v.y>=0 && v.y<=br::ROOM_HEIGHT+.001,"all party props stay within their chunk and ceiling");
                materials.insert(int(v.material));
            }
            for(std::size_t i=0;i<c.vertices.size();i+=3) {
                const auto &a=c.vertices[i],&b=c.vertices[i+1],&d=c.vertices[i+2];
                const double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=d.x-a.x,vy=d.y-a.y,vz=d.z-a.z;
                const double facing=(uy*vz-uz*vy)*a.nx+(uz*vx-ux*vz)*a.ny+(ux*vy-uy*vx)*a.nz;
                check(facing>=-1e-7,"party faces have outward winding");
            }
        }
        for(int id:{40,41,42,43,44,45,46,48,49,50,51,52,53}) check(materials.count(id)>0,"walls, carpet, chairs, tablecloth, plates, cake and decorations exist");
        // Test interior cells so the entire sweep stays in resident chunks.
        for(int z=-4;z<10;++z) for(int x=-4;x<10;++x) {
            const double cx=(x+.5)*br::CELL,cz=(z+.5)*br::CELL;
            check(!w.blocked(cx,cz,.25),"cell centre stays clear");
            for(bool east:{true,false}) if(br::edgeOpen(x,z,x+(east?1:0),z+(east?0:1),seed))
                for(int i=0;i<=40;++i) check(!w.blocked(cx+(east?br::CELL*i/40:0),cz+(east?0:br::CELL*i/40),.25),"decorations do not obstruct connected routes");
        }
        const auto original=digest(w.chunks().at({0,0}));
        check(!previous || original!=previous,"seed changes room contents");previous=original;
        w.update(6000,-6000,1);w.update(3.75,3.75,1);
        check(digest(w.chunks().at({0,0}))==original,"streamed party rooms regenerate identically");
        check(w.headClearance(3.75,3.75,.25)>=3,"spawn fits standing player");
        check(w.blocked(1.65,1.65,.23),"table collision matches furniture");
    }
    br::World zero(42);zero.update(3.75,3.75,0);
    check(!zero.isFun(),"default constructor preserves Level 0");
    std::cout<<"Level Fun tests passed: connected routes, prop materials, collision, winding, bounds, deterministic streaming and Level 0 isolation.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
