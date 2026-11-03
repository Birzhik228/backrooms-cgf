#include "common/gl_resource.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <vector>

using namespace br;
namespace {
std::unordered_set<GLuint> names;
GLuint nextName=1;
bool contextAlive=true;
bool invalidDelete=false;
unsigned deletes=0;
void APIENTRY generate(GLsizei count,GLuint* result){while(count--){*result=nextName++;names.insert(*result++);}}
void APIENTRY remove(GLsizei count,const GLuint* values){
    while(count--){if(!contextAlive||names.erase(*values++)!=1)invalidDelete=true;++deletes;}
}
GLuint APIENTRY createProgram(){GLuint value=0;generate(1,&value);return value;}
GLuint APIENTRY createShader(GLenum){return createProgram();}
void APIENTRY deleteOne(GLuint value){remove(1,&value);}
GLsync APIENTRY fence(GLenum,GLbitfield){return reinterpret_cast<GLsync>(std::uintptr_t(createProgram()));}
void APIENTRY deleteFence(GLsync value){deleteOne(GLuint(reinterpret_cast<std::uintptr_t>(value)));}
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void install(){
    glad_glGenBuffers=generate;glad_glDeleteBuffers=remove;
    glad_glGenVertexArrays=generate;glad_glDeleteVertexArrays=remove;
    glad_glGenTextures=generate;glad_glDeleteTextures=remove;
    glad_glGenFramebuffers=generate;glad_glDeleteFramebuffers=remove;
    glad_glCreateProgram=createProgram;glad_glDeleteProgram=deleteOne;
    glad_glCreateShader=createShader;glad_glDeleteShader=deleteOne;
    glad_glFenceSync=fence;glad_glDeleteSync=deleteFence;
}
struct PartialConstruction {
    gl::Texture texture;
    gl::Buffer buffer;
    PartialConstruction(){texture.create();buffer.create();throw std::runtime_error("injected constructor failure");}
};
}

int main(){
    static_assert(!std::is_copy_constructible_v<gl::Buffer>);
    static_assert(!std::is_copy_assignable_v<gl::Texture>);
    static_assert(std::is_nothrow_move_constructible_v<gl::Program>);
    static_assert(std::is_nothrow_move_assignable_v<gl::Sync>);
    install();
    {
        gl::Buffer first,second;first.create();second.create();
        const auto firstName=first.get(),secondName=second.get();
        second=std::move(first);
        check(!first&&second.get()==firstName,"Move transfers the name and empties the source");
        check(!names.count(secondName)&&deletes==1,"Move assignment releases its previous resource once");
        auto& alias=second;
        second=std::move(alias);
        check(second.get()==firstName&&deletes==1,"Self move retains ownership");
        second.reset();second.reset();
        check(deletes==2&&names.empty(),"Explicit reset is idempotent");
    }
    check(gl::snapshot().totalAlive()==0,"Moved owners leave no retained objects");
    bool failed=false;
    try {PartialConstruction partial;}
    catch(const std::runtime_error&){failed=true;}
    check(failed,"Failure fixture did not throw");
    check(names.empty()&&gl::snapshot().totalAlive()==0,"Partially constructed aggregates unwind their members");
    {
        std::vector<gl::Texture> textures;
        for(int i=0;i<200;++i){gl::Texture value;value.create();textures.push_back(std::move(value));}
        check(names.size()==200&&gl::snapshot().alive[std::size_t(gl::Kind::Texture)]==200,
              "Container relocation must transfer ownership, not clone or delete names");
    }
    {
        gl::VertexArray vao;vao.create();
        gl::Framebuffer fbo;fbo.create();
        gl::Program program;program.create();
        gl::ShaderObject shader;shader.create(GL_VERTEX_SHADER);
        gl::Sync sync;sync.create();gl::Sync moved(std::move(sync));
        check(!sync&&moved,"Fence ownership moves without deleting the fence");
    }
    contextAlive=false;
    const auto final=gl::snapshot();
    check(names.empty()&&final.totalAlive()==0&&!invalidDelete,"Every resource is released once before context teardown");
    for(std::size_t i=0;i<gl::kindCount;++i)
        check(final.created[i]>0&&final.created[i]==final.destroyed[i],"Per-kind acquisition/release counters balance");
    std::cout<<"GL ownership tests passed: moves, vector relocation, partial construction, idempotent reset and all resource counters.\n";
}
