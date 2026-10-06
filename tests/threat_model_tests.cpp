#include "common/threat_model.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace br;
static void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
struct Bounds {float minY=100,maxY=-100,minX=100,maxX=-100,minZ=100,maxZ=-100;};
static Bounds validate(const std::vector<Vertex>& mesh,bool grounded=true) {
    check(mesh.size()==8728*3,"The actual 8,728-triangle FBX mesh is retained");
    Bounds b;
    for(const auto& v:mesh) {
        for(float x:{v.x,v.y,v.z,v.nx,v.ny,v.nz,v.u,v.v,v.material})check(std::isfinite(x),"Finite vertex fields");
        check(std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)<.00001f,"Blended normals are normalized");
        check(v.material==20||v.material==21,"Original black and white material assignments");
        b.minY=std::min(b.minY,v.y);b.maxY=std::max(b.maxY,v.y);
        b.minX=std::min(b.minX,v.x);b.maxX=std::max(b.maxX,v.x);
        b.minZ=std::min(b.minZ,v.z);b.maxZ=std::max(b.maxZ,v.z);
    }
    if(grounded) {
        check(std::abs(b.minY-.012f)<.00001f,"A sole/claw contact stays grounded through blends");
        check(b.maxY<2.9f,"All locomotion fits the original 3.2 metre rooms");
    }
    for(std::size_t i=0;i<mesh.size();i+=3) {
        const auto &a=mesh[i],&c=mesh[i+1],&d=mesh[i+2];
        check(a.material==c.material&&a.material==d.material,"Every triangle has one material");
        check(a.x!=c.x||a.y!=c.y||a.z!=c.z,"Quantized triangle edge is not collapsed");
    }
    return b;
}
static float delta(const std::vector<Vertex>& a,const std::vector<Vertex>& b,float low=0,float high=10) {
    float sum=0;std::size_t count=0;
    for(std::size_t i=0;i<a.size();++i)if(a[i].y>=low&&a[i].y<high){
        sum+=std::abs(a[i].x-b[i].x)+std::abs(a[i].y-b[i].y)+std::abs(a[i].z-b[i].z);++count;
    }
    return sum/std::max<std::size_t>(1,count);
}
static void rejects(const std::filesystem::path& p) {
    bool rejected=false;try{validateThreatModelAsset(p);}catch(const std::runtime_error&){rejected=true;}
    check(rejected,"Malformed asset is rejected before it reaches rendering");
}
int main() {
    validateThreatModelAsset("assets/models/smiler/smiler.brm");
    auto neutral=threatModelVertices(0,0,false);
    auto b=validate(neutral);
    check(b.maxY>2.75f&&b.maxZ-b.minZ>1.5f,"Smiler silhouette and long claws retained");
    std::size_t white=0;for(const auto& v:neutral)if(v.material==21){++white;check(v.y>2.1f,"White eyes/teeth stay on original head");}
    check(white>500,"Imported eyes and teeth are volumetric geometry");
    float fullHeight=0,halfHeight=0;
    for(int i=0;i<64;++i)for(float speed:{0.f,.3f,1.1f,2.2f,3.3f}) {
        const float phase=i/64.f;
        auto current=threatModelVertices(i*.13f,speed,false,0,0,phase);
        validate(current);
        auto half=validate(threatModelVertices(i*.13f,speed,false,0,.5f,phase));
        auto full=validate(threatModelVertices(i*.13f,speed,false,0,1,phase));
        halfHeight=std::max(halfHeight,half.maxY);fullHeight=std::max(fullHeight,full.maxY);
        check(delta(current,threatModelVertices(i*.13f,speed,false,0,0,phase))==0,"Paused identical inputs produce identical geometry");
        check(delta(current,threatModelVertices(i*.13f,speed,true,0,0,phase))==0,"Fast Search uses same physical gait as Chase");
    }
    check(fullHeight<2.30f&&halfHeight<2.80f,"Articulated stoop clears both doorway heights");
    const auto walkA=threatModelVertices(0,1.1f,false,0,0,.25f),walkB=threatModelVertices(0,1.1f,false,0,0,.75f);
    const auto runA=threatModelVertices(0,3.3f,true,0,0,.25f),runB=threatModelVertices(0,3.3f,true,0,0,.75f);
    check(delta(walkA,walkB,0,1.3f)>.10f,"Walk alternates the articulated legs");
    check(delta(runA,runB,0,1.3f)>delta(walkA,walkB,0,1.3f),"Running has a longer leg stride than walking");
    check(delta(walkA,runA)>.05f,"Running is a distinct pose, not accelerated static bobbing");
    check(delta(threatModelVertices(0,1.1f,false,0,0,.99999f),threatModelVertices(0,1.1f,false,0,0,.00001f))<.0001f,"Walk loop closes continuously");
    check(delta(threatModelVertices(0,3.3f,true,0,0,.99999f),threatModelVertices(0,3.3f,true,0,0,.00001f))<.0001f,"Run loop closes continuously");
    check(delta(threatModelVertices(1,1.399f,false,0,0,.32f),threatModelVertices(1,1.401f,true,0,0,.32f))<.001f,"Speed transition preserves shared stride phase");
    for(int i=1;i<=50;++i)validate(threatModelVertices(0,0,true,i/50.f),false);
    check(delta(threatModelVertices(0,0,true,.02f),threatModelVertices(0,0,true,.7f))>.2f,"Capture uses articulated imported scream animation");
    validate(threatModelVertices(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),true,-3,8));
    std::filesystem::create_directories("tests/artifacts/v12");
    const auto bad=std::filesystem::path("tests/artifacts/v12/invalid-smiler.brm");
    std::ifstream file("assets/models/smiler/smiler.brm",std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(file)),{});
    auto write=[&](const std::string& s){std::ofstream out(bad,std::ios::binary);out.write(s.data(),static_cast<std::streamsize>(s.size()));};
    write(bytes.substr(0,40));rejects(bad);
    auto corrupted=bytes;corrupted[0]='X';write(corrupted);rejects(bad);
    corrupted=bytes;corrupted[12]=char(255);corrupted[13]=char(255);write(corrupted);rejects(bad);
    corrupted=bytes;corrupted[32]='X';write(corrupted);rejects(bad);
    corrupted=bytes;corrupted[228]=char(255);corrupted[229]=char(255);write(corrupted);rejects(bad);
    corrupted=bytes;corrupted.back()=0;corrupted[corrupted.size()-2]=0;corrupted[corrupted.size()-3]=0;corrupted[corrupted.size()-4]=0;corrupted[corrupted.size()-5]=0;corrupted[corrupted.size()-6]=0;write(corrupted);rejects(bad);
    write(bytes+"trailing");rejects(bad);
    std::filesystem::remove(bad);
    std::cout<<"Smiler model tests passed: 8728 imported triangles, articulated walk/run/capture, grounded blends, full stoop "<<fullHeight<<"m, half stoop "<<halfHeight<<"m; malformed asset validation passed.\n";
}
