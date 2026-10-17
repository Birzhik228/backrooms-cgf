#pragma once
#include "ui.h"
#include <algorithm>
#include <cmath>

namespace br {
struct GuideLayout {
    float scale,left,top;
    GuideLayout(int w,int h) {
        scale=std::min({1.2f,(w-40)/900.f,(h-40)/550.f});
        left=(w-900*scale)*.5f;top=(h-550*scale)*.5f;
    }
    bool contains(double x,double y,float bx,float by,float bw,float bh) const {
        x=(x-left)/scale;y=(y-top)/scale;
        return x>=bx && x<=bx+bw && y>=by && y<=by+bh;
    }
};

inline void drawControlsGuide(UI& ui,int width,int height,int page,bool fun=false) {
    const GuideLayout g(width,height);
    const Color gold{.78f,.66f,.36f,1},paper{.91f,.89f,.76f,1},muted{.53f,.55f,.43f,1};
    const Color ink{.045f,.055f,.036f,1},edge{.23f,.25f,.17f,1};
    auto rect=[&](float x,float y,float w,float h,Color c){ui.rect(g.left+x*g.scale,g.top+y*g.scale,w*g.scale,h*g.scale,c);};
    auto line=[&](float x,float y,float xx,float yy,float weight,Color c){ui.line(g.left+x*g.scale,g.top+y*g.scale,g.left+xx*g.scale,g.top+yy*g.scale,weight*g.scale,c);};
    auto text=[&](float x,float y,const std::string& s,float size,Color c){ui.text(g.left+x*g.scale,g.top+y*g.scale,s,size*g.scale,c);};
    auto oval=[&](float x,float y,float rx,float ry,Color c){
        for(int i=0;i<40;++i) {float a=i*6.2831853f/40,b=(i+1)*6.2831853f/40;
            line(x+std::cos(a)*rx,y+std::sin(a)*ry,x+std::cos(b)*rx,y+std::sin(b)*ry,2,c);}
    };
    auto key=[&](float x,float y,float w,const std::string& label){
        rect(x,y+4,w,34,{.015f,.022f,.014f,1});rect(x,y,w,32,edge);rect(x+1,y+1,w-2,28,{.12f,.14f,.085f,1});
        rect(x+4,y+2,w-8,1,muted);text(x+(w-UI::textWidth(label,1.5f))*.5f,y+11,label,1.5f,paper);
    };
    auto row=[&](float y,const std::string& button,const std::string& title,const std::string& detail){
        key(446,y,78,button);text(542,y+1,title,1.65f,paper);text(542,y+21,detail,1.13f,muted);
    };
    ui.rect(0,0,float(width),float(height),{.009f,.013f,.008f,.83f});
    rect(8,10,900,550,{0,0,0,.35f});rect(0,0,900,550,ink);rect(0,0,900,2,gold);
    text(32,24,"FIELD GUIDE",1.3f,gold);text(165,24,fun?"LEVEL FUN":"LEVEL 0",1.3f,muted);
    rect(838,14,38,32,{.11f,.13f,.08f,1});line(851,24,863,36,1.5f,paper);line(863,24,851,36,1.5f,paper);
    const char* stages[]={"MOVE","EXPLORE",fun?"WATCH":"SURVIVE"};
    for(int i=0;i<3;++i) {
        const float x=32+i*279;
        rect(x,61,260,3,i<=page?gold:edge);
        text(x,77,"0"+std::to_string(i+1),1.2f,i==page?gold:muted);
        text(x+30,77,stages[i],1.2f,i==page?paper:muted);
    }
    const char* titles[]={"FIND YOUR FOOTING","FOLLOW THE SIGNAL","STAY OUT OF SIGHT"};
    const char* subtitles[]={"Take a moment. Learn how to move.","Light the way. Find the key. Reach the exit.","Listen carefully. You are not alone."};
    text(32,116,fun&&page>0?"THE PARTY GOES ON":titles[page],2.65f,paper);
    text(32,150,fun&&page>0?"Partygoers move when you look away. Keep them in sight.":subtitles[page],1.35f,muted);
    rect(32,187,380,260,{.071f,.085f,.052f,1});
    // Blueprint-style illustrations are native UI geometry, crisp at every size.
    for(int i=0;i<9;++i) line(48+i*43,199,48+i*43,431,1,{.095f,.112f,.072f,1});
    for(int i=0;i<6;++i) line(48,205+i*43,397,205+i*43,1,{.095f,.112f,.072f,1});
    if(page==0) {
        key(117,239,48,"W");key(61,281,48,"A");key(117,281,48,"S");key(173,281,48,"D");
        oval(304,278,34,53,gold);line(270,270,338,270,2,gold);line(304,226,304,270,2,gold);
        rect(301,240,6,15,paper);line(304,225,304,214,2,muted);
        line(277,347,331,347,1.5f,muted);line(277,347,284,341,1.5f,muted);line(331,347,324,341,1.5f,muted);
        text(92,367,"MOVE",1.6f,gold);text(273,367,"LOOK",1.6f,gold);
        row(200,"WASD","MOVE AROUND","Walk through the rooms.");
        row(260,"MOUSE","LOOK AROUND","Turn and watch your corners.");
        row(320,"SHIFT","SPRINT","Rest to recover stamina.");
        row(380,"CTRL","CROUCH","Lower yourself. Move quietly.");
    } else if(fun) {
        rect(75,315,210,12,paper);rect(86,327,8,65,edge);rect(265,327,8,65,edge);
        rect(140,265,78,50,gold);rect(140,282,78,7,paper);rect(136,261,86,8,paper);
        for(int i=0;i<3;++i)rect(157+i*20,241,4,20,paper);
        oval(329,245,24,32,gold);line(329,277,306,386,1,muted);
        text(93,406,"WELCOME TO LEVEL FUN",1.4f,gold);
        if(page==1) {
            row(200,"E","OPEN PRESENTS","Search for the exit key.");
            row(260,"TAB","LOCAL MAP","Find your way between rooms.");
            row(320,"R","RETURN TO START","Stay on this level.");
            row(380,"E","UNLOCK THE EXIT","Find the key, then open the door.");
        } else {
            row(200,"ESC","TAKE A BREAK","Pause and release the cursor.");
            row(260,"M","MUTE SOUND","Toggle the game audio.");
            row(320,"LOOK","KEEP THEM IN SIGHT","Contact is deadly. Watch your back.");
            row(380,"H","OPEN THIS GUIDE","Revisit the controls.");
        }
    } else if(page==1) {
        rect(64,266,64,28,edge);rect(118,258,18,44,gold);rect(70,270,30,4,paper);
        line(138,260,207,220,2,gold);line(138,300,207,340,2,gold);
        line(155,280,200,280,1.5f,muted);
        oval(224,289,20,20,gold);oval(224,289,10,10,gold);
        rect(244,286,62,7,gold);rect(283,291,7,15,gold);rect(297,291,7,11,gold);
        rect(310,210,66,138,edge);rect(318,217,50,125,ink);line(326,228,358,220,2,paper);line(326,228,326,331,2,paper);line(326,331,358,339,2,paper);rect(331,277,4,4,gold);
        text(321,361,"EXIT",1.5f,gold);text(60,401,"THE KEY UNLOCKS THE EXIT",1.25f,paper);
        row(200,"F","FLASHLIGHT","Reveal what lies ahead.");
        row(260,"C","REVEAL CLUES","Look for marks in the light.");
        row(320,"TAB","LOCAL MAP","Follow the gold key marker.");
        row(380,"E","INTERACT","Open doors. Enter a vent.");
    } else {
        rect(61,218,204,129,edge);rect(68,225,190,115,ink);
        oval(162,278,24,33,muted);line(133,327,142,305,3,muted);line(142,305,184,305,3,muted);line(184,305,195,327,3,muted);
        for(int i=0;i<6;++i) rect(66,231+i*20,194,6,gold);
        rect(161,224,5,117,gold);text(91,363,"HIDE BEHIND THE GRILLE",1.15f,paper);
        oval(328,301,24,14,muted);oval(309,291,8,9,gold);oval(313,309,6,6,gold);
        line(352,302,369,315,2,muted);line(369,315,381,310,2,muted);
        line(295,272,289,260,2,gold);line(306,267,306,252,2,gold);line(318,269,325,257,2,gold);
        text(64,403,"SMALL SOUNDS CAN REVEAL YOU",1.15f,gold);
        row(200,"E","HIDE OR LEAVE","Break line of sight first.");
        row(260,"ESC","TAKE A BREAK","Pause and release the cursor.");
        row(320,"H","OPEN THIS GUIDE","Come back whenever you need.");
        text(446,384,"MICE SQUEAK WHEN TOUCHED.",1.45f,gold);
        text(446,408,"The creature comes to investigate.",1.22f,muted);
    }
    rect(32,468,836,1,edge);
    if(page>0) {rect(32,486,126,40,edge);text(61,501,"BACK",1.6f,paper);}
    text(185,502,"ESC TO SKIP",1.15f,muted);
    rect(652,484,216,44,gold);text(675,499,page==2?"BEGIN EXPLORING":"NEXT",1.5f,ink);
    if(page<2) {line(841,501,847,506,2,ink);line(847,506,841,511,2,ink);}
    text(492,502,"ENTER TO CONTINUE",1.1f,muted);
}
} // namespace br
