#pragma once
#include "ending.h"
#include "ending_city.h"
#include "ending_city_feet.h"
#include "ending_sun.h"
#include "Shader.h"
#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace br {
// The ending owns one bounded, cached city set. Animated limbs and traffic use
// model transforms; no geometry is rebuilt while the film plays.
class EndingRenderer {
    struct V {float x,y,z,nx,ny,nz,u,v,r,g,b,m;};
    struct Mesh {gl::VertexArray vao;gl::Buffer vbo;GLsizei count=0;};
    Shader shader_,sunDepth_;
    gl::Framebuffer sunFbo_;
    std::array<gl::Texture,2> staticSun_{},movingSun_{};
    bool shadowPass_=false,staticSunReady_=false;
    float shadowTime_=-1;
    unsigned staticSunUpdates_=0,movingSunUpdates_=0;
    Mesh cabin_,garage_,lobby_,city_,sky_,leftDoor_,rightDoor_,exitDoor_;
    Mesh torso_,hips_,head_,face_,upperArm_,lowerArm_,hand_,thigh_,shin_,shoe_;
    Mesh jacket_,backpack_,bag_,cap_,wheel_,brakeLights_,signalLens_;
    std::array<Mesh,3> hair_{},carPaint_{},carDetails_{};
    std::array<CityFeetPose,EndingCity::pedestrianCount> cityFeet_{};
    float actorTime_=-1;
    std::array<Mesh,4> indicators_{};
    bool ready_=false;
    using Vertices=std::vector<V>;
    static void quad(Vertices& out,Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n,Vec3 color,float material,float w=1,float h=1) {
        const std::array<Vec3,6> pts={a,b,c,a,c,d};
        const float uv[12]={0,0,w,0,w,h,0,0,w,h,0,h};
        for(int i=0;i<6;++i)out.push_back({pts[i].x,pts[i].y,pts[i].z,n.x,n.y,n.z,uv[i*2],uv[i*2+1],color.x,color.y,color.z,material});
    }
    static void box(Vertices& v,Vec3 a,Vec3 b,Vec3 c,float m) {
        const float x=b.x-a.x,y=b.y-a.y,z=b.z-a.z;
        quad(v,{a.x,a.y,b.z},{b.x,a.y,b.z},{b.x,b.y,b.z},{a.x,b.y,b.z},{0,0,1},c,m,x,y);
        quad(v,{b.x,a.y,a.z},{a.x,a.y,a.z},{a.x,b.y,a.z},{b.x,b.y,a.z},{0,0,-1},c,m,x,y);
        quad(v,{a.x,a.y,a.z},{a.x,a.y,b.z},{a.x,b.y,b.z},{a.x,b.y,a.z},{-1,0,0},c,m,z,y);
        quad(v,{b.x,a.y,b.z},{b.x,a.y,a.z},{b.x,b.y,a.z},{b.x,b.y,b.z},{1,0,0},c,m,z,y);
        quad(v,{a.x,b.y,b.z},{b.x,b.y,b.z},{b.x,b.y,a.z},{a.x,b.y,a.z},{0,1,0},c,m,x,z);
        quad(v,{a.x,a.y,a.z},{b.x,a.y,a.z},{b.x,a.y,b.z},{a.x,a.y,b.z},{0,-1,0},c,m,x,z);
    }
    static Mat4 rotateY(float a){auto m=Mat4::identity();m.m[0]=m.m[10]=std::cos(a);m.m[8]=std::sin(a);m.m[2]=-std::sin(a);return m;}
    static Mat4 rotateX(float a){auto m=Mat4::identity();m.m[5]=m.m[10]=std::cos(a);m.m[9]=-std::sin(a);m.m[6]=std::sin(a);return m;}
    static Mat4 rotateZ(float a){auto m=Mat4::identity();m.m[0]=m.m[5]=std::cos(a);m.m[4]=-std::sin(a);m.m[1]=std::sin(a);return m;}
    static Mat4 scale(float x,float y,float z){auto m=Mat4::identity();m.m[0]=x;m.m[5]=y;m.m[10]=z;return m;}
    // Eight-sided tapered volumes soften silhouettes without subdividing flat
    // surfaces or introducing texture/model dependencies.
    static void tapered(Vertices& v,float bottom,float top,Vec3 lower,Vec3 upper,Vec3 color,float material) {
        for(int i=0;i<8;++i) {
            const float a=2*PI*i/8,b=2*PI*(i+1)/8;
            const Vec3 p{lower.x*std::cos(a),bottom,lower.z*std::sin(a)},q{lower.x*std::cos(b),bottom,lower.z*std::sin(b)},
                       r{upper.x*std::cos(b),top,upper.z*std::sin(b)},s{upper.x*std::cos(a),top,upper.z*std::sin(a)};
            quad(v,p,q,r,s,normalize(cross(s-p,q-p)),color,material,1,top-bottom);
            quad(v,{0,bottom,0},q,p,{0,bottom,0},{0,-1,0},color,material);
            quad(v,{0,top,0},s,r,{0,top,0},{0,1,0},color,material);
        }
    }
    static void ellipsoid(Vertices& v,Vec3 c,Vec3 h,Vec3 color,float m,int bands=6,int sides=10) {
        auto point=[&](float a,float b){return Vec3{c.x+h.x*std::sin(a)*std::cos(b),c.y+h.y*std::cos(a),c.z+h.z*std::sin(a)*std::sin(b)};};
        for(int row=0;row<bands;++row)for(int col=0;col<sides;++col) {
            const float a=row*PI/bands,b=(row+1)*PI/bands,t=col*2*PI/sides,u=(col+1)*2*PI/sides;
            const Vec3 n=normalize(Vec3{std::sin((a+b)*.5f)*std::cos((t+u)*.5f)/h.x,std::cos((a+b)*.5f)/h.y,std::sin((a+b)*.5f)*std::sin((t+u)*.5f)/h.z});
            quad(v,point(a,t),point(a,u),point(b,u),point(b,t),n,color,m);
        }
    }
    static void window(Vertices& v,float x,float y,float z,float w,float h,bool front=true) {
        const Vec3 frame{.72f,.75f,.73f},glass{.22f,.38f,.47f};
        box(v,{x-.07f,y-.06f,z-.04f},{x+w+.07f,y+h+.07f,z+.04f},frame,1);
        const float face=z+(front?.046f:-.046f),n=front?1.f:-1.f;
        quad(v,{x,y,face},{x+w,y,face},{x+w,y+h,face},{x,y+h,face},{0,0,n},glass,13,w,h);
        box(v,{x+w*.5f-.025f,y,z-.06f},{x+w*.5f+.025f,y+h,z+.06f},frame,1);
        box(v,{x-.12f,y-.12f,z-.10f},{x+w+.12f,y-.055f,z+.13f},{.65f,.64f,.59f},0);
    }
    static void tree(Vertices& v,float x,float z,float size=1) {
        box(v,{x-.17f,0,z-.17f},{x+.17f,3.2f*size,z+.17f},{.24f,.17f,.105f},2);
        ellipsoid(v,{x,3.3f*size,z},{1.35f*size,1.65f*size,1.3f*size},{.29f,.42f,.19f},14,6,9);
        ellipsoid(v,{x-.7f*size,3.2f*size,z+.15f},{.95f*size,1.15f*size,.95f*size},{.36f,.48f,.23f},14,5,8);
        box(v,{x-.75f,-.01f,z-.75f},{x+.75f,.03f,z+.75f},{.23f,.25f,.19f},0);
    }
    static void storefront(Vertices& out,float x,float z,float width,int kind,bool front=true) {
        Vertices v;kind%=6;
        const std::array<Vec3,6> paints={Vec3{.14f,.28f,.24f},Vec3{.19f,.25f,.34f},Vec3{.52f,.29f,.18f},Vec3{.34f,.40f,.29f},Vec3{.37f,.23f,.33f},Vec3{.28f,.34f,.38f}};
        const Vec3 paint=paints[kind],ivory{.83f,.79f,.66f};
        const char* labels[]={"CAFE","BOOKS","BAKERY","MARKET","FLOWERS","CORNER"};
        box(v,{.08f,.04f,0},{width-.08f,.40f,.12f},paint,19);
        box(v,{.08f,3.12f,0},{width-.08f,3.78f,.20f},paint,19);
        box(v,{.04f,3.78f,-.02f},{width-.04f,3.88f,.25f},ivory,19);
        const float bay=(width-.5f)/3;
        for(int i=0;i<3;++i) {
            const float a=.25f+i*bay,b=a+bay-.14f;
            const Vec3 glass=i==1?Vec3{.16f,.24f,.26f}:Vec3{.21f,.30f,.31f};
            quad(v,{a,.42f,.02f},{b,.42f,.02f},{b,3.1f,.02f},{a,3.1f,.02f},{0,0,1},glass,13,b-a,2.7f);
            for(float side:{a-.06f,b})box(v,{side,.35f,0},{side+.06f,3.16f,.16f},ivory,19);
            box(v,{a,2.63f,.07f},{b,2.70f,.16f},ivory,19);
            if(i==1) {
                box(v,{b-.19f,1.05f,.15f},{b-.14f,1.39f,.21f},{.55f,.48f,.29f},1);
                box(v,{a+.25f,1.8f,.055f},{b-.25f,2.12f,.07f},paint,0);
                words(v,"OPEN",a+(b-a)*.5f-.24f,1.87f,.08f,.020f,ivory,0);
                continue;
            }
            if(kind==1) { // Books: differently sized spines and shelf rails.
                for(int row=0;row<3;++row) {
                    const float y=.64f+row*.58f;
                    box(v,{a+.10f,y,.08f},{b-.10f,y+.055f,.20f},{.36f,.23f,.14f},2);
                    for(int book=0;book<10;++book) {
                        const float xx=a+.18f+book*(b-a-.38f)/10;
                        const Vec3 c=paints[(book+row)%6];
                        box(v,{xx,y+.06f,.09f},{xx+.16f,y+.34f+.05f*(book%3),.14f},c,19);
                    }
                }
            }else if(kind==2) { // Bakery: warm display shelves and bread silhouettes.
                for(int row=0;row<2;++row) {
                    const float y=.70f+row*.72f;
                    box(v,{a+.10f,y,.08f},{b-.10f,y+.07f,.21f},ivory,19);
                    for(int bread=0;bread<5;++bread)ellipsoid(v,{a+.4f+bread*(b-a-.8f)/4,y+.15f,.15f},{.20f,.11f,.08f},{.66f,.42f,.18f},20,4,6);
                }
            }else if(kind==4) {
                for(int flower=0;flower<5;++flower) {
                    const float xx=a+.4f+flower*(b-a-.8f)/4,y=.9f+.12f*(flower%2);
                    box(v,{xx-.10f,.56f,.10f},{xx+.10f,.84f,.25f},{.46f,.29f,.19f},19);
                    box(v,{xx-.012f,.80f,.15f},{xx+.012f,y+.3f,.18f},{.20f,.35f,.13f},0);
                    ellipsoid(v,{xx,y+.27f,.18f},{.15f,.15f,.08f},flower%2?Vec3{.75f,.61f,.24f}:Vec3{.63f,.28f,.32f},0,4,6);
                }
            }else {
                // Menu cards / produce crate accents stay behind the outer frames.
                box(v,{a+.25f,.72f,.08f},{b-.25f,.81f,.25f},{.33f,.24f,.15f},2);
                for(int item=0;item<3;++item)box(v,{a+.4f+item*(b-a-.9f)/3,.82f,.09f},{a+.67f+item*(b-a-.9f)/3,1.25f+.12f*(item%2),.15f},item%2?ivory:paint,19);
                box(v,{a+.45f,1.6f,.04f},{b-.45f,2.23f,.07f},ivory,19);
                for(int line=0;line<4;++line)box(v,{a+.58f,1.72f+line*.10f,.075f},{b-.59f,1.735f+line*.10f,.08f},paint,0);
            }
        }
        const float textSize=.060f,textWidth=float(std::char_traits<char>::length(labels[kind]))*textSize*6;
        words(v,labels[kind],(width-textWidth)*.5f,3.22f,.21f,textSize,ivory,0);
        // Sloped fabric and a short valance; striped cafe/bakery awnings are
        // distinguishable from the bookshop's projecting flat canopy.
        const float awningDepth=kind==1?.46f:.84f;
        for(int strip=0;strip<24;++strip) {
            const float a=.12f+(width-.24f)*strip/24,b=.12f+(width-.24f)*(strip+1)/24;
            const Vec3 c=((kind==0||kind==2)&&strip%2)?ivory:paint;
            quad(v,{a,3.11f,.17f},{b,3.11f,.17f},{b,2.88f,awningDepth},{a,2.88f,awningDepth},{0,.96f,.28f},c,20,b-a,awningDepth);
            box(v,{a,2.76f,awningDepth-.02f},{b,2.88f,awningDepth+.02f},c,20);
        }
        const float sign=front?1.f:-1.f;
        for(auto q:v) {
            q.x=front?x+q.x:x+width-q.x;q.z=z+q.z*sign;
            q.nx*=sign;q.nz*=sign;out.push_back(q);
        }
    }
    void buildCity() {
        Vertices v;const Vec3 pavement{.66f,.65f,.60f},curb{.64f,.65f,.63f},asphalt{.20f,.22f,.24f};
        // Continuous road and pavements extend behind distant fog, without a
        // visible platform edge. Traffic and camera live on separate surfaces.
        box(v,{-110,-.24f,-27},{110,-.15f,-17},asphalt,12);
        box(v,{-110,-.18f,-17},{110,0,-10.69f},pavement,11);
        box(v,{-110,-.18f,-32},{110,0,-27},pavement,11);
        box(v,{-110,-.16f,-17.16f},{110,.035f,-16.94f},curb,0);
        box(v,{-110,-.16f,-27.04f},{110,.035f,-26.84f},curb,0);
        for(int x=-100;x<100;x+=7)box(v,{float(x),-.144f,-22.08f},{float(x+3),-.138f,-21.92f},{.91f,.87f,.62f},0);
        // Zebra crossing sits down the street, leaving the lift's view open.
        for(int lane=0;lane<9;++lane)box(v,{15.4f,-.14f,-26.6f+lane},{19.7f,-.135f,-26.1f+lane},{.88f,.89f,.86f},0);
        const std::array<Vec3,6> facades={Vec3{.65f,.48f,.36f},Vec3{.72f,.70f,.61f},Vec3{.47f,.56f,.57f},Vec3{.75f,.64f,.46f},Vec3{.56f,.40f,.33f},Vec3{.65f,.67f,.65f}};
        // Mid-rise blocks: different heights, real inset shop windows, cornices,
        // balconies and rooftop machinery supply recognizable street scale.
        for(int k=0;k<13;++k) {
            const float x=-78.f+k*12.5f,w=11.8f,height=12.f+float((k*7)%5)*3.f;
            const Vec3 color=facades[k%6];
            box(v,{x,0,-45},{x+w,height,-32},color,k%3==1?19:10);
            box(v,{x-.13f,height,-45.1f},{x+w+.13f,height+.26f,-31.85f},{.46f,.47f,.43f},0);
            box(v,{x+.7f,height+.25f,-40},{x+3.2f,height+1.1f,-37.5f},{.35f,.40f,.39f},1);
            for(float y=4.2f;y<height-1.6f;y+=3.3f) {
                box(v,{x-.06f,y-.43f,-32.05f},{x+w+.06f,y-.25f,-31.86f},{.72f,.71f,.65f},0);
                for(int col=0;col<4;++col)window(v,x+.8f+col*2.7f,y,-31.94f,1.65f,1.95f);
            }
            storefront(v,x+.15f,-31.90f,w-.3f,k%6);
            // Selective balconies and pilasters break the repeated window grid.
            if(k%3==1)for(float y=7.5f;y<height-2;y+=6.6f)for(int col=0;col<2;++col) {
                const float bx=x+1.f+col*5.5f;
                box(v,{bx-.2f,y-.28f,-32.15f},{bx+3.5f,y-.12f,-31.05f},{.55f,.53f,.48f},19);
                box(v,{bx-.15f,y+.76f,-31.12f},{bx+3.45f,y+.82f,-31.04f},{.18f,.22f,.23f},1);
                for(int rail=0;rail<9;++rail)box(v,{bx+rail*.4f,y-.1f,-31.1f},{bx+.035f+rail*.4f,y+.8f,-31.06f},{.18f,.22f,.23f},1);
            }
            if(k%3==2)for(float px:{x+.1f,x+w-.35f})box(v,{px,.1f,-32.03f},{px+.24f,height,-31.81f},{.72f,.69f,.60f},19);

        }
        // Towers beyond the street row form a layered skyline.
        for(int k=0;k<9;++k) {
            const float x=-93+k*23.f,h=25.f+(k%4)*7;
            box(v,{x,0,-80},{x+15,h,-58},{.51f+.025f*(k%3),.61f,.66f},13);
            for(float y=4;y<h;y+=3)box(v,{x-.03f,y,-58.06f},{x+15.03f,y+.14f,-57.94f},{.71f,.73f,.71f},0);
            for(int col=0;col<5;++col)box(v,{x+.7f+col*3,0,-58.08f},{x+.87f+col*3,h,-57.92f},{.68f,.72f,.71f},1);
        }
        // Building containing the exit: no solid volume crosses the door hole.
        const Vec3 entrance{.62f,.61f,.55f};
        box(v,{-12,0,-10.78f},{-1.035f,18,-10.70f},entrance,10);
        box(v,{1.035f,0,-10.78f},{12,18,-10.70f},entrance,10);
        box(v,{-1.035f,2.65f,-10.78f},{1.035f,18,-10.70f},entrance,10);
        for(float y=4.4f;y<16;y+=3.4f)for(int col=0;col<8;++col)window(v,-10.7f+col*2.9f,y,-10.83f,1.8f,2.f,false);
        for(int side:{-1,1})for(int k=0;k<5;++k) {
            const float x=side>0?12.2f+k*14.f:-26.1f-k*14.f,h=14.f+(k%3)*5.f;
            box(v,{x,0,-10.8f},{x+13.9f,h,3},facades[(k+side+6)%6],10);
            for(float y=4;y<h-1;y+=3.3f)for(int col=0;col<4;++col)window(v,x+.9f+col*3.2f,y,-10.86f,1.9f,2.f,false);
            storefront(v,x+.25f,-10.87f,13.4f,(k+side+7)%6,false);
        }
        // Trees, streetlamps, bollards, bins and benches, outside walking lanes.
        for(int k=0;k<8;++k) {
            const float x=-52.f+k*16.f;
            tree(v,x,-30.7f,1.0f+.06f*(k%3));
            if(std::abs(x)>8)tree(v,x+6,-12.1f,.95f);
            box(v,{x+3,-.01f,-27.5f},{x+3.12f,5.5f,-27.38f},{.23f,.27f,.28f},1);
            box(v,{x+2.4f,5.35f,-27.52f},{x+3.12f,5.47f,-27.35f},{.20f,.24f,.25f},1);
            box(v,{x+2.2f,5.29f,-27.68f},{x+2.9f,5.38f,-27.18f},{.76f,.78f,.72f},0);
            box(v,{x+5.5f,0,-31.2f},{x+5.92f,.8f,-30.75f},{.22f,.28f,.25f},1);
            for(float bx:{x+8,x+9.5f})box(v,{bx,0,-30.8f},{bx+.10f,.47f,-30.1f},{.19f,.22f,.21f},1);
            for(int slat=0;slat<4;++slat)box(v,{x+7.8f,.46f,-30.75f+slat*.17f},{x+9.8f,.53f,-30.62f+slat*.17f},{.39f,.26f,.15f},2);
        }
        for(float x:{13.6f,21.5f})for(float z:{-16.75f,-27.6f})box(v,{x-.09f,0,z-.09f},{x+.09f,.8f,z+.09f},{.20f,.23f,.23f},1);
        // A pair of crossing signals with a clear coloured lens.
        for(float z:{-16.8f,-27.45f}){
            box(v,{20.8f,0,z-.07f},{20.94f,3.7f,z+.07f},{.25f,.28f,.28f},1);
            box(v,{20.65f,2.7f,z-.18f},{21.1f,3.6f,z+.18f},{.06f,.075f,.075f},0);

        }
        // A few real kerb drains and tactile crossing pads establish street scale.
        for(float x:{-14.f,11.f,29.f})for(float z:{-17.06f,-26.96f}) {
            box(v,{x-.44f,-.145f,z-.22f},{x+.44f,-.136f,z+.22f},{.11f,.14f,.14f},1);
            for(int grate=0;grate<8;++grate)box(v,{x-.38f+grate*.105f,-.135f,z-.19f},{x-.34f+grate*.105f,-.130f,z+.19f},{.31f,.34f,.33f},1);
        }
        for(float z:{-16.8f,-27.6f})box(v,{15.4f,.003f,z},{19.7f,.01f,z+.55f},{.63f,.55f,.34f},21);
        upload(city_,v);v.clear();
        ellipsoid(v,{0,0,0},{.11f,.11f,.035f},{1,1,1},22,5,8);upload(signalLens_,v);v.clear();
        ellipsoid(v,{0,0,0},{180,180,180},{.54f,.72f,.87f},18,12,24);upload(sky_,v);v.clear();
        // Rounded, tapered body parts with distinct clothing and hair silhouettes.
        tapered(v,.84f,1.29f,{.17f,0,.105f},{.23f,0,.135f},{1,1,1},20);
        tapered(v,1.29f,1.40f,{.23f,0,.135f},{.14f,0,.10f},{1,1,1},20);
        box(v,{-.025f,.89f,-.113f},{.025f,1.35f,-.109f},{.68f,.70f,.69f},20);
        upload(torso_,v);v.clear();
        tapered(v,.73f,.90f,{.17f,0,.105f},{.18f,0,.115f},{1,1,1},20);upload(hips_,v);v.clear();
        ellipsoid(v,{0,0,0},{.123f,.165f,.126f},{1,1,1},15,8,12);
        for(float x:{-.128f,.128f})ellipsoid(v,{x,0,.005f},{.026f,.048f,.030f},{1,1,1},15,4,6);
        tapered(v,-.23f,-.10f,{.055f,0,.055f},{.058f,0,.058f},{1,1,1},15);upload(head_,v);v.clear();
        for(int style=0;style<3;++style) {
            ellipsoid(v,{0,.103f,.014f},{.132f,.087f,.134f},{1,1,1},2,6,10);
            if(style==1) {
                ellipsoid(v,{0,-.02f,.101f},{.136f,.19f,.075f},{1,1,1},2,6,8);
                ellipsoid(v,{.03f,-.12f,.17f},{.07f,.17f,.064f},{1,1,1},2,5,8);
            }
            if(style==2)for(float x:{-.07f,0.f,.07f})ellipsoid(v,{x,.15f,.02f},{.068f,.065f,.10f},{1,1,1},2,5,7);
            upload(hair_[style],v);v.clear();
        }
        for(float x:{-.044f,.044f})box(v,{x-.009f,.025f,-.127f},{x+.009f,.042f,-.122f},{.06f,.065f,.055f},0);
        ellipsoid(v,{0,.002f,-.131f},{.021f,.037f,.028f},{.75f,.52f,.39f},15,4,6);
        box(v,{-.029f,-.055f,-.12f},{.029f,-.047f,-.116f},{.38f,.24f,.20f},15);upload(face_,v);v.clear();
        tapered(v,-.30f,0,{.060f,0,.059f},{.079f,0,.075f},{1,1,1},20);upload(upperArm_,v);v.clear();
        tapered(v,-.27f,0,{.047f,0,.047f},{.061f,0,.057f},{1,1,1},20);upload(lowerArm_,v);v.clear();
        ellipsoid(v,{0,-.04f,-.012f},{.050f,.072f,.034f},{1,1,1},15,5,8);upload(hand_,v);v.clear();
        tapered(v,-.37f,0,{.066f,0,.066f},{.089f,0,.083f},{1,1,1},20);upload(thigh_,v);v.clear();
        tapered(v,-.37f,0,{.056f,0,.055f},{.069f,0,.067f},{1,1,1},20);upload(shin_,v);v.clear();
        ellipsoid(v,{0,-.008f,-.058f},{.080f,.048f,.152f},{.075f,.078f,.076f},0,4,8);
        box(v,{-.07f,-.05f,-.18f},{.07f,-.037f,.06f},{.25f,.26f,.24f},0);upload(shoe_,v);v.clear();
        // Coat hem, shoulder bag, backpack and cap use shared immutable meshes.
        tapered(v,.61f,.91f,{.215f,0,.14f},{.181f,0,.12f},{1,1,1},20);upload(jacket_,v);v.clear();
        ellipsoid(v,{0,1.12f,.22f},{.18f,.25f,.10f},{1,1,1},20,6,8);
        for(float x:{-.13f,.10f})box(v,{x,.94f,-.151f},{x+.03f,1.39f,-.14f},{.15f,.16f,.14f},20);
        box(v,{-.13f,.98f,.31f},{.13f,1.14f,.34f},{.23f,.25f,.22f},20);upload(backpack_,v);v.clear();
        box(v,{-.06f,-.26f,-.11f},{.08f,-.04f,.11f},{1,1,1},20);
        box(v,{-.05f,-.05f,-.12f},{.055f,.10f,-.10f},{.24f,.20f,.15f},2);
        box(v,{-.05f,-.05f,.10f},{.055f,.10f,.12f},{.24f,.20f,.15f},2);upload(bag_,v);v.clear();
        ellipsoid(v,{0,.11f,.007f},{.14f,.077f,.138f},{1,1,1},20,5,10);
        box(v,{-.13f,.104f,-.225f},{.13f,.121f,-.065f},{1,1,1},20);upload(cap_,v);v.clear();
        const Vec3 glass{.12f,.23f,.30f},chrome{.48f,.52f,.53f};
        for(int kind=0;kind<3;++kind) {
            const float length=kind==1?1.82f:kind==2?2.16f:2.12f;
            const float rear=kind==2?1.72f:kind==1?1.42f:1.16f;
            const float roofRear=kind==2?1.42f:kind==1?.90f:.78f;
            const float roof=kind==2?1.65f:kind==1?1.53f:1.48f;
            // Three bevelled body rings, with a real nose/shoulder contour.
            for(int ring=0;ring<2;++ring) {
                const float ya=ring==0?.36f:.60f,yb=ring==0?.60f:.87f;
                const float wa=ring==0?.79f:.89f,wb=ring==0?.89f:.84f;
                const float la=length-(ring==0?.12f:0),lb=length-(ring==0?0:.08f);
                const std::array<Vec3,8> lower={Vec3{-wa+.15f,ya,-la},Vec3{wa-.15f,ya,-la},Vec3{wa,ya,-la+.24f},Vec3{wa,ya,la-.24f},Vec3{wa-.15f,ya,la},Vec3{-wa+.15f,ya,la},Vec3{-wa,ya,la-.24f},Vec3{-wa,ya,-la+.24f}};
                const std::array<Vec3,8> upper={Vec3{-wb+.15f,yb,-lb},Vec3{wb-.15f,yb,-lb},Vec3{wb,yb,-lb+.24f},Vec3{wb,yb,lb-.24f},Vec3{wb-.15f,yb,lb},Vec3{-wb+.15f,yb,lb},Vec3{-wb,yb,lb-.24f},Vec3{-wb,yb,-lb+.24f}};
                for(int side=0;side<8;++side) {
                    const int next=(side+1)%8;
                    quad(v,lower[side],lower[next],upper[next],upper[side],normalize(cross(upper[side]-lower[side],lower[next]-lower[side])),{1,1,1},16);
                }
            }
            box(v,{-.82f,.84f,-length+.1f},{.82f,.89f,-.84f},{1,1,1},16);
            box(v,{-.82f,.84f,rear},{.82f,.91f,length-.1f},{1,1,1},16);
            box(v,{-.68f,roof-.05f,-.42f},{.68f,roof+.015f,roofRear},{1,1,1},16);
            for(float side:{-1.f,1.f}) {
                const float x=side*.79f;
                box(v,{x-.025f,.86f,-.75f},{x+.025f,.98f,rear},{1,1,1},16);
                // B pillar is solid paint, with separate side window openings.
                box(v,{x-.035f,.94f,.19f},{x+.035f,roof-.05f,.26f},{1,1,1},16);
                quad(v,{x,.92f,-.84f},{x+.055f*side,.92f,-.84f},{side*.70f,roof,-.42f},{side*.66f,roof,-.42f},{side,0,0},{1,1,1},16);
            }
            upload(carPaint_[kind],v);v.clear();
            quad(v,{-.77f,.91f,-.85f},{.77f,.91f,-.85f},{.65f,roof-.05f,-.40f},{-.65f,roof-.05f,-.40f},{0,.6f,-.8f},glass,13);
            quad(v,{.77f,.94f,rear},{-.77f,.94f,rear},{-.65f,roof-.05f,roofRear},{.65f,roof-.05f,roofRear},{0,.6f,.8f},glass,13);
            for(float side:{-1.f,1.f})for(int pane=0;pane<2;++pane) {
                const float a=pane==0?-.75f:.28f,b=pane==0?.18f:rear-.04f;
                const float upperA=pane==0?-.39f:.28f,upperB=pane==0?.18f:roofRear-.02f;
                quad(v,{side*.80f,.97f,a},{side*.80f,.97f,b},{side*.69f,roof-.07f,upperB},{side*.69f,roof-.07f,upperA},{side,.12f,0},glass,13);
            }
            for(float side:{-1.f,1.f}) {
                box(v,{side*.86f-.015f,.55f,-length+.30f},{side*.86f+.015f,.59f,length-.25f},{.12f,.15f,.17f},0);
                for(float door:{-.18f,.92f})box(v,{side*.845f-.022f,.80f,door},{side*.845f+.022f,.835f,door+.16f},chrome,1);
                box(v,{side*.935f-.07f,.99f,-.76f},{side*.935f+.07f,1.10f,-.56f},{.24f,.29f,.30f},1);
            }
            box(v,{-.70f,.39f,-length-.012f},{.70f,.48f,-length+.045f},{.15f,.18f,.19f},0);
            box(v,{-.70f,.39f,length-.045f},{.70f,.48f,length+.012f},chrome,1);
            box(v,{-.32f,.51f,-length-.014f},{.32f,.65f,-length+.018f},{.07f,.09f,.10f},0);
            box(v,{-.19f,.51f,length+.014f},{.19f,.63f,length+.030f},{.81f,.82f,.74f},0);
            for(float x:{-.64f,.36f}) {
                box(v,{x,.64f,-length+.01f},{x+.28f,.77f,-length+.025f},{.79f,.82f,.73f},13);
                box(v,{x,.64f,length+.01f},{x+.28f,.77f,length+.025f},{.29f,.035f,.018f},0);
            }
            upload(carDetails_[kind],v);v.clear();
        }
        for(float x:{-.64f,.36f})box(v,{x,.65f,0},{x+.28f,.76f,.01f},{1,1,1},22);
        upload(brakeLights_,v);v.clear();
        for(int i=0;i<16;++i){
            const float a=2*PI*i/16,b=2*PI*(i+1)/16;
            Vec3 aa{-.105f,.32f*std::cos(a),.32f*std::sin(a)},bb{.105f,.32f*std::cos(a),.32f*std::sin(a)},
                 cc{.105f,.32f*std::cos(b),.32f*std::sin(b)},dd{-.105f,.32f*std::cos(b),.32f*std::sin(b)};
            quad(v,aa,bb,cc,dd,{0,std::cos((a+b)*.5f),std::sin((a+b)*.5f)},{.045f,.05f,.052f},0);
            for(float side:{-.11f,.11f}) {
                const Vec3 p1{side,0,0},p2{side,.30f*std::cos(a),.30f*std::sin(a)},p3{side,.30f*std::cos(b),.30f*std::sin(b)};
                for(auto q:{p1,p2,p3})v.push_back({q.x,q.y,q.z,side<0?-1.f:1.f,0,0,0,0,.075f,.08f,.08f,0});
            }
        }
        for(float x:{-.115f,.115f})for(int spoke=0;spoke<5;++spoke){
            const float a=spoke*2*PI/5;
            quad(v,{x,-.025f,-.025f},{x,.025f,.025f},{x,.22f*std::cos(a)+.025f,.22f*std::sin(a)+.025f},{x,.22f*std::cos(a)-.025f,.22f*std::sin(a)-.025f},{x<0?-1.f:1.f,0,0},chrome,1);
        }
        upload(wheel_,v);
    }
    static void upload(Mesh& mesh,const Vertices& v) {
        mesh.vao.create();mesh.vbo.create();
        glBindVertexArray(mesh.vao);glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(v.size()*sizeof(V)),v.data(),GL_STATIC_DRAW);
        mesh.count=GLsizei(v.size());
        for(int i=0;i<5;++i)glEnableVertexAttribArray(i);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V),nullptr);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(3*sizeof(float)));
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(6*sizeof(float)));
        glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(8*sizeof(float)));
        glVertexAttribPointer(4,1,GL_FLOAT,GL_FALSE,sizeof(V),(void*)(11*sizeof(float)));
    }
    static const char* glyph(char c) {
        switch(c) {
        case 'C':return "01111100001000010000100001000001111";
        case 'K':return "10001100101010011000101001001010001";
        case 'L':return "10000100001000010000100001000011111";
        case 'B':return "11110100011000111110100011000111110";
        case 'P':return "11110100011000111110100001000010000";
        case 'A':return "01110100011000111111100011000110001";
        case 'E':return "11111100001000011110100001000011111";
        case 'F':return "11111100001000011110100001000010000";
        case 'G':return "01111100001000010111100011000101110";
        case 'H':return "10001100011000111111100011000110001";
        case 'I':return "11111001000010000100001000010011111";
        case 'M':return "10001110111010110101100011000110001";
        case 'N':return "10001110011010110011100011000110001";
        case 'O':return "01110100011000110001100011000101110";
        case 'R':return "11110100011000111110101001001010001";
        case 'S':return "01111100001000001110000010000111110";
        case 'T':return "11111001000010000100001000010000100";
        case 'U':return "10001100011000110001100011000101110";
        case 'X':return "10001100010101000100010101000110001";
        case 'W':return "10001100011000110101101011101110001";
        case 'Y':return "10001100010101000100001000010000100";
        default:return "00000000000000000000000000000000000";
        }
    }
    static void words(Vertices& v,const char* text,float x,float y,float z,float size,Vec3 color,float m=7) {
        for(const char* p=text;*p;++p,x+=size*6) {
            const char* bits=glyph(*p);
            for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(bits[row*5+col]=='1') {
                const float xx=x+col*size,yy=y+(6-row)*size;
                quad(v,{xx,yy,z},{xx+size*.86f,yy,z},{xx+size*.86f,yy+size*.86f,z},{xx,yy+size*.86f,z},{0,0,1},color,m);
            }
        }
    }
    void init() {
        const char* vertex=R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;layout(location=3) in vec3 aColor;layout(location=4) in float aMaterial;
uniform mat4 uMVP,uModel,uSunMatrix0,uSunMatrix1;out vec3 p,n,color;out vec2 uv;out vec4 sunPosition0,sunPosition1;flat out int material;
void main(){vec4 world=uModel*vec4(aPosition,1);p=world.xyz;n=transpose(inverse(mat3(uModel)))*aNormal;color=aColor;uv=aUV;material=int(aMaterial+.1);sunPosition0=uSunMatrix0*world;sunPosition1=uSunMatrix1*world;gl_Position=uMVP*world;}
)GLSL";
        const char* fragment=R"GLSL(#version 330 core
in vec3 p,n,color;in vec2 uv;in vec4 sunPosition0,sunPosition1;flat in int material;out vec4 frag;
uniform vec3 uEye,uTint,uSunDirection;uniform float uTime,uExposure,uCabin,uLobby;uniform int uSurface,uSunShadows;
uniform sampler2D uStaticSun0,uMovingSun0,uStaticSun1,uMovingSun1;
float hash(vec2 v){return fract(sin(dot(v,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 v){vec2 i=floor(v),f=fract(v);f=f*f*(3-2*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
vec3 point(vec3 pos,vec3 tint,float power,vec3 normal){vec3 d=pos-p;float r=dot(d,d);return tint*power*(.18+.82*max(0,dot(normal,normalize(d))))/(1+r*.42);}
float sunMap(sampler2D fixedMap,sampler2D movingMap,vec3 q,float bias){
 if(any(lessThan(q,vec3(.001)))||any(greaterThan(q,vec3(.999))))return 1;
 vec2 fixedTexel=1./vec2(textureSize(fixedMap,0)),movingTexel=1./vec2(textureSize(movingMap,0));float visible=0;
 for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x){vec2 offset=vec2(x,y);
  float fixedDepth=texture(fixedMap,q.xy+offset*fixedTexel).r;
  float movingDepth=texture(movingMap,q.xy+offset*movingTexel).r;
  visible+=step(q.z-bias,min(fixedDepth,movingDepth));}
 return visible/9.;
}
float sunlight(vec3 normal){
 if(uSunShadows==0)return 1;
 vec3 nearQ=sunPosition0.xyz/sunPosition0.w*.5+.5,farQ=sunPosition1.xyz/sunPosition1.w*.5+.5;
 float bias=.000025+.00007*(1-max(0,dot(normal,uSunDirection)));
 float edge=min(min(nearQ.x,1-nearQ.x),min(nearQ.y,1-nearQ.y));
 if(edge>=.06&&nearQ.z>0&&nearQ.z<1)return sunMap(uStaticSun0,uMovingSun0,nearQ,bias);
 float farLight=sunMap(uStaticSun1,uMovingSun1,farQ,bias*1.6);
 if(edge<=.01||nearQ.z<=0||nearQ.z>=1)return farLight;
 return mix(farLight,sunMap(uStaticSun0,uMovingSun0,nearQ,bias),smoothstep(.01,.06,edge));
}
void main(){
 if(material==18){vec3 ray=normalize(p-uEye);float high=clamp(ray.y,0,1);vec3 sky=mix(vec3(.77,.84,.88),vec3(.34,.59,.82),pow(high,.6));
 float cloud=noise(ray.xz/max(.12,ray.y)*1.8);sky=mix(sky,vec3(.91,.92,.90),smoothstep(.60,.80,cloud)*smoothstep(.04,.25,ray.y)*.58);
 float alignment=max(dot(ray,uSunDirection),0);
 sky+=vec3(.32,.23,.10)*pow(alignment,48)+vec3(.23,.17,.07)*pow(alignment,650);
 float disc=smoothstep(.99945,.99970,alignment);sky=mix(sky,vec3(1,.98,.82),disc);
 frag=vec4(sky,1);return;}
 vec3 normal=normalize(n),base=color*uTint;float grain=noise(uv*170);float rough=.75;
 if(material==1){base*=.82+.18*noise(vec2(uv.x*230,uv.y*4));rough=.3;}
 if(material==2){base*=.76+.24*noise(vec2(uv.x*90,uv.y*3));}
 if(material==3){vec2 tile=fract(uv*1.8);float edge=min(min(tile.x,1-tile.x),min(tile.y,1-tile.y));base*=mix(.62,1,smoothstep(.006,.023,edge));base*=.95+.05*noise(floor(uv*1.8));rough=.4;}
 if(material==4){float stripe=sin(uv.x*37)*sin(uv.x*37);float stain=noise(uv*2.1)*noise(uv*.58);base*=.64+.22*stain+.14*stripe;base*=.95+.05*grain;}
 if(material==5){base*=.68+.27*grain+.05*noise(uv*11);base*=.78+.22*noise(uv*.55);}
 if(material==6){vec2 tile=fract(uv*1.45);float edge=min(min(tile.x,1-tile.x),min(tile.y,1-tile.y));base*=mix(.36,1,smoothstep(.003,.018,edge));base*=.92+.08*grain;}
 if(material==9)base*=.72+.25*noise(uv*1.7)+.03*grain;
 if(material==10){vec2 brick=uv*vec2(2.6,4.4);brick.x+=mod(floor(brick.y),2)*.5;vec2 f=fract(brick);float joint=min(min(f.x,1-f.x),min(f.y,1-f.y));base*=mix(.78,1,smoothstep(.015,.055,joint));base*=.93+.07*noise(floor(brick));}
 if(material==11){vec2 tile=fract(uv*1.3);float edge=min(min(tile.x,1-tile.x),min(tile.y,1-tile.y));base*=mix(.7,1,smoothstep(.006,.024,edge));base*=.97+.03*grain;}
 if(material==12){base*=.82+.18*grain;float roadPatch=noise(p.xz*.27);base*=.91+.09*smoothstep(.3,.7,roadPatch);rough=.95;}
 if(material==13){float fresnel=pow(1-max(0,dot(normal,normalize(uEye-p))),4);vec3 reflected=reflect(normalize(p-uEye),normal);
  vec3 reflectedSky=mix(vec3(.29,.36,.38),vec3(.66,.78,.87),smoothstep(-.1,.55,reflected.y));
  base=mix(base,reflectedSky,.28+.48*fresnel);base*=.95+.05*noise(uv*1.2);rough=.12;}
 if(material==16){rough=.22;base*=.96+.04*noise(uv*60);}
 if(material==19){base*=.93+.045*noise(uv*3)+.025*grain;rough=.87;}
 if(material==20){base*=.96+.04*grain;rough=.96;}
 if(material==21){vec2 cell=fract(uv*12)-.5;float stud=1-smoothstep(.13,.23,length(cell));base*=.86+.18*stud;rough=.9;}
 if(material==14)base*=.82+.18*noise(p.xz*4+p.y);
 float ambient=p.z>-1.7?.16:p.z>-10.5?(uSurface==1?.32*uLobby:.045):.20;
 vec3 light=vec3(ambient);
 light+=point(vec3(0,2.72,0),vec3(.88,.93,1),1.15*uCabin,normal);
 if(p.z<-.9&&p.z>-11.){
  if(uSurface==1){light+=point(vec3(0,2.95,-4.2),vec3(.96,.98,1),2.1*uLobby,normal);light+=point(vec3(0,2.95,-8),vec3(1,.96,.87),2.3*uLobby,normal);}
  else {light+=point(vec3(-2,3.1,-6),vec3(.55,.67,.67),.45,normal);}
 }
 if(uSurface==1 && p.z< -10.68){float diffuse=max(0,dot(normal,uSunDirection));light=vec3(.30,.37,.46)+vec3(1.25,1.09,.83)*diffuse*sunlight(normal);}
 else if(uSurface==1 && p.z< -8)light+=vec3(.18,.19,.17)*smoothstep(8,10.6,-p.z);
 float contact=.70+.30*smoothstep(0,.38,p.y);if(normal.y>.5)contact=.95;
 vec3 rgb=base*light*contact;
 if(material==1||material==3||material==13||material==16){bool sunlit=uSurface==1&&p.z< -10.68;vec3 lightDirection=sunlit?uSunDirection:normalize(vec3(0,2.8,-4)-p);
  vec3 h=normalize(normalize(uEye-p)+lightDirection);float visibility=sunlit?sunlight(normal):1;
  rgb+=(sunlit?vec3(1,.89,.72):vec3(1))*pow(max(dot(normal,h),0),mix(24,72,1-rough))*(sunlit?.29:.11)*(1-rough)*visibility;}
 if(material==22)rgb=color*uTint;
 if(material==7)rgb=color;
 if(material==8)rgb=color*(.8+.2*noise(uv*80));
 float distance=length(uEye-p);bool outside=uSurface==1&&p.z< -10.68;
 rgb=mix(rgb,outside?vec3(.66,.76,.83):vec3(.025,.024,.014),1-exp(-max(distance-(outside?32:11),0)*(outside?.009:.032)));
 rgb=vec3(1)-exp(-rgb*uExposure*1.25);rgb=pow(max(rgb,vec3(0)),vec3(.82));
 rgb+=(hash(gl_FragCoord.xy+vec2(fract(uTime*.71)*100,fract(uTime*.43)*100))-.5)*.008;
 frag=vec4(rgb,1);}
)GLSL";
        shader_=Shader(vertex,fragment,true);
        sunDepth_=Shader(R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;uniform mat4 uMVP,uModel;
void main(){gl_Position=uMVP*uModel*vec4(aPosition,1);}
)GLSL",R"GLSL(#version 330 core
void main(){}
)GLSL",true);
        Vertices v;
        const Vec3 steel{.40f,.43f,.43f},dark{.085f,.095f,.092f},trim{.22f,.23f,.22f};
        // Cabin: real floor, inset panels, ceiling luminaire and side rails.
        box(v,{-1.46f,-.10f,-1.65f},{1.46f,0,1.6f},{.20f,.21f,.19f},3);
        box(v,{-1.5f,0,-1.65f},{-1.4f,2.9f,1.6f},steel,1);
        box(v,{1.4f,0,-1.65f},{1.5f,2.9f,1.6f},steel,1);
        box(v,{-1.5f,0,1.5f},{1.5f,2.9f,1.6f},steel,1);
        for(float x:{-1.39f,1.36f})for(int panel=0;panel<3;++panel)
            box(v,{x,.20f,-1.28f+panel*.9f},{x+.03f,2.42f,-.43f+panel*.9f},{.22f,.17f,.12f},2);
        for(float x:{-1.34f,1.29f})box(v,{x,.94f,-1.2f},{x+.05f,1.0f,1.2f},steel,1);
        box(v,{-1.5f,2.8f,-1.65f},{1.5f,2.95f,1.6f},dark,1);
        box(v,{-.65f,2.77f,-.55f},{.65f,2.8f,.8f},{.84f,.91f,.94f},7);
        box(v,{-1.4f,0,-1.63f},{-.82f,2.8f,-1.45f},steel,1);
        box(v,{.82f,0,-1.63f},{1.4f,2.8f,-1.45f},steel,1);
        box(v,{-.82f,2.35f,-1.63f},{.82f,2.8f,-1.45f},steel,1);
        box(v,{-.82f,.003f,-1.65f},{.82f,.025f,-1.4f},trim,1);
        for(float z:{-1.62f,-1.55f,-1.48f})box(v,{-.82f,.026f,z},{.82f,.029f,z+.012f},dark,0);
        box(v,{-.37f,2.43f,-1.43f},{.37f,2.67f,-1.40f},{.012f,.018f,.012f},0);
        box(v,{1.04f,.95f,-1.41f},{1.28f,1.62f,-1.37f},dark,1);
        for(int i=0;i<4;++i)box(v,{1.12f,1.05f+i*.125f,-1.355f},{1.19f,1.12f+i*.125f,-1.34f},i==3?Vec3{.50f,.72f,.41f}:steel,i==3?7:1);
        upload(cabin_,v);v.clear();
        // Local door coordinates make their openings explicit and independent.
        box(v,{-.815f,.035f,-1.56f},{-.005f,2.34f,-1.48f},steel,1);upload(leftDoor_,v);v.clear();
        box(v,{.005f,.035f,-1.56f},{.815f,2.34f,-1.48f},steel,1);upload(rightDoor_,v);v.clear();
        // Garage glimpsed only before the closed-door change of location.
        box(v,{-5,-.13f,-13},{5,-.01f,-1.65f},{.15f,.16f,.14f},9);
        box(v,{-5,0,-13},{5,3.6f,-12.7f},{.19f,.21f,.19f},9);
        box(v,{-5,3.4f,-13},{5,3.55f,-1.65f},{.12f,.13f,.12f},9);
        for(float x:{-3.f,3.f}) {
            box(v,{x-.3f,0,-7},{x+.3f,3.4f,-6.4f},{.29f,.30f,.25f},9);
            box(v,{x-.31f,.6f,-7.01f},{x+.31f,1.3f,-6.39f},{.51f,.42f,.10f},0);
            box(v,{x-1.05f,.32f,-11},{x+1.05f,1.02f,-8},{.06f,.075f,.08f},1);
            box(v,{x-.85f,1,-10.5f},{x+.85f,1.53f,-8.7f},{.035f,.044f,.048f},8);
        }
        upload(garage_,v);v.clear();
        // Bright tiled surface lobby, an ordinary place with no inhabitants.
        box(v,{-2.8f,-.13f,-10.5f},{2.8f,-.01f,-1.65f},{.67f,.68f,.64f},3);
        box(v,{-2.9f,0,-10.65f},{-2.7f,3.15f,-1.65f},{.70f,.72f,.68f},0);
        box(v,{2.7f,0,-10.65f},{2.9f,3.15f,-1.65f},{.70f,.72f,.68f},0);
        box(v,{-2.7f,3.1f,-10.65f},{2.7f,3.25f,-1.65f},{.78f,.78f,.73f},0);
        for(float x:{-2.69f,2.64f}) {
            box(v,{x,.12f,-10.4f},{x+.05f,1.12f,-1.7f},{.28f,.39f,.36f},0);
            box(v,{x,.08f,-10.4f},{x+.06f,.17f,-1.7f},{.12f,.16f,.14f},0);
            box(v,{x,1.13f,-10.4f},{x+.055f,1.17f,-1.7f},{.48f,.50f,.44f},1);
        }
        for(float z:{-4.2f,-8.f}){
            box(v,{-.85f,3.06f,z-.2f},{.85f,3.1f,z+.2f},{.19f,.21f,.21f},1);
            box(v,{-.76f,3.045f,z-.13f},{.76f,3.06f,z+.13f},{.88f,.91f,.85f},7);
        }
        // Mailboxes and a low bench add an everyday scale reference.
        for(int row=0;row<3;++row)for(int col=0;col<4;++col){
            const float z=-3.4f-col*.42f,y=.7f+row*.35f;
            box(v,{-2.69f,y,z-.35f},{-2.53f,y+.29f,z},{.37f,.40f,.38f},1);
            box(v,{-2.52f,y+.20f,z-.28f},{-2.51f,y+.22f,z-.07f},dark,0);
        }
        box(v,{1.9f,.42f,-6.7f},{2.6f,.52f,-4.6f},{.28f,.20f,.12f},2);
        for(float z:{-6.5f,-4.8f})box(v,{2.04f,0,z},{2.45f,.43f,z+.12f},trim,1);
        box(v,{-2.7f,0,-10.7f},{-.98f,3.1f,-10.45f},{.70f,.72f,.68f},0);
        box(v,{.98f,0,-10.7f},{2.7f,3.1f,-10.45f},{.70f,.72f,.68f},0);
        box(v,{-.98f,2.6f,-10.7f},{.98f,3.1f,-10.45f},{.70f,.72f,.68f},0);
        for(float x:{-1.02f,.96f})box(v,{x,0,-10.42f},{x+.06f,2.63f,-10.35f},trim,1);
        box(v,{-1.02f,2.58f,-10.42f},{1.02f,2.64f,-10.35f},trim,1);
        box(v,{-.48f,2.74f,-10.40f},{.48f,3.f,-10.36f},{.025f,.23f,.08f},7);
        words(v,"EXIT",-.32f,2.79f,-10.34f,.026f,{.80f,.94f,.80f});
        upload(lobby_,v);v.clear();
        // Exit leaf pivots at its left edge. Opaque frosted glazing conceals the
        // city street until the animated opening, rather than a screen cut.
        box(v,{0,0,-.04f},{1.9f,2.56f,.04f},{.15f,.24f,.24f},1);
        box(v,{.16f,.70f,.045f},{1.74f,2.34f,.06f},{.60f,.69f,.65f},8);
        box(v,{.16f,.70f,-.06f},{1.74f,2.34f,-.045f},{.60f,.69f,.65f},8);
        box(v,{1.62f,1.04f,.06f},{1.70f,1.30f,.13f},steel,1);
        box(v,{.16f,.59f,.05f},{1.75f,.65f,.10f},steel,1);
        upload(exitDoor_,v);v.clear();
        buildCity();
        // Four tiny physical LED displays are switched during the ascent.
        for(int stage=0;stage<4;++stage) {
            const char* label=stage==3?"G":"I";
            if(stage==3)words(v,label,-.055f,2.475f,-1.385f,.021f,{.65f,.90f,.43f});
            else {
                for(int bar=0;bar<3-stage;++bar)box(v,{-.18f+bar*.15f,2.48f,-1.39f},{-.09f+bar*.15f,2.62f,-1.38f},{.64f,.87f,.37f},7);
            }
            upload(indicators_[stage],v);v.clear();
        }
        ready_=true;
    }
    void mesh(const Mesh& m,const Mat4& model=Mat4::identity()) {
        (shadowPass_?sunDepth_:shader_).set("uModel",model);glBindVertexArray(m.vao);glDrawArrays(GL_TRIANGLES,0,m.count);
    }
    void tint(Vec3 color){if(!shadowPass_)shader_.set("uTint",color);}
    void drawCityActors(float time) {
        // Pure clock samples also control glances, braking and the crossing.
        // All variants reuse a bounded mesh set in colour and shadow passes.
        if(actorTime_!=time) {
            for(int i=0;i<EndingCity::pedestrianCount;++i)cityFeet_[i]=EndingCityFeet::sample(i,time);
            actorTime_=time;
        }
        const std::array<Vec3,6> shirts={Vec3{.19f,.32f,.43f},Vec3{.53f,.25f,.19f},Vec3{.65f,.60f,.47f},Vec3{.22f,.34f,.27f},Vec3{.34f,.26f,.40f},Vec3{.66f,.67f,.64f}};
        const std::array<Vec3,4> skins={Vec3{.76f,.55f,.40f},Vec3{.49f,.32f,.21f},Vec3{.88f,.69f,.53f},Vec3{.62f,.43f,.29f}};
        const std::array<Vec3,6> cars={Vec3{.55f,.13f,.085f},Vec3{.13f,.27f,.40f},Vec3{.70f,.70f,.65f},Vec3{.18f,.20f,.22f},Vec3{.53f,.38f,.12f},Vec3{.30f,.39f,.34f}};
        for(int i=0;i<EndingCity::pedestrianCount;++i) {
            const auto p=EndingCity::pedestrian(i,time);
            const float motion=std::clamp(p.speed/.8f,0.f,1.f),height=EndingCityFeet::height(i),width=EndingCityFeet::width(i);
            const auto& feet=cityFeet_[i];
            const float bounce=.009f*(1-std::cos(p.gait*2))*motion+(feet.pelvisHeight-p.position.y)/height;
            const Mat4 ground=translate(p.position.x,p.position.y,p.position.z)*rotateY(p.yaw)*scale(width,height,1);
            const Mat4 root=ground*translate(0,bounce,0)*rotateZ(.012f*std::sin(p.gait)*motion);
            const Vec3 shirt=shirts[i%shirts.size()],skin=skins[i%skins.size()],pants=i%3==0?Vec3{.29f,.26f,.21f}:Vec3{.12f,.16f,.20f};
            tint(shirt);mesh(torso_,root);
            if(i%4==2)mesh(jacket_,root);
            tint(pants);mesh(hips_,root);
            if(i%4==0){tint(Vec3{.25f,.29f,.25f});mesh(backpack_,root);}
            const Mat4 head=root*translate(0,1.56f,0)*rotateY(p.headYaw+.07f*std::sin(time*.6f+i));
            tint(skin);mesh(head_,head);tint(i%3?Vec3{.13f,.085f,.055f}:Vec3{.40f,.29f,.16f});mesh(hair_[i%3],head);
            tint(Vec3{1,1,1});mesh(face_,head);
            if(i%5==4){tint(Vec3{.28f,.34f,.37f});mesh(cap_,head);}
            for(int side:{-1,1}) {
                const float swing=std::sin(p.gait+(side>0?PI:0))*motion;
                const Mat4 arm=root*translate(side*.25f,1.33f,0)*rotateX(-swing*.34f)*rotateZ(side*.06f);
                tint(shirt);mesh(upperArm_,arm);
                const Mat4 elbow=arm*translate(0,-.30f,0)*rotateX(-.12f-.11f*std::max(0.f,-swing));
                mesh(lowerArm_,elbow);tint(skin);mesh(hand_,elbow*translate(0,-.29f,0));
                if(i%4==1&&side<0){tint(Vec3{.50f,.38f,.24f});mesh(bag_,elbow*translate(0,-.33f,0));}
                // World-planted feet hold through turns; the swing/kerb
                // trajectory is shared with CPU continuity/clearance tests.
                const auto& foot=feet.feet[side>0?1:0];
                const float dx=foot.position.x-p.position.x,dz=foot.position.z-p.position.z;
                const Vec3 target{(dx*std::cos(p.yaw)-dz*std::sin(p.yaw))/width,
                                  (foot.position.y-p.position.y)/height,dx*std::sin(p.yaw)+dz*std::cos(p.yaw)};
                const Vec3 hip{side*.105f,.81f+bounce,0},delta=target-hip;
                const float distance=std::sqrt(dot(delta,delta)),length=std::max(.40f,distance*.501f);
                const Vec3 direction=normalize(delta),forward{0,0,-1};
                const Vec3 bend=normalize(forward-direction*dot(forward,direction));
                const Vec3 knee=hip+delta*.5f+bend*std::sqrt(std::max(0.f,length*length-distance*distance*.25f));
                auto segment=[&](Vec3 a,Vec3 b) {
                    const Vec3 up=normalize(a-b),right=normalize(cross(up,Vec3{0,0,1})),front=cross(right,up);
                    Mat4 m=Mat4::identity();
                    m.m[0]=right.x;m.m[1]=right.y;m.m[2]=right.z;
                    m.m[4]=up.x;m.m[5]=up.y;m.m[6]=up.z;
                    m.m[8]=front.x;m.m[9]=front.y;m.m[10]=front.z;
                    m.m[12]=a.x;m.m[13]=a.y;m.m[14]=a.z;
                    return ground*m*scale(1,length/.37f,1);
                };
                tint(pants);mesh(thigh_,segment(hip,knee));mesh(shin_,segment(knee,target));
                tint(Vec3{1,1,1});mesh(shoe_,translate(foot.position.x,foot.position.y,foot.position.z)*rotateY(foot.yaw)*scale(width,height,1));
            }
        }
        for(int i=0;i<EndingCity::carCount;++i) {
            const auto p=EndingCity::car(i,time);const int kind=i%3;
            const Mat4 root=translate(p.position.x,p.position.y,p.position.z)*rotateY(p.yaw);
            tint(cars[i%cars.size()]);mesh(carPaint_[kind],root);
            tint(Vec3{1,1,1});mesh(carDetails_[kind],root);
            const float length=kind==1?1.82f:kind==2?2.16f:2.12f,wheelbase=kind==1?1.12f:1.35f;
            tint(Vec3{.29f+.61f*p.brake,.025f+.045f*p.brake,.012f});mesh(brakeLights_,root*translate(0,0,length+.028f));
            tint(Vec3{1,1,1});
            for(float x:{-.87f,.87f})for(float z:{-wheelbase,wheelbase})mesh(wheel_,root*translate(x,.32f,z)*rotateX(-p.gait));
        }
        for(float z:{-16.8f,-27.45f}) {
            const bool red=EndingCity::crossingActive(time),amber=EndingCity::trafficAmber(time);
            tint(red?Vec3{.88f,.055f,.018f}:Vec3{.13f,.022f,.012f});mesh(signalLens_,translate(20.875f,3.37f,z+.19f));
            tint(amber?Vec3{.95f,.53f,.045f}:Vec3{.12f,.07f,.015f});mesh(signalLens_,translate(20.875f,3.16f,z+.19f));
            tint(!red&&!amber?Vec3{.065f,.65f,.19f}:Vec3{.015f,.09f,.032f});mesh(signalLens_,translate(20.875f,2.95f,z+.19f));
        }
        tint(Vec3{1,1,1});
    }
    static void release(Mesh& m){m={};}
    void doors(const EndingPose& p) {
        mesh(leftDoor_,translate(-p.elevatorOpen*.84f,0,0));mesh(rightDoor_,translate(p.elevatorOpen*.84f,0,0));
        if(p.surface)mesh(exitDoor_,translate(-.95f,0,-10.46f)*rotateY(p.exitOpen*PI*.52f));
    }
    void initSun() {
        if(sunFbo_)return;
        sunFbo_.create();
        for(auto& texture:staticSun_)texture.create();
        for(auto& texture:movingSun_)texture.create();
        glActiveTexture(GL_TEXTURE0);
        for(int cascade=0;cascade<2;++cascade)for(int moving=0;moving<2;++moving) {
            const int size=moving?EndingSun::dynamicResolution(cascade):EndingSun::staticResolution(cascade);
            glBindTexture(GL_TEXTURE_2D,moving?movingSun_[cascade]:staticSun_[cascade]);
            glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,size,size,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
            const GLfloat border[]={1,1,1,1};glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
        }
    }
    void sunMap(int cascade,bool moving,float time,const EndingPose& p) {
        const int size=moving?EndingSun::dynamicResolution(cascade):EndingSun::staticResolution(cascade);
        glBindFramebuffer(GL_FRAMEBUFFER,sunFbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,moving?movingSun_[cascade]:staticSun_[cascade],0);
        glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("City sun framebuffer incomplete");
        glViewport(0,0,size,size);glClear(GL_DEPTH_BUFFER_BIT);
        sunDepth_.use();sunDepth_.set("uMVP",EndingSun::matrix(cascade));
        if(moving){drawCityActors(time);doors(p);}
        else {mesh(city_);mesh(lobby_);mesh(cabin_);}
    }
    void updateSun(float time,const EndingPose& p) {
        initSun();shadowPass_=true;
        glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.3f,2.f);
        if(!staticSunReady_) {
            for(int c=0;c<2;++c)sunMap(c,false,time,p);
            staticSunReady_=true;++staticSunUpdates_;
        }
        if(shadowTime_!=time) {
            for(int c=0;c<2;++c)sunMap(c,true,time,p);
            shadowTime_=time;++movingSunUpdates_;
        }
        glDisable(GL_POLYGON_OFFSET_FILL);shadowPass_=false;
    }
public:
    EndingRenderer()=default;
    EndingRenderer(const EndingRenderer&)=delete;
    EndingRenderer& operator=(const EndingRenderer&)=delete;
    unsigned staticShadowUpdates()const{return staticSunUpdates_;}
    unsigned dynamicShadowUpdates()const{return movingSunUpdates_;}
    void draw(const EndingSequence& sequence,int width,int height,float exposure=1,bool sunlightShadows=true) {
        if(width<=0||height<=0)return;
        GLint program=0,vao=0,polygon[2]={GL_FILL,GL_FILL},drawFbo=0,readFbo=0,activeTexture=0,depthFunc=0;
        GLint textures[4]{};GLboolean depthWrite=GL_TRUE;
        GLfloat offsetFactor=0,offsetUnits=0;
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);glGetIntegerv(GL_POLYGON_MODE,polygon);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&drawFbo);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&readFbo);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&activeTexture);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
        glGetFloatv(GL_POLYGON_OFFSET_FACTOR,&offsetFactor);glGetFloatv(GL_POLYGON_OFFSET_UNITS,&offsetUnits);
        for(int unit=0;unit<4;++unit){glActiveTexture(GL_TEXTURE0+unit);glGetIntegerv(GL_TEXTURE_BINDING_2D,&textures[unit]);}
        const auto depth=glIsEnabled(GL_DEPTH_TEST),cull=glIsEnabled(GL_CULL_FACE),blend=glIsEnabled(GL_BLEND);
        const auto offset=glIsEnabled(GL_POLYGON_OFFSET_FILL);
        if(!ready_)init();
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDisable(GL_CULL_FACE);glDisable(GL_BLEND);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        const auto p=sampleEndingPose(sequence.time());
        if(p.surface&&sunlightShadows)updateSun(sequence.time(),p);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER,drawFbo);glViewport(0,0,width,height);
        glClearColor(.012f,.014f,.012f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        const Mat4 vp=perspective(p.fov*PI/180,float(width)/height,.045f,260)*lookAt(p.eye,p.forward);
        shader_.use();shader_.set("uTint",Vec3{1,1,1});shader_.set("uMVP",vp);shader_.set("uEye",p.eye);shader_.set("uTime",sequence.time());
        shader_.set("uSunDirection",EndingSun::direction());shader_.set("uSunShadows",int(p.surface&&sunlightShadows));
        shader_.set("uSunMatrix0",EndingSun::matrix(0));shader_.set("uSunMatrix1",EndingSun::matrix(1));
        const char* names[]={"uStaticSun0","uMovingSun0","uStaticSun1","uMovingSun1"};
        for(int c=0;c<2;++c)for(int moving=0;moving<2;++moving) {
            const int unit=c*2+moving;glActiveTexture(GL_TEXTURE0+unit);
            glBindTexture(GL_TEXTURE_2D,moving?movingSun_[c]:staticSun_[c]);shader_.set(names[unit],unit);
        }
        shader_.set("uExposure",std::clamp(exposure,.6f,1.5f));shader_.set("uCabin",p.cabinLight);shader_.set("uLobby",p.lobbyLight);shader_.set("uSurface",int(p.surface));
        if(p.surface){glDepthMask(GL_FALSE);mesh(sky_);glDepthMask(GL_TRUE);}
        mesh(cabin_);mesh(indicators_[sequence.time()<5?0:sequence.time()<7?1:sequence.time()<9.6f?2:3]);
        if(p.surface){mesh(lobby_);mesh(city_);drawCityActors(sequence.time());}else mesh(garage_);
        doors(p);
        if(!depth)glDisable(GL_DEPTH_TEST);
        if(cull)glEnable(GL_CULL_FACE);
        if(blend)glEnable(GL_BLEND);
        glDepthMask(depthWrite);glDepthFunc(depthFunc);glPolygonOffset(offsetFactor,offsetUnits);if(offset)glEnable(GL_POLYGON_OFFSET_FILL);
        for(int unit=0;unit<4;++unit){glActiveTexture(GL_TEXTURE0+unit);glBindTexture(GL_TEXTURE_2D,textures[unit]);}
        glActiveTexture(activeTexture);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,drawFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,readFbo);
        glPolygonMode(GL_FRONT_AND_BACK,polygon[0]);glBindVertexArray(GLuint(vao));glUseProgram(GLuint(program));
    }
    void destroy() {
        release(cabin_);release(garage_);release(lobby_);release(city_);release(sky_);release(leftDoor_);release(rightDoor_);release(exitDoor_);
        for(auto* mesh:{&torso_,&hips_,&head_,&face_,&upperArm_,&lowerArm_,&hand_,&thigh_,&shin_,&shoe_,&jacket_,&backpack_,&bag_,&cap_,&wheel_,&brakeLights_,&signalLens_})release(*mesh);
        for(auto& m:hair_)release(m);
        for(auto& m:carPaint_)release(m);
        for(auto& m:carDetails_)release(m);
        for(auto& m:indicators_)release(m);
        staticSun_={};movingSun_={};
        sunFbo_.reset();
        staticSunReady_=false;shadowTime_=-1;shadowPass_=false;staticSunUpdates_=movingSunUpdates_=0;
        actorTime_=-1;cityFeet_={};
        sunDepth_.destroy();shader_.destroy();ready_=false;
    }
};
}
