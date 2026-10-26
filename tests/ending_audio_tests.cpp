#include "../common/audio.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace br;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
double rms(const std::vector<std::int16_t>& data){double n=0;for(auto v:data)n+=double(v)*v;return std::sqrt(n/data.size());}
std::vector<std::int16_t> block(AudioMixer& mixer,std::size_t frames){std::vector<std::int16_t> b(frames*2);mixer.render(b.data(),frames);return b;}
void clips() {
    const std::array<SoundKind,5> kinds={SoundKind::ElevatorDoor,SoundKind::ElevatorMotor,SoundKind::ElevatorBell,SoundKind::SurfaceAmbience,SoundKind::CityAmbience};
    const std::array<double,5> durations={2.8,7,1.4,11,20};
    std::vector<std::vector<std::int16_t>> samples;
    for(std::size_t i=0;i<kinds.size();++i) {
        auto pcm=synthesizeSound({kinds[i],Gait::Walk,Surface::HardFloor,42});
        check(std::abs(double(pcm.size())/AUDIO_SAMPLE_RATE-durations[i])<.001,"ending cue length matches timeline");
        check(pcm.front()==0&&pcm.back()==0,"cue edges fade to zero");
        check(rms(pcm)>80&&rms(pcm)<8000,"cue has audible bounded energy");
        check(*std::max_element(pcm.begin(),pcm.end())<31000&&*std::min_element(pcm.begin(),pcm.end())>-31000,"cue preserves headroom");
        check(pcm==synthesizeSound({kinds[i],Gait::Walk,Surface::HardFloor,42}),"synthesis is deterministic");
        samples.push_back(std::move(pcm));
    }
    for(std::size_t a=0;a<samples.size();++a)for(std::size_t b=a+1;b<samples.size();++b)
        check(!std::equal(samples[a].begin(),samples[a].begin()+1000,samples[b].begin()),"cue openings are distinct");
}
void isolationAndLifecycle() {
    AudioScene scene;scene.cinematic=true;scene.lightPower=1;scene.tension=1;
    AudioMixer mixer;mixer.setScene(scene);mixer.reset(42);
    for(int i=0;i<32;++i)check(rms(block(mixer,AUDIO_SAMPLE_RATE))==0,"cinematic mode suppresses hum, danger and autonomous ambience");
    check(mixer.activeVoices()==0,"no hidden world voices accumulate during finale");
    for(auto kind:{SoundKind::ThreatRoar,SoundKind::DarknessJumpscare,SoundKind::WaterDrip,SoundKind::VentRattle})mixer.play(kind,{});
    check(mixer.activeVoices()==0,"late world-only events cannot interrupt cinematic ownership");
    mixer.play(SoundKind::ElevatorMotor,{});check(rms(block(mixer,6000))>100,"motor plays in isolated scene");
    auto continuation=block(mixer,4096);
    mixer.reset(42);mixer.play(SoundKind::ElevatorMotor,{});block(mixer,6000);
    mixer.setPaused(true);check(rms(block(mixer,AUDIO_SAMPLE_RATE))==0,"paused finale is silent");
    check(mixer.activeVoices()==1,"pause retains cinematic cue cursor");
    mixer.setPaused(false);check(block(mixer,4096)==continuation,"resume continues same sample and envelope");
    mixer.reset(42);check(mixer.activeVoices()==0&&rms(block(mixer,8192))==0,"reset clears cue tails");
    for(auto kind:{SoundKind::Door,SoundKind::ElectricalStinger,SoundKind::ElevatorDoor,SoundKind::ElevatorBell,SoundKind::SurfaceAmbience,SoundKind::CityAmbience}) {
        mixer.reset(42);mixer.play(kind,{});check(rms(block(mixer,6000))>0,"explicit scripted cues remain audible");
    }
    mixer.reset(42);mixer.play(SoundKind::CityAmbience,{});block(mixer,6*AUDIO_SAMPLE_RATE);
    const auto cityContinuation=block(mixer,4096);
    mixer.reset(42);mixer.play(SoundKind::CityAmbience,{});block(mixer,6*AUDIO_SAMPLE_RATE);
    mixer.setPaused(true);check(rms(block(mixer,AUDIO_SAMPLE_RATE))==0,"city pause is silent");
    mixer.setPaused(false);check(block(mixer,4096)==cityContinuation,"city traffic resumes without restarting");
    check(mixer.activeVoices()==1,"city bed occupies one bounded cached voice");
    mixer.reset(42);mixer.enqueue({SoundKind::Footstep,Gait::Walk,Surface::HardFloor,1});
    check(rms(block(mixer,6000))>0,"scripted walking still uses footstep mixer");
    mixer.setVolume(0);mixer.reset(42);mixer.play(SoundKind::ElevatorDoor,{});
    check(rms(block(mixer,3*AUDIO_SAMPLE_RATE))==0&&mixer.activeVoices()==0,"master mute suppresses and consumes cues without late replay");
    mixer.setVolume(1);mixer.reset(42);mixer.play(SoundKind::SurfaceAmbience,{});
    scene.cinematic=false;scene.level0Atmosphere=true;scene.tension=0;mixer.setScene(scene);
    check(mixer.activeVoices()==0,"leaving finale clears all cinematic voices");
    check(rms(block(mixer,6000))>0,"ordinary fluorescent hum returns after leaving finale");
    mixer.play(SoundKind::AmbientCreak,{});mixer.setPaused(true);
    check(mixer.activeVoices()==0,"legacy world pause behavior remains unchanged");
    mixer.setPaused(false);scene.cinematic=true;mixer.setScene(scene);mixer.reset(42);
    for(int i=0;i<40;++i)mixer.play(SoundKind::ElevatorDoor,{});
    check(mixer.activeVoices()==12,"scripted sound uses existing bounded voice pool");
}
void little(std::ostream& f,std::uint32_t v,int bytes){for(int i=0;i<bytes;++i)f.put(char(v>>(8*i)));}
void preview(const std::string& path) {
    AudioScene scene;scene.cinematic=true;AudioMixer mixer;mixer.setScene(scene);mixer.setVolume(.75f);mixer.reset(42);
    std::vector<std::int16_t> pcm(42*AUDIO_SAMPLE_RATE*2);
    for(int frame=0;frame<42*60;++frame) {
        if(frame==0||frame==600)mixer.play(SoundKind::ElevatorDoor,{});
        if(frame==180)mixer.play(SoundKind::ElevatorMotor,{});
        if(frame==600)mixer.play(SoundKind::ElevatorBell,{});
        if(frame==780)mixer.play(SoundKind::SurfaceAmbience,{});
        if(frame==1200)mixer.play(SoundKind::Door,{});
        if(frame==1380){mixer.reset(42);mixer.play(SoundKind::CityAmbience,{},.75f);}
        if(frame>=39*60)mixer.setVolume(.75f*float(42*60-frame)/(3*60));
        if(((frame>=780&&frame<1200)||(frame>=23*60&&frame<30*60))&&(frame-780)%34==0)mixer.enqueue({SoundKind::Footstep,Gait::Walk,Surface::HardFloor,std::uint32_t(frame)});
        const std::size_t start=std::size_t(frame)*AUDIO_SAMPLE_RATE/60,end=std::size_t(frame+1)*AUDIO_SAMPLE_RATE/60;
        mixer.render(pcm.data()+2*start,end-start);
    }
    std::ofstream f(path,std::ios::binary);check(bool(f),"preview file opens");
    f.write("RIFF",4);little(f,36+std::uint32_t(pcm.size()*2),4);f.write("WAVEfmt ",8);little(f,16,4);little(f,1,2);little(f,2,2);
    little(f,AUDIO_SAMPLE_RATE,4);little(f,AUDIO_SAMPLE_RATE*4,4);little(f,4,2);little(f,16,2);f.write("data",4);little(f,std::uint32_t(pcm.size()*2),4);
    for(auto value:pcm)little(f,std::uint16_t(value),2);
    check(bool(f),"preview is written");
}
}
int main(int argc,char** argv){try{clips();isolationAndLifecycle();if(argc==3&&std::string(argv[1])=="--preview")preview(argv[2]);std::cout<<"Ending audio passed: five cached distinct clips including city traffic, cinematic isolation, sample-exact pause/resume, volume, reset, mode cleanup and voice bound.\n";}catch(const std::exception& e){std::cerr<<"Ending audio failed: "<<e.what()<<'\n';return 1;}}
