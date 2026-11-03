#pragma once

#include "../common/gl_resource.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "../common/math.h"
#include "../world.h"

namespace br::app {

// Shared mesh storage for streamed chunks and dynamic scene objects.
struct RenderSection {
    Vec3 center{}, half{};
    GLint first=0, distantFirst=0;
    GLsizei count=0, distantCount=0;
};
struct GpuChunk {
    std::vector<RenderSection> sections;
    gl::VertexArray vao;
    gl::Buffer vbo;
    GLsizei count = 0;
    std::uint64_t revision = 0;
};

void release(GpuChunk& chunk);
void uploadVertices(GpuChunk& chunk, const std::vector<Vertex>& vertices,
                    GLenum usage = GL_STATIC_DRAW);
void upload(GpuChunk& chunk, const Chunk& data);
std::size_t drawSections(const GpuChunk& mesh, const Mat4& vp, Vec3 offset,
                         Vec3 eye, bool distantDetail);

// Bound outstanding GPU submissions without an uncapped CPU busy loop.
struct FrameQueue {
    std::array<gl::Sync, 2> fences{};
    std::size_t next = 0;
    void wait();
    void submit();
    void destroy();
};

// Platform details stay in the implementation, keeping windows.h out of users.
class FramePacer {
#ifdef _WIN32
    void* timer_ = nullptr;
#endif
public:
    FramePacer();
    FramePacer(const FramePacer&) = delete;
    FramePacer& operator=(const FramePacer&) = delete;
    ~FramePacer();
    void wait(double seconds);
};

// Scene framebuffer used by the camera treatment and ambient-occlusion passes.
struct SceneTarget {
    gl::Framebuffer fbo;
    gl::Texture color, depth;
    int width = 0, height = 0;
    void resize(int w, int h);
    void destroy();
};

void screenshot(const std::filesystem::path& path, int width, int height);

} // namespace br::app
