#include "common/threat_model.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace br;
static void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
constexpr std::size_t SOURCE_VERTICES=11657,SOURCE_TRIANGLES=15220;
struct Bounds {float minY=100,maxY=-100,minX=100,maxX=-100,minZ=100,maxZ=-100;};
struct SourceVertex {std::uint8_t head;std::array<std::uint8_t,4> feet;float u,v;};
static std::vector<SourceVertex> expandedSource;
static std::array<std::size_t,SOURCE_VERTICES> sourceToExpanded;
// These are the two complete orbital rim meshes in the supplied GLB, recorded
// by the baker in aranea-manifest.json. Average each source vertex once: the
// render mesh repeats corners, which otherwise biases the anatomical center.
static Vertex socketMean(const std::vector<Vertex>& mesh,std::size_t first) {
    constexpr std::size_t count=187;
    Vertex mean{};
    for(std::size_t source=first;source<first+count;++source) {
        const auto expanded=sourceToExpanded[source];
        check(expanded<mesh.size()&&expandedSource[expanded].head==255,
              "Orbital landmarks belong to the animated skull");
        const auto& v=mesh[expanded];mean.x+=v.x;mean.y+=v.y;mean.z+=v.z;
    }
    mean.x/=count;mean.y/=count;mean.z/=count;return mean;
}
static std::array<Vertex,2> eyeSockets(const std::vector<Vertex>& mesh) {
    return {{socketMean(mesh,2167),socketMean(mesh,7084)}};
}
static Bounds validate(const std::vector<Vertex>& mesh,bool locomotion=true) {
    check(mesh.size()==SOURCE_TRIANGLES*3,"All 15,220 triangles of the supplied Aranea mesh are retained");
    Bounds b;
    std::array<std::size_t,3> materials{};
    for(std::size_t i=0;i<mesh.size();++i) {
        const auto& v=mesh[i];
        for(float x:{v.x,v.y,v.z,v.nx,v.ny,v.nz,v.u,v.v,v.material})check(std::isfinite(x),"Finite vertex fields");
        check(std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)<.00001f,"Blended normals are normalized");
        check(v.material==20||v.material==21||v.material==22,"Original gray, white and black material assignments");
        ++materials[static_cast<std::size_t>(v.material)-20];
        check(v.u==expandedSource[i].u&&v.v==expandedSource[i].v,"Animation preserves the original per-vertex texture coordinates");
        b.minY=std::min(b.minY,v.y);b.maxY=std::max(b.maxY,v.y);
        b.minX=std::min(b.minX,v.x);b.maxX=std::max(b.maxX,v.x);
        b.minZ=std::min(b.minZ,v.z);b.maxZ=std::max(b.maxZ,v.z);
    }
    check(std::all_of(materials.begin(),materials.end(),[](std::size_t n){return n>0;}),"Every original material survives the import");
    if(locomotion) {
        check(b.minY>=.0119f&&b.minY<.025f,"At least one planted foot keeps the spider grounded");
        check(b.maxY<2.7f,"Locomotion remains below the model height budget");
    }
    for(std::size_t i=0;i<mesh.size();i+=3) {
        const auto &a=mesh[i],&c=mesh[i+1],&d=mesh[i+2];
        check(a.material==c.material&&a.material==d.material,"Every triangle has one material");
        check(a.x!=c.x||a.y!=c.y||a.z!=c.z,"Quantized triangle edge is not collapsed");
    }
    return b;
}
static float delta(const std::vector<Vertex>& a,const std::vector<Vertex>& b,int headFilter=-1) {
    float sum=0;std::size_t count=0;
    for(std::size_t i=0;i<a.size();++i) {
        const bool head=expandedSource[i].head>0;
        if(headFilter>=0&&head!=(headFilter!=0))continue;
        sum+=std::abs(a[i].x-b[i].x)+std::abs(a[i].y-b[i].y)+std::abs(a[i].z-b[i].z);++count;
    }
    check(count>0,"Motion probe has vertices");
    return sum/count;
}
static Vertex footMean(const std::vector<Vertex>& mesh,std::size_t foot) {
    Vertex mean{};std::size_t count=0;
    for(std::size_t i=0;i<mesh.size();++i)if(expandedSource[i].feet[foot]==255) {
        const auto& v=mesh[i];mean.x+=v.x;mean.y+=v.y;mean.z+=v.z;++count;
    }
    check(count>=3,"Each leg has rigid tip vertices for independent contact probes");
    mean.x/=count;mean.y/=count;mean.z/=count;return mean;
}
static float wrap(float phase){return phase-std::floor(phase);}
static void rejects(const std::filesystem::path& path) {
    bool rejected=false;try{validateThreatModelAsset(path);}catch(const std::runtime_error&){rejected=true;}
    check(rejected,"Malformed asset is rejected before it reaches rendering");
}
int main() {
    const std::filesystem::path assetPath="assets/models/aranea/aranea.brm";
    validateThreatModelAsset(assetPath);
    std::ifstream assetFile(assetPath,std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(assetFile)),{});
    auto readU32=[&](std::size_t at) {
        check(at+4<=bytes.size(),"Source field is within the asset");
        return std::uint32_t(static_cast<unsigned char>(bytes[at]))|
            (std::uint32_t(static_cast<unsigned char>(bytes[at+1]))<<8)|
            (std::uint32_t(static_cast<unsigned char>(bytes[at+2]))<<16)|
            (std::uint32_t(static_cast<unsigned char>(bytes[at+3]))<<24);
    };
    auto readFloat=[&](std::size_t at){const auto bits=readU32(at);float f;std::memcpy(&f,&bits,4);return f;};
    const auto sourceVertices=readU32(12),sourceTriangles=readU32(16);
    check(sourceVertices==SOURCE_VERTICES&&sourceTriangles==SOURCE_TRIANGLES,"Source topology has its documented vertex and triangle counts");
    const std::size_t topologyStart=32+7*28,maskStart=topologyStart+sourceTriangles*16;
    const std::size_t framesStart=maskStart+sourceVertices*13;
    expandedSource.reserve(sourceTriangles*3);
    sourceToExpanded.fill(std::numeric_limits<std::size_t>::max());
    for(std::size_t triangle=0;triangle<sourceTriangles;++triangle)for(int corner=0;corner<3;++corner) {
        const auto source=readU32(topologyStart+triangle*16+corner*4);
        check(source<sourceVertices,"Expanded topology references a valid supplied vertex");
        const auto at=maskStart+source*13;
        SourceVertex v{};v.head=static_cast<std::uint8_t>(bytes[at]);
        for(std::size_t foot=0;foot<4;++foot)v.feet[foot]=static_cast<std::uint8_t>(bytes[at+1+foot]);
        v.u=readFloat(at+5);v.v=readFloat(at+9);
        sourceToExpanded[source]=expandedSource.size();expandedSource.push_back(v);
    }
    const auto neutral=threatModelVertices(0,0,false);
    const auto neutralBounds=validate(neutral);
    const auto neutralEyes=eyeSockets(neutral);
    check(neutralEyes[0].x<-.08f&&neutralEyes[1].x>.08f&&
          std::abs(neutralEyes[0].x+neutralEyes[1].x)<.001f,
          "The neutral skull is centered on the movement axis, not offset sideways");
    check(std::abs(neutralEyes[1].y-neutralEyes[0].y)<.001f&&
          std::abs(neutralEyes[1].z-neutralEyes[0].z)<.001f&&neutralEyes[0].z<-.7f,
          "The actual baked orbital rims are level and face -Z, with no source-pose yaw bias");
    check(neutralBounds.maxY>1.9f&&neutralBounds.maxX-neutralBounds.minX>2.2f&&neutralBounds.maxZ-neutralBounds.minZ>2.5f,
          "The wide four-legged source silhouette is retained");
    float fullHeight=0,halfHeight=0;
    for(int i=0;i<64;++i)for(float speed:{0.f,.3f,1.1f,2.2f,3.3f}) {
        const float phase=i/64.f;
        const auto current=threatModelVertices(i*.13f,speed,false,0,0,phase);
        validate(current);
        const auto half=validate(threatModelVertices(i*.13f,speed,false,0,.5f,phase));
        const auto full=validate(threatModelVertices(i*.13f,speed,false,0,1,phase));
        halfHeight=std::max(halfHeight,half.maxY);fullHeight=std::max(fullHeight,full.maxY);
        check(delta(current,threatModelVertices(i*.13f,speed,false,0,0,phase))==0,"Paused identical inputs produce identical geometry");
        check(delta(current,threatModelVertices(i*.13f,speed,true,0,0,phase))==0,"Fast Search uses the same physical gait as Chase");
    }
    check(fullHeight<2.30f&&halfHeight<2.7f,"Crouched locomotion clears the doorway height");
    const auto walkA=threatModelVertices(0,1.1f,false,0,0,.25f),walkB=threatModelVertices(0,1.1f,false,0,0,.75f);
    const auto runA=threatModelVertices(0,3.3f,true,0,0,.25f),runB=threatModelVertices(0,3.3f,true,0,0,.75f);
    check(delta(walkA,walkB,0)>.03f,"Walking articulates the legs through separate steps");
    check(delta(runA,runB,0)>.03f,"Running articulates the legs through separate steps");
    check(delta(walkA,runA,0)>.03f,"Running has a distinct four-legged gait");
    check(std::abs(threatStrideLength(1.1f)-1.30f)<.00001f&&std::abs(threatStrideLength(3.3f)-2.0f)<.00001f,
          "Animation and world motion share the baked full-cycle stride lengths");
    check(std::abs(threatStrideLength(2.25f)-1.65f)<.00001f,"Stride blending matches the smooth walk/run midpoint");
    const std::array<float,4> walkOffsets{{0,.5f,.75f,.25f}},runOffsets{{0,.5f,.5f,0}};
    for(float speed:{1.1f,3.3f})for(std::size_t foot=0;foot<4;++foot) {
        const auto& offsets=speed<2?walkOffsets:runOffsets;
        const float phaseA=wrap(.1f-offsets[foot]),phaseB=wrap(.2f-offsets[foot]);
        const auto a=footMean(threatModelVertices(0,speed,false,0,0,phaseA),foot);
        const auto b=footMean(threatModelVertices(0,speed,false,0,0,phaseB),foot);
        const float rootTravel=threatStrideLength(speed)*.1f;
        check(std::abs(b.z-a.z-rootTravel)<.015f,"Every planted foot cancels root travel instead of sliding");
        check(std::abs(b.x-a.x)<.01f&&std::abs(b.y-a.y)<.01f,"Support tips stay flat during body weight transfer");
        const float swingPhase=wrap((speed<2?.89f:.79f)-offsets[foot]);
        const auto swing=footMean(threatModelVertices(0,speed,false,0,0,swingPhase),foot);
        check(swing.y>a.y+.02f,"Every swinging leg lifts its tip clear of the floor");
    }
    for(float speed:{1.1f,2.2f,3.3f}) {
        check(delta(threatModelVertices(0,speed,false,0,0,.99999f),threatModelVertices(0,speed,false,0,0,.00001f))<.0001f,
              "Locomotion loops and blended gaits close continuously");
        const float blend=std::clamp((speed-1.4f)/1.7f,0.f,1.f),runWeight=blend*blend*(3-2*blend);
        const float duration=1.2f*(1-runWeight)+.65f*runWeight;
        check(delta(threatModelVertices(.37f,speed,false),threatModelVertices(.37f,speed,false,0,0,wrap(.37f/duration)))<.000001f,
              "Time-based fallback uses the actual blended clip duration");
    }
    check(delta(threatModelVertices(1,1.399f,false,0,0,.32f),threatModelVertices(1,1.401f,true,0,0,.32f))<.001f,
          "Speed transitions preserve the shared stride phase");
    const auto headLeft=threatModelVertices(0,1.1f,false,0,0,.25f,-.8f,-.15f);
    const auto headRight=threatModelVertices(0,1.1f,false,0,0,.25f,.8f,.10f);
    validate(headLeft);validate(headRight);
    check(delta(headLeft,headRight,1)>.1f,"The head turns within the mesh");
    check(delta(headLeft,headRight,0)==0,"Head tracking leaves all non-head geometry stationary, including high leg joints");
    check(delta(headLeft,threatModelVertices(0,1.1f,false,0,0,.25f,-.8f,-.15f))==0,"Paused head pose is deterministic");
    check(delta(threatModelVertices(0,1.1f,false,0,0,.25f,.8f,.2f),threatModelVertices(0,1.1f,false,0,0,.25f,9,9))==0,
          "Head controls clamp to their anatomical rotation limits");
    for(float speed:{1.1f,3.3f}) {
        std::array<int,4> landings{};
        auto previous=threatFootContacts(speed,.0001f);
        for(int frame=1;frame<=400;++frame) {
            const auto current=threatFootContacts(speed,float(frame)/400+.0001f);
            int support=0;
            for(std::size_t foot=0;foot<4;++foot) {
                check(current.feet[foot]>=0&&current.feet[foot]<=1,"Foot contact confidence is bounded");
                landings[foot]+=current.feet[foot]>.5f&&previous.feet[foot]<=.5f;
                support+=current.feet[foot]>.5f;
            }
            check(support>=(speed<2?3:2),"Walk keeps three supporting feet; diagonal running keeps at least two");
            previous=current;
        }
        check(std::all_of(landings.begin(),landings.end(),[](int n){return n==1;}),"Each of the four feet lands exactly once per cycle");
    }
    for(std::size_t foot=0;foot<4;++foot) {
        const auto contacts=threatFootContacts(1.1f,wrap(.89f-walkOffsets[foot]));
        for(std::size_t other=0;other<4;++other)
            check(other==foot?contacts.feet[other]<.01f:contacts.feet[other]>.99f,"Walking lifts one leg at a time in the baked order");
    }
    const auto diagonalA=threatFootContacts(3.3f,.8f),diagonalB=threatFootContacts(3.3f,.3f);
    check(diagonalA.feet[0]<.01f&&diagonalA.feet[3]<.01f&&diagonalA.feet[1]>.99f&&diagonalA.feet[2]>.99f&&
          diagonalB.feet[0]>.99f&&diagonalB.feet[3]>.99f&&diagonalB.feet[1]<.01f&&diagonalB.feet[2]<.01f,
          "Running alternates opposing diagonal support pairs");
    // Sample every authored capture frame plus the interpolated half-frames.
    // Zero selects gameplay idle, so a tiny positive amount probes scare start.
    for(int i=0;i<=54;++i) {
        const auto capture=threatModelVertices(0,0,true,std::max(.000001f,i/54.f));
        validate(capture,false);
        const auto eyes=eyeSockets(capture);
        check(std::abs((eyes[0].x+eyes[1].x)*.5f)<.001f&&
              std::abs((eyes[0].y+eyes[1].y)*.5f-THREAT_EYE_HEIGHT)<.001f&&
              std::abs((eyes[0].z+eyes[1].z)*.5f)<.001f,
              "Capture keeps the actual orbital midpoint at the camera-facing eye anchor");
        check(eyes[1].x-eyes[0].x>.17f&&std::abs(eyes[1].z-eyes[0].z)<.009f,
              "Capture head jitter preserves a frontal face instead of turning to profile");
    }
    const auto captureStart=threatModelVertices(0,0,true,.02f),captureLunge=threatModelVertices(0,0,true,.7f);
    check(delta(captureStart,captureLunge,1)>.04f,"The capture head and jaw move despite their stabilized eye anchor");
    for(std::size_t foot=0;foot<2;++foot) {
        const auto a=footMean(captureStart,foot),b=footMean(captureLunge,foot);
        check(b.y-a.y>.5f&&b.z-a.z<-.2f,"Capture raises both front legs and reaches toward the player");
    }
    check(delta(threatModelVertices(0,0,true,.7f,0,-1,-.8f),threatModelVertices(0,0,true,.7f,0,-1,.8f))==0,
          "Capture keeps its authored head motion independent of gameplay tracking");
    validate(threatModelVertices(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),true,-3,8));
    std::filesystem::create_directories("tests/artifacts/aranea");
    const std::filesystem::path bad="tests/artifacts/aranea/invalid-aranea.brm";
    auto write=[&](const std::string& s){std::ofstream out(bad,std::ios::binary);out.write(s.data(),static_cast<std::streamsize>(s.size()));check(bool(out),"Malformed fixture was written");};
    auto putU32=[](std::string& s,std::size_t at,std::uint32_t value){for(int i=0;i<4;++i)s[at+i]=static_cast<char>((value>>(i*8))&255);};
    auto corrupt=[&](auto edit){auto s=bytes;edit(s);write(s);rejects(bad);};
    write(bytes.substr(0,40));rejects(bad);
    corrupt([](std::string& s){s[0]='X';});
    corrupt([&](std::string& s){putU32(s,8,4);});
    corrupt([&](std::string& s){putU32(s,12,20001);});
    corrupt([&](std::string& s){putU32(s,16,25001);});
    corrupt([](std::string& s){s[32]='X';});
    corrupt([&](std::string& s){putU32(s,topologyStart,sourceVertices);});
    corrupt([&](std::string& s){s[topologyStart+12]=char(23);});
    corrupt([&](std::string& s){s[maskStart+1]=char(255);s[maskStart+2]=char(255);});
    corrupt([&](std::string& s){for(std::size_t i=0;i<sourceVertices;++i)s[maskStart+i*13]=0;});
    corrupt([&](std::string& s){for(std::size_t i=0;i<sourceVertices;++i)s[maskStart+i*13+4]=0;});
    corrupt([&](std::string& s){putU32(s,maskStart+5,0x7fc00000);});
    corrupt([&](std::string& s){putU32(s,maskStart+9,0x47c35000);}); // U/V must stay strictly below 100,000.
    corrupt([&](std::string& s){s[framesStart+6]=char(16);});
    corrupt([&](std::string& s){s[framesStart+7]=char(1);});
    corrupt([&](std::string& s){for(std::size_t i=0;i<6;++i)s[framesStart+8+6+i]=0;});
    write(bytes+"trailing");rejects(bad);
    std::filesystem::remove(bad);
    std::cout<<"Aranea model tests passed: 15220 supplied triangles, four articulated feet, sequential walk and diagonal run, planted contacts, forward-facing orbital landmarks, anchored capture, independent head, preserved UVs, full stoop "
             <<fullHeight<<"m, half stoop "<<halfHeight<<"m; malformed asset validation passed.\n";
}
