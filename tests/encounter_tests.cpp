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
    encounter.update(1);
    check(encounter.gameOver() && encounter.progress()==1 && !encounter.alive(),"Scare finishes at a locked game-over state");
    encounter.update(10);
    check(encounter.gameOver() && !encounter.catchPlayer(),"Game-over cannot resume or retrigger capture");
    encounter.reset();
    check(encounter.alive() && encounter.progress()==0,"Restart clears capture state");
    encounter.catchPlayer(); encounter.skipScare();
    check(encounter.gameOver(),"Skipping the presentation still ends the run");
    std::cout<<"Encounter tests passed: one-shot capture, suspension, timed game over, restart and skip.\n";
}
