#include "common/encounter.h"
#include <iostream>
#include <limits>
#include <stdexcept>
static void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
int main() {
    br::Encounter encounter;
    check(encounter.alive(),"Run begins alive");
    check(encounter.catchPlayer() && encounter.scaring(),"Capture begins one jumpscare");
    encounter.update(.4f);
    const float progress=encounter.progress();
    check(!encounter.catchPlayer() && encounter.progress()==progress,"Repeated capture cannot restart or replay scare");
    encounter.update(.3f,true);
    encounter.update(-1);
    encounter.update(std::numeric_limits<float>::quiet_NaN());
    check(encounter.progress()==progress,"Suspension and invalid delta preserve scare");
    check(encounter.state()==br::JumpscareState::Hold,"Lunge transitions to hold");
    encounter.update(1.6f);
    check(encounter.black() && !encounter.gameOver(),"Hard black interval precedes game over");
    encounter.update(.31f);
    check(encounter.gameOver() && encounter.progress()==1 && !encounter.alive(),"Scare finishes at a locked game-over state");
    encounter.update(10);
    check(encounter.gameOver() && !encounter.catchPlayer(),"Game-over cannot resume or retrigger capture");
    encounter.reset();
    check(encounter.alive() && encounter.progress()==0,"Restart clears capture state");
    check(!encounter.shouldTrigger(1,false,false),"Walls block proximity trigger");
    check(encounter.shouldTrigger(1.5f,true,false),"Default trigger is 1.5 metres");
    check(!encounter.shouldTrigger(1.51f,true,false),"Distance is bounded");
    check(encounter.shouldTrigger(10,false,true),"Catch bypasses proximity");
    encounter.catchPlayer();
    check(encounter.flashForDraw()==1 && encounter.flashForDraw()==1 && encounter.flashForDraw()==0,"Flash lasts exactly two rendered frames");
    encounter.skipScare(); encounter.update(.31f);
    check(encounter.gameOver(),"Skipping the presentation still ends the run");
    for(float dt:{1.f/30,1.f/60,1.f/144}) {
        br::JumpscareManager timed;
        timed.trigger(); float seconds=0;
        while(!timed.gameOver()) { timed.update(dt); seconds+=dt; }
        check(std::abs(seconds-2.3f)<dt*1.1f,"Duration is independent of frame rate");
        timed.reset(); check(timed.alive() && timed.flashForDraw()==0,"Reset clears effect state");
        check(timed.trigger(),"Restart can trigger again");
    }
    br::JumpscareConfig disabled; disabled.enabled=false;
    check(!br::JumpscareManager(disabled).trigger(),"Master switch disables scare");
    std::cout<<"Encounter tests passed: one-shot capture, suspension, timed game over, restart and skip.\n";
}
