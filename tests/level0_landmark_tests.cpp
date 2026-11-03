#include "world.h"
#include "landmark_route.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>

void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
using Notes=std::map<std::uint64_t,br::Level0Observation>;
Notes notes(const br::World& world) {
    Notes result;
    for(const auto& entry:world.chunks())for(const auto note:entry.second.observations)
        require(result.emplace(note.id,note).second,"observation IDs are unique");
    return result;
}
br::ChunkCoord parentOf(br::ChunkCoord cell,std::uint64_t seed) {
    for(const auto d:{br::ChunkCoord{1,0},{-1,0},{0,1},{0,-1}}) {
        const br::ChunkCoord next{cell.x+d.x,cell.z+d.z};
        if(std::abs(next.x)+std::abs(next.z)<std::abs(cell.x)+std::abs(cell.z) &&
           br::edgeReserved(cell.x,cell.z,next.x,next.z,seed))return next;
    }
    throw std::runtime_error("branch has no reserved parent");
}
bool sameNote(const br::Level0Observation& a,const br::Level0Observation& b) {
    return a.id==b.id && a.kind==b.kind && a.position.x==b.position.x && a.position.z==b.position.z;
}
int main(){try {
    std::size_t tested=0;std::size_t maxVertices=0;bool checkedWall=false;
    for(std::uint64_t fixture=0;fixture<65;++fixture) {
        const std::uint64_t seed=fixture<64?fixture:12071998;
        br::World world(seed);world.update(0,0,3);
        const auto discoveries=notes(world);
        require(discoveries.size()==2,"exactly two optional notes in the initial route area");
        require(discoveries.begin()->second.kind!=discoveries.rbegin()->second.kind,"the two optional discoveries are visually distinct");
        const auto route=landmarkRoute(seed);
        std::vector<br::ChunkCoord> routeCells;
        for(std::size_t i=0;i+1<route.size();++i)
            routeCells.push_back({std::int64_t(std::floor(route[i].x/br::CELL)),std::int64_t(std::floor(route[i].z/br::CELL))});
        for(const auto& entry:world.chunks()) {
            const auto& chunk=entry.second;
            maxVertices=std::max(maxVertices,chunk.vertices.size());
            require(chunk.vertices.size()<50000,"landmark mesh remains bounded per chunk");
            for(const auto& v:chunk.vertices)
                require(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"finite mesh positions");
            for(std::size_t i=0;i+2<chunk.vertices.size();i+=3) {
                const auto a=chunk.vertices[i],b=chunk.vertices[i+1],c=chunk.vertices[i+2];
                if(a.y<.15f && b.y<.15f && c.y<.15f && a.ny>.9f) {
                    const float signedArea=(b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z);
                    require(signedArea>0,"floor details face upward and remain visible with backface culling");
                }
            }
        }
        for(const auto& entry:discoveries) {
            const auto note=entry.second;
            require(note.kind>=0 && note.kind<3,"recognized landmark/note kind");
            const br::ChunkCoord cell{std::int64_t(std::floor(note.position.x/br::CELL)),std::int64_t(std::floor(note.position.z/br::CELL))};
            require(std::find(routeCells.begin(),routeCells.end(),cell)==routeCells.end(),"optional note is off the mandatory route");
            auto junction=cell;int depth=0;
            while(std::find(routeCells.begin(),routeCells.end(),junction)==routeCells.end() && depth<3) {
                junction=parentOf(junction,seed);++depth;
            }
            require(depth>=1 && depth<=2,"optional detour is only one or two reserved edges");
            const double approachX=cell.x*br::CELL+4.8,approachZ=cell.z*br::CELL+5.0;
            require(!world.blocked(approachX,approachZ,.23),"note approach is physically accessible");
            require(!world.blocked(note.position.x,note.position.z,.23),"low note/debris can be walked over");
            br::Level0Observation found{};
            require(world.nearestObservation(approachX,approachZ,1.8,found)&&sameNote(found,note),"nearby clear note can be inspected");
            require(!world.nearestObservation(approachX,approachZ,.2,found),"out-of-reach note cannot be read");
            for(const auto offset:{br::WorldPoint{2.05,0},br::WorldPoint{0,1.62}}) {
                const double x=note.position.x+offset.x,z=note.position.z+offset.z;
                if(!world.blocked(x,z,.23) && world.soundTransmission(x,z,note.position.x,note.position.z)<.5f) {
                    require(!world.nearestObservation(x,z,2.5,found),"walls prevent reading a nearby note from another room");
                    checkedWall=true;
                }
            }
            // Walk every reserved side edge and its centre-to-note approach.
            auto branch=cell;
            for(int step=0;step<depth;++step) {
                const auto next=parentOf(branch,seed);
                for(int sample=0;sample<=30;++sample) {
                    const double t=sample/30.0;
                    const double x=(branch.x+.5+(next.x-branch.x)*t)*br::CELL;
                    const double z=(branch.z+.5+(next.z-branch.z)*t)*br::CELL;
                    require(!world.blocked(x,z,.23),"short optional branch is physically traversable");
                }
                branch=next;
            }
            for(int sample=0;sample<=30;++sample) {
                const double t=sample/30.0;
                const double x=(cell.x+.5)*br::CELL+(approachX-(cell.x+.5)*br::CELL)*t;
                const double z=(cell.z+.5)*br::CELL+(approachZ-(cell.z+.5)*br::CELL)*t;
                require(!world.blocked(x,z,.23),"branch-centre to note approach is clear");
            }
            ++tested;
        }
        if(seed<8) {
            // The same route test used by progression catches new obstructions
            // even though the objective coordinates themselves stay unchanged.
            double routeLength=0;
            for(std::size_t i=1;i<route.size();++i) {
                routeLength+=std::hypot(route[i].x-route[i-1].x,route[i].z-route[i-1].z);
                for(int sample=0;sample<=30;++sample) {
                    const double t=sample/30.0;
                    require(!world.blocked(route[i-1].x+(route[i].x-route[i-1].x)*t,
                                           route[i-1].z+(route[i].z-route[i-1].z)*t,.23),"mandatory key/exit route remains clear");
                }
            }
            require(routeLength<=140,"optional notes do not lengthen the mandatory route");
            world.update(6000,6000,1);world.update(0,0,3);
            const auto restored=notes(world);
            require(restored.size()==discoveries.size(),"notes survive chunk eviction");
            for(const auto& entry:discoveries)require(sameNote(entry.second,restored.at(entry.first)),"note identities and positions are stable");
        }
    }
    require(checkedWall,"wall-separated observation interaction was exercised");
    for(auto level:{br::Level::Fun,br::Level::Parking}) {
        br::World world(12071998,level);world.update(0,0,1);
        require(notes(world).empty(),"other stages have no Level0 observations");
        br::Level0Observation result{};
        require(!world.nearestObservation(3.75,3.75,4,result),"other stages reject observation interaction");
    }
    br::World world(42);world.update(0,0,3);br::Level0Observation result{};
    require(!world.nearestObservation(NAN,0,1.8,result),"invalid position rejected");
    require(!world.nearestObservation(0,0,INFINITY,result),"invalid range rejected");
    std::cout<<"Level0 landmarks passed: "<<tested<<" short branches, max "<<maxVertices<<" vertices/chunk.\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}}
