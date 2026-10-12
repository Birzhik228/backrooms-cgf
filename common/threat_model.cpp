#include "threat_model.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace br {
namespace {

constexpr std::size_t MAX_ASSET_BYTES=48*1024*1024;
constexpr std::size_t CLIP_COUNT=7;
constexpr float POSITION_UNIT=1.f/8192.f,NORMAL_UNIT=1.f/32767.f;
struct Point { float x=0,y=0,z=0; };
Point operator+(Point a,Point b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Point operator*(Point a,float b){return {a.x*b,a.y*b,a.z*b};}
Point normalized(Point p) {
    const float d=p.x*p.x+p.y*p.y+p.z*p.z;
    return d>1e-12f?p*(1.f/std::sqrt(d)):Point{0,1,0};
}
struct Sample { Point p,n; };
struct PackedVertex { std::int16_t px,py,pz,nx,ny,nz; };
struct Triangle { std::array<std::uint32_t,3> vertices; std::uint8_t material; };
struct FrameInfo { Point pivot; std::uint8_t contacts=0; };
struct SkinMask { std::uint8_t head; std::array<std::uint8_t,4> feet; };
struct TexCoord { float u,v; };
struct Clip { std::uint32_t frames=0; float duration=0; bool loop=false; std::vector<PackedVertex> values; std::vector<FrameInfo> info; };
struct Model { std::uint32_t vertices=0; std::vector<Triangle> triangles; std::vector<SkinMask> masks; std::vector<TexCoord> uvs; std::array<Clip,CLIP_COUNT> clips; };

struct Reader {
    std::vector<unsigned char> bytes;
    std::size_t offset=0;
    explicit Reader(const std::filesystem::path& path) {
        std::ifstream input(path,std::ios::binary|std::ios::ate);
        if(!input)throw std::runtime_error("Aranea model is missing: "+path.string()+". Keep assets/models/aranea beside Backrooms.exe.");
        const auto size=input.tellg();
        if(size<32||size>static_cast<std::streamoff>(MAX_ASSET_BYTES))fail("invalid file size");
        bytes.resize(static_cast<std::size_t>(size));
        input.seekg(0);
        input.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
        if(!input)fail("incomplete read");
    }
    [[noreturn]] static void fail(const char* reason) {
        throw std::runtime_error(std::string("Invalid Aranea animation asset: ")+reason);
    }
    void require(std::size_t count)const {if(count>bytes.size()-offset)fail("truncated data");}
    std::uint8_t u8(){require(1);return bytes[offset++];}
    std::uint16_t u16(){const auto a=u8();return static_cast<std::uint16_t>(a|(std::uint16_t(u8())<<8));}
    std::int16_t i16(){const auto n=u16();return static_cast<std::int16_t>(n<=32767?n:static_cast<int>(n)-65536);}
    std::uint32_t u32(){const auto a=u16();return a|(std::uint32_t(u16())<<16);}
    float f32(){const auto bits=u32();float out;std::memcpy(&out,&bits,4);return out;}
    std::string chars(std::size_t count){require(count);std::string s(reinterpret_cast<const char*>(bytes.data()+offset),count);offset+=count;return s;}
};
Model readModel(const std::filesystem::path& path) {
    Reader in(path);
    if(in.chars(8)!=std::string("BRARAN3\0",8)||in.u32()!=3)Reader::fail("unsupported format");
    Model model;
    model.vertices=in.u32();
    const auto triangles=in.u32(),clips=in.u32();
    if(model.vertices<100||model.vertices>20000||triangles<100||triangles>25000||clips!=CLIP_COUNT)
        Reader::fail("count exceeds model budget");
    if(in.f32()!=POSITION_UNIT||in.f32()!=NORMAL_UNIT)Reader::fail("unsupported quantization");
    constexpr std::array<const char*,CLIP_COUNT> names={"idle","walk","run","idle_duck","walk_duck","run_duck","scare"};
    std::size_t totalFrames=0;
    for(std::size_t k=0;k<CLIP_COUNT;++k) {
        auto& c=model.clips[k];
        const auto raw=in.chars(16);
        const auto name=raw.substr(0,raw.find('\0'));
        if(name!=names[k])Reader::fail("unexpected clip name/order");
        c.frames=in.u32();c.duration=in.f32();const auto loop=in.u32();
        if(c.frames<2||c.frames>60||!std::isfinite(c.duration)||c.duration<.1f||c.duration>10.f||loop!=std::uint32_t(k!=6))
            Reader::fail("invalid clip descriptor");
        c.loop=loop!=0;totalFrames+=c.frames;
    }
    const std::size_t expected=in.offset+std::size_t(triangles)*16+model.vertices*13+totalFrames*(model.vertices*12+8);
    if(expected!=in.bytes.size())Reader::fail("size/count mismatch or trailing data");
    model.triangles.reserve(triangles);
    for(std::uint32_t k=0;k<triangles;++k) {
        Triangle t{{in.u32(),in.u32(),in.u32()},in.u8()};
        if(t.material!=20&&t.material!=21&&t.material!=22)Reader::fail("unknown material");
        if(t.vertices[0]>=model.vertices||t.vertices[1]>=model.vertices||t.vertices[2]>=model.vertices||
           t.vertices[0]==t.vertices[1]||t.vertices[0]==t.vertices[2]||t.vertices[1]==t.vertices[2])Reader::fail("invalid triangle indices");
        if(in.u8()!=0||in.u8()!=0||in.u8()!=0)Reader::fail("nonzero reserved bytes");
        model.triangles.push_back(t);
    }
    std::size_t headCount=0;
    std::array<std::size_t,4> footCounts{};
    model.masks.reserve(model.vertices);model.uvs.reserve(model.vertices);
    for(std::size_t k=0;k<model.vertices;++k) {
        SkinMask mask{in.u8(),{{in.u8(),in.u8(),in.u8(),in.u8()}}};
        unsigned footWeight=0;
        for(std::size_t foot=0;foot<4;++foot) {
            footWeight+=mask.feet[foot];footCounts[foot]+=mask.feet[foot]>128;
        }
        if(footWeight>255)Reader::fail("overlapping foot weights");
        headCount+=mask.head>128;
        model.masks.push_back(mask);
        const TexCoord uv{in.f32(),in.f32()};
        if(!std::isfinite(uv.u)||!std::isfinite(uv.v)||std::abs(uv.u)>=100000.f||std::abs(uv.v)>=100000.f)
            Reader::fail("invalid texture coordinates");
        model.uvs.push_back(uv);
    }
    if(headCount<5 || std::any_of(footCounts.begin(),footCounts.end(),[](std::size_t count){return count<3;}))
        Reader::fail("missing head or foot weights");
    for(auto& clip:model.clips) {
        clip.values.reserve(std::size_t(clip.frames)*model.vertices);
        for(std::size_t frame=0;frame<clip.frames;++frame) {
            FrameInfo info{{in.i16()*POSITION_UNIT,in.i16()*POSITION_UNIT,in.i16()*POSITION_UNIT},in.u8()};
            if(info.contacts>15 || in.u8()!=0 || info.pivot.y<.5f || info.pivot.y>3.3f ||
               std::abs(info.pivot.x)>2 || std::abs(info.pivot.z)>3)Reader::fail("invalid pose metadata");
            clip.info.push_back(info);
            for(std::size_t k=0;k<model.vertices;++k) {
                PackedVertex p{in.i16(),in.i16(),in.i16(),in.i16(),in.i16(),in.i16()};
                const float n=float(p.nx)*p.nx+float(p.ny)*p.ny+float(p.nz)*p.nz;
                if(n<.90f/(NORMAL_UNIT*NORMAL_UNIT)||n>1.10f/(NORMAL_UNIT*NORMAL_UNIT))Reader::fail("invalid vertex normal");
                clip.values.push_back(p);
            }
        }
    }
    return model;
}
const Model& cachedModel() {
    static const Model model=readModel("assets/models/aranea/aranea.brm");
    return model;
}
float finite(float x,float fallback=0){return std::isfinite(x)?x:fallback;}
float unit(float x){return std::clamp(finite(x),0.f,1.f);}
float smooth(float x){x=unit(x);return x*x*(3-2*x);}
float cycle(float x){return x-std::floor(x);}
struct Cursor {const Clip* clip;std::size_t a,b;float fraction;};
Cursor cursor(const Model& model,std::size_t clip,float phase) {
    const auto& c=model.clips[clip];
    const float value=c.loop?cycle(phase)*c.frames:unit(phase)*(c.frames-1);
    const auto a=static_cast<std::size_t>(value);
    const auto b=c.loop?(a+1)%c.frames:std::min(a+1,std::size_t(c.frames-1));
    return {&c,a*model.vertices,b*model.vertices,value-static_cast<float>(a)};
}
Sample sample(const Cursor& c,std::size_t index) {
    const auto& a=c.clip->values[c.a+index];const auto& b=c.clip->values[c.b+index];
    const float t=c.fraction,s=1-t;
    return {{(a.px*s+b.px*t)*POSITION_UNIT,(a.py*s+b.py*t)*POSITION_UNIT,(a.pz*s+b.pz*t)*POSITION_UNIT},
            {(a.nx*s+b.nx*t)*NORMAL_UNIT,(a.ny*s+b.ny*t)*NORMAL_UNIT,(a.nz*s+b.nz*t)*NORMAL_UNIT}};
}
} // namespace

ThreatFootContacts threatFootContacts(float speed,float locomotionPhase) {
    const auto& model=cachedModel();
    const float run=smooth((finite(speed)-1.4f)/1.7f);
    const float phase=cycle(finite(locomotionPhase));
    auto support=[&](std::size_t clip,int bit) {
        const auto c=cursor(model,clip,phase);
        const auto a=model.clips[clip].info[c.a/model.vertices].contacts;
        const auto b=model.clips[clip].info[c.b/model.vertices].contacts;
        return float((a>>bit)&1)*(1-c.fraction)+float((b>>bit)&1)*c.fraction;
    };
    ThreatFootContacts contacts;
    for(int foot=0;foot<4;++foot)
        contacts.feet[foot]=support(1,foot)*(1-run)+support(2,foot)*run;
    return contacts;
}

float threatStrideLength(float speed) {
    const float run=smooth((finite(speed)-1.4f)/1.7f);
    return 1.30f*(1-run)+2.0f*run;
}

void validateThreatModelAsset(const std::filesystem::path& path){(void)readModel(path);}

std::vector<Vertex> threatModelVertices(float animationTime,float speed,bool chasing,
                                      float scareAmount,float stoopAmount,float locomotionPhase,float headYaw,float headPitch) {
    (void)chasing; // Actual motion selects the gait, including Search and stalls.
    const auto& model=cachedModel();
    const float time=std::fmod(std::max(0.f,finite(animationTime)),100000.f);
    speed=std::clamp(finite(speed),0.f,8.f);
    const float scare=unit(scareAmount),duck=unit(stoopAmount);
    const float walkWeight=smooth(speed/.7f),runWeight=smooth((speed-1.4f)/1.7f);
    const float phase=std::isfinite(locomotionPhase)&&locomotionPhase>=0?cycle(locomotionPhase):cycle(time/(model.clips[1].duration*(1-runWeight)+model.clips[2].duration*runWeight));
    std::array<Cursor,CLIP_COUNT> cursors={cursor(model,0,time/3.2f),cursor(model,1,phase),cursor(model,2,phase),
        cursor(model,3,time/3.2f),cursor(model,4,phase),cursor(model,5,phase),cursor(model,6,scare)};
    const std::array<float,6> weights={(1-walkWeight)*(1-duck),walkWeight*(1-runWeight)*(1-duck),walkWeight*runWeight*(1-duck),
        (1-walkWeight)*duck,walkWeight*(1-runWeight)*duck,walkWeight*runWeight*duck};
    Point pivot{};
    for(std::size_t k=0;k<6;++k)if(weights[k]>0) {
        const auto& c=cursors[k];
        const auto& a=c.clip->info[c.a/model.vertices];
        const auto& b=c.clip->info[c.b/model.vertices];
        pivot=pivot+(a.pivot*(1-c.fraction)+b.pivot*c.fraction)*weights[k];
    }
    headYaw=std::clamp(finite(headYaw),-.8f,.8f);
    headPitch=std::clamp(finite(headPitch),-.25f,.20f);
    std::vector<Sample> posed(model.vertices);
    float floor=std::numeric_limits<float>::max();
    for(std::size_t i=0;i<model.vertices;++i) {
        auto& v=posed[i];
        if(scare>0) v=sample(cursors[6],i);
        else for(std::size_t k=0;k<6;++k)if(weights[k]>0) {
            const auto s=sample(cursors[k],i);
            v.p=v.p+s.p*weights[k];v.n=v.n+s.n*weights[k];
        }
        if(scare<=0 && model.masks[i].head>0) {
            const float weight=model.masks[i].head/255.f;
            const float cy=std::cos(headYaw*weight),sy=std::sin(headYaw*weight);
            const float cp=std::cos(headPitch*weight),sp=std::sin(headPitch*weight);
            auto rotate=[&](Point p) {
                const float y=p.y*cp-p.z*sp,z=p.y*sp+p.z*cp;
                return Point{p.x*cy-z*sy,y,p.x*sy+z*cy};
            };
            const auto r=rotate({v.p.x-pivot.x,v.p.y-pivot.y,v.p.z-pivot.z});
            v.p=pivot+r;v.n=rotate(v.n);
        }
        v.n=normalized(v.n);floor=std::min(floor,v.p.y);
    }
    // Interpolation between alternating foot contacts can lift both soles a
    // fraction; floor normalization keeps locomotion grounded, never the scare.
    const float groundOffset=scare>0?0.f:std::max(0.f,.012f-floor);
    std::vector<Vertex> vertices;vertices.reserve(model.triangles.size()*3);
    for(const auto& t:model.triangles)for(const auto index:t.vertices) {
        const auto& v=posed[index];const auto& uv=model.uvs[index];
        vertices.push_back({v.p.x,v.p.y+groundOffset,v.p.z,v.n.x,v.n.y,v.n.z,
                            uv.u,uv.v,static_cast<float>(t.material)});
    }
    return vertices;
}
} // namespace br
