#include "app/gl_context.h"
#include "app/render_resources.h"
#include "common/Shader.h"
#include "common/graphics.h"
#include "common/ending_renderer.h"
#include "common/ui.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>

using namespace br;
namespace {
const std::string vertex=R"(#version 330 core
void main(){gl_Position=vec4(0,0,0,1);})";
const std::string fragment=R"(#version 330 core
out vec4 color;void main(){color=vec4(1);})";
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void empty(const char* message){check(gl::snapshot().totalAlive()==0,message);}
struct StartupFailure {
    Shader good;
    UI ui;
    Shader bad;
    StartupFailure():good(vertex,fragment,true),bad(vertex,"malformed fragment shader",true){}
};
GLenum APIENTRY incomplete(GLenum){return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;}
struct IncompleteFramebuffer {
    PFNGLCHECKFRAMEBUFFERSTATUSPROC original=glad_glCheckFramebufferStatus;
    IncompleteFramebuffer(){glad_glCheckFramebufferStatus=incomplete;}
    ~IncompleteFramebuffer(){glad_glCheckFramebufferStatus=original;}
};
template<class Operation> void expectFailure(Operation operation,const char* message){
    bool failed=false;
    try {operation();} catch(const std::runtime_error&){failed=true;}
    check(failed,message);
    empty("Exception retained owned GL objects");
    check(glfwGetCurrentContext()!=nullptr,"Exception destroyed context before object cleanup");
}
}
int main(){
    try {
        static_assert(!std::is_copy_constructible_v<Shader>);
        static_assert(!std::is_copy_constructible_v<app::GpuChunk>);
        static_assert(!std::is_copy_constructible_v<EndingRenderer>);
        {
            app::WindowContext context(64,64,"Lifetime verification",true);
            empty("Unexpected initial GL ownership");
            expectFailure([] {StartupFailure partial;},"Malformed shader should fail partial session construction");
            expectFailure([] {Shader missing("missing-shader-for-test.vert","missing-shader-for-test.frag");},
                          "Missing shader did not report failure");
            expectFailure([] {
                Shader invalidVertex("this is not GLSL",fragment,true);
            },"Invalid vertex source did not report compilation failure");
            expectFailure([] {
                Shader mismatch(R"(#version 330 core
out vec3 sharedValue;void main(){sharedValue=vec3(1);gl_Position=vec4(0,0,0,1);})",
R"(#version 330 core
in vec4 sharedValue;out vec4 color;void main(){color=sharedValue;})",true);
            },"Mismatched stages did not report link failure");
            expectFailure([] {
                IncompleteFramebuffer failure;
                app::SceneTarget target;target.resize(32,32);
            },"Incomplete scene framebuffer did not report failure");
            expectFailure([] {
                IncompleteFramebuffer failure;
                ShadowMaps maps;maps.init();
            },"Incomplete shadow framebuffer did not report failure");
            expectFailure([] {
                IncompleteFramebuffer failure;
                AmbientTarget target;target.resize(32,32);
            },"Incomplete AO framebuffer did not report failure");
            {
                Shader original(vertex,fragment,true);
                const auto handle=original.id.get();
                Shader moved(std::move(original));
                check(!original.id&&moved.id.get()==handle,"Shader move lost ownership");
                check(glIsProgram(handle)==GL_TRUE,"Moving a shader deleted the live program");
                moved.destroy();moved.destroy();
                check(glIsProgram(handle)==GL_FALSE,"Explicit program reset left a live program");
            }
            empty("Shader move/reset retained resources");
            {
                UI ui;
                app::GpuChunk mesh;
                app::uploadVertices(mesh,{});
                app::SceneTarget scene;scene.resize(64,64);scene.resize(80,48);
                AmbientTarget ao;ao.resize(64,64);ao.resize(80,48);
                ShadowMaps maps;maps.init();maps.destroy();maps.init();
                app::FrameQueue queue;queue.submit();queue.wait();queue.submit();
                check(gl::snapshot().alive[std::size_t(gl::Kind::Sync)]>0,"Fence fixture did not create a sync object");
                check(gl::snapshot().alive[std::size_t(gl::Kind::Shader)]==0,"Linked shader stages were retained");
                // No manual cleanup: member ownership must empty the complete set.
            }
            empty("Automatic renderer resource destruction leaked objects");
            {
                EndingRenderer renderer;
                EndingSequence sequence;sequence.start(37);
                renderer.draw(sequence,64,64);
                const auto first=gl::snapshot().alive;
                check(gl::snapshot().totalAlive()>30,"City fixture did not initialize its resources");
                renderer.destroy();renderer.destroy();
                empty("City reset retained resources");
                renderer.draw(sequence,64,64);
                check(gl::snapshot().alive==first,"City recreation accumulated resources");
                // Destructor alone must release the recreated city and shadows.
            }
            empty("City renderer destruction leaked objects");
            check(glGetError()==GL_NO_ERROR,"OpenGL errors during lifetime verification");
        }
        check(glfwGetCurrentContext()==nullptr,"WindowContext did not terminate its context");
        empty("Objects outlived the first context");
        {
            app::WindowContext context(64,64,"Recreated context",true);
            Shader program(vertex,fragment,true);
        }
        empty("Resources outlived recreated context");
        const auto final=gl::snapshot();
        for(std::size_t i=0;i<gl::kindCount;++i)
            check(final.created[i]==final.destroyed[i],"Resource acquisition/release totals do not balance");
        std::cout<<"GL lifetime tests passed: shader read/compile/link failures, partial construction, framebuffer failures, resize, fence cleanup, city recreate and context teardown.\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
