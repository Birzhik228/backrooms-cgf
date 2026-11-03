#pragma once

#include <glad/glad.h>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace br::gl {

// Counts application-owned GL objects, not driver allocations or VRAM bytes.
// Creation, release and sampling occur on the thread owning the current context.
enum class Kind { Buffer, VertexArray, Texture, Framebuffer, Program, Shader, Sync, Count };
inline constexpr std::size_t kindCount = static_cast<std::size_t>(Kind::Count);
struct Snapshot {
    std::array<std::uint64_t, kindCount> alive{}, created{}, destroyed{};
    std::uint64_t totalAlive() const {
        std::uint64_t result=0;
        for(auto count:alive) result+=count;
        return result;
    }
};
inline Snapshot counters;
inline Snapshot snapshot() noexcept { return counters; }
inline const char* name(Kind kind) noexcept {
    constexpr const char* names[]={"buffers","vertex_arrays","textures","framebuffers","programs","shaders","syncs"};
    return names[static_cast<std::size_t>(kind)];
}
inline void acquired(Kind kind) noexcept {
    const auto index=static_cast<std::size_t>(kind);
    ++counters.alive[index]; ++counters.created[index];
}
inline void released(Kind kind) noexcept {
    const auto index=static_cast<std::size_t>(kind);
    --counters.alive[index]; ++counters.destroyed[index];
}

// An owner is empty or holds one GL name. Moving transfers ownership; implicit
// conversion supplies only a borrowed name to OpenGL, never a writable pointer.
// Its lifetime must be nested inside the context lifetime (see WindowContext).
template<Kind K> class Object {
    static_assert(K!=Kind::Sync && K!=Kind::Count, "Use the typed Sync owner");
    GLuint value_=0;
public:
    Object() noexcept = default;
    Object(const Object&)=delete;
    Object& operator=(const Object&)=delete;
    Object(Object&& other) noexcept : value_(std::exchange(other.value_,0)) {}
    Object& operator=(Object&& other) noexcept {
        if(this!=&other){reset();value_=std::exchange(other.value_,0);}
        return *this;
    }
    ~Object(){reset();}
    GLuint get() const noexcept {return value_;}
    operator GLuint() const noexcept {return value_;}
    void reset() noexcept {
        if(!value_)return;
        if constexpr(K==Kind::Buffer)glDeleteBuffers(1,&value_);
        else if constexpr(K==Kind::VertexArray)glDeleteVertexArrays(1,&value_);
        else if constexpr(K==Kind::Texture)glDeleteTextures(1,&value_);
        else if constexpr(K==Kind::Framebuffer)glDeleteFramebuffers(1,&value_);
        else if constexpr(K==Kind::Program)glDeleteProgram(value_);
        else if constexpr(K==Kind::Shader)glDeleteShader(value_);
        value_=0;released(K);
    }
    // For the rare API which returns a newly created name. Borrowed names must
    // never be adopted. Existing ownership is released before replacement.
    void adopt(GLuint value) noexcept {
        if(value==value_)return;
        reset();value_=value;
        if(value_)acquired(K);
    }
    void create(GLenum shaderType=0) {
        GLuint value=0;
        if constexpr(K==Kind::Buffer)glGenBuffers(1,&value);
        else if constexpr(K==Kind::VertexArray)glGenVertexArrays(1,&value);
        else if constexpr(K==Kind::Texture)glGenTextures(1,&value);
        else if constexpr(K==Kind::Framebuffer)glGenFramebuffers(1,&value);
        else if constexpr(K==Kind::Program)value=glCreateProgram();
        else if constexpr(K==Kind::Shader)value=glCreateShader(shaderType);
        if(!value)throw std::runtime_error("Cannot create OpenGL object");
        adopt(value);
    }
};
using Buffer=Object<Kind::Buffer>;
using VertexArray=Object<Kind::VertexArray>;
using Texture=Object<Kind::Texture>;
using Framebuffer=Object<Kind::Framebuffer>;
using Program=Object<Kind::Program>;
using ShaderObject=Object<Kind::Shader>;

class Sync {
    GLsync value_=nullptr;
public:
    Sync() noexcept = default;
    Sync(const Sync&)=delete;
    Sync& operator=(const Sync&)=delete;
    Sync(Sync&& other) noexcept : value_(std::exchange(other.value_,nullptr)) {}
    Sync& operator=(Sync&& other) noexcept {
        if(this!=&other){reset();value_=std::exchange(other.value_,nullptr);}
        return *this;
    }
    ~Sync(){reset();}
    GLsync get() const noexcept {return value_;}
    operator GLsync() const noexcept {return value_;}
    void reset() noexcept {
        if(value_){glDeleteSync(value_);value_=nullptr;released(Kind::Sync);}
    }
    void create() {
        const auto value=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);
        if(!value)throw std::runtime_error("Cannot create GPU frame fence");
        reset();value_=value;acquired(Kind::Sync);
    }
};

} // namespace br::gl
