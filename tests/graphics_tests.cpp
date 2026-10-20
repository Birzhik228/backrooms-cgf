#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "common/graphics.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace br;
static void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
    require(glfwInit()!=0,"GLFW init");glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    auto* window=glfwCreateWindow(32,32,"Graphics probes",nullptr,nullptr);require(window!=nullptr,"GL context");
    glfwMakeContextCurrent(window);require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)!=0,"GLAD");
    struct TestLight { Vec3 position; int slot=-1; };
    ShadowMaps slots;
    std::vector<TestLight> lamps;
    for(int i=0;i<24;++i) lamps.push_back({{float(i),3,0},-1});
    slots.assignSlots(lamps,0,0);
    for(int i=0;i<24;++i) {
        require(lamps[i].slot==i,"Initial slots unique");
        slots.cache[i]={lamps[i].position,7,true,double(i),0};
    }
    // A new lamp sorts first, but must not evict any of the 23 retained lamps.
    lamps.erase(lamps.begin()+5);lamps.insert(lamps.begin(),{{-4,3,0},-1});
    for(auto& lamp:lamps)lamp.slot=-1;
    slots.assignSlots(lamps,0,0);
    require(lamps[0].slot==5 && !slots.cache[5].valid,"Only departed lamp slot is evicted");
    for(size_t i=1;i<lamps.size();++i)
        require(lamps[i].slot==int(lamps[i].position.x) && slots.cache[lamps[i].slot].valid,"Retained maps survive selection changes");
    lamps.erase(lamps.begin());
    for(auto& lamp:lamps){lamp.position.x-=100;lamp.slot=-1;}
    slots.assignSlots(lamps,100,0);
    for(auto& lamp:lamps)require(lamp.slot==int(lamp.position.x+100),"Origin rebasing retains global lamp identity");
    std::cout<<"Shadow slot retention, eviction and rebasing passed.\n";
    ShadowMaps maps;maps.init();Shader shadow("vshader_shadow.glsl","fshader_shadow.glsl");
    const std::string vertex=R"(#version 330 core
uniform vec3 uReceiver,uNormal;
out vec3 vPosition,vNormal;out vec2 vUV;flat out float vMaterial;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2-1,0,1);
vPosition=uReceiver;vNormal=uNormal;vUV=vec2(.5);vMaterial=25;})";
    std::ifstream file("fshader_backrooms.glsl");std::ostringstream contents;contents<<file.rdbuf();
    Shader probe(vertex,contents.str(),true);
    GLuint vao=0,vbo=0;glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,4*sizeof(float),nullptr);
    glEnableVertexAttribArray(3);glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(3*sizeof(float)));
    auto blocker=[&](Vec3 dir,int material){
        Vec3 up=std::abs(dir.y)>.9?Vec3{0,0,1}:Vec3{0,1,0};
        Vec3 s=normalize(cross(dir,up)),u=cross(s,dir),c=dir*.5f;
        std::vector<float> vertices;
        for(auto xy:std::array<std::array<float,2>,6>{{{{-1,-1}},{{1,-1}},{{1,1}},{{-1,-1}},{{1,1}},{{-1,1}}}}){
            Vec3 p=c+s*(xy[0]*.25f)+u*(xy[1]*.25f);vertices.insert(vertices.end(),{p.x,p.y,p.z,float(material)});
        }
        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_DYNAMIC_DRAW);
    };
    auto sample=[&](Vec3 dir,bool flash){
        glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,32,32);glDisable(GL_DEPTH_TEST);
        probe.use();probe.set("uReceiver",dir);probe.set("uNormal",dir*-1.f);probe.set("uCamera",Vec3{});
        probe.set("uForward",dir);probe.set("uFlashPosition",Vec3{});probe.set("uWorldPhase",Vec3{});
        probe.set("uExposure",1.f);probe.set("uFlashlight",int(flash));probe.set("uFlashShadows",int(flash));
        probe.set("uFlashVP",perspective(70*PI/180,1,.05f,35.f)*lightView({},dir,std::abs(dir.y)>.9?Vec3{0,0,1}:Vec3{0,1,0}));
        probe.set("uFlashShadow",8);probe.set("uLampShadows",9);
        probe.set("uLightPower[0]",flash?0.f:1.f);probe.set("uLightColor[0]",Vec3{1,1,1});
        glActiveTexture(GL_TEXTURE8);glBindTexture(GL_TEXTURE_2D,maps.flash);
        glActiveTexture(GL_TEXTURE9);glBindTexture(GL_TEXTURE_2D_ARRAY,maps.lamps);
        glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);
        unsigned char pixel[3]{};glReadPixels(16,16,1,1,GL_RGB,GL_UNSIGNED_BYTE,pixel);
        return int(pixel[0])+pixel[1]+pixel[2];
    };
    const std::array<Vec3,6> dirs={Vec3{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    int cases=0;
    for(int face=0;face<6;face++)for(int material:{17,7,20,5}){
        shadow.use();glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);maps.begin(face);
        const int open=sample(dirs[face],false);
        shadow.use();glEnable(GL_DEPTH_TEST);maps.begin(face);shadow.set("uLightVP",lampFace({},face));shadow.set("uModel",Mat4::identity());
        blocker(dirs[face],material);glDrawArrays(GL_TRIANGLES,0,6);
        const int blocked=sample(dirs[face],false);
        require(material==5?std::abs(open-blocked)<4:open>blocked+30,"Lamp shadow: solid occlusion / decal transparency");++cases;
    }
    for(int material:{17,7,20,5}){
        Vec3 dir{0,0,-1};shadow.use();glEnable(GL_DEPTH_TEST);maps.begin();const int open=sample(dir,true);
        shadow.use();glEnable(GL_DEPTH_TEST);maps.begin();shadow.set("uLightVP",perspective(70*PI/180,1,.05f,35.f)*lookAt({},dir));shadow.set("uModel",Mat4::identity());
        blocker(dir,material);glDrawArrays(GL_TRIANGLES,0,6);const int blocked=sample(dir,true);
        require(material==5?std::abs(open-blocked)<4:open>blocked+30,"Flashlight shadow: door / grille / creature / clue");++cases;
    }
    Shader ao("vshader_lens.glsl",argc>1?argv[1]:"fshader_ao.glsl");AmbientTarget target;
    int width=64,height=64;
    GLuint depth=0;glGenTextures(1,&depth);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,depth);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    Mat4 projection=perspective(70*PI/180,1,.06f,110.f);
    auto depthValue=[](float distance){return 110.f/109.94f-6.6f/(109.94f*distance);};
    std::vector<float> depths(64*64,depthValue(2));
    auto renderAO=[&](){
        target.resize(width,height);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,depth);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,width,height,0,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());
        glBindFramebuffer(GL_FRAMEBUFFER,target.fbo);glViewport(0,0,target.width,target.height);
        ao.use();ao.set("uDepth",0);ao.set("uProjection",projection);ao.set("uSize",Vec3{float(width),float(height),0});
        glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);
        std::vector<unsigned char> pixels(target.width*target.height);
        glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,target.width,target.height,GL_RED,GL_UNSIGNED_BYTE,pixels.data());
        glPixelStorei(GL_PACK_ALIGNMENT,4);return pixels;
    };
    auto flat=renderAO();require(flat[16*32+16]>=253,"AO must not darken an isolated flat wall");
    for(int y=0;y<64;y++)for(int x=0;x<64;x++){
        const float rayY=((y+.5f)/64*2-1)/projection.m[5];
        const float distance=rayY<0?std::min(2.f,-.5f/rayY):2.f;
        depths[y*64+x]=depthValue(distance);
    }
    auto corner=renderAO();int darkest=255;
    for(int y=8;y<13;y++)darkest=std::min(darkest,int(corner[y*32+16]));
    require(darkest<245,"AO must darken a real floor/wall contact");

    // Analytic infinite planes have no surrounding geometry to occlude them.
    // Test all usable pixels, including steep horizontal and two-axis tilts:
    // a single front-facing center sample missed the wall-stripe regression.
    const std::array<Vec3,6> normals={Vec3{.95f,0,.31f},{-.95f,.08f,.30f},
        {.30f,.92f,.25f},{.72f,.64f,.27f},{1,0,0},{0,1,0}};
    int planeCases=0;
    for(auto dimensions:std::array<std::array<int,2>,4>{{{1920,1000},{1920,1027},{1919,1027},{127,95}}}) {
        width=dimensions[0];height=dimensions[1];
        projection=perspective(70*PI/180,float(width)/height,.06f,110.f);
        depths.resize(size_t(width)*height);
        int worst=255;size_t checked=0;
        for(Vec3 normal:normals) {
            normal=normalize(normal);
            std::vector<float> distances(depths.size());
            for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
                Vec3 ray{((x+.5f)/width*2-1)/projection.m[0],
                         ((y+.5f)/height*2-1)/projection.m[5],-1};
                float denominator=dot(normal,ray);
                float distance=denominator<0?-.8f/denominator:110.f;
                distances[size_t(y)*width+x]=distance;
                depths[size_t(y)*width+x]=distance>.06f&&distance<110.f?depthValue(distance):1.f;
            }
            auto pixels=renderAO();int minimum=255;size_t samples=0;
            for(int y=2;y<target.height-2;++y)for(int x=2;x<target.width-2;++x) {
                const float distance=distances[size_t(std::min(y*2+1,height-1))*width+std::min(x*2+1,width-1)];
                if(distance<.4f||distance>12.f)continue;
                minimum=std::min(minimum,int(pixels[size_t(y)*target.width+x]));++samples;
            }
            require(samples>100,"Slanted-plane fixture has too few visible samples");
            if(minimum<253)std::cerr<<"Plane AO minimum="<<minimum<<" at "<<width<<'x'<<height
                <<" normal="<<normal.x<<','<<normal.y<<','<<normal.z<<'\n';
            require(minimum>=253,"AO falsely darkens an isolated slanted plane");
            worst=std::min(worst,minimum);checked+=samples;++planeCases;
        }
        std::cout<<"Slanted-plane AO "<<width<<'x'<<height<<": minimum="<<worst
                 <<", unoccluded samples="<<checked<<'\n';
    }

    // Exercise the production upsampler directly to isolate AO reconstruction
    // from film grain and grading. A smooth input must stay smooth between the
    // representative full-resolution pixel centers, also at odd dimensions.
    std::ifstream lensFile(argc>2?argv[2]:"fshader_lens.glsl");std::ostringstream lensContents;lensContents<<lensFile.rdbuf();
    const auto lensText=lensContents.str();const auto lensBody=lensText.find("uniform float uTime;");
    require(lensBody!=std::string::npos,"Cannot find lens AO helper");
    std::ifstream vertexFile("vshader_lens.glsl");std::ostringstream vertexContents;vertexContents<<vertexFile.rdbuf();
    Shader upsample(vertexContents.str(),lensText.substr(0,lensBody)+"\nvoid main(){frag=vec4(vec3(contactAO()),1);}",true);
    GLuint upsampleFbo=0,upsampleColor=0;glGenFramebuffers(1,&upsampleFbo);glGenTextures(1,&upsampleColor);
    glBindTexture(GL_TEXTURE_2D,upsampleColor);
    glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,width,height,0,GL_RED,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER,upsampleFbo);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,upsampleColor,0);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"AO upsampling test framebuffer");
    std::vector<float> aoValues(size_t(target.width)*target.height);
    auto renderUpsample=[&](){
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,target.texture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,target.width,target.height,0,GL_RED,GL_FLOAT,aoValues.data());
        glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,depth);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,width,height,0,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());
        glBindFramebuffer(GL_FRAMEBUFFER,upsampleFbo);glViewport(0,0,width,height);
        upsample.use();upsample.set("uAO",0);upsample.set("uDepth",1);upsample.set("uProjection",projection);
        glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,3);
        std::vector<float> pixels(size_t(width)*height);glReadPixels(0,0,width,height,GL_RED,GL_FLOAT,pixels.data());return pixels;
    };
    std::fill(depths.begin(),depths.end(),depthValue(2));
    for(int y=0;y<target.height;++y)for(int x=0;x<target.width;++x)
        aoValues[size_t(y)*target.width+x]=.55f+.25f*std::min(2*x+1.5f,width-.5f)/width
            +.1f*std::min(2*y+1.5f,height-.5f)/height;
    auto smoothAO=renderUpsample();float largestError=0;
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
        float expected=.55f+.25f*std::max(x+.5f,1.5f)/width+.1f*std::max(y+.5f,1.5f)/height;
        largestError=std::max(largestError,std::abs(smoothAO[size_t(y)*width+x]-expected));
    }
    require(largestError<.000002f,"AO upsampling introduces coarse texel steps into a smooth gradient");
    for(int y=0;y<height;++y)for(int x=0;x<width;++x)depths[size_t(y)*width+x]=depthValue(x<64?2.f:4.f);
    for(int y=0;y<target.height;++y)for(int x=0;x<target.width;++x)aoValues[size_t(y)*target.width+x]=x<32?.55f:1.f;
    auto edgeAO=renderUpsample();
    for(int y=2;y<height-2;++y)for(int x=62;x<=65;++x)
        require(std::abs(edgeAO[size_t(y)*width+x]-(x<64?.55f:1.f))<.000002f,"AO upsampling bleeds across a depth silhouette");
    upsample.destroy();glDeleteFramebuffers(1,&upsampleFbo);glDeleteTextures(1,&upsampleColor);
    ao.destroy();target.destroy();glDeleteTextures(1,&depth);
    require(glGetError()==GL_NO_ERROR,"OpenGL errors in probes");
    std::cout<<"Graphics tests passed: "<<cases<<" rendered occlusion probes, "<<planeCases
             <<" slanted-plane AO fixtures, flat-wall/contact checks, smooth/depth-edge AO reconstruction, six lamp directions, flashlight, doors, metal, creature and transparent clues.\n";
    probe.destroy();shadow.destroy();maps.destroy();glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glfwDestroyWindow(window);glfwTerminate();
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
