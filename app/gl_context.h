#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace br::app {

// The library outlives the window, and WindowContext outlives every renderer.
// Member guards also run when context creation/loading throws partway through.
class WindowContext {
    struct Library {
        Library() {
            if(!glfwInit())throw std::runtime_error("GLFW could not initialize. Check your graphics driver.");
        }
        ~Library(){glfwTerminate();}
        Library(const Library&)=delete;
        Library& operator=(const Library&)=delete;
    } library_;
    struct DeleteWindow {
        void operator()(GLFWwindow* window) const noexcept {if(window)glfwDestroyWindow(window);}
    };
    std::unique_ptr<GLFWwindow,DeleteWindow> window_;
public:
    WindowContext(int width,int height,const char* title,bool hidden) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        // Required by GLFW 3.3 on macOS; GLFW 3.4+ supplies this automatically.
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GLFW_TRUE);
        // UI hit testing and --width/--height use one coordinate per render pixel.
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER,GLFW_FALSE);
#endif
        glfwWindowHint(GLFW_VISIBLE,hidden?GLFW_FALSE:GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES,0);
        window_.reset(glfwCreateWindow(width,height,title,nullptr,nullptr));
        if(!window_) {
            const char* description=nullptr;
            glfwGetError(&description);
            throw std::runtime_error(std::string("Cannot create an OpenGL 3.3 core window. ")+
                                     (description?description:"No driver diagnostic was provided."));
        }
        glfwMakeContextCurrent(window_.get());
        if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            throw std::runtime_error("OpenGL loader failed.");
    }
    WindowContext(const WindowContext&)=delete;
    WindowContext& operator=(const WindowContext&)=delete;
    GLFWwindow* window() const noexcept {return window_.get();}
};

} // namespace br::app
