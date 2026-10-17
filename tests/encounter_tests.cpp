#include "common/encounter.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
static void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
int main() {
    br::JumpscareConfig noContactScare;noContactScare.enabled=false;
    br::Encounter contact(noContactScare);
    check(contact.defeatImmediately() && contact.gameOver() && contact.black() && !contact.presenting(),"contact death bypasses jumpscare even when scare disabled");
    check(!contact.defeatImmediately() && contact.flashForDraw()==0,"contact death cannot retrigger or flash");
    contact.reset();check(contact.alive(),"checkpoint resets contact death");
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

    br::JumpscareManager camera;
    check(camera.visualFov()==70 && camera.shake().x==0,"Inactive camera has no scare effects");
    camera.trigger(); camera.update(.10f);
    check(std::abs(camera.cameraTurn()-.875f)<.0001f,"Camera faces the creature with cubic ease-out");
    check(camera.lunge()<camera.cameraTurn(),"Camera turn and lunge have independent timings");
    camera.update(.02f);
    check(std::abs(camera.visualFov()-95)<.0001f,"FOV punches from 70 to 95 degrees in 120 ms");
    camera.update(.08f);
    check(camera.cameraTurn()==1 && camera.lunge()<1,"Camera finishes its turn at 200 ms before the lunge");
    camera.update(.02f);
    check(camera.lunge()==1,"Lunge finishes at 220 ms");
    const auto heldShake=camera.shake(); const float heldFov=camera.visualFov();
    camera.update(.8f,true);
    check(camera.shake().x==heldShake.x && camera.shake().roll==heldShake.roll && camera.visualFov()==heldFov,"Suspension freezes all random shake and FOV effects");

    br::JumpscareManager shaking,twin;
    shaking.trigger(); twin.trigger();
    bool channelsDiffer=false,samplesChange=false; float previousX=shaking.shake().x;
    for(int sample=1;sample<=399;++sample) {
        shaking.update(.005f); twin.update(.005f);
        const auto value=shaking.shake(),same=twin.shake();
        const float envelope=std::pow(std::max(0.f,1.f-shaking.progress()),1.6f);
        const float bound=shaking.config().shakeOffset*envelope+1e-7f;
        check(std::abs(value.x)<=bound && std::abs(value.y)<=bound && std::abs(value.z)<=bound,"Random position channels remain inside the decaying shake envelope");
        check(std::abs(value.roll)<=shaking.config().shakeRoll*envelope+1e-7f,"Random roll remains bounded and decays");
        check(value.x==same.x && value.y==same.y && value.z==same.z && value.roll==same.roll,"Shake sampling is deterministic for matching simulation time");
        channelsDiffer|=value.x!=value.y && value.y!=value.z;
        samplesChange|=value.x!=previousX; previousX=value.x;
        check(shaking.visualFov()>=70 && shaking.visualFov()<=96.6f,"FOV oscillations stay small after the initial punch");
    }
    check(channelsDiffer && samplesChange,"High-frequency camera shake uses independently varying axes");
    check(std::abs(shaking.shake().x)<.00001f,"Shake settles nearly to zero before the presentation ends");
    shaking.update(.02f);
    check(shaking.black() && shaking.shake().x==0 && shaking.shake().y==0 && shaking.shake().z==0 && shaking.shake().roll==0,"Black interval clears camera shake completely");

    br::JumpscareConfig effectsOff;
    effectsOff.cameraTurn=false; effectsOff.lunge=false; effectsOff.shake=false; effectsOff.fovPunch=false; effectsOff.flash=false;
    br::JumpscareManager withoutEffects(effectsOff);
    withoutEffects.trigger(); withoutEffects.update(.15f);
    check(withoutEffects.cameraTurn()==0 && withoutEffects.lunge()==0 && withoutEffects.shake().x==0 && withoutEffects.shake().roll==0 && withoutEffects.visualFov()==70 && withoutEffects.flashForDraw()==0,"Individual toggles disable their effects");

    for(float duration:{.5f,3.f}) {
        br::JumpscareConfig settings; settings.duration=duration; settings.blackDelay=.4f;
        br::JumpscareManager configurable(settings); configurable.trigger(); configurable.update(duration-.01f);
        check(configurable.presenting(),"Configured scare presents until its selected duration");
        configurable.update(.02f); check(configurable.black() && !configurable.gameOver(),"Configured scare enters black at the selected duration");
        configurable.update(.4f); check(configurable.gameOver(),"Configured black delay ends at game over");
    }

    const auto configFolder=std::filesystem::path("tests")/"artifacts"/"encounter-config";
    std::filesystem::create_directories(configFolder);
    const auto configFile=configFolder/"parser-fixture.cfg";
    {
        std::ofstream fixture(configFile);
        fixture<<"# Only this disposable test fixture is read or written.\n"
                  "duration = 3 # inline comment\n"
                  "camera_turn_duration=.9\n"
                  "lunge_duration=.01\n"
                  "face_distance=-5\n"
                  "closeup_scale=2.9\n"
                  "shake_offset=0.032\n"
                  "shake_roll=nan\n"
                  "fov_start=70 junk\n"
                  "fov_peak=999\n"
                  "black_delay=inf\n"
                  "unknown_option=88\n"
                  "enabled=maybe\n"
                  "shake=false\n"
                  "flash=0\n"
                  "invalid line without equals\n";
    }
    const auto loaded=br::loadJumpscareConfig(configFile);
    check(loaded.duration==3 && loaded.closeupScale==2.9f && loaded.shakeOffset==.032f,"Optional config reads valid finite values and comments");
    check(loaded.cameraTurnDuration==.25f && loaded.lungeDuration==.1f && loaded.faceDistance==.6f && loaded.fovPeak==120,"Out-of-range settings are safely bounded");
    check(loaded.shakeRoll==br::JUMPSCARE.shakeRoll && loaded.fovStart==70 && loaded.blackDelay==.3f && loaded.enabled,"Malformed, nonfinite and unknown config entries retain defaults");
    check(!loaded.shake && !loaded.flash,"Config booleans accept false and zero");
    std::filesystem::remove(configFile);
    const auto missing=br::loadJumpscareConfig(configFile);
    check(missing.duration==2 && missing.cameraTurnDuration==.20f && missing.faceDistance==.90f && missing.closeupScale==2.2f,"Missing optional config uses safe presentation defaults");
    br::JumpscareConfig invalid;
    invalid.duration=std::numeric_limits<float>::quiet_NaN();
    invalid.cameraTurnDuration=std::numeric_limits<float>::infinity();
    invalid.faceDistance=-500; invalid.shakeRoll=500; invalid.fovStart=110; invalid.fovPeak=std::numeric_limits<float>::quiet_NaN();
    const auto sanitized=br::JumpscareManager(invalid).config();
    check(sanitized.duration==2 && sanitized.cameraTurnDuration==.20f && sanitized.faceDistance==.6f && sanitized.shakeRoll==.12f && sanitized.fovPeak>=sanitized.fovStart,"Programmatic config is also finite and safely bounded");
    std::cout<<"Encounter tests passed: one-shot capture, timed presentation/restart, camera turn, FOV punch, random decaying shake, suspension, toggles and optional configuration.\n";
}
