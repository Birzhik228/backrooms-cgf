#include "../common/audio.h"
#include "../common/gameplay.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition,const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
double energy(const std::vector<std::int16_t>& sound) {
    double total=0;
    for (auto sample:sound) total+=double(sample)*sample;
    return std::sqrt(total/sound.size());
}
void recordedBank() {
    check(br::loadFootstepSamples(),"load all six shipped real footfall recordings");
    check(br::recordedFootstepCount()==6,"complete bank is cached before playback");
    std::set<std::vector<std::int16_t>> distinct;
    for (int index=1;index<=6;++index) {
        const auto original=br::readFootstepWav("assets/audio/footsteps/step-0"+std::to_string(index)+".wav");
        check(original.size()>br::AUDIO_SAMPLE_RATE/8 && original.size()<br::AUDIO_SAMPLE_RATE/2,
              "recorded shoe contact has a short natural duration");
        check(original.front()==0 && original.back()==0,"recordings have click-free ends");
        check(energy(original)>1000,"recordings contain audible real contact rather than silence");
        int peak=0;
        for (auto sample:original) peak=std::max(peak,std::abs(int(sample)));
        check(peak>20000 && peak<30000,"source bank has normalized unclipped headroom");
        const auto played=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,br::Surface::Carpet,std::uint32_t(index)});
        distinct.insert(played);
    }
    check(distinct.size()==6,"six successive footfalls use six distinct recordings");
    // An incomplete bank is never mixed with stale clips from a previous load.
    check(!br::loadFootstepSamples("missing-footstep-bank") && br::recordedFootstepCount()==0,
          "missing bank explicitly reports fallback without a partial recording set");
    check(!br::synthesizeSound({}).empty(),"missing-bank fallback keeps gameplay safe");
    check(br::loadFootstepSamples(),"recorded bank can be restored after a failed load");
}
void wavReader() {
    const std::string source="assets/audio/footsteps/step-03.wav",temporary="audio-test-malformed.wav";
    std::ifstream stream(source,std::ios::binary);
    const std::vector<unsigned char> valid((std::istreambuf_iterator<char>(stream)),{});
    check(valid.size()>44,"test source WAV exists");
    auto write32=[](std::vector<unsigned char>& bytes,std::size_t offset,std::uint32_t value) {
        for (int i=0;i<4;++i) bytes[offset+i]=static_cast<unsigned char>(value>>(i*8));
    };
    auto read=[&](const std::vector<unsigned char>& bytes) {
        std::ofstream output(temporary,std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
        output.close();
        return br::readFootstepWav(temporary);
    };
    auto bytes=valid;
    bytes.insert(bytes.begin()+12,{'J','U','N','K',1,0,0,0,42,0});
    write32(bytes,4,static_cast<std::uint32_t>(bytes.size()-8));
    check(read(bytes)==br::readFootstepWav(source),"RIFF reader skips an odd metadata chunk and its pad byte");
    bytes=valid; bytes.pop_back();
    check(read(bytes).empty(),"reject truncated RIFF file");
    bytes=valid; bytes[0]='X';
    check(read(bytes).empty(),"reject a non-RIFF container");
    bytes=valid; write32(bytes,24,44100);
    check(read(bytes).empty(),"reject unsupported sample rate instead of playing at wrong speed");
    bytes=valid; bytes[22]=2;
    check(read(bytes).empty(),"reject stereo rather than reading channels as consecutive frames");
    bytes=valid; bytes[20]=3;
    check(read(bytes).empty(),"reject float WAV instead of interpreting it as integer PCM");
    bytes=valid; write32(bytes,16,10);
    check(read(bytes).empty(),"reject short format chunk before reading its fields");
    bytes=valid; write32(bytes,40,0xffffffffu);
    check(read(bytes).empty(),"reject huge claimed data chunks before allocating samples");
    bytes=valid; write32(bytes,40,static_cast<std::uint32_t>(bytes.size()-45));
    check(read(bytes).empty(),"reject an incomplete 16-bit sample");
    bytes=valid; bytes.resize(3*1024*1024);
    write32(bytes,4,static_cast<std::uint32_t>(bytes.size()-8));
    check(read(bytes).empty(),"bound total file size before allocation");
    std::remove(temporary.c_str());
}
void samples() {
    for (auto surface:{br::Surface::Carpet,br::Surface::HardFloor,br::Surface::DampCarpet}) {
        const auto walk=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,surface,17});
        const auto crouch=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Crouch,surface,17});
        const auto sprint=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Sprint,surface,17});
        check(energy(crouch)<energy(walk)*.60,"crouching steps are appreciably quieter");
        check(energy(sprint)>energy(walk)*1.20,"sprinting steps are louder");
        check(walk==br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,surface,17}),"repeatable cached recorded PCM");
        check(walk!=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,surface,18}),"natural per-step variation");
        for (const auto& clip:{walk,crouch,sprint}) {
            check(!clip.empty() && std::isfinite(energy(clip)),"finite nonempty recorded signal");
            check(clip.front()==0 && clip.back()==0,"sample endpoints prevent splice clicks");
            check(energy(clip)>100,"audible nonempty step signal");
            for (auto value:clip) check(std::abs(int(value))<16000,"comfortable peak headroom");
        }
    }
    const auto carpet=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,br::Surface::Carpet,2});
    const auto tile=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,br::Surface::HardFloor,2});
    const auto damp=br::synthesizeSound({br::SoundKind::Footstep,br::Gait::Walk,br::Surface::DampCarpet,2});
    check(carpet!=tile,"hard floors and carpet have distinct timbres");
    check(damp!=carpet && damp!=tile,"damp carpet adds a muffled wet sole timbre");
    const std::vector<std::int16_t> drag(carpet.begin()+br::AUDIO_SAMPLE_RATE/10,
                                        carpet.begin()+br::AUDIO_SAMPLE_RATE/4);
    check(energy(drag)>25,"heavy carpet step includes a quiet sole-drag tail");
    const std::vector<std::int16_t> onset(carpet.begin(),carpet.begin()+br::AUDIO_SAMPLE_RATE/10);
    const std::vector<std::int16_t> release(carpet.end()-br::AUDIO_SAMPLE_RATE/50,carpet.end());
    check(energy(release)<energy(onset)*.08,"heavy step decays without an abrupt cutoff");
    const auto cloth=br::synthesizeSound({br::SoundKind::ClothRustle,br::Gait::Crouch,br::Surface::Carpet,2});
    check(energy(cloth)>10 && energy(cloth)<energy(carpet),"cloth rustle is quiet but present");
}
void positionalAudio() {
    br::AudioScene scene;
    scene.listener={0,1.65,0};
    const auto left=br::positionalGains(scene,{-4,1.65,0});
    const auto right=br::positionalGains(scene,{4,1.65,0});
    check(left.left>left.right*10 && right.right>right.left*10,"world sources pan toward their camera-relative side");
    const auto front=br::positionalGains(scene,{0,1.65,-4});
    check(std::abs(front.left-front.right)<1e-6,"front source is centered");
    const auto distant=br::positionalGains(scene,{0,1.65,-24});
    const auto elevated=br::positionalGains(scene,{0,12,-4});
    check(distant.left<front.left*.20 && elevated.left<front.left*.5,"full 3D distance attenuates far and high sources");
    const auto out=br::positionalGains(scene,{0,1.65,-48});
    check(out.left==0 && out.right==0,"sources beyond the range are silent");
    scene.forward={0,0,1};
    const auto turned=br::positionalGains(scene,{-4,1.65,0});
    check(turned.right>turned.left*10,"turning the listener reverses source panning");
    scene.forward={0,0,-1}; scene.listener={1e10,1.65,-1e10};
    const auto translated=br::positionalGains(scene,{1e10-4,1.65,-1e10});
    check(std::abs(translated.left-left.left)<1e-6,"world-space audio remains stable at remote chunk coordinates");
}
std::vector<std::int16_t> render(br::AudioMixer& mixer,std::size_t frames,std::size_t block=512) {
    std::vector<std::int16_t> output(frames*2);
    for (std::size_t offset=0;offset<frames;offset+=block)
        mixer.render(output.data()+offset*2,std::min(block,frames-offset));
    return output;
}
double channelEnergy(const std::vector<std::int16_t>& samples,int channel) {
    double sum=0;
    for (std::size_t i=std::size_t(channel);i<samples.size();i+=2) sum+=double(samples[i])*samples[i];
    return std::sqrt(sum/(samples.size()/2));
}
double relativeHighFrequency(const std::vector<std::int16_t>& sound) {
    double differences=0,total=0;
    for (std::size_t i=1;i<sound.size();++i) {
        const double difference=double(sound[i])-sound[i-1];
        differences+=difference*difference;
        total+=double(sound[i])*sound[i];
    }
    return std::sqrt(differences/total);
}
void threatAudio() {
    const auto step=br::synthesizeSound({br::SoundKind::ThreatStep});
    const auto notice=br::synthesizeSound({br::SoundKind::ThreatNotice});
    const auto scare=br::synthesizeSound({br::SoundKind::Jumpscare});
    check(step.size()>.3*br::AUDIO_SAMPLE_RATE && step.size()<.6*br::AUDIO_SAMPLE_RATE,
          "creature steps are short enough for a chase cadence");
    check(notice.size()>.7*br::AUDIO_SAMPLE_RATE && notice.size()<1.2*br::AUDIO_SAMPLE_RATE,
          "recognition cue is brief rather than looping over footsteps");
    check(scare.size()>.6*br::AUDIO_SAMPLE_RATE && scare.size()<br::AUDIO_SAMPLE_RATE,
          "capture sting finishes before the one-second game-over transition");
    check(relativeHighFrequency(scare)>relativeHighFrequency(step)*1.6,
          "capture chord has a brighter spectrum than the creature's low footfall");
    check(energy(scare)>energy(notice)*1.4 && energy(scare)>energy(step),
          "capture sting is distinct from quiet positional tracking cues");
    for (auto kind:{br::SoundKind::ThreatStep,br::SoundKind::ThreatNotice,br::SoundKind::Jumpscare}) {
        for (std::uint32_t variation=1;variation<=4;++variation) {
            const br::SoundEvent event{kind,br::Gait::Walk,br::Surface::Carpet,variation};
            const auto clip=br::synthesizeSound(event);
            check(clip==br::synthesizeSound(event),"threat clip variants are deterministic and cacheable");
            check(clip.front()==0 && clip.back()==0 && energy(clip)>300,
                  "threat sounds have soft endpoints and audible signal");
            for (auto value:clip) check(std::abs(int(value))<22000,"threat cue leaves substantial source headroom");
            const std::vector<std::int16_t> tail(clip.end()-br::AUDIO_SAMPLE_RATE/50,clip.end());
            check(energy(tail)<energy(clip)*.3,"threat cue fades before playback ends");
        }
    }
    br::AudioScene scene;
    scene.listener={0,1.65,0}; scene.lightPower=0;
    br::AudioMixer mixer;
    mixer.setScene(scene);
    for (auto kind:{br::SoundKind::ThreatStep,br::SoundKind::ThreatNotice}) {
        mixer.reset(543); mixer.play(kind,{-3,1.65,0});
        auto near=render(mixer,br::AUDIO_SAMPLE_RATE);
        check(channelEnergy(near,0)>150 && channelEnergy(near,1)==0,
              "creature footsteps and recognition identify its world-space direction");
        mixer.reset(543); mixer.play(kind,{-25,1.65,0});
        auto far=render(mixer,br::AUDIO_SAMPLE_RATE);
        check(channelEnergy(far,0)<channelEnergy(near,0)*.2,
              "distant threat cues fade rather than staying at full volume");
    }
    mixer.reset(543);
    for (int i=0;i<12;++i) mixer.play(br::SoundKind::Door,scene.listener);
    mixer.play(br::SoundKind::Jumpscare,scene.listener,0);
    check(mixer.activeVoices()==12,"inaudible capture request does not cancel other sounds");
    mixer.play(br::SoundKind::Jumpscare,{1000,1.65,1000});
    check(mixer.activeVoices()==1,"capture replaces overlapping one-shots even when all slots were occupied");
    auto output=render(mixer,br::AUDIO_SAMPLE_RATE);
    check(channelEnergy(output,0)>1000 && channelEnergy(output,0)==channelEnergy(output,1),
          "capture sting is centered and cannot vanish with enemy distance or facing");
    check(mixer.activeVoices()==0,"capture completes as a one-shot without a looping voice");
    mixer.setVolume(0); mixer.reset(543); mixer.play(br::SoundKind::Jumpscare,scene.listener);
    check(energy(render(mixer,br::AUDIO_SAMPLE_RATE))==0,"master mute covers the capture sting");
    mixer.setVolume(1); mixer.reset(543); mixer.play(br::SoundKind::Jumpscare,scene.listener);
    render(mixer,br::AUDIO_SAMPLE_RATE/5); mixer.setPaused(true);
    check(mixer.activeVoices()==0 && energy(render(mixer,512))==0,"game-over menu clears and silences threat cues");
    mixer.play(br::SoundKind::ThreatNotice,scene.listener);
    mixer.setPaused(false);
    check(mixer.activeVoices()==0,"paused threat cues cannot leak into a restart");
}
void stereoMixer() {
    for (auto kind:{br::SoundKind::AmbientCreak,br::SoundKind::WaterDrip,
                   br::SoundKind::ElectricalStinger,br::SoundKind::Door}) {
        const br::SoundEvent event{kind,br::Gait::Walk,br::Surface::Carpet,4};
        const auto sound=br::synthesizeSound(event);
        check(sound==br::synthesizeSound(event),"new synthesized placeholders are deterministic");
        check(sound.size()>br::AUDIO_SAMPLE_RATE/2 && sound.size()<2*br::AUDIO_SAMPLE_RATE,"environment clips stay bounded");
        check(sound.front()==0 && sound.back()==0 && energy(sound)>100,"environment clips have soft endpoints and audible signal");
    }
    br::AudioScene scene;
    scene.listener={0,1.65,0}; scene.lightPower=0;
    br::AudioMixer mixer;
    mixer.setScene(scene); mixer.reset(1234);
    mixer.play(br::SoundKind::Door,{-3,1.65,0});
    auto output=render(mixer,br::AUDIO_SAMPLE_RATE);
    check(channelEnergy(output,0)>100 && channelEnergy(output,1)==0,"stereo PCM matches positional source gain");
    mixer.reset(1234); mixer.enqueue({});
    output=render(mixer,br::AUDIO_SAMPLE_RATE);
    check(channelEnergy(output,0)==channelEnergy(output,1),"listener-local footsteps remain centered");
    mixer.reset(1234); scene.lightPower=1; scene.fluorescent={-2,1.65,0}; mixer.setScene(scene);
    output=render(mixer,br::AUDIO_SAMPLE_RATE);
    check(channelEnergy(output,0)>100 && channelEnergy(output,1)==0,"fluorescent hum follows nearest lamp position");
    mixer.setVolume(0); mixer.reset(1234); mixer.enqueue({});
    output=render(mixer,br::AUDIO_SAMPLE_RATE);
    check(energy(output)==0,"master volume zero mutes all sound layers");
    mixer.setVolume(1); mixer.reset(1234);
    for (int i=0;i<40;++i) mixer.play(br::SoundKind::Door,scene.listener);
    check(mixer.activeVoices()==12,"fixed voice budget bounds overlapping sound work");
    output=render(mixer,br::AUDIO_SAMPLE_RATE);
    for (auto sample:output) check(std::abs(int(sample))<29000,"soft limiting leaves output headroom under overlapping sources");

    scene.lightPower=0; scene.tension=0; mixer.setScene(scene); mixer.reset(99);
    render(mixer,3*br::AUDIO_SAMPLE_RATE);
    scene.tension=.95f; mixer.setScene(scene);
    output=render(mixer,128);
    check(mixer.activeVoices()==1,"rising environmental tension triggers a stinger");
    mixer.setPaused(true);
    check(mixer.activeVoices()==0,"pause clears queued one-shot sounds");
    check(energy(render(mixer,1024))==0,"pause produces silence without advancing its timeline");
    mixer.setPaused(false); render(mixer,512);
    check(mixer.activeVoices()==0,"resume does not replay the paused stinger");
    mixer.reset(99);
    check(mixer.activeVoices()==0,"world reset clears stale voices");

    br::AudioMixer first,second;
    scene.lightPower=.7f; scene.tension=.8f;
    first.setScene(scene); second.setScene(scene); first.reset(879); second.reset(879);
    const auto firstRun=render(first,18*br::AUDIO_SAMPLE_RATE,512);
    const auto secondRun=render(second,18*br::AUDIO_SAMPLE_RATE,317);
    check(firstRun==secondRun,"seeded ambient scheduling is deterministic and independent of render block size");
    second.reset(880);
    check(firstRun!=render(second,18*br::AUDIO_SAMPLE_RATE),"different seeds vary distant ambience");
}
int stepsAt(float speed,bool crouched,bool sprinting) {
    br::FootstepCadence cadence;
    int steps=0;
    for (int frame=0;frame<300;++frame)
        for (const auto& event:cadence.update(1.0f/60,speed/60,crouched,sprinting,br::Surface::Carpet))
            steps+=event.kind==br::SoundKind::Footstep;
    return steps;
}
void movement() {
    br::FootstepCadence cadence;
    for (int frame=0;frame<180;++frame)
        check(cadence.update(1.0f/60,0,false,true,br::Surface::HardFloor).empty(),"blocked or still movement is silent");
    auto events=cadence.update(1.0f/60,0,true,false,br::Surface::Carpet);
    check(events.size()==1 && events.front().kind==br::SoundKind::ClothRustle,"entering crouch rustles once");
    check(cadence.update(1.0f/60,0,true,false,br::Surface::Carpet).empty(),"holding crouch does not repeat rustle");
    events=cadence.update(1.0f/60,0,false,false,br::Surface::Carpet);
    check(events.size()==1 && events.front().kind==br::SoundKind::ClothRustle,"standing up rustles once");
    check(cadence.update(1.0f/60,5,true,true,br::Surface::Carpet,true).empty(),"paused state suppresses every motion sound");
    check(cadence.update(1.0f/60,100,true,false,br::Surface::Carpet).empty(),"teleports cannot queue a step burst");
    check(stepsAt(0,false,false)==0,"no distance means no steps");
    const int walk=stepsAt(br::WALK_SPEED,false,false),crouch=stepsAt(br::CROUCH_SPEED,true,false),
              sprint=stepsAt(br::SPRINT_SPEED,false,true);
    check(crouch>0 && crouch<walk && walk<sprint,"actual gait speed produces slower crouch and quicker sprint cadence");
    check(crouch>=6 && crouch<=7,"crouching has an unhurried quiet cadence");
    check(walk>=8 && walk<=9,"walking stays near 1.7 deliberate footsteps per second");
    check(sprint>=11 && sprint<=12,"sprinting has a faster but measured cadence");
    check(stepsAt(br::CROUCH_SPEED,true,true)==crouch,"crouching overrides the sprint key");
    br::FootstepCadence slow,fast;
    int slowCount=0,fastCount=0;
    for (int frame=0;frame<150;++frame) slowCount+=int(slow.update(1.0f/30,br::WALK_SPEED/30,false,false,br::Surface::Carpet).size());
    for (int frame=0;frame<600;++frame) fastCount+=int(fast.update(1.0f/120,br::WALK_SPEED/120,false,false,br::Surface::Carpet).size());
    check(slowCount==fastCount,"footstep cadence is independent of frame rate");
}
void offlinePreview() {
    const std::string path="audio-test-preview.wav";
    check(br::writeAudioPreview(path),"write offline preview WAV");
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    check(file.tellg()==44+12*br::AUDIO_SAMPLE_RATE*4,"correct 12-second stereo PCM WAV size");
    file.seekg(0);
    char header[44]{}; file.read(header,44);
    check(std::string(header,4)=="RIFF" && std::string(header+8,4)=="WAVE" && std::string(header+36,4)=="data","valid WAV chunks");
    check(header[22]==2 && header[32]==4,"preview declares two channels and a four-byte stereo frame");
    file.close(); std::remove(path.c_str());
    br::Audio silent;
    silent.update(1.0f/60,.08f,true,false); silent.stop();
    check(!silent.available(),"offline audio lifecycle never opens a device");
}
}
int main() {
    try { recordedBank(); wavReader(); samples(); movement(); positionalAudio(); stereoMixer(); threatAudio(); offlinePreview(); std::cout<<"Audio tests passed: recorded carpet/damp/tile footsteps, bounded WAV parsing, cadence, 3D distance and stereo panning, seeded ambience, creature tracking cues, capture sting priority/spectrum/headroom, master volume, pause/reset, 12-second stereo preview.\n"; }
    catch (const std::exception& error) { std::cerr<<"Audio tests failed: "<<error.what()<<'\n'; return 1; }
}
