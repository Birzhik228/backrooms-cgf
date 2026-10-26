#include "world.h"
#include "common/mouse.h"
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
            for(bool east:{true,false}) if(br::edgeOpen(x,z,x+(east?1:0),z+(east?0:1),seed,0,br::Level::Fun))
                for(int i=0;i<=40;++i) check(!w.blocked(cx+(east?br::CELL*i/40:0),cz+(east?0:br::CELL*i/40),.25),"decorations do not obstruct connected routes");
        }
        const auto original=digest(w.chunks().at({0,0}));
        check(!previous || original!=previous,"seed changes room contents");previous=original;
        w.update(6000,-6000,1);w.update(3.75,3.75,1);
        check(digest(w.chunks().at({0,0}))==original,"streamed party rooms regenerate identically");
        check(w.headClearance(3.75,3.75,.25)>=3,"spawn fits standing player");
        check(w.blocked(1.65,1.65,.23),"table collision matches furniture");
    }
    bool postKeyMouse=false,postKeyVerity=false;
    // Chance curve is bounded, deterministic, and never needs more than 15 boxes.
    check(br::World::presentKeyChance(0)==5 && br::World::presentKeyChance(1)==8 &&
          br::World::presentKeyChance(14)==100,"soft pity curve and hard guarantee");
    for(auto seed:{0ULL,42ULL,12071998ULL}) {
        br::World w(seed,br::Level::Fun);w.update(3.75,3.75,1);
        std::vector<br::Present> boxes;
        for(const auto& entry:w.chunks())for(auto gift:entry.second.presents)boxes.push_back(gift);
        check(boxes.size()>=15,"enough searchable presents near the checkpoint");
        auto exit=w.exitLocation();w.update(exit.x,exit.z);
        br::DoorInfo door{};check(w.nearestDoor(exit.x,exit.z,2,door) && door.isExit,"Fun exit exists");
        check(!w.toggleDoor(door.id,exit.x,exit.z),"Fun exit requires its own key");
        br::Present first{};bool haveFirst=false;
        for(const auto gift:boxes) {
            if(w.keyCollected())break;
            w.update(gift.x,gift.z,1);
            check(w.openPresent(gift.id,gift.x+20,gift.z)==br::PresentResult::Invalid,"remote present interaction rejected");
            const auto result=w.openPresent(gift.id,gift.x+1.05,gift.z);
            check(result!=br::PresentResult::Invalid,"gift approach reachable beside corner");
            if(result==br::PresentResult::Mouse) {
                br::Mouse mouse;mouse.reset(seed);mouse.releaseFromPresent(gift.x,gift.z);
                check(mouse.active && mouse.alerted,"present releases a fleeing mouse");
                for(int i=0;i<30;++i)check(!mouse.update(w,gift.x+1.05,gift.z,.016f,false,false,false),"released mouse cannot emit a second contact alarm");
                mouse.x=6000;mouse.z=6000;mouse.update(w,gift.x,gift.z,.016f,false,false,false);
                check(!mouse.active,"present mouse cannot respawn elsewhere");
            }
            check(!w.blocked(gift.x,gift.z,.12),"searched present removes collision");
            check(w.openPresent(gift.id,gift.x+1.05,gift.z)==br::PresentResult::Invalid,"one roll per present");
            if(!haveFirst){first=gift;haveFirst=true;}
            check(w.presentsOpened()<=15,"key obtained within hard pity limit");
        }
        check(w.keyCollected() && w.presentsOpened()<=15,"searching gifts grants the Fun key");
        const int count=w.presentsOpened();
        w.update(6000,6000,0);w.update(first.x,first.z,1);
        br::Present restored{};
        check(!w.nearestPresent(first.x,first.z,.1,restored) && !w.blocked(first.x,first.z,.12),"searched present stays absent after eviction");
        check(w.presentsOpened()==count,"streaming retains pity progress");
        w.update(exit.x,exit.z);
        check(w.nearestDoor(exit.x,exit.z,2,door) && !door.open,"Fun key unlocks without auto opening");
        check(w.toggleDoor(door.id,exit.x,exit.z),"player opens Fun exit with key");
        for(int i=0;i<60;++i)w.advanceDoors(.016f,exit.x,exit.z);
        check(w.exitCrossed(door.x,door.z-.8),"Fun exit threshold completes level");
        for(const auto gift:boxes) {
            if(postKeyMouse && postKeyVerity)break;
            w.update(gift.x,gift.z,1);br::Present available{};
            if(!w.nearestPresent(gift.x,gift.z,.1,available))continue;
            const auto dropsBefore=w.verityDrops().size();
            const auto result=w.openPresent(gift.id,gift.x+1.05,gift.z);
            check(result!=br::PresentResult::Invalid && result!=br::PresentResult::Key,"post-key presents remain searchable without duplicate keys");
            check(w.keyCollected(),"later present rolls never remove the key");
            postKeyMouse|=result==br::PresentResult::Mouse;
            postKeyVerity|=w.verityDrops().size()>dropsBefore;
            check(w.openPresent(gift.id,gift.x+1.05,gift.z)==br::PresentResult::Invalid,"post-key present still opens only once");
        }
        w=br::World(seed,br::Level::Fun);w.update(3.75,3.75,0);
        check(w.isFun() && !w.keyCollected() && w.presentsOpened()==0,"checkpoint restarts Fun with fresh objectives");
    }
    check(postKeyMouse && postKeyVerity,"mice and Verity both remain available after the key");
    br::World mouseWorld(2,br::Level::Fun);mouseWorld.update(3.75,3.75,0);
    br::Present mouseGift{};
    check(mouseWorld.nearestPresent(.8,5.9,.1,mouseGift),"mouse fixture gift exists");
    check(mouseWorld.openPresent(mouseGift.id,1.85,5.9)==br::PresentResult::Mouse,"seeded mouse fixture releases mouse instead of key");
    check(!mouseWorld.keyCollected() && mouseWorld.presentsOpened()==1 && br::World::presentKeyChance(1)==8,"mouse preserves key pity");
    br::Mouse released;released.reset(2);released.releaseFromPresent(.8,5.9);
    for(int i=0;i<30;++i)check(!released.update(mouseWorld,1.85,5.9,.016f,false,false,false),"present mouse cannot call twice");
    released.x=6000;released.update(mouseWorld,1.85,5.9,.016f,false,false,false);
    check(!released.active,"present mouse never respawns");
    br::World bonus(3,br::Level::Fun);bonus.update(3.75,3.75,0);
    br::Present gift{};check(bonus.nearestPresent(.8,5.9,.1,gift),"Verity fixture present exists");
    bonus.openPresent(gift.id,1.85,5.9);
    check(bonus.verityDrops().size()==1 && !bonus.infiniteStamina(),"5 percent bonus appears without auto collection");
    const auto model=bonus.verityVertices(0,0,1.85,5.9,0);
    check(!model.empty() && model.size()<5000,"bounded procedural Verity model");
    for(const auto& v:model)check(std::isfinite(v.x) && (v.material==59 || v.material==60),"yellow sphere and dark smile materials");
    check(!bonus.collectVerity(3.75,3.75),"remote Verity collection rejected");
    bonus.update(6000,6000,0);bonus.update(.8,5.9,0);
    check(bonus.verityDrops().size()==1 && bonus.collectVerity(1.3,5.9),"drop survives streaming and can be collected");
    check(bonus.infiniteStamina() && bonus.verityDrops().empty() && !bonus.collectVerity(1.3,5.9),"one-shot level-local reward");
    bonus=br::World(3,br::Level::Fun);check(!bonus.infiniteStamina(),"checkpoint reset clears reward");
    br::World zero(42);zero.update(3.75,3.75,0);
    check(!zero.isFun() && !zero.infiniteStamina(),"Level 0 has no Verity bonus");
    std::cout<<"Level Fun tests passed: connected routes, prop materials, collision, winding, bounds, deterministic streaming and Level 0 isolation.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
