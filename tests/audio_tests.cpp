#include "../common/audio.h"
#include "../common/gameplay.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
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
        const auto original=br::readFootstepWav("assets/audio/footsteps-player/step-0"+std::to_string(index)+".wav");
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
    const std::string source="assets/audio/footsteps-player/step-03.wav",temporary="audio-test-malformed.wav";
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
    check(carpet==tile,"same player recording on hard floors and carpet");
    check(damp==carpet,"same player recording on damp carpet");
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
        check(channelEnergy(far,0)<channelEnergy(near,0)*(kind==br::SoundKind::ThreatNotice?.35:.2),
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
void suppliedScare() {
    const auto squeak=br::synthesizeSound({br::SoundKind::MouseSqueak});
    const auto quietMouse=br::synthesizeSound({br::SoundKind::MouseAmbient});
    check(energy(quietMouse)>100 && energy(quietMouse)<energy(squeak)*.3,"discovery chirp is audible but quieter than the contact alarm");
    check(quietMouse.front()==0 && quietMouse.back()==0,"quiet mouse audio has clean edges");
    check(energy(squeak)>100 && squeak.front()==0 && squeak.back()==0,"mouse squeak is audible and has clean edges");
    br::AudioMixer squeakMixer;br::AudioScene squeakScene;squeakScene.lightPower=0;
    squeakMixer.setScene(squeakScene);squeakMixer.play(br::SoundKind::MouseSqueak,{0,.1,0});
    check(squeakMixer.activeVoices()==1 && energy(render(squeakMixer,22050))>100,"mouse squeak reaches positional mixer");
    squeakMixer.play(br::SoundKind::MouseAmbient,{1,.12,0},.7f);
    check(squeakMixer.activeVoices()==1 && energy(render(squeakMixer,22050))>100,"quiet discovery cue reaches positional mixer");
    check(br::loadJumpscareSample(),"supplied roar loads as bounded PCM");
    const auto growl=br::synthesizeSound({br::SoundKind::ThreatRoar});
    const auto scream=br::readFootstepWav("assets/audio/jumpscare/scream.wav");
    check(growl!=scream && growl.size()==std::size_t(1.35*br::AUDIO_SAMPLE_RATE),"pursuit has a separate short growl");
    check(energy(growl)>0 && growl.front()==0 && growl.back()==0,"growl is audible with clean edges");
    br::AudioMixer mixer;
    br::AudioScene scene; scene.lightPower=1; scene.tension=1;
    mixer.setScene(scene); mixer.reset(321);
    mixer.play(br::SoundKind::Door,scene.listener);
    mixer.play(br::SoundKind::Jumpscare,scene.listener);
    check(mixer.activeVoices()==1,"supplied roar takes exclusive audio ownership");
    const auto attack=render(mixer,128);
    check(std::abs(int(attack.front()))>1000,"impact begins immediately without fade-in");
    mixer.play(br::SoundKind::Door,scene.listener);
    mixer.enqueue({br::SoundKind::Footstep});
    check(mixer.activeVoices()==1,"capture rejects footsteps and other one-shots");
    const auto body=render(mixer,2*br::AUDIO_SAMPLE_RATE);
    for(auto sample:body) check(std::abs(int(sample))<29000,"supplied roar mix leaves headroom");
    check(energy(render(mixer,3*br::AUDIO_SAMPLE_RATE))==0,"hum drone and scheduler stay silent after scream");
    mixer.reset(321); mixer.play(br::SoundKind::Jumpscare,scene.listener);
    check(attack==render(mixer,128),"reset permits repeatable scare playback");
    mixer.reset(322); mixer.play(br::SoundKind::Door,scene.listener);
    check(mixer.activeVoices()==1,"restart releases exclusive audio ownership");
}
void suppliedMonsterSteps() {
    check(br::loadMonsterFootsteps(),"load four supplied heavy footsteps");
    const auto clip=br::synthesizeSound({br::SoundKind::ThreatStep});
    check(clip==br::readFootstepWav("assets/audio/footsteps-monster/step-02.wav"),"monster uses supplied heavy recording");
    br::AudioMixer mixer;br::AudioScene scene;scene.lightPower=0;
    scene.listener={0,1.65,0};mixer.setScene(scene);mixer.reset(123);
    mixer.play(br::SoundKind::ThreatStep,{3,1.65,0});const auto clear=render(mixer,22050);
    scene.threatTransmission=.38f;mixer.setScene(scene);mixer.reset(123);
    mixer.play(br::SoundKind::ThreatStep,{3,1.65,0});const auto wall=render(mixer,22050);
    // The output limiter compresses the louder clear signal more strongly.
    check(energy(wall)<energy(clear)*.55,"wall transmission attenuates recorded steps");
    scene.threatTransmission=1;mixer.setScene(scene);mixer.reset(123);
    mixer.play(br::SoundKind::ThreatStep,{30,1.65,0});const auto distant=render(mixer,22050);
    check(energy(distant)<energy(clear)*.15,"distance attenuates recorded steps");
}
void threatVocalCadence() {
    using br::SoundKind;
    br::ThreatVocalCadence cues;
    auto is=[](const std::vector<SoundKind>& events,SoundKind kind){return events==std::vector<SoundKind>{kind};};
    check(cues.update(.016f,true,false,false).empty(),"Hearing without sight does not play a visual notice");
    check(is(cues.update(.016f,true,true,false),SoundKind::ThreatNotice),"Acquiring sight plays a notice even after a hearing-only chase");
    check(cues.update(6.99f,true,true,false).empty(),"Continuous visibility waits seven seconds before repeating");
    check(is(cues.update(.02f,true,true,false),SoundKind::ThreatNotice),"Sustained visible pursuit repeats a notice without needing the key");
    check(cues.update(0,true,true,false).empty(),"Sustained pursuit never emits a burst of repeated calls");
    cues.reset();cues.update(0,true,true,false);
    check(cues.update(10,true,false,false,true).empty(),"Menus freeze vocal clocks and visibility history");
    check(cues.update(0,true,true,false).empty(),"Resuming while still visible does not create a new notice");
    cues.update(.1f,true,false,false);
    check(cues.update(.1f,true,true,false).empty(),"Rapid sight loss and reacquisition respects the five-second cooldown");
    check(cues.update(4.79f,true,true,false).empty(),"Reacquired sight waits until the notice cooldown expires");
    check(is(cues.update(.02f,true,true,false),SoundKind::ThreatNotice),"Reacquired sight inside cooldown is queued rather than lost");
    cues.update(.1f,true,false,false);cues.update(.1f,true,true,false);cues.update(.1f,true,false,false);
    check(cues.update(6,true,false,false).empty(),"Losing sight cancels queued reacquisition and sustained repeats");
    cues.update(5,true,false,false);
    check(is(cues.update(0,true,true,false),SoundKind::ThreatNotice),"A later visual reacquisition can play a fresh notice");
    cues.reset();cues.keyCollected();
    check(is(cues.update(0,true,true,true),SoundKind::ThreatNotice),"Visual notice takes priority over simultaneous key roar");
    check(cues.update(1.99f,true,true,true).empty(),"Notice and key roar cannot start within two seconds");
    check(is(cues.update(.02f,true,true,true),SoundKind::ThreatRoar),"Key roar waits until the shared vocal interval ends");
    check(cues.update(6.9f,true,true,true).empty(),"Rage does not repeat before seven seconds");
    check(is(cues.update(.11f,true,true,true),SoundKind::ThreatRoar),"Rage retains its seven-second recurrence");
    cues.reset();cues.keyCollected();
    check(cues.update(1,false,false,true).empty(),"Key roar waits for an active creature");
    check(is(cues.update(0,true,false,true),SoundKind::ThreatRoar),"Pending key roar starts when the creature becomes active");
    check(cues.update(.1f,true,true,true).empty(),"Sight acquired during a roar queues a notice");
    check(cues.update(1.89f,true,true,true).empty(),"Queued notice still honors the shared interval");
    check(is(cues.update(.02f,true,true,true),SoundKind::ThreatNotice),"Queued notice plays once if the player is still visible");
    cues.reset();cues.keyCollected();cues.update(0,true,false,true);cues.update(.1f,true,true,true);
    cues.update(.1f,true,false,true);
    check(cues.update(2,true,false,true).empty(),"A queued notice is cancelled if sight is lost");
    cues.keyCollected();
    check(cues.update(10,true,true,true,false,false).empty(),"Death suppresses pending and periodic vocals");
    cues.reset();
    check(is(cues.update(0,true,true,false),SoundKind::ThreatNotice),"Restart resets vocal acquisition and cooldowns");
}
void araneaAudio() {
    using br::SoundKind;
    const std::array<SoundKind,4> kinds={SoundKind::ThreatStep,SoundKind::ThreatRunStep,SoundKind::ThreatNotice,SoundKind::ThreatRoar};
    const std::array<const char*,4> names={"walk","run","notice","roar"};
    check(!br::loadAraneaSamples("missing-aranea-bank"),"Missing Aranea bank restores legacy fallback");
    std::array<std::vector<std::int16_t>,4> fallback;
    for(std::size_t kind=0;kind<kinds.size();++kind)fallback[kind]=br::synthesizeSound({kinds[kind]});
    check(fallback[0]==fallback[1],"Legacy running falls back to the supplied monster footstep bank");
    const auto playerBefore=br::synthesizeSound({SoundKind::Footstep});
    br::AudioScene scene;scene.listener={0,1.65,0};scene.lightPower=0;scene.tension=0;
    br::AudioMixer oldCapture;oldCapture.setScene(scene);oldCapture.reset(987);
    oldCapture.play(SoundKind::Jumpscare,scene.listener);const auto captureBefore=render(oldCapture,2*br::AUDIO_SAMPLE_RATE);
    check(br::loadAraneaSamples(),"Load all sixteen Aranea walking/running/notice/roar samples");
    std::array<std::vector<std::int16_t>,4> first;
    for(std::size_t kind=0;kind<kinds.size();++kind) {
        std::set<std::vector<std::int16_t>> variants;
        for(std::uint32_t variant=1;variant<=4;++variant) {
            const auto clip=br::synthesizeSound({kinds[kind],br::Gait::Walk,br::Surface::Carpet,variant});
            const auto source=br::readFootstepWav(std::string("assets/audio/aranea/")+names[kind]+"-0"+std::to_string(variant)+".wav");
            check(clip==source,"Creature cues use their exact preloaded Aranea recording");
            check(!clip.empty()&&clip.size()<4*br::AUDIO_SAMPLE_RATE&&energy(clip)>100,"Aranea clips remain bounded and audible");
            check(clip.front()==0&&clip.back()==0,"Aranea sample endpoints prevent clicks");
            for(auto sample:clip)check(std::abs(int(sample))<29000,"Aranea recordings leave source headroom");
            variants.insert(clip);if(variant==1)first[kind]=clip;
        }
        check(variants.size()==4,"Each Aranea cue bank contains four distinct variations");
    }
    check(first[0]!=first[1],"Walking and running use different ground-contact recordings");
    check(first[2]!=first[3],"Visual notice and recurring roar remain distinct cues");
    check(br::synthesizeSound({SoundKind::Footstep})==playerBefore,"Player footsteps are unchanged by Aranea loading");
    br::AudioMixer capture;capture.setScene(scene);capture.reset(987);capture.play(SoundKind::Jumpscare,scene.listener);
    check(render(capture,2*br::AUDIO_SAMPLE_RATE)==captureBefore,"Existing capture scream and mix are unchanged by Aranea loading");

    // Fail late in the last bank, after fifteen valid files, to catch partial
    // updates that would silently mix new recordings with stale loaded clips.
    namespace fs=std::filesystem;
    const fs::path partial=fs::temp_directory_path()/("backrooms-aranea-test-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(partial);
    std::vector<fs::path> copied;
    for(std::size_t kind=0;kind<names.size();++kind)for(int variant=1;variant<=4;++variant) {
        if(kind==3&&variant==4)continue;
        const auto filename=std::string(names[kind])+"-0"+std::to_string(variant)+".wav";
        fs::copy_file(fs::path("assets/audio/aranea")/filename,partial/filename);copied.push_back(partial/filename);
    }
    const bool partialLoaded=br::loadAraneaSamples(partial.string());
    for(const auto& path:copied)fs::remove(path);
    fs::remove(partial);
    check(!partialLoaded,"An incomplete final Aranea bank is rejected atomically");
    for(std::size_t kind=0;kind<kinds.size();++kind)
        check(br::synthesizeSound({kinds[kind]})==fallback[kind],"A partial reload clears every Aranea override");
    check(br::loadAraneaSamples(),"Aranea bank can be restored after an atomic load failure");

    br::AudioMixer mixer;mixer.setScene(scene);
    for(auto kind:kinds) {
        mixer.reset(123);scene.threatTransmission=1;mixer.setScene(scene);
        mixer.play(kind,{-3,1.65,0});const auto near=render(mixer,br::AUDIO_SAMPLE_RATE);
        check(channelEnergy(near,0)>100&&channelEnergy(near,1)==0,"All Aranea cues retain stereo source direction");
        mixer.reset(123);scene.threatTransmission=.38f;mixer.setScene(scene);
        mixer.play(kind,{-3,1.65,0});const auto blocked=render(mixer,br::AUDIO_SAMPLE_RATE);
        check(energy(blocked)<energy(near)*.55,"Walls attenuate Aranea walk/run/notice/roar cues");
        mixer.reset(123);scene.threatTransmission=1;mixer.setScene(scene);
        mixer.play(kind,{-30,1.65,0});const auto far=render(mixer,br::AUDIO_SAMPLE_RATE);
        const bool vocal=kind==SoundKind::ThreatNotice||kind==SoundKind::ThreatRoar;
        check(energy(far)<energy(near)*(vocal?.30:.2),"Distant Aranea cues are attenuated");
    }
    // Each bank owns its variant counter: inserting a run must not skip the
    // next walking contact. Diagonal pairs must use two distinct variations.
    const auto frames=std::max(first[0].size(),first[1].size())+64;
    mixer.reset(731);mixer.play(SoundKind::ThreatStep,scene.listener);const auto firstWalk=render(mixer,frames);
    mixer.play(SoundKind::ThreatStep,scene.listener);const auto secondWalk=render(mixer,frames);
    check(firstWalk!=secondWalk,"Successive walking contacts rotate sample variations");
    mixer.reset(731);mixer.play(SoundKind::ThreatStep,scene.listener);render(mixer,frames);
    mixer.play(SoundKind::ThreatRunStep,scene.listener);render(mixer,frames);
    mixer.play(SoundKind::ThreatStep,scene.listener);
    check(render(mixer,frames)==secondWalk,"Running has an independent variation counter from walking");
    mixer.reset(731);mixer.play(SoundKind::ThreatRunStep,scene.listener);const auto firstRun=render(mixer,frames);
    mixer.play(SoundKind::ThreatRunStep,scene.listener);const auto secondRun=render(mixer,frames);
    check(firstRun!=secondRun,"Successive running contacts rotate sample variations");
    mixer.reset(731);mixer.play(SoundKind::ThreatRunStep,scene.listener);render(mixer,frames);
    mixer.play(SoundKind::ThreatStep,scene.listener);render(mixer,frames);
    mixer.play(SoundKind::ThreatRunStep,scene.listener);
    check(render(mixer,frames)==secondRun,"Walking does not skip the next running variation");

    mixer.reset(52);
    mixer.play(SoundKind::ThreatRunStep,scene.listener,.8f);mixer.play(SoundKind::ThreatRunStep,scene.listener,.8f);
    mixer.play(SoundKind::ThreatNotice,scene.listener,1.f);
    check(mixer.activeVoices()==3,"Two diagonal foot contacts and the detection cue can overlap");
    const auto combined=render(mixer,2*br::AUDIO_SAMPLE_RATE);
    for(auto sample:combined)check(std::abs(int(sample))<29000,"Paired ground contacts plus notice retain mixer headroom");
    mixer.reset(52);mixer.play(SoundKind::ThreatNotice,scene.listener);mixer.setPaused(true);
    check(energy(render(mixer,1024))==0&&mixer.activeVoices()==0,"Pause clears Aranea audio without a delayed restart");
    mixer.play(SoundKind::ThreatRunStep,scene.listener);mixer.setPaused(false);
    check(mixer.activeVoices()==0,"Footstep requests made while paused cannot leak into play");
    mixer.setVolume(0);mixer.reset(52);mixer.play(SoundKind::ThreatNotice,scene.listener);
    check(energy(render(mixer,1024))==0,"Master mute applies to the new Aranea recordings");
    mixer.setVolume(1);mixer.reset(731);mixer.play(SoundKind::ThreatStep,scene.listener);
    check(render(mixer,frames)==firstWalk,"Reset restores deterministic monster variation playback");
    mixer.reset(52);mixer.play(SoundKind::ThreatRunStep,scene.listener);mixer.play(SoundKind::Jumpscare,scene.listener);
    check(mixer.activeVoices()==1,"Capture still replaces all new monster cues exclusively");
}
void resumedThreatAudio() {
    using br::SoundKind;
    // A pending vocal becomes due on the very first frame after Resume. The
    // production order must synchronize pause before consuming that event.
    auto firstResumedFrame=[](bool notice,bool syncBeforeEvents) {
        br::ThreatVocalCadence cues;br::AudioMixer mixer;br::AudioScene scene;
        scene.listener={0,1.65,0};scene.lightPower=0;scene.tension=0;
        mixer.setScene(scene);mixer.reset(842);cues.keyCollected();
        cues.update(0,true,!notice,true); // Start either notice or roar cooldown.
        check(cues.update(1.99f,true,true,true).empty(),"A pending vocal waits before pause");
        mixer.setPaused(true);
        check(cues.update(30,true,true,true,true).empty(),"Pause does not consume a queued vocal");
        if(syncBeforeEvents)mixer.setPaused(false);
        const auto due=cues.update(.02f,true,true,true);
        check(due==std::vector<SoundKind>{notice?SoundKind::ThreatNotice:SoundKind::ThreatRoar},
              "The queued vocal is due on the first resumed frame");
        for(auto cue:due)mixer.play(cue,scene.listener);
        // The later Audio::update repeats the same pause state without clearing
        // accepted voices. In the old order the one-shot had already been lost.
        mixer.setPaused(false);
        check(cues.update(0,true,true,true).empty(),"A resumed vocal remains one-shot");
        return render(mixer,4096);
    };
    for(bool notice:{false,true}) {
        check(energy(firstResumedFrame(notice,false))==0,"Negative control reproduces the lost cue with late resume sync");
        check(energy(firstResumedFrame(notice,true))>100,"Early pause sync preserves the first resumed notice/roar in PCM");
    }
    br::AudioMixer capture;capture.play(SoundKind::Jumpscare,{});
    capture.setPaused(false);capture.play(SoundKind::ThreatNotice,{});
    check(capture.activeVoices()==1,"Repeated live pause synchronization preserves capture-only playback");
    capture.setPaused(true);capture.setPaused(false);capture.play(SoundKind::ThreatRunStep,{});
    check(capture.activeVoices()==0,"Pause synchronization does not release capture-only ownership");
    br::Audio deviceLess;
    deviceLess.setPaused(true);deviceLess.setPaused(false);
    check(!deviceLess.available(),"Synchronizing pause neither opens nor requires an audio device");
}
void audibleThreatCalls() {
    using br::SoundKind;
    br::AudioScene scene;scene.listener={0,1.65,0};scene.lightPower=0;scene.tension=0;
    br::AudioMixer mixer;mixer.setScene(scene);mixer.setVolume(.75f);
    for(auto kind:{SoundKind::ThreatNotice,SoundKind::ThreatRoar})for(double distance:{14.32,22.0}) {
        std::array<double,4> levels{};double largestOld=0;
        for(std::uint32_t variant=1;variant<=4;++variant) {
            const auto clip=br::synthesizeSound({kind,br::Gait::Walk,br::Surface::Carpet,variant});
            const br::AudioPosition source{0,1.65,-distance};
            mixer.reset(53);
            // Advance only this bank's selector. These voices are out of range.
            for(std::uint32_t skip=1;skip<variant;++skip)mixer.play(kind,{0,1.65,-1000});
            mixer.play(kind,source,1.f);
            const auto actual=render(mixer,clip.size());levels[variant-1]=energy(actual);
            // Negative control: the former 3m reference distance and notice .8
            // gain, with the same sample, master level, pan and output limiter.
            const auto oldGains=br::positionalGains(scene,source,3,64);
            std::vector<std::int16_t> old(clip.size()*2);
            for(std::size_t i=0;i<clip.size();++i)for(std::size_t channel=0;channel<2;++channel) {
                const double value=clip[i]/32767.0*(kind==SoundKind::ThreatNotice?.8:1.0)*
                    (channel?oldGains.right:oldGains.left);
                old[i*2+channel]=std::int16_t(std::lround(32767*.75*.88*value/(1+std::abs(value))));
            }
            const double oldLevel=energy(old),floor=distance<20?1250:850;
            largestOld=std::max(largestOld,oldLevel);
            check(levels[variant-1]>floor,"Every scream variant meets the 14-22m pursuit audibility floor at master .75");
            check(oldLevel<floor,"Former attenuation fails the pursuit audibility floor with identical samples");
            check(levels[variant-1]>oldLevel*2,"Pursuit call is at least twice as strong as its old-distance negative control");
            for(auto value:actual)check(std::abs(int(value))<29000,"Distant pursuit calls leave limiter headroom");
        }
        double mean=0,variance=0;for(auto value:levels)mean+=value/levels.size();
        for(auto value:levels)variance+=(value-mean)*(value-mean)/levels.size();
        const double cv=std::sqrt(variance)/mean;
        check(cv<.18,"Call variants have consistent perceived level instead of intermittent quiet variants");
        std::cout<<(kind==SoundKind::ThreatNotice?"Notice":"Roar")<<" at "<<distance
                 <<"m, master .75: PCM RMS "<<*std::min_element(levels.begin(),levels.end())
                 <<".."<<*std::max_element(levels.begin(),levels.end())<<", variant CV "<<cv
                 <<", old 3m maximum "<<largestOld<<'\n';
    }
}
void threatVoicePriority() {
    using br::SoundKind;
    br::AudioScene scene;scene.listener={0,1.65,0};scene.lightPower=0;scene.tension=0;
    br::AudioMixer full,isolated;full.setScene(scene);isolated.setScene(scene);
    for(auto call:{SoundKind::ThreatNotice,SoundKind::ThreatRoar}) {
        full.reset(93);isolated.reset(93);
        // The oldest slot is audible: replacing any other slot would leave it
        // in the PCM. Other slots are outside the audible range.
        full.play(SoundKind::Door,scene.listener);
        for(int i=1;i<12;++i)full.play(SoundKind::Door,{0,1.65,-1000});
        check(full.activeVoices()==12,"Priority regression starts with every voice occupied");
        full.play(call,{0,1.65,-14.32});isolated.play(call,{0,1.65,-14.32});
        check(full.activeVoices()==12,"Important call steals one lower-priority slot without increasing budget");
        full.enqueue({SoundKind::Footstep});full.play(SoundKind::ThreatRunStep,scene.listener);
        full.play(SoundKind::AmbientCreak,scene.listener);
        check(render(full,br::AUDIO_SAMPLE_RATE)==render(isolated,br::AUDIO_SAMPLE_RATE),
              "Call replaces the oldest ordinary voice and later feet/ambience cannot displace it");
        full.reset(93);
        for(int i=0;i<12;++i)full.play(call,{0,1.65,-1000});
        full.play(call,scene.listener);
        check(energy(render(full,1024))==0,"A call never steals an equal-priority call");
        full.play(SoundKind::Jumpscare,scene.listener);
        check(full.activeVoices()==1,"Capture retains exclusive priority even over twelve calls");
        full.play(call,scene.listener);full.enqueue({SoundKind::Footstep});
        check(full.activeVoices()==1,"Ordinary priority logic cannot bypass capture-only ownership");
    }
}
void writeAraneaMixerPreview(const std::filesystem::path& directory) {
    using br::SoundKind;
    std::filesystem::create_directories(directory);
    auto write=[&](const std::string& name,const std::vector<std::int16_t>& samples) {
        std::ofstream file(directory/name,std::ios::binary);check(bool(file),"Create actual mixer preview");
        auto u16=[&](std::uint16_t value){file.put(char(value));file.put(char(value>>8));};
        auto u32=[&](std::uint32_t value){u16(std::uint16_t(value));u16(std::uint16_t(value>>16));};
        const auto bytes=std::uint32_t(samples.size()*2);
        file.write("RIFF",4);u32(36+bytes);file.write("WAVEfmt ",8);u32(16);u16(1);u16(2);
        u32(br::AUDIO_SAMPLE_RATE);u32(br::AUDIO_SAMPLE_RATE*4);u16(4);u16(16);
        file.write("data",4);u32(bytes);for(auto value:samples)u16(std::uint16_t(value));
        check(bool(file),"Write stereo PCM from the real mixer without normalization");
    };
    br::AudioScene scene;scene.listener={0,1.65,0};scene.lightPower=.5f;scene.tension=.3f;
    scene.fluorescent={3,3.2,3};
    br::AudioMixer mixer;mixer.setScene(scene);mixer.setVolume(.75f);mixer.reset(123);
    std::vector<std::int16_t> preview(12*br::AUDIO_SAMPLE_RATE*2);std::size_t cursor=0;
    for(int frame=0;frame<12*60;++frame) {
        // Paired ground contacts walk at 6m, then run at 8m. At 7s, a
        // left/front creature cries from the measured 14.32m encounter range.
        if(frame<4*60&&frame%48==0)for(int contact=0;contact<2;++contact)mixer.play(SoundKind::ThreatStep,{-3,.2,-5.0},.8f);
        if(frame>=4*60&&frame<7*60&&frame%27==0)for(int contact=0;contact<2;++contact)mixer.play(SoundKind::ThreatRunStep,{4,.2,-6.8},.8f);
        if(frame==7*60)mixer.play(SoundKind::ThreatNotice,{-7.16,1.65,-12.401},1.f);
        if(frame==10*60)mixer.play(SoundKind::ThreatRoar,{11,1.65,-19.05256},1.f);
        const auto end=std::size_t((frame+1)*br::AUDIO_SAMPLE_RATE/60);
        mixer.render(preview.data()+cursor*2,end-cursor);cursor=end;
    }
    write("soil-and-calls-mixer.wav",preview);
    scene.lightPower=scene.tension=0;mixer.setScene(scene);
    for(double distance:{14.32,22.0}) {
        mixer.reset(123);mixer.play(SoundKind::ThreatNotice,{0,1.65,-distance},1.f);
        write(distance<20?"notice-14m-mixer.wav":"notice-22m-mixer.wav",render(mixer,2*br::AUDIO_SAMPLE_RATE));
    }
    std::cout<<"Actual mixer previews saved to "<<directory.string()<<" (master .75, no normalization).\n";
}
void darknessAudio() {
    using namespace br;
    const auto clip=synthesizeSound({SoundKind::DarknessJumpscare});
    check(clip.size()==std::size_t(1.7*AUDIO_SAMPLE_RATE) && energy(clip)>500,"darkness has a distinct audible short sting");
    check(clip.front()==0 && clip.back()==0,"darkness sting has click-free edges");
    check(clip!=synthesizeSound({SoundKind::Jumpscare}),"parking does not reuse the Aranea scream");
    check(clip==synthesizeSound({SoundKind::DarknessJumpscare}),"darkness PCM is deterministic and cacheable");
    for(auto s:clip)check(std::abs(int(s))<24000,"darkness synthesis retains peak headroom");
    AudioMixer mixer;AudioScene scene;scene.lightPower=scene.tension=0;mixer.setScene(scene);
    for(int i=0;i<12;++i)mixer.play(SoundKind::Door,scene.listener);
    mixer.play(SoundKind::DarknessJumpscare,{1000,0,1000});
    check(mixer.activeVoices()==1,"darkness replaces a full mix with one centered capture sound");
    mixer.play(SoundKind::Door,scene.listener);
    check(mixer.activeVoices()==1,"ambient sounds cannot overlap darkness capture");
    auto stereo=render(mixer,2*AUDIO_SAMPLE_RATE);
    check(channelEnergy(stereo,0)>500 && channelEnergy(stereo,0)==channelEnergy(stereo,1),"darkness remains audible and centered regardless of distance");
    check(mixer.activeVoices()==0,"darkness does not loop after the sting");
    mixer.reset(1);mixer.setVolume(0);mixer.play(SoundKind::DarknessJumpscare,{});
    check(energy(render(mixer,1024))==0,"master mute covers darkness capture");
    mixer.setVolume(.75f);mixer.reset(1);mixer.play(SoundKind::DarknessJumpscare,{});
    stereo=render(mixer,2*AUDIO_SAMPLE_RATE);
    std::filesystem::create_directories("tests/artifacts");
    std::ofstream file("tests/artifacts/darkness-jumpscare.wav",std::ios::binary);
    auto u16=[&](std::uint16_t v){file.put(char(v));file.put(char(v>>8));};
    auto u32=[&](std::uint32_t v){u16(std::uint16_t(v));u16(std::uint16_t(v>>16));};
    const auto bytes=std::uint32_t(stereo.size()*2);
    file.write("RIFF",4);u32(36+bytes);file.write("WAVEfmt ",8);u32(16);u16(1);u16(2);
    u32(AUDIO_SAMPLE_RATE);u32(AUDIO_SAMPLE_RATE*4);u16(4);u16(16);file.write("data",4);u32(bytes);
    for(auto s:stereo)u16(std::uint16_t(s));
    check(bool(file),"actual mixer preview written");
    mixer.reset(1);mixer.play(SoundKind::DarknessJumpscare,{});mixer.setPaused(true);
    check(mixer.activeVoices()==0 && energy(render(mixer,512))==0,"pause clears darkness capture audio");
    std::cout<<"Darkness audio passed: distinct sting, centered capture priority, headroom, mute, pause, one-shot duration and mixer preview.\n";
}
int main(int argc,char** argv) {
    const auto opening=br::synthesizeSound({br::SoundKind::PresentOpen});
    check(opening.size()==std::size_t(.58*br::AUDIO_SAMPLE_RATE) && energy(opening)>500,"present opening is a short audible paper sound");
    check(opening.front()==0 && opening.back()==0,"present opening has click-free edges");
    check(opening!=br::synthesizeSound({br::SoundKind::Door}),"present opening is distinct from door sound");

    try { recordedBank(); wavReader(); samples(); movement(); positionalAudio(); stereoMixer(); threatAudio(); offlinePreview(); std::cout<<"Audio tests passed: recorded carpet/damp/tile footsteps, bounded WAV parsing, cadence, 3D distance and stereo panning, seeded ambience, creature tracking cues, capture sting priority/spectrum/headroom, master volume, pause/reset, 12-second stereo preview.\n"; }
    catch (const std::exception& error) { std::cerr<<"Audio tests failed: "<<error.what()<<'\n'; return 1; }
    try { suppliedScare(); suppliedMonsterSteps(); darknessAudio(); std::cout<<"Supplied scream and heavy steps: attack, exclusive mix, attenuation, headroom and reset passed.\n"; }
    catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
    try { threatVocalCadence();araneaAudio();resumedThreatAudio();audibleThreatCalls();threatVoicePriority();
        if(argc==3&&std::string(argv[1])=="--aranea-preview")writeAraneaMixerPreview(argv[2]);
        std::cout<<"Aranea audio passed: atomic banks, soil walk/run variants, queued and sustained visual calls, 14-22m audibility, voice priority, wall attenuation, paired-contact headroom, first-frame resume cues, pause/mute/reset and unchanged player/capture audio.\n"; }
    catch(const std::exception& error) { std::cerr<<"Aranea audio tests failed: "<<error.what()<<'\n';return 1; }
}
