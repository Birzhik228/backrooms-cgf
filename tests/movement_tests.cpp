#include "common/gameplay.h"
#include "common/movement.h"
#include "common/player_state.h"
#include "common/settings.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace br;
static void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
static float journey(int hz) {
    PlayerMotion motion;
    float distance = 0;
    for (int i = 0; i < hz * 2; ++i) distance += motion.advance({WALK_SPEED,0,0}, 1.f/hz).x;
    check(std::abs(motion.speed()-WALK_SPEED)<.001f, "Walking reaches its capped speed");
    for (int i = 0; i < hz; ++i) distance += motion.advance({}, 1.f/hz).x;
    check(motion.speed() == 0, "Braking reaches zero without drift");
    return distance;
}
int main() {
    check(WALK_SPEED == 2.5f && SPRINT_SPEED == 4.f && CROUCH_SPEED == 1.1f, "Slow exploration speeds");
    check(std::abs(journey(30)-journey(144)) < .002f, "Acceleration and braking independent of frame rate");
    PlayerMotion motion;
    motion.advance({WALK_SPEED,0,0}, 1.f/60);
    check(motion.speed()>0 && motion.speed()<WALK_SPEED*.1f, "No instant full-speed start");
    for (int i=0;i<120;++i) motion.advance({SPRINT_SPEED,0,0},1.f/60);
    const Vec3 diagonal = normalize({1,0,1})*SPRINT_SPEED;
    for(int i=0;i<120;++i) {
        motion.advance(diagonal,1.f/60);
        check(motion.speed()<=SPRINT_SPEED+.001f,"Turning cannot increase speed above cap");
    }
    for(int i=0;i<120;++i) motion.advance({-WALK_SPEED,0,0},1.f/60);
    check(motion.velocity.x<0 && std::abs(motion.velocity.z)<.001f,"Reversal settles in requested direction");
    motion.advance({CROUCH_SPEED,0,0},1.f);
    check(std::abs(motion.speed()-CROUCH_SPEED)<.001f,"Changing stance slows to crouch speed");
    motion.reset();
    check(motion.speed()==0,"Pause/reset clears momentum");
    check(motion.advance({},.05f).x==0,"Stationary input produces no travel");
    Stamina stamina;
    stamina.update(3, true, false);
    check(std::abs(stamina.value-46)<.001f, "Actual sprint travel consumes stamina");
    stamina.update(20,true,true);
    check(std::abs(stamina.value-46)<.001f, "Pause freezes stamina");
    stamina.update(3,true,false);
    check(!stamina.canSprint() && stamina.value==0,"Exhaustion caps at zero and stops sprinting");
    stamina.update(1.5f,false,false);
    check(stamina.value==0,"Recovery waits after sprinting");
    stamina.update(1,false,false);
    check(!stamina.canSprint(),"Exhausted player must recover sufficiently before restarting");
    stamina.update(1,false,false);
    check(stamina.canSprint() && stamina.value==28,"Recovery unlocks sprinting");
    stamina.update(100,false,false);
    check(stamina.value==100,"Recovery caps at full stamina");
    auto staminaAt=[](int hz) {
        Stamina a;
        for(int i=0;i<4*hz;++i)a.update(1.f/hz,true,false);
        for(int i=0;i<3*hz;++i)a.update(1.f/hz,false,false);
        return a.value;
    };
    check(std::abs(staminaAt(30)-staminaAt(144))<.005f,"Stamina drain/recovery independent of FPS");
    SmoothLook low,high;
    low.add(100,1000,.095f);high.add(100,1000,.095f);
    check(low.yaw==-90 && low.targetPitch==-85,"Mouse input sets targets and clamps pitch");
    for(int i=0;i<3;++i)low.update(1.f/30);
    for(int i=0;i<12;++i)high.update(1.f/120);
    check(std::abs(low.yaw-high.yaw)<.0001f,"Mouse smoothing independent of FPS");
    low.reset(42,3); low.update(.1f);
    check(low.yaw==42 && low.pitch==3,"Mouse reset prevents stale delta after pause");
    Settings settings;
    const auto path=std::filesystem::temp_directory_path()/"backrooms-v10-settings-test.cfg";
    settings.volume=.3f;settings.blur=0;settings.vsync=false;
    settings.musicEnabled=false;settings.musicVolume=.65f;
    check(settings.save(path.string()),"Settings save succeeds");
    Settings loaded;
    check(loaded.load(path.string()) && loaded.volume==.3f && loaded.blur==0 && !loaded.vsync,"Settings reload preserves user preferences");
    check(!loaded.musicEnabled && loaded.musicVolume==.65f,"Music toggle and independent volume persist");
    {std::ofstream malformed(path);malformed<<"volume 200\nfov -500\nbrightness nan\nunknown 10\nsensitivity garbage\nblur -1\n";}
    Settings bounded;bounded.load(path.string());
    check(bounded.volume==1 && bounded.fov==65 && bounded.brightness==1 && bounded.sensitivity==.095f && bounded.blur==0,"Malformed and out-of-range settings stay safe");
    check(bounded.musicEnabled && bounded.musicVolume==.35f,"Old settings files retain new music defaults");
    {std::ofstream musicSettings(path);musicSettings<<"musicVolume 12\nmusicEnabled 0\n";}
    bounded.load(path.string());
    check(bounded.musicVolume==1 && !bounded.musicEnabled,"Music volume is bounded and toggle works independently");
    std::filesystem::remove(path);
    std::cout << "Movement/settings tests passed: movement, stamina, recovery, smoothing, reset, settings roundtrip and malformed input.\n";
}
