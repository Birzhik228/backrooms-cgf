#include "../common/parking_audio.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>

using namespace br;
namespace {
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
double energy(const std::vector<std::int16_t>& pcm){double sum=0;for(auto n:pcm)sum+=double(n)*n;return std::sqrt(sum/pcm.size());}
bool same(const std::vector<ParkingAudioCue>& a,const std::vector<ParkingAudioCue>& b) {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)if(a[i].kind!=b[i].kind||a[i].position.x!=b[i].position.x||
        a[i].position.y!=b[i].position.y||a[i].position.z!=b[i].position.z||a[i].circuit!=b[i].circuit||a[i].transmission!=b[i].transmission)return false;
    return true;
}
void powerAll(World& world) {
    for(int i=0;i<3;++i){const auto panel=world.breakerLocation(i);world.update(panel.x,panel.z,1);check(world.activateBreaker(i,panel.x,panel.z+1),"power fixture activates real cabinet");}
    world.update(3.75,3.75,1);
}
void validateProp(const World& world,const ParkingAudioCue& cue) {
    const auto gx=std::int64_t(std::floor(cue.position.x/CELL)),gz=std::int64_t(std::floor(cue.position.z/CELL));
    const auto h=hashCell(gx,gz,world.seed()^0x5041524b494e47ULL);
    check(cue.circuit<0&&world.chunks().count(chunkAt(cue.position.x,cue.position.z)),"ambience belongs to resident geometry");
    check(cue.transmission>=.12f&&cue.transmission<=1,"source attenuation bounded");
    if(cue.kind==SoundKind::PipeTick) {
        check(std::abs(cue.position.x-(gx*CELL+.75))<.0001&&std::abs(cue.position.z-(gz*CELL+1))<.0001,"tick comes from existing service pipe");
        check(cue.position.y==2.75,"pipe tick has physical height");
    } else {
        check(!(ChunkCoord{gx,gz}==parkingExitCell(world.seed())),"exit has no fake decorative source");
        for(int i=0;i<3;++i)check(!(ChunkCoord{gx,gz}==parkingBreakerCell(world.seed(),i)),"cabinet has no fake decorative source");
        if(cue.kind==SoundKind::WaterDrip) {
            check((h>>40)%11==0&&h%11!=0&&(gx!=0||gz!=0),"drip requires the real seeded leak patch");
            check(std::abs(cue.position.x-(gx*CELL+.85))<.0001&&std::abs(cue.position.z-(gz*CELL+4.05))<.0001&&cue.position.y==2.70,"drip anchor matches patch/pipe");
        } else {
            check(cue.kind==SoundKind::ParkingVent&&(h>>32)%7==0,"fan requires the real grille");
            check(world.breakerPowered(parkingSector(gx,gz)),"only powered sectors have ventilation");
            check(std::abs(cue.position.x-(gx*CELL+5.85))<.0001&&std::abs(cue.position.z-(gz*CELL+3))<.0001&&cue.position.y==3.175,"fan anchor matches ceiling grille");
        }
    }
}
void ambience() {
    std::set<SoundKind> kinds;
    for(const auto seed:{0ULL,42ULL,12071998ULL})for(const bool powered:{false,true}) {
        World world(seed,Level::Parking);world.update(3.75,3.75,1);
        if(powered)powerAll(world);
        Darkness darkness;ParkingAudioContext context;context.listener={3.75,1.65,3.75};
        ParkingAudioDirector a(seed),b(seed);int count=0,last=-10000;
        for(int frame=0;frame<3600;++frame) {
            auto cues=a.update(world,darkness,context,.05f);
            check(same(cues,b.update(world,darkness,context,.05f)),"seeded schedule repeats exactly");
            check(cues.size()<=1,"ambient events do not burst");
            for(const auto& cue:cues){
                check(frame-last>=17*20-1,"at least 17 seconds between ambience");last=frame;
                validateProp(world,cue);kinds.insert(cue.kind);++count;
                if(!powered)check(cue.kind!=SoundKind::ParkingVent,"unpowered fan is silent");
            }
        }
        check(count>=3&&count<=11,"ambient schedule is audible but sparse");
        check(a.candidateChecks()<49*50,"rare bounded scans");
        context.paused=true;
        for(int i=0;i<300;++i)check(a.update(world,darkness,context,.1f).empty()&&a.quiet(),"pause suppresses cues");
        context.paused=false;
        for(int i=0;i<500;++i)check(same(a.update(world,darkness,context,.05f),b.update(world,darkness,context,.05f)),"pause preserves schedule time");
        a.reset(seed);b.reset(seed);
        for(int i=0;i<400;++i)check(same(a.update(world,darkness,context,.05f),b.update(world,darkness,context,.05f)),"reset schedule is deterministic");
        darkness.eyes[0].active=true;darkness.eyes[0].x=3.75;darkness.eyes[0].z=9;
        for(int i=0;i<600;++i)check(a.update(world,darkness,context,.1f).empty()&&a.quiet(),"real nearby eyes suppress ambience");
        darkness.eyes[0].active=false;
        for(int i=0;i<49;++i)check(a.update(world,darkness,context,.1f).empty(),"five second quiet recovery");
        context.alive=false;
        for(int i=0;i<300;++i)check(a.update(world,darkness,context,.1f).empty(),"death suppresses cues");
    }
    check(kinds.size()==3,"pipes, leaks and powered vents are represented");
}
void powerAndLifecycle() {
    World world(42,Level::Parking);world.update(3.75,3.75,1);
    Darkness darkness;ParkingAudioContext context;context.listener={3.75,1.65,3.75};ParkingAudioDirector director(42);
    check(director.update(world,darkness,context,.05f).empty(),"initial circuit snapshot is silent");
    for(const int circuit:{2,0,1}) {
        const auto panel=world.breakerLocation(circuit);world.update(panel.x,panel.z,1);context.listener={panel.x,1.65,panel.z+1};
        check(world.activateBreaker(circuit,panel.x,panel.z+1),"activate actual circuit");
        auto cues=director.update(world,darkness,context,.05f);
        check(cues.size()==1&&cues[0].kind==SoundKind::PowerRelay&&cues[0].circuit==circuit,"one cue per true circuit edge");
        check(cues[0].position.x==panel.x&&cues[0].position.z==panel.z,"relay originates at the cabinet");
        check(director.update(world,darkness,context,.05f).empty(),"restored circuit cannot repeat the relay");
        director.reset(42);
        check(director.update(world,darkness,context,.05f).empty(),"R-style retained power snapshot does not replay startup");
    }
    context.paused=true;director.reset(42);
    check(director.update(world,darkness,context,.1f).empty(),"paused restored snapshot stays silent");
    context.paused=false;check(director.update(world,darkness,context,.05f).empty(),"resume cannot replay a snapshot");
    for(const auto level:{Level::Zero,Level::Fun}) {
        World other(42,level);other.update(3.75,3.75,0);
        for(int i=0;i<300;++i)check(director.update(other,darkness,context,.1f).empty(),"director isolated from earlier stages");
    }
    check(director.update(world,darkness,context,.05f).empty(),"level reentry snapshots power silently");
    for(const float dt:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        check(director.update(world,darkness,context,dt).empty(),"invalid time rejected");
    context.listener.x=std::numeric_limits<double>::quiet_NaN();
    check(director.update(world,darkness,context,.05f).empty(),"invalid listener rejected");
}
void samplesAndMixer() {
    for(const auto kind:{SoundKind::PipeTick,SoundKind::ParkingVent,SoundKind::PowerRelay}) {
        std::set<std::vector<std::int16_t>> variants;
        for(unsigned variant=1;variant<=4;++variant) {
            const auto clip=synthesizeSound({kind,Gait::Walk,Surface::HardFloor,variant});
            check(!clip.empty()&&clip.size()<3*AUDIO_SAMPLE_RATE,"short bounded procedural cue");
            check(clip.front()==0&&clip.back()==0,"click-free endpoints");
            check(energy(clip)>100&&energy(clip)<3500,"restrained but audible PCM energy");
            int peak=0;for(auto sample:clip)peak=std::max(peak,std::abs(int(sample)));
            check(peak<18000,"mix headroom retained");
            check(clip==synthesizeSound({kind,Gait::Walk,Surface::HardFloor,variant}),"deterministic sample generation");
            variants.insert(clip);
        }
        check(variants.size()==4,"four cached variants");
    }
    AudioScene scene;scene.parkingAtmosphere=true;scene.lightPower=0;scene.atmosphereQuiet=true;
    AudioMixer mixer;mixer.setScene(scene);mixer.reset(42);
    std::vector<std::int16_t> block(6000*2);
    for(const auto kind:{SoundKind::PipeTick,SoundKind::ParkingVent,SoundKind::WaterDrip}) {
        mixer.reset(42);mixer.play(kind,{1,1,0});mixer.render(block.data(),6000);
        check(energy(block)==0,"decorative tails duck during danger");
    }
    mixer.reset(42);mixer.play(SoundKind::PowerRelay,{1,1,0});mixer.render(block.data(),6000);
    check(energy(block)>100,"power feedback remains audible during danger");
    mixer.reset(42);mixer.play(SoundKind::AmbientCreak,{1,1,0});mixer.render(block.data(),6000);
    check(energy(block)>0,"actual existing eyes arrival cue is not ducked");
    mixer.setPaused(true);mixer.render(block.data(),6000);
    check(energy(block)==0&&mixer.activeVoices()==0,"pause silences and clears voices");
    mixer.setPaused(false);mixer.reset(42);
    for(int i=0;i<80;++i)mixer.render(block.data(),6000);
    check(energy(block)==0&&mixer.activeVoices()==0,"Parking disables legacy random timer");
    scene.atmosphereQuiet=false;mixer.setScene(scene);mixer.reset(42);
    for(int i=0;i<20;++i)mixer.play(SoundKind::ParkingVent,{0,2,2});
    mixer.play(SoundKind::PowerRelay,{0,1,1});mixer.play(SoundKind::DarknessJumpscare,{});
    check(mixer.activeVoices()==1,"capture remains exclusive despite full industrial queue");
}
}
int main(){try{ambience();powerAndLifecycle();samplesAndMixer();std::cout<<"Parking audio tests passed: physical pipes/leaks/vents, power edges, quiet danger, pause/reset/isolation and PCM/mixer.\n";}
catch(const std::exception& error){std::cerr<<"Parking audio test failed: "<<error.what()<<'\n';return 1;}}
