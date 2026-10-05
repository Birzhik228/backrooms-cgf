#pragma once
#include <string>
#include <vector>
#include "Shader.h"
namespace br {
struct Color{float r,g,b,a=1;};
class UI {
    struct Point{float x,y,r,g,b,a;};
    std::vector<Point> vertices;
    Shader shader;
    GLuint vao=0,vbo=0;
public:
    UI();
    void rect(float x,float y,float w,float h,Color c);
    void line(float x,float y,float x2,float y2,float width,Color c);
    void text(float x,float y,const std::string& s,float scale,Color c);
    static float textWidth(const std::string& s,float scale){return static_cast<float>(s.size())*6*scale;}
    void render(int w,int h);
    void destroy();
};
}
