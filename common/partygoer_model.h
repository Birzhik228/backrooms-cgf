#pragma once
#include "partygoer.h"
namespace br {
// Procedural, low-poly costume: stained yellow sack, painted face, red balloon.
// Fewer than 1500 triangles per figure; no external model or texture dependency.
inline std::vector<Vertex> partygoerVertices(const Partygoers& group,double originX,double originZ) {
    std::vector<Vertex> out;out.reserve(16000);
    for(const auto& actor:group.actors) {
        if(!actor.active)continue;
        const float cy=std::cos(actor.yaw),sy=std::sin(actor.yaw);
        auto emit=[&](Vec3 p,Vec3 n,float u,float v,float mat) {
            out.push_back({float(actor.x-originX)+p.x*cy-p.z*sy,p.y,float(actor.z-originZ)+p.x*sy+p.z*cy,
                n.x*cy-n.z*sy,n.y,n.x*sy+n.z*cy,u,v,mat});
        };
        auto quad=[&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,float mat) {
            Vec3 n=normalize(cross(b-a,c-a));
            const Vec3 p[]={a,b,c,d};
            const float uv[][2]={{0,0},{1,0},{1,1},{0,1}};
            for(int i:{0,1,2,0,2,3})emit(p[i],n,uv[i][0],uv[i][1],mat);
        };
        auto box=[&](float x,float y,float z,float rx,float ry,float rz,float mat) {
            quad({x-rx,y-ry,z-rz},{x-rx,y+ry,z-rz},{x+rx,y+ry,z-rz},{x+rx,y-ry,z-rz},mat);
            quad({x+rx,y-ry,z+rz},{x+rx,y+ry,z+rz},{x-rx,y+ry,z+rz},{x-rx,y-ry,z+rz},mat);
            quad({x-rx,y-ry,z+rz},{x-rx,y+ry,z+rz},{x-rx,y+ry,z-rz},{x-rx,y-ry,z-rz},mat);
            quad({x+rx,y-ry,z-rz},{x+rx,y+ry,z-rz},{x+rx,y+ry,z+rz},{x+rx,y-ry,z+rz},mat);
            quad({x-rx,y+ry,z-rz},{x-rx,y+ry,z+rz},{x+rx,y+ry,z+rz},{x+rx,y+ry,z-rz},mat);
            quad({x-rx,y-ry,z+rz},{x-rx,y-ry,z-rz},{x+rx,y-ry,z-rz},{x+rx,y-ry,z+rz},mat);
        };
        auto orb=[&](Vec3 center,Vec3 radius,float mat) {
            auto point=[&](float a,float b){return center+Vec3{radius.x*std::cos(a)*std::cos(b),radius.y*std::sin(a),radius.z*std::cos(a)*std::sin(b)};};
            for(int r=0;r<6;++r)for(int s=0;s<10;++s) {
                float a=-PI/2+r*PI/6,b=a+PI/6,c=s*2*PI/10,d=c+2*PI/10;
                Vec3 points[]={point(a,c),point(b,c),point(b,d),point(a,d)};
                for(int i:{0,1,2,0,2,3}) {
                    Vec3 delta=points[i]-center;
                    emit(points[i],normalize({delta.x/(radius.x*radius.x),delta.y/(radius.y*radius.y),delta.z/(radius.z*radius.z)}),float(s)/10,float(r)/6,mat);
                }
            }
        };
        auto limb=[&](Vec3 a,Vec3 b,float radius,float mat) {
            Vec3 axis=normalize(b-a),right=normalize(cross(axis,{0,0,1})),up=cross(axis,right);
            for(int i=0;i<8;++i) {
                float t=i*2*PI/8,u=(i+1)*2*PI/8;
                Vec3 p=right*(std::cos(t)*radius)+up*(std::sin(t)*radius),q=right*(std::cos(u)*radius)+up*(std::sin(u)*radius);
                quad(a+p,a+q,b+q,b+p,mat);
            }
        };
        const float swing=std::sin(actor.phase),bob=std::sin(actor.phase*2)*.012f;
        // Broad, slightly uneven sack hem narrowing into the shoulders.
        const float ys[]={.12f,.38f,1.15f,1.64f,1.77f};
        const float widths[]={.48f,.46f,.32f,.39f,.23f};
        for(int r=0;r<4;++r)for(int i=0;i<12;++i) {
            auto point=[&](int ring,int side){float t=side*2*PI/12;return Vec3{std::cos(t)*widths[ring],ys[ring]+bob+(ring==0?std::sin(t*3+actor.phase)*.018f:0),std::sin(t)*widths[ring]*.75f};};
            quad(point(r,i),point(r+1,i),point(r+1,i+1),point(r,i+1),55);
        }
        for(float side:{-1.f,1.f}) {
            box(side*.20f,.095f,-side*swing*.05f,.115f,.095f,.17f,55);
            Vec3 shoulder{side*.37f,1.62f+bob,0},elbow{side*.56f,1.23f+bob,side<0?-.20f:.08f+swing*.045f};
            Vec3 hand{side*.64f,side<0?1.12f: .88f,side<0?-.43f:-.05f+swing*.06f};
            orb(shoulder,{.17f,.18f,.18f},55);limb(shoulder,elbow,.13f,55);orb(elbow,{.13f,.13f,.13f},55);limb(elbow,hand,.105f,55);orb(hand,{.12f,.13f,.12f},55);
        }
        // Rounded head with the painted face mapped onto the curved surface,
        // rather than a flat card that would retain the old square silhouette.
        constexpr int headRings=8,headSides=16;
        const Vec3 headCenter{0,1.99f+bob,0},headRadius{.275f,.31f,.26f};
        auto headPoint=[&](float latitude,float longitude) {
            return Vec3{headRadius.x*std::cos(latitude)*std::cos(longitude),
                headRadius.y*std::sin(latitude),headRadius.z*std::cos(latitude)*std::sin(longitude)};
        };
        for(int ring=0;ring<headRings;++ring)for(int side=0;side<headSides;++side) {
            float a=-PI/2+ring*PI/headRings,b=a+PI/headRings;
            float c=side*2*PI/headSides,d=c+2*PI/headSides;
            const Vec3 points[]={headPoint(a,c),headPoint(b,c),headPoint(b,d),headPoint(a,d)};
            auto headVertex=[&](int i) {
                const Vec3 p=points[i];
                const Vec3 normal=normalize({p.x/(headRadius.x*headRadius.x),
                    p.y/(headRadius.y*headRadius.y),p.z/(headRadius.z*headRadius.z)});
                emit(headCenter+p,normal,.5f-p.x/(2*headRadius.x),.5f+p.y/(2*headRadius.y),
                    side>=headSides/2?56:55);
            };
            if(ring<headRings-1)for(int i:{0,1,2})headVertex(i);
            if(ring>0)for(int i:{0,2,3})headVertex(i);
        }
        const Vec3 balloon{-.72f+std::sin(actor.phase*.7f)*.035f,2.63f,-.39f};
        orb(balloon,{.22f,.29f,.22f},48);
        orb(balloon+Vec3{0,-.29f,0},{.025f,.035f,.025f},48);
        limb({-.64f,1.12f,-.43f},balloon+Vec3{0,-.32f,0},.005f,45);
    }
    return out;
}
} // namespace br
