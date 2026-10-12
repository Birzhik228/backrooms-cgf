#pragma once
#include "Shader.h"
#include <array>
#include <cstdint>
#include <stdexcept>

namespace br {
// Six ordinary 2D depth layers per point light work on the existing GL 3.3
// baseline. Static maps survive camera movement; doors invalidate the cache.
inline Mat4 lightView(Vec3 eye, Vec3 f, Vec3 up) {
    Vec3 s=normalize(cross(f,up)), u=cross(s,f);
    Mat4 a=Mat4::identity();
    a.m[0]=s.x;a.m[4]=s.y;a.m[8]=s.z;
    a.m[1]=u.x;a.m[5]=u.y;a.m[9]=u.z;
    a.m[2]=-f.x;a.m[6]=-f.y;a.m[10]=-f.z;
    a.m[12]=-dot(s,eye);a.m[13]=-dot(u,eye);a.m[14]=dot(f,eye);
    return a;
}
inline Mat4 lampFace(Vec3 eye,int face) {
    const std::array<Vec3,6> dirs={Vec3{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    return perspective(PI*.5f,1,.05f,10.f)*lightView(eye,dirs[face],face==2||face==3?Vec3{0,0,1}:Vec3{0,1,0});
}
struct ShadowMaps {
    GLuint fbo=0,flash=0,lamps=0;
    struct Cache {Vec3 position{};std::uint64_t revision=0;bool valid=false;};
    std::array<Cache,24> cache{};
    static constexpr int flashSize=1024,lampSize=256;
    void init() {
        glGenFramebuffers(1,&fbo);
        glGenTextures(1,&flash);glBindTexture(GL_TEXTURE_2D,flash);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,flashSize,flashSize,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
        setup(GL_TEXTURE_2D);
        glGenTextures(1,&lamps);glBindTexture(GL_TEXTURE_2D_ARRAY,lamps);
        glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_DEPTH_COMPONENT24,lampSize,lampSize,144,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
        setup(GL_TEXTURE_2D_ARRAY);
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,flash,0);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Shadow framebuffer incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }
    static void setup(GLenum type) {
        glTexParameteri(type,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(type,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(type,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(type,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexParameteri(type,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_REF_TO_TEXTURE);glTexParameteri(type,GL_TEXTURE_COMPARE_FUNC,GL_LEQUAL);
    }
    void begin(int layer=-1) {
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        if(layer<0) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,flash,0);
        else glFramebufferTextureLayer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,lamps,0,layer);
        const int size=layer<0?flashSize:lampSize;glViewport(0,0,size,size);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    void destroy() {glDeleteTextures(1,&flash);glDeleteTextures(1,&lamps);glDeleteFramebuffers(1,&fbo);}
};
struct AmbientTarget {
    GLuint fbo=0,texture=0;int width=0,height=0;
    void resize(int w,int h) {
        w=(w+1)/2;h=(h+1)/2;if(w==width&&h==height)return;
        destroy();width=w;height=h;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R8,w,h,0,GL_RED,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("AO framebuffer incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER,0);
    }
    void destroy(){if(texture)glDeleteTextures(1,&texture);if(fbo)glDeleteFramebuffers(1,&fbo);texture=fbo=0;width=height=0;}
};
}
