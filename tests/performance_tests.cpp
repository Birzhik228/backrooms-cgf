#include "common/performance.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace br;
static void check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
int main(){
    TimingStatistics t;
    check(t.percentile(.95)==0,"Empty percentile");
    for(int i=1;i<=100;++i)t.add(i);
    check(t.count()==100&&t.mean()==50.5&&t.maximum()==100,"Statistics cover every valid frame");
    check(t.percentile(.95)>=95&&t.percentile(.95)<=95.25,"P95 histogram resolution");
    t.add(std::numeric_limits<double>::quiet_NaN());t.add(-1);t.add(std::numeric_limits<double>::infinity());
    check(t.count()==100,"Invalid samples must not corrupt statistics/JSON");
    t.add(2500);check(t.maximum()==2500&&t.percentile(1)==2500,"Large stall is not clipped to histogram range");
    FrameStatistics frames;
    for(int i=0;i<1000;++i)frames.add(i,33.34,4,33.34,0,false);
    check(frames.slowCount()==0,"Steady capped menu frames are not stalls");
    frames.add(1001,80,70,16.67,1,false);
    check(frames.slowCount()==1&&frames.worst()[0].level==1,"A real stall retains level and work time");
    for(int i=0;i<10000;++i)frames.add(i+1002,100+i,90,0,2,false);
    check(frames.worst().size()==16&&frames.worst().front().frameMs==10099,"Long sessions retain only worst bounded samples");
    check(frames.slowCount()==10001,"Total slow count is independent of retention");
    std::cout<<"Performance statistics tests passed.\n";
}
