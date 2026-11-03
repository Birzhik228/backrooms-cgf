#include "render_resources.h"
#include "diagnostics.h"
#include "../common/render_limits.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <thread>

namespace br::app {
namespace fs = std::filesystem;

void release(GpuChunk& chunk) {
    chunk = {};
}

void uploadVertices(GpuChunk& chunk, const std::vector<Vertex>& vertices,
                           GLenum usage) {
    if (!chunk.vao) {
        chunk.vao.create();
        chunk.vbo.create();
    }
    chunk.count = static_cast<GLsizei>(vertices.size());

    glBindVertexArray(chunk.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), usage);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(i);
    }
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, x)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, nx)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, u)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, material)));
    glBindVertexArray(0);
}

void upload(GpuChunk& chunk, const Chunk& data) {
    // Partition triangles by cell, but retain their actual bounds: walls and
    // large floors crossing a cell edge must never disappear at that edge.
    constexpr int cells=CHUNK_CELLS*CHUNK_CELLS;
    std::array<std::vector<Vertex>,cells> nearParts, farParts;
    auto partition=[](const std::vector<Vertex>& source,auto& parts) {
        for(size_t i=0;i+2<source.size();i+=3) {
            const auto &a=source[i],&b=source[i+1],&c=source[i+2];
            int x=std::clamp(int(std::floor((a.x+b.x+c.x)/(3*CELL))),0,CHUNK_CELLS-1);
            int z=std::clamp(int(std::floor((a.z+b.z+c.z)/(3*CELL))),0,CHUNK_CELLS-1);
            auto& part=parts[z*CHUNK_CELLS+x];
            part.insert(part.end(),source.begin()+i,source.begin()+i+3);
        }
    };
    partition(data.vertices,nearParts);partition(data.distantVertices,farParts);
    std::vector<Vertex> packed;
    packed.reserve(data.vertices.size()+data.distantVertices.size());
    chunk.sections.clear();
    for(int i=0;i<cells;++i) {
        if(nearParts[i].empty())continue;
        RenderSection section;
        Vec3 lo{1e9f,1e9f,1e9f},hi{-1e9f,-1e9f,-1e9f};
        auto append=[&](const auto& part) {
            for(const auto& v:part) {
                lo.x=std::min(lo.x,v.x);lo.y=std::min(lo.y,v.y);lo.z=std::min(lo.z,v.z);
                hi.x=std::max(hi.x,v.x);hi.y=std::max(hi.y,v.y);hi.z=std::max(hi.z,v.z);
            }
            packed.insert(packed.end(),part.begin(),part.end());
        };
        section.first=GLint(packed.size());section.count=GLsizei(nearParts[i].size());append(nearParts[i]);
        section.distantFirst=GLint(packed.size());section.distantCount=GLsizei(farParts[i].size());append(farParts[i]);
        section.center=(lo+hi)*.5f;section.half=(hi-lo)*.5f+Vec3{.01f,.01f,.01f};
        chunk.sections.push_back(section);
    }
    uploadVertices(chunk,packed);
    chunk.revision = data.revision;
}

// Multi-draw keeps cell culling from turning into hundreds of driver calls.
std::size_t drawSections(const GpuChunk& mesh,const Mat4& vp,Vec3 offset,
                           Vec3 eye,bool distantDetail) {
    std::array<GLint,CHUNK_CELLS*CHUNK_CELLS> starts{};
    std::array<GLsizei,CHUNK_CELLS*CHUNK_CELLS> counts{};
    GLsizei n=0;size_t triangles=0;
    for(const auto& section:mesh.sections) {
        Vec3 center=section.center+offset;
        if(!visibleBox(vp,center,section.half))continue;
        // Use distance to the nearest bound, not centre: nearby objects keep
        // their detail even when their section extends far from the camera.
        float dx=std::max(0.f,std::abs(eye.x-center.x)-section.half.x);
        float dz=std::max(0.f,std::abs(eye.z-center.z)-section.half.z);
        if(distantDetail && dx*dx+dz*dz>SCENE_VIEW_DISTANCE*SCENE_VIEW_DISTANCE)continue;
        bool low=distantDetail && section.distantCount && dx*dx+dz*dz>24.f*24.f;
        starts[n]=low?section.distantFirst:section.first;
        counts[n]=low?section.distantCount:section.count;
        triangles+=counts[n]/3;++n;
    }
    if(n)glMultiDrawArrays(GL_TRIANGLES,starts.data(),counts.data(),n);
    return triangles;
}

void FrameQueue::wait() {
    if (!fences[next]) return;
    GLenum status;
    do {
        status = glClientWaitSync(fences[next], GL_SYNC_FLUSH_COMMANDS_BIT, 50000000);
        if (status == GL_WAIT_FAILED) throw std::runtime_error("GPU frame synchronization failed.");
    } while (status == GL_TIMEOUT_EXPIRED);
    fences[next].reset();
}

void FrameQueue::submit() {
    fences[next].create();
    next = (next + 1) % fences.size();
}

void FrameQueue::destroy() {
    for (auto& fence : fences) {
        fence.reset();
    }
}

FramePacer::FramePacer() {
#ifdef _WIN32
    timer_=CreateWaitableTimerExW(nullptr,nullptr,0x00000002,TIMER_ALL_ACCESS);
#endif
}

FramePacer::~FramePacer() {
#ifdef _WIN32
    if(timer_)CloseHandle(timer_);
#endif
}

void FramePacer::wait(double seconds) {
    if(seconds<=0)return;
#ifdef _WIN32
    if(timer_) {
        LARGE_INTEGER due;due.QuadPart=-std::max<LONGLONG>(1,LONGLONG(seconds*10000000.0));
        if(SetWaitableTimer(timer_,&due,0,nullptr,nullptr,FALSE)) {
            WaitForSingleObject(timer_,INFINITE);return;
        }
    }
#endif
    std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
}

void SceneTarget::resize(int w, int h) {
    if (width == w && height == h) {
        return;
    }
    destroy();
    width = w;
    height = h;

    fbo.create();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    color.create();
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, color, 0);

    depth.create();
    glBindTexture(GL_TEXTURE_2D,depth);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,w,h,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth,0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        destroy();
        throw std::runtime_error("Offscreen framebuffer is incomplete.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneTarget::destroy() {
    depth.reset();color.reset();fbo.reset();
    width = height = 0;
}

void screenshot(const fs::path& path, int width, int height) {
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }
    const int rowSize = (width * 3 + 3) & ~3;
    std::vector<unsigned char> pixels(size_t(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // BMP rows and OpenGL's origin both start at the bottom of the image.
    std::array<unsigned char, 54> header{};
    header[0] = 'B';
    header[1] = 'M';
    auto put = [&](int offset, uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            header[offset + i] = static_cast<unsigned char>(value >> (8 * i));
        }
    };
    put(2, 54 + rowSize * height);
    put(10, 54);
    put(14, 40);
    put(18, width);
    put(22, height);
    header[26] = 1;
    header[28] = 24;
    put(34, rowSize * height);

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot save screenshot: " + path.string());
    }
    file.write(reinterpret_cast<char*>(header.data()), header.size());
    std::vector<unsigned char> scan(rowSize);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t pixel = (size_t(y) * width + x) * 3;
            scan[x * 3] = pixels[pixel + 2];
            scan[x * 3 + 1] = pixels[pixel + 1];
            scan[x * 3 + 2] = pixels[pixel];
        }
        file.write(reinterpret_cast<char*>(scan.data()), rowSize);
    }
    if (!file) {
        throw std::runtime_error("Failed to write screenshot.");
    }
    log("Screenshot saved: " + path.string());
}

} // namespace br::app
