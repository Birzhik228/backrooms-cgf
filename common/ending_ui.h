#pragma once
#include "ui.h"
#include "ending.h"
#include <algorithm>
#include <cmath>

namespace br {
struct EndingButtons {
    float scale,left,top;
    EndingButtons(int w,int h):scale(std::min({1.3f,w/1000.f,h/650.f})),left(w*.5f-230*scale),top(h*.62f){}
    int at(double x,double y)const {
        if(x<left||x>left+460*scale)return -1;
        for(int i=0;i<2;++i)if(y>=top+i*54*scale&&y<top+(i*54+42)*scale)return i;
        return -1;
    }
};
inline void drawEndingUI(UI& ui,int w,int h,const EndingSequence& ending,bool paused,int selected) {
    const float scale=std::min({1.3f,w/1000.f,h/650.f});
    const float time=ending.time();
    auto centered=[&](float y,const std::string& text,float size,Color c){
        ui.text((w-UI::textWidth(text,size*scale))*.5f,y,text,size*scale,c);
    };
    const Color pale{.88f,.88f,.78f,1},gold{.72f,.65f,.39f,1},dim{.52f,.56f,.53f,1};
    const float bar=h*.085f;
    ui.rect(0,0,float(w),bar,{.006f,.008f,.007f,1});
    ui.rect(0,h-bar,float(w),bar,{.006f,.008f,.007f,1});
    ui.text(28*scale,bar*.38f,"THRESHOLD / THE SURFACE",1.3f*scale,dim);
    const float fade=sampleEndingPose(time).fade;
    if(fade>0)ui.rect(0,0,float(w),float(h),{.006f,.008f,.007f,fade});
    if(ending.finished()) {
        ui.rect(0,0,float(w),float(h),{.006f,.008f,.007f,1});
        centered(h*.23f,"THE SURFACE",3.7f,pale);
        centered(h*.34f,"YOU FOUND THE WAY OUT.",1.9f,gold);
        centered(h*.43f,"The city kept moving without you.",1.35f,dim);
        const EndingButtons buttons(w,h);
        const char* names[]={"REPLAY / R","BACK TO MENU / ENTER"};
        for(int i=0;i<2;++i) {
            const float y=buttons.top+i*54*scale;
            ui.rect(buttons.left,y,460*scale,42*scale,selected==i?Color{.15f,.17f,.11f,1}:Color{.04f,.05f,.035f,1});
            ui.rect(buttons.left,y,2*scale,42*scale,gold);
            centered(y+14*scale,names[i],1.55f,selected==i?pale:dim);
        }
        centered(h*.9f,"TEST ENDING / ELEVATOR TO THE SURFACE",1.1f,dim);
    } else {
        const char* line=time<3?"The doors are finally closing.":time<10?"Above you, somewhere, there is a street.":
                         time<13?"GROUND FLOOR":time<20?"You can hear the outside.":time<23?"Just one more door.":
                         time<30?"Fresh air. Traffic. People.":time<39?"Life went on.":"";
        if(*line)centered(h-bar-30*scale,line,1.5f,pale);
        centered(h-bar*.58f,"ESC PAUSE   /   SPACE SKIP   /   M SOUND",1.1f,dim);
        if(paused) {
            ui.rect(0,bar,float(w),h-2*bar,{.008f,.012f,.01f,.65f});
            centered(h*.44f,"PAUSED",3.f,pale);
            centered(h*.53f,"ESC RESUME / SPACE SKIP",1.4f,gold);
        }
    }
}
}
