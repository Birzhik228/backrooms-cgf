#pragma once
#include "ui.h"
#include <algorithm>
#include <array>

namespace br {
struct LevelMenuLayout {
    float scale,left,top;
    LevelMenuLayout(int width,int height) {
        scale=std::min({1.2f,(width-40)/720.f,(height-40)/540.f});
        left=(width-720*scale)*.5f;top=(height-540*scale)*.5f;
    }
    int rowAt(double x,double y) const {
        x=(x-left)/scale;y=(y-top)/scale;
        if(x<32 || x>688)return -1;
        for(int row=0;row<4;++row) {
            const float topY=row<3?154.f+row*86.f:434.f;
            const float height=row<3?72.f:44.f;
            if(y>=topY&&y<topY+height)return row;
        }
        return -1;
    }
};

inline void drawLevelMenu(UI& ui,int width,int height,int selection,int current) {
    const LevelMenuLayout box(width,height);
    const Color gold{.78f,.66f,.36f,1},paper{.91f,.89f,.76f,1},muted{.58f,.60f,.49f,1};
    auto rect=[&](float x,float y,float w,float h,Color color){
        ui.rect(box.left+x*box.scale,box.top+y*box.scale,w*box.scale,h*box.scale,color);
    };
    auto text=[&](float x,float y,const std::string& value,float size,Color color){
        ui.text(box.left+x*box.scale,box.top+y*box.scale,value,size*box.scale,color);
    };
    ui.rect(0,0,float(width),float(height),{.01f,.015f,.009f,.82f});
    rect(0,0,720,540,{.032f,.039f,.027f,.98f});rect(0,0,720,2,gold);
    text(32,26,"BACKROOMS / TRAVEL",1.3f,gold);
    text(32,59,"CHOOSE A LEVEL",3.3f,paper);
    text(32,108,"Start at its entrance with fresh level progress.",1.35f,muted);
    const std::array<const char*,3> names={"1 / YELLOW ROOMS","2 / LEVEL FUN","3 / PARKING"};
    const std::array<const char*,3> details={
        "Find the key. Avoid Aranea. Reach the red exit.",
        "Search the presents. Keep Partygoers in sight.",
        "Repel the eyes with light. Restore three circuits."};
    for(int row=0;row<3;++row) {
        const float y=154.f+row*86.f;
        rect(32,y,656,72,selection==row?Color{.19f,.21f,.13f,1}:Color{.065f,.075f,.049f,1});
        if(selection==row)rect(32,y,3,72,gold);
        text(50,y+14,names[row],1.9f,selection==row?gold:paper);
        if(current==row)text(588,y+17,"CURRENT",1.15f,muted);
        text(50,y+46,details[row],1.2f,muted);
    }
    rect(32,434,656,44,selection==3?Color{.19f,.21f,.13f,1}:Color{.065f,.075f,.049f,1});
    text(50,449,"BACK",1.6f,selection==3?gold:paper);
    text(32,499,"UP/DOWN + ENTER OR CLICK   /   ESC BACK",1.25f,muted);
    text(32,519,"BACK RETURNS TO THE MENU. THE GAME STAYS PAUSED.",1.1f,muted);
}
}
