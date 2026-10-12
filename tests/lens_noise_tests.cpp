#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "common/Shader.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Render the production lens shader on a uniform scene. Remove only its noise
// contributions for the control image, so grading/vignette cannot hide bands.
using namespace br;
static void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
static std::string read(const std::string& path) {
    std::ifstream file(path);
    require(bool(file), "Cannot read " + path);
    std::ostringstream data; data << file.rdbuf(); return data.str();
}
static void removeOnce(std::string& source, const std::string& expression) {
    const auto at = source.find(expression);
    require(at != std::string::npos, "Missing production noise expression: " + expression);
    source.erase(at, expression.size());
}
static std::vector<float> difference(const std::vector<float>& a, const std::vector<float>& b) {
    std::vector<float> result(a.size());
    for (size_t i=0; i<a.size(); ++i) result[i]=a[i]-b[i];
    return result;
}
struct Statistics {
    double mean=0, deviation=0, maxCorrelation=0, columnBand=0, rowBand=0;
    float minimum=0, maximum=0;
};
static Statistics analyze(const std::vector<float>& noise, int width, int height) {
    Statistics s;
    std::vector<double> columns(width), rows(height);
    for (int y=0; y<height; ++y) for (int x=0; x<width; ++x) {
        double value=noise[size_t(y)*width+x];
        s.mean+=value; columns[x]+=value/height; rows[y]+=value/width;
    }
    s.mean/=noise.size();
    double variance=0;
    for (float value:noise) variance+=(value-s.mean)*(value-s.mean);
    variance/=noise.size(); s.deviation=std::sqrt(variance);
    for (double value:columns) s.columnBand+=(value-s.mean)*(value-s.mean)/width;
    for (double value:rows) s.rowBand+=(value-s.mean)*(value-s.mean)/height;
    s.columnBand=std::sqrt(s.columnBand)/s.deviation;
    s.rowBand=std::sqrt(s.rowBand)/s.deviation;
    s.minimum=*std::min_element(noise.begin(),noise.end());
    s.maximum=*std::max_element(noise.begin(),noise.end());
    // Horizontal, vertical and diagonal small-lag correlations reveal repeated
    // screen-space structure even if the total noise histogram looks healthy.
    const std::array<std::array<int,2>,24> offsets={{{1,0},{2,0},{3,0},{4,0},
        {5,0},{6,0},{7,0},{8,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},{0,7},{0,8},
        {1,1},{1,-1},{2,2},{2,-2},{4,4},{4,-4},{6,1},{6,-1}}};
    for (auto offset:offsets) {
        const int dx=offset[0],dy=offset[1];
        double product=0; size_t count=0;
        for (int y=std::max(0,-dy); y<std::min(height,height-dy); ++y)
            for (int x=0; x<width-dx; ++x) {
                product+=(noise[size_t(y)*width+x]-s.mean)
                    *(noise[size_t(y+dy)*width+x+dx]-s.mean);
                ++count;
            }
        s.maxCorrelation=std::max(s.maxCorrelation,std::abs(product/(count*variance)));
    }
    return s;
}
int main(int argc, char** argv) {
    try {
        require(glfwInit()!=0,"GLFW initialization failed");
        glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        GLFWwindow* window=glfwCreateWindow(32,32,"Lens noise regression",nullptr,nullptr);
        require(window!=nullptr,"GL 3.3 context failed");
        glfwMakeContextCurrent(window);
        require(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)!=0,"GLAD initialization failed");
        std::cout << "Lens noise renderer: " << glGetString(GL_RENDERER) << '\n';
        const std::string vertex=read("vshader_lens.glsl");
        const std::string fragment=read(argc>1?argv[1]:"fshader_lens.glsl");
        std::string control=fragment;
        removeOnce(control,"color += grain * mix(0.021, 0.010, luminance);");
        removeOnce(control,"color+=grain*fear*.045;");
        removeOnce(control,"color+=staticNoise*uStatic;");
        Shader lens(vertex,fragment,true), baseline(vertex,control,true);
        GLuint vao=0, scene=0, output=0, fbo=0;
        glGenVertexArrays(1,&vao); glBindVertexArray(vao);
        glGenTextures(1,&scene); glBindTexture(GL_TEXTURE_2D,scene);
        const float uniformScene[]={.4f,.4f,.4f,1};
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,1,1,0,GL_RGBA,GL_FLOAT,uniformScene);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glGenTextures(1,&output); glGenFramebuffers(1,&fbo);
        glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_DITHER);
        int width=1920,height=1027;
        auto resize=[&]() {
            glBindTexture(GL_TEXTURE_2D,output);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA32F,width,height,0,GL_RGBA,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER,fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,output,0);
            require(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Float FBO incomplete");
            glViewport(0,0,width,height);
        };
        resize();
        auto render=[&](Shader& shader,float time,float scareTime=0,float staticAmount=0,
                        float flash=0,float black=0) {
            shader.use(); shader.set("uScene",0); shader.set("uAO",0); shader.set("uDepth",0);
            shader.set("uSize",Vec3{float(width),float(height),0});
            shader.set("uAOAmount",0.f); shader.set("uTime",time);
            shader.set("uScareTime",scareTime); shader.set("uStatic",staticAmount);
            shader.set("uFlash",flash); shader.set("uBlack",black);
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,scene);
            glDrawArrays(GL_TRIANGLES,0,3);
            std::vector<float> pixels(size_t(width)*height);
            glReadPixels(0,0,width,height,GL_RED,GL_FLOAT,pixels.data());
            return pixels;
        };
        auto checkNoise=[&](const std::vector<float>& noise,float amplitude,const std::string& label) {
            const auto s=analyze(noise,width,height);
            std::cout << std::fixed << std::setprecision(6) << label
                << ": mean=" << s.mean << " sd=" << s.deviation
                << " max-correlation=" << s.maxCorrelation
                << " column-band=" << s.columnBand << " row-band=" << s.rowBand << '\n';
            require(std::isfinite(s.deviation)&&s.deviation>0,"Noise output is not finite/nonzero");
            require(s.maxCorrelation<.03,"Repeated short-lag noise pattern: "+label);
            require(std::abs(s.mean)<amplitude*.005,"Noise acquired a brightness bias: "+label);
            require(s.deviation>amplitude*.27&&s.deviation<amplitude*.31,"Noise strength changed: "+label);
            require(s.minimum>=-amplitude*.501f&&s.maximum<=amplitude*.501f,"Noise exceeds its range: "+label);
            require(s.columnBand<.055&&s.rowBand<.055,"Noise forms screen-wide bands: "+label);
        };
        const auto clean=render(baseline,0);
        const auto initial=render(lens,0);
        for (float time:{0.f,60.f,300.f,1200.f,3600.f,10000.f,30000.f}) {
            const auto pixels=render(lens,time);
            checkNoise(difference(pixels,clean),.0166f,"grain time="+std::to_string(int(time)));
        }
        require(initial==render(lens,0),"Frozen grain time must reproduce the exact image");
        require(initial==render(lens,.01f),"Grain must remain fixed within a 24 Hz frame");
        const auto advanced=render(lens,1.f/24.f);
        double changed=0;
        for (size_t i=0;i<initial.size();++i) if(std::abs(initial[i]-advanced[i])>.0001f) ++changed;
        require(changed/initial.size()>.95,"Grain does not animate on the next 24 Hz frame");
        for (float time:{0.f,60.f,300.f,1200.f,3600.f,10000.f,30000.f}) {
            const auto pixels=render(lens,0,time,.12f);
            checkNoise(difference(pixels,initial),.12f,"scare static time="+std::to_string(int(time)));
        }
        const auto stillStatic=render(lens,0,0,.12f);
        require(stillStatic==render(lens,0,.005f,.12f),"Static must remain fixed within a 90 Hz frame");
        require(stillStatic!=render(lens,0,1.f/90.f,.12f),"Static must advance on the next 90 Hz frame");
        auto requireSolid=[&](float expected,const std::string& message) {
            std::vector<float> rgb(size_t(width)*height*3);
            glReadPixels(0,0,width,height,GL_RGB,GL_FLOAT,rgb.data());
            require(std::all_of(rgb.begin(),rgb.end(),[&](float v){return v==expected;}),message);
        };
        render(lens,3600,3600,.12f,1.f);
        requireSolid(1.f,"Scare flash must stay white in every color channel");
        render(lens,3600,3600,.12f,1.f,1.f);
        requireSolid(0.f,"Scare blackout must stay black in every color channel");
        width=1919; height=1027; resize();
        checkNoise(difference(render(lens,3600),render(baseline,3600)),.0166f,"odd-size grain time=3600");
        require(glGetError()==GL_NO_ERROR,"OpenGL error in lens regression");
        lens.destroy(); baseline.destroy(); glDeleteTextures(1,&scene); glDeleteTextures(1,&output);
        glDeleteFramebuffers(1,&fbo); glDeleteVertexArrays(1,&vao);
        glfwDestroyWindow(window); glfwTerminate();
        std::cout << "Lens noise tests passed: production shader, long-session grain/static, spatial statistics, "
                     "frozen/advanced time, odd dimensions, scare flash/blackout.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        glfwTerminate(); return 1;
    }
}
