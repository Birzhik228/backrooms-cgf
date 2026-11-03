#include "../common/fun_audio.h"
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
bool same(const std::vector<FunAudioCue>& a,const std::vector<FunAudioCue>& b) {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)
        if(a[i].kind!=b[i].kind||a[i].position.x!=b[i].position.x||a[i].position.z!=b[i].position.z||
           a[i].position.y!=b[i].position.y||a[i].transmission!=b[i].transmission||a[i].actor!=b[i].actor)return false;
    return true;
}
void validateProp(const World& world,const FunAudioCue& cue) {
    const auto gx=std::int64_t(std::floor(cue.position.x/CELL)),gz=std::int64_t(std::floor(cue.position.z/CELL));
    const auto h=hashCell(gx,gz,world.seed()^0x46554e);
    check(!(exitCell(world.seed(),Level::Fun)==ChunkCoord{gx,gz}),"exit cannot invent party props");
    check(world.chunks().count(chunkAt(cue.position.x,cue.position.z)),"cue belongs to resident geometry");
    check(cue.transmission>=.12f&&cue.transmission<=1,"geometry attenuates sources");
    if(cue.kind==SoundKind::BalloonRub) {
        check((h>>16)%3!=0||(gx==0&&gz==0),"rub comes from an existing balloon cluster");
        check(std::abs(cue.position.x-(gx*CELL+6.1))<.0001&&std::abs(cue.position.z-(gz*CELL+1.45))<.0001,"balloon anchor matches mesh");
    } else {
        check(cue.kind==SoundKind::PartyMelody,"only the two decorative sound types");
        check((h>>36)%7==0&&(h%2==0||(gx==0&&gz==0)),"melody requires the actual music box");
        const double jitter=double((h>>8)%201)*.001-.1;
        check(std::abs(cue.position.x-(gx*CELL+1.85+jitter))<.0001&&std::abs(cue.position.z-(gz*CELL+1.22))<.0001,"music-box anchor matches mesh");
    }
}
void ambience() {
    std::set<SoundKind> kinds;
    for(const auto seed:{1ULL,42ULL,12071998ULL}) {
        World world(seed,Level::Fun);world.update(3.75,3.75);
        Partygoers party;FunAudioContext context;context.listener={3.75,1.65,3.75};
        FunAudioDirector a(seed),b(seed);
        int count=0,last=-10000;
        for(int frame=0;frame<3600;++frame) {
            auto cues=a.update(world,party,context,.05f);
            check(same(cues,b.update(world,party,context,.05f)),"seeded sequence repeats exactly");
            check(cues.size()<=1,"decoration cannot burst multiple sounds");
            for(const auto& cue:cues) {
                check(frame-last>=23*20-1,"decorative events remain at least 23 seconds apart");last=frame;
                validateProp(world,cue);kinds.insert(cue.kind);++count;
            }
        }
        check(count>=2&&count<=8,"ambient schedule is audible but sparse: seed="+std::to_string(seed)+" count="+std::to_string(count));
        check(a.candidateChecks()<49*40,"source scans are bounded and infrequent");
        a.reset(seed);FunAudioDirector fresh(seed);
        for(int frame=0;frame<400;++frame)check(same(a.update(world,party,context,.05f),fresh.update(world,party,context,.05f)),"reset restores the original schedule");
        context.paused=true;
        for(int i=0;i<300;++i)check(a.update(world,party,context,.1f).empty(),"pause suppresses all cues");
        context.paused=false;
        for(int frame=0;frame<600;++frame)check(same(a.update(world,party,context,.05f),fresh.update(world,party,context,.05f)),"pause does not advance ambient clocks");
        context.alive=false;
        for(int i=0;i<600;++i)check(a.update(world,party,context,.1f).empty(),"death suppresses all cues");
        context.alive=true;party.alarmRemaining=6;
        for(int i=0;i<600;++i)check(a.update(world,party,context,.1f).empty()&&a.quiet(),"mouse alarm suppresses decorative noise");
        party.alarmRemaining=0;party.actors[0].active=true;party.actors[0].x=3.75;party.actors[0].z=6.0;party.actors[0].observed=true;
        for(int i=0;i<600;++i)check(a.update(world,party,context,.1f).empty()&&a.quiet(),"close frozen Partygoer keeps the scene quiet");
    }
    check(kinds.size()==2,"both balloon friction and the music box can be heard");
}
void footsteps() {
    World world(12071998,Level::Fun);world.update(3.75,3.75);
    Partygoers party;auto& actor=party.actors[0];actor.active=true;actor.x=3.75;actor.z=3.75;
    FunAudioContext context;context.listener={3.75,1.65,1.0};FunAudioDirector director(42);
    check(director.update(world,party,context,.05f).empty(),"spawn does not make a footstep");
    int steps=0;
    for(int i=0;i<48;++i) {
        actor.z+=.05;
        check(!world.blocked(actor.x,actor.z,.12),"test actor occupies real walkable space");
        for(const auto& cue:director.update(world,party,context,.05f)) {
            check(cue.kind==SoundKind::PartyFootstep&&cue.actor==0,"moving actor owns its step");
            check(cue.position.x==actor.x&&cue.position.z==actor.z,"footstep position follows the actual actor");++steps;
        }
        check(director.movingMask()==1,"moving mask identifies the actor");
    }
    check(steps==1,"cadence follows distance instead of frame count");
    for(int i=0;i<100;++i)check(director.update(world,party,context,.05f).empty()&&director.movingMask()==0,"blocked pursuit and turning in place stay silent");
    actor.observed=true;
    for(int i=0;i<100;++i){actor.z-=.01;check(director.update(world,party,context,.05f).empty()&&director.movingMask()==0,"observed actors cannot produce footfalls");}
    actor.observed=false;actor.x+=40;
    check(director.update(world,party,context,.05f).empty()&&director.movingMask()==0,"relocation is not converted into footsteps");
    actor.active=false;
    check(director.update(world,party,context,.05f).empty()&&director.movingMask()==0,"inactive actors stay silent");
    for(const auto level:{Level::Zero,Level::Parking}) {
        World other(42,level);other.update(3.75,3.75);
        for(int i=0;i<500;++i)check(director.update(other,party,context,.1f).empty(),"director is isolated to Fun");
    }
    for(const float dt:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        check(director.update(world,party,context,dt).empty(),"invalid time is rejected");
    context.listener.x=std::numeric_limits<double>::infinity();
    check(director.update(world,party,context,.05f).empty(),"invalid listener is rejected");
}
void samplesAndMixer() {
    for(const auto kind:{SoundKind::PartyFootstep,SoundKind::BalloonRub,SoundKind::PartyMelody}) {
        std::set<std::vector<std::int16_t>> variants;
        for(unsigned variant=1;variant<=4;++variant) {
            const auto clip=synthesizeSound({kind,Gait::Walk,Surface::Carpet,variant});
            check(!clip.empty()&&clip.size()<3*AUDIO_SAMPLE_RATE,"new samples are short and bounded");
            check(clip.front()==0&&clip.back()==0,"new clips have click-free endpoints");
            check(energy(clip)>100&&energy(clip)<3500,"new sound has audible soft energy");
            int peak=0;for(auto sample:clip)peak=std::max(peak,std::abs(int(sample)));
            check(peak<18000,"new samples retain ample mix headroom");
            check(clip==synthesizeSound({kind,Gait::Walk,Surface::Carpet,variant}),"procedural sound is deterministic");
            check(clip!=synthesizeSound({SoundKind::ThreatStep,Gait::Walk,Surface::Carpet,variant}),"Partygoer and props do not reuse Aranea audio");
            variants.insert(clip);
        }
        check(variants.size()==4,"cached four-way variation avoids identical contacts");
    }
    AudioScene scene;scene.funAtmosphere=true;scene.lightPower=0;scene.partyMovingMask=3;
    AudioMixer mixer;mixer.setScene(scene);mixer.reset(42);
    mixer.playPartyStep(0,{2,.2,0});mixer.playPartyStep(1,{-2,.2,0});
    mixer.play(SoundKind::PartyMelody,{0,1,-4});
    check(mixer.activeVoices()==3,"actor footfalls and decoration coexist");
    std::vector<std::int16_t> block(1024*2);mixer.render(block.data(),1024);
    check(energy(block)>0,"new cached voices reach the stereo mixer");
    scene.partyMovingMask=2;mixer.setScene(scene);mixer.render(block.data(),1024);
    check(mixer.activeVoices()==2,"gaze freeze releases only that actor's tail");
    scene.partyMovingMask=0;mixer.setScene(scene);mixer.render(block.data(),1024);
    check(mixer.activeVoices()==1,"other actor can stop without cancelling music");
    mixer.playPartyStep(-1,{});mixer.playPartyStep(4,{});
    check(mixer.activeVoices()==1,"invalid actor tags cannot enqueue voices");
    mixer.setPaused(true);mixer.render(block.data(),1024);
    check(mixer.activeVoices()==0&&energy(block)==0,"pause clears every one-shot and outputs silence");
    mixer.setPaused(false);mixer.reset(42);
    for(int i=0;i<400;++i)mixer.render(block.data(),1024);
    check(mixer.activeVoices()==0&&energy(block)==0,"Fun disables legacy random drip/creak scheduling");
}
}
int main(){try{ambience();footsteps();samplesAndMixer();std::cout<<"Fun audio tests passed: physical props, sparse timing, real movement, gaze release, pause/reset/isolation and soft PCM.\n";}
catch(const std::exception& error){std::cerr<<"Fun audio test failed: "<<error.what()<<'\n';return 1;}}
