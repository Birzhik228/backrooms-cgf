#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "common/ending_renderer.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace br;
namespace fs=std::filesystem;
static constexpr int width=1280,height=800;
using Pixels=std::vector<unsigned char>;
static void require(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
static void checkGL(const std::string& label){
    const GLenum error=glGetError();
    require(error==GL_NO_ERROR,label+": OpenGL error "+std::to_string(error));
}
static void saveBMP(const fs::path& path,const Pixels& rgb){
    std::ofstream out(path,std::ios::binary);
    require(bool(out),"Cannot create "+path.string());
    auto u16=[&](std::uint16_t v){for(int n=0;n<2;++n)out.put(char((v>>(n*8))&255));};
    auto u32=[&](std::uint32_t v){for(int n=0;n<4;++n)out.put(char((v>>(n*8))&255));};
    const int stride=(width*3+3)&~3;
    out.put('B');out.put('M');u32(54+stride*height);u32(0);u32(54);
    u32(40);u32(width);u32(height);u16(1);u16(24);u32(0);u32(stride*height);
    u32(2835);u32(2835);u32(0);u32(0);
    // OpenGL and positive-height BMP both store the bottom scanline first.
    for(int y=0;y<height;++y){
        for(int x=0;x<width;++x){const auto at=(size_t(y)*width+x)*3;
            out.put(char(rgb[at+2]));out.put(char(rgb[at+1]));out.put(char(rgb[at]));}
        for(int n=width*3;n<stride;++n)out.put(0);
    }
    require(bool(out),"Cannot finish "+path.string());
}
static Pixels render(EndingRenderer& renderer,float time,bool shadows=true){
    EndingSequence sequence;sequence.start(time);
    renderer.draw(sequence,width,height,1.f,shadows);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Drawing target incomplete");
    GLint framebuffer=-1;glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&framebuffer);
    require(framebuffer==0,"Shadow depth framebuffer leaked into the cinematic draw");
    GLint viewport[4]{};glGetIntegerv(GL_VIEWPORT,viewport);
    require(viewport[0]==0&&viewport[1]==0&&viewport[2]==width&&viewport[3]==height,
            "Shadow texture viewport leaked into the cinematic draw");
    require(!glIsEnabled(GL_POLYGON_OFFSET_FILL),"Shadow polygon offset leaked into the cinematic draw");
    checkGL("Draw at "+std::to_string(time));
    Pixels pixels(size_t(width)*height*3);
    glReadBuffer(GL_BACK);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT,4);checkGL("Read pixels");
    return pixels;
}
struct Difference {size_t darkened=0,brightened=0;double meanDarkening=0;Pixels mask;};
static Difference compare(const Pixels& plain,const Pixels& shaded){
    Difference result;result.mask.resize(plain.size());
    for(size_t i=0;i<plain.size();i+=3){
        int before=int(plain[i])+plain[i+1]+plain[i+2];
        int after=int(shaded[i])+shaded[i+1]+shaded[i+2];
        const int delta=before-after;
        if(delta>12){++result.darkened;result.meanDarkening+=delta/3.;}
        if(delta< -6)++result.brightened;
        const auto value=static_cast<unsigned char>(std::clamp(delta*2,0,255));
        result.mask[i]=result.mask[i+1]=result.mask[i+2]=value;
    }
    if(result.darkened)result.meanDarkening/=result.darkened;
    return result;
}
int main(int argc,char** argv){
    GLFWwindow* window=nullptr;
    try{
        const fs::path evidence=argc>1?argv[1]:"tests/artifacts/ending-sun";
        fs::create_directories(evidence);
        require(glfwInit()!=0,"GLFW initialization failed");
        glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES,0);
        window=glfwCreateWindow(width,height,"Ending sunlight verification",nullptr,nullptr);
        require(window!=nullptr,"OpenGL 3.3 window failed");glfwMakeContextCurrent(window);
        require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)!=0,"GLAD initialization failed");
        glfwSwapInterval(0);glDisable(GL_DITHER);
        std::cout<<"Sunlight renderer: "<<glGetString(GL_RENDERER)<<'\n';
        EndingRenderer renderer;
        auto cabin=render(renderer,1.5f);
        require(renderer.staticShadowUpdates()==0&&renderer.dynamicShadowUpdates()==0,
                "Garage-only frame should not allocate/render city shadows");
        saveBMP(evidence/"cabin.bmp",cabin);
        auto door=render(renderer,22.f);saveBMP(evidence/"door-22.bmp",door);
        const auto staticBuilt=renderer.staticShadowUpdates();
        require(staticBuilt>0,"Static sunlight map was not built");
        require(renderer.dynamicShadowUpdates()>0,"Dynamic sunlight map was not built");
        for(float time:{26.f,32.f,37.f}){
            const std::string label="city-"+std::to_string(int(time));
            const auto plain=render(renderer,time,false);
            const auto shaded=render(renderer,time,true);
            const auto difference=compare(plain,shaded);
            saveBMP(evidence/(label+"-unshadowed.bmp"),plain);
            saveBMP(evidence/(label+"-shadows.bmp"),shaded);
            saveBMP(evidence/(label+"-shadow-mask.bmp"),difference.mask);
            // Same geometry, clock, exposure and sun in both images. A broad
            // changed region proves actual shadow sampling reached the image.
            require(difference.darkened>500,label+": no visible sunlight shadows");
            require(difference.brightened<difference.darkened/20+16,
                    label+": enabling shadows unexpectedly brightens the scene");
            std::cout<<label<<": shadow pixels="<<difference.darkened
                     <<", mean shadow darkening="<<difference.meanDarkening<<'\n';
            const auto dynamicBuilt=renderer.dynamicShadowUpdates();
            require(render(renderer,time)==shaded,label+": paused scene changed pixels");
            require(renderer.staticShadowUpdates()==staticBuilt,"Static city map rebuilt during the film");
            require(renderer.dynamicShadowUpdates()==dynamicBuilt,"Paused actors regenerated their shadow map");
            render(renderer,time+.25f);
            require(renderer.dynamicShadowUpdates()>dynamicBuilt,"Moving actors did not refresh their shadow map");
            require(renderer.staticShadowUpdates()==staticBuilt,"Moving actors rebuilt the static city map");
        }
        const auto reference=render(renderer,32.f);
        render(renderer,1.5f);require(render(renderer,32.f)==reference,"Replay did not reproduce sunlight/city shadows");
        // Warm the same scene before measuring GPU throughput. No image reads
        // or per-frame synchronization are included; this is a local diagnostic,
        // not a hardware requirement or an estimate of whole-game frame time.
        EndingSequence benchmark;
        for(int i=0;i<3;++i){benchmark.start(32.f);renderer.draw(benchmark,width,height);}
        glFinish();
        const auto started=std::chrono::steady_clock::now();
        constexpr int frames=120;
        for(int i=0;i<frames;++i){
            benchmark.start(32.f+2.f*i/(frames-1));renderer.draw(benchmark,width,height);
        }
        glFinish();
        const double milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        checkGL("Warmed city throughput");
        require(renderer.staticShadowUpdates()==staticBuilt,"City throughput run rebuilt static shadows");
        std::cout<<"Warmed city draw throughput: "<<frames<<" frames, "<<milliseconds/frames
                 <<" ms/frame at "<<width<<'x'<<height<<" (local GPU diagnostic, no pass threshold).\n";
        renderer.destroy();checkGL("Destroy sunlight resources");
        renderer.destroy();checkGL("Destroy twice");
        require(render(renderer,32.f)==reference,"Recreated renderer did not reproduce the city image");
        renderer.destroy();checkGL("Final destroy");
        glfwDestroyWindow(window);window=nullptr;glfwTerminate();
        std::cout<<"PASS: daylight occlusion, moving and paused shadows, cached static geometry, replay/recreate and GL errors 0.\n";
        return 0;
    }catch(const std::exception& error){
        std::cerr<<"FAIL: "<<error.what()<<'\n';
        if(window)glfwDestroyWindow(window);
        glfwTerminate();return 1;
    }
}
