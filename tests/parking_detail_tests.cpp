#include "world.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
std::uint64_t digest(const std::vector<br::Vertex>& vertices) {
    std::uint64_t result=1469598103934665603ULL;
    for(const auto& v:vertices)for(float f:{v.x,v.y,v.z,v.nx,v.ny,v.nz,v.u,v.v,v.material}) {
        std::uint32_t bits;std::memcpy(&bits,&f,sizeof(bits));result=(result^bits)*1099511628211ULL;
    }
    return result;
}
std::vector<br::Vertex> details(const std::vector<br::Vertex>& vertices) {
    std::vector<br::Vertex> result;
    for(const auto& v:vertices) {
        const bool wet=near(v.y,.004f)&&v.material==74;
        const bool cable=near(v.y,.018f)&&v.material==75;
        const bool grille=(near(v.y,3.18f)&&v.material==75&&v.ny<-.99f)||
                           (near(v.y,3.175f)&&v.material==7&&v.ny<-.99f);
        const bool paint=(near(v.y,.012f)||near(v.y,.014f))&&v.material==73;
        const bool sign=v.material==90 && v.y>1.78f && v.y<2.42f;
        if(wet||cable||grille||paint||sign)result.push_back(v);
    }
    return result;
}
void validFaces(const std::vector<br::Vertex>& vertices) {
    check(vertices.size()%3==0,"complete triangle buffer");
    for(std::size_t i=0;i<vertices.size();i+=3) {
        const auto& a=vertices[i];const auto& b=vertices[i+1];const auto& c=vertices[i+2];
        const double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
        check((uy*vz-uz*vy)*a.nx+(uz*vx-ux*vz)*a.ny+(ux*vy-uy*vx)*a.nz>=-1e-6,"winding agrees with normals");
        for(const auto* v:{&a,&b,&c}) {
            const double length=v->nx*v->nx+v->ny*v->ny+v->nz*v->nz;
            check(std::isfinite(length)&&std::abs(length-1)<.0001,"finite unit normals");
            check(v->x>=-.001f&&v->z>=-.001f&&v->x<=br::CHUNK_SIZE+.001f&&v->z<=br::CHUNK_SIZE+.001f&&v->y>=0&&v->y<=3.201f,"details stay inside existing room bounds");
        }
    }
}
}
int main(){try {
    using namespace br;
    int vents=0,leaks=0,cables=0,arrows=0;std::size_t maximum=0;
    for(std::uint64_t seed:{0ULL,1ULL,2ULL,3ULL,7ULL,42ULL,101ULL,12071998ULL}) {
        World world(seed,Level::Parking);world.update(3.75,3.75,0);
        const auto& chunk=world.chunks().at({0,0});maximum=std::max(maximum,chunk.vertices.size());
        validFaces(chunk.vertices);validFaces(chunk.distantVertices);
        const auto fullDetails=details(chunk.vertices),farDetails=details(chunk.distantVertices);
        check(fullDetails.size()==farDetails.size()&&digest(fullDetails)==digest(farDetails),"wayfinding and maintenance details match both LODs");
        int directions[4]={};
        for(const auto& v:fullDetails) {
            if(v.material==90&&v.y>1.78f&&v.y<2.42f) {
                if(v.nx>.99f)++directions[0];
                if(v.nx<-.99f)++directions[1];
                if(v.nz>.99f)++directions[2];
                if(v.nz<-.99f)++directions[3];
            }
        }
        for(int n:directions)check(n>0,"sector letters are visible on all four column faces");
        for(int z=0;z<CHUNK_CELLS;++z)for(int x=0;x<CHUNK_CELLS;++x) {
            const auto hash=hashCell(x,z,seed^0x5041524b494e47ULL);
            if((hash>>32)%7==0)++vents;
            if((hash>>40)%11==0&&hash%11!=0&&!(x==0&&z==0)) {
                ++leaks;
                check(!world.blocked(x*CELL+.85,z*CELL+4.05,.2),"pipe leak remains shallow walkable floor");
            }
            if((hash>>48)%13==0&&!(x==0&&z==0)) {
                ++cables;
                check(!world.blocked(x*CELL+5.85,z*CELL+3.,.2),"cable detail remains walk-over");
            }
        }
        const auto before=digest(chunk.vertices),distantBefore=digest(chunk.distantVertices);
        world.update(6000,-6000,0);world.update(3.75,3.75,0);
        check(digest(world.chunks().at({0,0}).vertices)==before&&digest(world.chunks().at({0,0}).distantVertices)==distantBefore,"detail identities survive chunk eviction");
        for(int i=0;i<3;++i) {
            const auto p=world.breakerLocation(i);world.update(p.x,p.z,0);
            const auto coord=chunkAt(p.x,p.z);const auto& panel=world.chunks().at(coord);
            const auto paint=details(panel.vertices),paintDistant=details(panel.distantVertices);
            check(digest(paint)==digest(paintDistant),"cabinet arrows retain detail at distance");
            const auto cell=parkingBreakerCell(seed,i);
            bool tip=false;
            for(const auto& v:panel.vertices)if(near(v.y,.012f)&&v.material==73) {
                if(std::abs(v.x+coord.x*CHUNK_SIZE-(cell.x*CELL+5.825))<.001 &&
                   std::abs(v.z+coord.z*CHUNK_SIZE-(cell.z*CELL+1.72))<.001)tip=true;
            }
            check(tip,"local floor arrow points to the actual cabinet front");++arrows;
            check(world.activateBreaker(i,p.x,p.z+1),"paint does not obstruct electrical cabinet interaction");
        }
    }
    check(vents>0&&leaks>0&&cables>0&&arrows==24,"seed sweep exercises every new landmark type");
    std::cout<<"Parking details passed: "<<vents<<" vents, "<<leaks<<" leaks, "<<cables<<" cables, "<<arrows<<" cabinet arrows; max "<<maximum<<" vertices/chunk.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
