#include "audio.h"
#include "gameplay.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace br {
namespace {
constexpr double PI=3.14159265358979323846;
constexpr std::size_t FOOTSTEP_COUNT=6;
constexpr std::size_t POSITIONED_CLIP_COUNT=
    std::size_t(SoundKind::CityAmbience)-std::size_t(SoundKind::AmbientCreak)+1;
bool cinematicSound(SoundKind kind) {
    return kind>=SoundKind::ElevatorDoor && kind<=SoundKind::CityAmbience;
}
bool captureSound(SoundKind kind) {
    return kind==SoundKind::Jumpscare || kind==SoundKind::DarknessJumpscare || kind==SoundKind::PartygoerJumpscare;
}
std::array<std::vector<std::int16_t>,FOOTSTEP_COUNT> recordedSteps;
std::size_t loadedSteps=0;
std::array<std::vector<std::int16_t>,4> monsterSteps;
std::vector<std::int16_t> recordedScream, partygoerScream, verityPickup;
struct AraneaSamples {
    std::array<std::vector<std::int16_t>,4> walk,run,notice,roar;
};
AraneaSamples araneaSamples;
double stride(Gait gait) {
    // Each impact follows real movement: about 1.2 crouch, 1.7 walk and
    // 2.2 sprint steps per second at the exploration movement speeds.
    return gait==Gait::Crouch?.90:(gait==Gait::Sprint?1.85:1.50);
}
double noise(std::uint32_t& state) {
    state^=state<<13; state^=state>>17; state^=state<<5;
    return double(state)/double(std::numeric_limits<std::uint32_t>::max())*2-1;
}
std::int16_t pcm(double sample) {
    return static_cast<std::int16_t>(std::lround(std::clamp(sample,-.95,.95)*32767));
}
void little16(std::ostream& stream,std::uint16_t value) {
    const char bytes[]={char(value&255),char((value>>8)&255)};
    stream.write(bytes,2);
}
void little32(std::ostream& stream,std::uint32_t value) {
    const char bytes[]={char(value&255),char((value>>8)&255),char((value>>16)&255),char((value>>24)&255)};
    stream.write(bytes,4);
}
std::uint16_t read16(const std::vector<unsigned char>& bytes,std::size_t offset) {
    return std::uint16_t(bytes[offset]) | (std::uint16_t(bytes[offset+1])<<8);
}
std::uint32_t read32(const std::vector<unsigned char>& bytes,std::size_t offset) {
    return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset+1])<<8)
        | (std::uint32_t(bytes[offset+2])<<16) | (std::uint32_t(bytes[offset+3])<<24);
}
bool tag(const std::vector<unsigned char>& bytes,std::size_t offset,const char* value) {
    return std::memcmp(bytes.data()+offset,value,4)==0;
}
std::vector<std::int16_t> recordedSound(const SoundEvent& event) {
    const auto& source=recordedSteps[event.variation%FOOTSTEP_COUNT];
    const double pitch=.98+.008*double((event.variation/FOOTSTEP_COUNT)%6);
    const double gain=event.gait==Gait::Crouch?.105:(event.gait==Gait::Sprint?.48:.36);
    const auto count=static_cast<std::size_t>((source.size()-1)/pitch)+1;
    std::vector<std::int16_t> result(count);
    double filtered=0,wetNoise=0;
    std::uint32_t wetState=event.variation*747796405u+2891336453u;
    for (std::size_t i=0;i<count;++i) {
        const double position=std::min(double(source.size()-1),i*pitch);
        const auto left=static_cast<std::size_t>(position);
        const auto right=std::min(left+1,source.size()-1);
        const double fraction=position-left;
        const double value=(source[left]*(1-fraction)+source[right]*fraction)/32767;
        // A soft carpet absorbs the brightest part of the real shoe impact.
        const bool damp=event.surface==Surface::DampCarpet;
        filtered+=(event.surface==Surface::HardFloor?1.0:(damp?.30:.65))*(value-filtered);
        wetNoise+=.14*(noise(wetState)-wetNoise);
        const double t=double(i)/AUDIO_SAMPLE_RATE;
        const double wetSole=damp?.25*wetNoise*std::exp(-std::pow((t-.09)/.068,2)):0;
        result[i]=pcm((filtered*(damp?.90:1.0)+wetSole)*gain);
    }
    result.front()=0; result.back()=0;
    return result;
}
std::vector<std::int16_t> environmentalSound(const SoundEvent& event) {
    const bool creak=event.kind==SoundKind::AmbientCreak;
    const bool drip=event.kind==SoundKind::WaterDrip;
    const bool door=event.kind==SoundKind::Door;
    const double duration=creak?1.6:(drip?.64:(door?.72:1.85));
    const auto count=static_cast<std::size_t>(duration*AUDIO_SAMPLE_RATE);
    std::vector<std::int16_t> result(count);
    std::uint32_t state=event.variation*747796405u+2891336453u;
    if (!state) state=1;
    const double pitch=.94+.04*(event.variation%4);
    double smooth=0;
    for (std::size_t i=0;i<count;++i) {
        const double t=double(i)/AUDIO_SAMPLE_RATE;
        smooth+=.10*(noise(state)-smooth);
        double value=0;
        if (creak) {
            const double envelope=std::pow(std::sin(PI*t/duration),1.9);
            value=.29*envelope*(.56*std::sin(2*PI*pitch*(94*t+23*t*t))
                +.25*std::sin(2*PI*pitch*(187*t+41*t*t))+.65*smooth);
        } else if (drip) {
            const double onset=std::min(1.0,t/.002);
            value=.35*onset*std::exp(-t*12)*std::sin(2*PI*pitch*(1160*t-560*t*t));
            value+=.065*smooth*std::exp(-std::pow((t-.075)/.045,2));
        } else if (door) {
            value=.24*std::sin(2*PI*79*pitch*t)*std::exp(-t*11)
                +.10*smooth*std::exp(-t*6)
                +.12*std::sin(2*PI*pitch*(230*t+140*t*t))*std::exp(-std::pow((t-.26)/.17,2));
        } else {
            // A restrained electrical dissonance marks an environmental change.
            const double envelope=std::pow(std::sin(PI*t/duration),1.6);
            value=.085*envelope*(std::sin(2*PI*58*pitch*t)
                +.60*std::sin(2*PI*61.3*pitch*t)+.50*smooth);
        }
        const double fade=std::min({1.0,t/.005,(duration-t)/.045});
        result[i]=pcm(value*fade);
    }
    result.front()=0; result.back()=0;
    return result;
}
std::vector<std::int16_t> threatSound(const SoundEvent& event) {
    const auto* bank=event.kind==SoundKind::ThreatStep?&araneaSamples.walk:
        event.kind==SoundKind::ThreatRunStep?&araneaSamples.run:
        event.kind==SoundKind::ThreatNotice?&araneaSamples.notice:
        event.kind==SoundKind::ThreatRoar?&araneaSamples.roar:nullptr;
    if(bank&&!(*bank)[0].empty())return (*bank)[(event.variation?event.variation-1:0)%4];
    const bool step=event.kind==SoundKind::ThreatStep||event.kind==SoundKind::ThreatRunStep;
    if(step && !monsterSteps[event.variation%4].empty())
        return monsterSteps[event.variation%4];
    if(event.kind==SoundKind::ThreatRoar && recordedScream.size()>AUDIO_SAMPLE_RATE*2/3) {
        // Pursuit is a short, low throaty growl; capture keeps its sharp scream.
        const double pitch=.67+.018*(event.variation%4),duration=1.35;
        std::vector<std::int16_t> growl(std::size_t(duration*AUDIO_SAMPLE_RATE));
        double low=0;
        for(std::size_t i=0;i<growl.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE;
            const double source=.35*AUDIO_SAMPLE_RATE+i*pitch;
            const auto j=std::min(std::size_t(source),recordedScream.size()-2);
            const double sample=(recordedScream[j]+(recordedScream[j+1]-recordedScream[j])*(source-std::floor(source)))/32768.;
            low+=.16*(sample-low);
            const double envelope=std::min(1.,t/.055)*std::min(1.,(duration-t)/.24);
            growl[i]=pcm(low*envelope*(.76+.24*std::sin(2*PI*31*t)));
        }
        growl.front()=growl.back()=0;
        return growl;
    }
    const bool caught=event.kind==SoundKind::Jumpscare;
    const double duration=step?.42:(caught?.82:.95);
    const auto count=static_cast<std::size_t>(duration*AUDIO_SAMPLE_RATE);
    std::vector<std::int16_t> result(count);
    std::uint32_t state=event.variation*747796405u+2891336453u;
    if (!state) state=1;
    const double pitch=.97+.015*(event.variation%5);
    double lowNoise=0,midNoise=0;
    for (std::size_t i=0;i<count;++i) {
        const double t=double(i)/AUDIO_SAMPLE_RATE;
        const double white=noise(state);
        lowNoise+=.055*(white-lowNoise);
        midNoise+=.32*(white-midNoise);
        double value=0;
        if (step) {
            // A low, dragging footfall differentiates the creature from the
            // player's recorded shoes without announcing it through loud clicks.
            const double heel=std::sin(2*PI*pitch*(48*t-15*t*t))*std::exp(-t*17);
            const double drag=std::exp(-std::pow((t-.16)/.095,2));
            value=.37*heel+.22*lowNoise*(std::exp(-t*8)+drag);
        } else if (caught) {
            // Short rough chord, not a full-scale white-noise blast. Slightly
            // detuned falling partials create the sting; a 12ms attack and
            // 90ms release keep it click-free even at the loudest setting.
            const double envelope=std::exp(-t*2.3)*(1+.12*std::sin(2*PI*19*t));
            const double chord=.48*std::sin(2*PI*pitch*(171*t-44*t*t))
                +.30*std::sin(2*PI*pitch*(257*t-35*t*t))
                +.19*std::sin(2*PI*pitch*(739*t-220*t*t));
            value=.57*envelope*(chord+.46*(midNoise-lowNoise));
        } else {
            const double envelope=std::pow(std::sin(PI*t/duration),1.6);
            const double whisper=midNoise-lowNoise;
            value=.18*envelope*(.75*std::sin(2*PI*pitch*(71*t+26*t*t))
                +.4*std::sin(2*PI*pitch*(108*t+32*t*t))+1.2*whisper);
        }
        const double fade=std::min({1.0,t/(caught?.012:.008),(duration-t)/(caught?.09:.05)});
        result[i]=pcm(value*fade);
    }
    result.front()=0; result.back()=0;
    return result;
}

// Original synthesized placeholders, cached once with the other mixer clips.
// Smooth envelopes avoid sharp clicks; only the arrival bell is a bright tone.
std::vector<std::int16_t> cinematicClip(const SoundEvent& event) {
    const bool door=event.kind==SoundKind::ElevatorDoor;
    const bool motor=event.kind==SoundKind::ElevatorMotor;
    const bool bell=event.kind==SoundKind::ElevatorBell;
    const bool city=event.kind==SoundKind::CityAmbience;
    const double duration=door?2.8:(motor?7.0:(bell?1.4:(city?20.0:11.0)));
    std::vector<std::int16_t> result(std::size_t(duration*AUDIO_SAMPLE_RATE));
    std::uint32_t random=event.variation*2891336453u+747796405u;
    double air=0,low=0,phase=0;
    const double pitch=.99+.005*(event.variation%4);
    for(std::size_t i=0;i<result.size();++i) {
        const double t=double(i)/AUDIO_SAMPLE_RATE,n=noise(random);
        air+=.10*(n-air);low+=.008*(n-low);
        double value=0;
        if(door) {
            const double slide=std::pow(std::sin(PI*std::min(1.,t/2.55)),1.2);
            phase+=2*PI*pitch*(83+12*std::sin(PI*t/duration))/AUDIO_SAMPLE_RATE;
            value=slide*(.075*std::sin(phase)+.026*std::sin(phase*2.7)+.23*air);
            const double latch=t-2.55;
            if(latch>=0)value+=.18*std::exp(-latch*26)*std::sin(2*PI*134*latch);
        } else if(motor) {
            const double acceleration=std::clamp(t/1.3,0.,1.);
            const double braking=std::clamp((duration-t)/1.3,0.,1.);
            const double speed=std::min(acceleration,braking);
            phase+=2*PI*pitch*(39+24*speed)/AUDIO_SAMPLE_RATE;
            const double envelope=std::min(1.,t/.28)*std::min(1.,(duration-t)/.45);
            value=envelope*(.13*std::sin(phase)+.035*std::sin(phase*2)+.40*low+.04*air);
            value*=.87+.13*std::sin(t*2*PI*4.6);
        } else if(bell) {
            for(int note=0;note<2;++note) {
                const double age=t-note*.28;
                if(age<0)continue;
                const double hz=(note==0?784.:622.25)*pitch;
                const double envelope=std::min(1.,age/.007)*std::exp(-age*3.5);
                value+=.23*envelope*(std::sin(2*PI*hz*age)+.18*std::sin(2*PI*hz*2.003*age));
            }
        } else if(city) {
            // Original street bed: tire wash and passing engines, distant
            // non-verbal crowd murmur, pavement steps and restrained birds.
            // It is a cached clip, not one voice or generator per city actor.
            value=.15*low+.065*air;
            for(int car=0;car<4;++car) {
                const double age=t-(2.8+car*4.65+.12*(event.variation%3));
                const double pass=std::exp(-age*age/(2.0+car*.15));
                const double hz=car%2?87.:105.;
                const double engine=std::sin(2*PI*pitch*(hz*age-2.0*age*age));
                const double harmonic=std::sin(2*PI*pitch*(2*hz*age-4.0*age*age));
                value+=pass*(.045*engine+.012*harmonic+.30*air+.25*low);
            }
            // Broad, indistinct voices: no words, threat growls or alarm cues.
            const double conversation=.5+.5*std::sin(t*1.7)*std::sin(t*.63+.8);
            value+=conversation*(.035*air+.008*std::sin(2*PI*172*t)*std::sin(t*7.3));
            for(int group=0;group<3;++group)for(int step=0;step<7;++step) {
                const double age=t-(1.0+group*6.2+step*.54);
                if(age>=0&&age<.16) {
                    const double envelope=std::min(1.,age/.007)*std::exp(-age*31);
                    value+=envelope*(.045*std::sin(2*PI*118*age)+.055*air);
                }
            }
            for(int bird=0;bird<8;++bird) {
                const double age=t-(.6+bird*2.4);
                if(age<0||age>.24)continue;
                const double envelope=std::pow(std::sin(PI*age/.24),2);
                value+=.022*envelope*std::sin(2*PI*pitch*(2100*age+950*age*age));
            }
            value*=std::min(1.,t/.35)*std::min(1.,(duration-t)/1.8);
        } else {
            // Distant road wash, a passing vehicle, and a few restrained birds.
            const double pass=std::exp(-std::pow((t-5.1)/2.4,2));
            value=.20*low+.032*air+pass*(.035*std::sin(2*PI*(72*t-.9*t*t))+.26*low);
            for(int bird=0;bird<5;++bird) {
                const double age=t-(.8+bird*1.85);
                if(age<0||age>.34)continue;
                const double envelope=std::pow(std::sin(PI*age/.34),2);
                value+=.035*envelope*std::sin(2*PI*pitch*(1800*age+850*age*age));
            }
            value*=std::min(1.,t/1.0)*std::min(1.,(duration-t)/1.2);
        }
        const double edge=std::min({1.,t/.01,(duration-t)/.035});
        result[i]=pcm(value*edge);
    }
    result.front()=result.back()=0;
    return result;
}

std::vector<std::int16_t> atmosphereSound(const SoundEvent& event) {
    if(event.kind==SoundKind::DistantFootsteps) {
        // Reuse the soft soil bank rather than adding a hard heel or toe click.
        // Irregular, separate contacts hint at movement beyond a wall; the
        // scheduler supplies a stable world position and that wall's loss.
        const unsigned contacts=2+event.variation%2;
        const double interval=.61+.035*(event.variation%3);
        std::vector<double> mixed(std::size_t((.65+contacts*interval)*AUDIO_SAMPLE_RATE));
        for(unsigned contact=0;contact<contacts;++contact) {
            auto source=threatSound({SoundKind::ThreatStep,Gait::Walk,Surface::Carpet,event.variation+contact});
            const auto onset=std::size_t((.08+contact*interval)*AUDIO_SAMPLE_RATE);
            double muffled=0;
            for(std::size_t i=0;i<source.size()&&onset+i<mixed.size();++i) {
                muffled+=.075*(double(source[i])/32767.-muffled);
                const double t=double(i)/AUDIO_SAMPLE_RATE;
                const double duration=double(source.size())/AUDIO_SAMPLE_RATE;
                const double fade=std::min({1.,t/.022,(duration-t)/.065});
                mixed[onset+i]+=1.45*muffled*fade*(contact==1?.83:1.);
            }
        }
        std::vector<std::int16_t> result(mixed.size());
        std::transform(mixed.begin(),mixed.end(),result.begin(),[](double value){return pcm(value);});
        result.front()=result.back()=0;
        return result;
    }
    const bool vent=event.kind==SoundKind::VentRattle;
    const double duration=vent?2.35:1.28;
    std::vector<std::int16_t> result(std::size_t(duration*AUDIO_SAMPLE_RATE));
    std::uint32_t state=event.variation*747796405u+2891336453u;
    if(!state)state=1;
    double body=0,soft=0;
    for(std::size_t i=0;i<result.size();++i) {
        const double t=double(i)/AUDIO_SAMPLE_RATE,n=noise(state);
        body+=.024*(n-body);soft+=(vent?.22:.12)*(n-soft);
        // Filtered air and a loose duct settling; no pitched electrical chord
        // or metallic impulses. The rustle is a low, short exhale and drag,
        // distinct from the creature's detection scream and pursuit roar.
        const double swell=vent?std::pow(std::sin(PI*t/duration),1.4):
            std::exp(-std::pow((t-.39)/.28,2))+.45*std::exp(-std::pow((t-.88)/.20,2));
        const double texture=vent?(.72+.28*std::pow(std::sin((8+.4*event.variation)*t),2)):
            (.80+.20*std::sin(t*29)*std::sin(t*19));
        const double envelope=std::min({1.,t/.09,(duration-t)/.18});
        const double value=vent?(.28*body+.24*(soft-body)):(1.375*body+.625*(soft-body));
        result[i]=pcm(value*swell*texture*envelope);
    }
    result.front()=result.back()=0;
    return result;
}
}

std::vector<std::int16_t> readFootstepWav(const std::string& path) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if (!file) return {};
    const auto size=file.tellg();
    // Clips are tiny; bound allocation before trusting any RIFF chunk length.
    if (size<44 || size>2*1024*1024) return {};
    std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(bytes.data()),size)) return {};
    if (!tag(bytes,0,"RIFF") || !tag(bytes,8,"WAVE") || read32(bytes,4)!=bytes.size()-8) return {};
    bool format=false,data=false;
    std::size_t dataOffset=0,dataBytes=0,offset=12;
    while (offset<bytes.size()) {
        if (bytes.size()-offset<8) return {};
        const auto chunkSize=static_cast<std::size_t>(read32(bytes,offset+4));
        const auto payload=offset+8;
        if (chunkSize>bytes.size()-payload) return {};
        if (tag(bytes,offset,"fmt ")) {
            if (format || chunkSize<16 || read16(bytes,payload)!=1 || read16(bytes,payload+2)!=1
                || read32(bytes,payload+4)!=AUDIO_SAMPLE_RATE || read32(bytes,payload+8)!=AUDIO_SAMPLE_RATE*2
                || read16(bytes,payload+12)!=2 || read16(bytes,payload+14)!=16) return {};
            format=true;
        } else if (tag(bytes,offset,"data")) {
            if (data || chunkSize%2 || chunkSize<200 || chunkSize>2*AUDIO_SAMPLE_RATE*2) return {};
            data=true; dataOffset=payload; dataBytes=chunkSize;
        }
        const auto padded=chunkSize+(chunkSize&1);
        if (padded>bytes.size()-payload) return {};
        offset=payload+padded;
    }
    if (!format || !data) return {};
    std::vector<std::int16_t> result(dataBytes/2);
    for (std::size_t i=0;i<result.size();++i) {
        const auto value=read16(bytes,dataOffset+i*2);
        result[i]=static_cast<std::int16_t>(value<32768?int(value):int(value)-65536);
    }
    return result;
}

bool loadMonsterFootsteps(const std::string& directory) {
    std::array<std::vector<std::int16_t>,4> pending;
    for(std::size_t i=0;i<4;++i) {
        pending[i]=readFootstepWav(directory+"/step-0"+std::to_string(i+1)+".wav");
        if(pending[i].empty()) { monsterSteps={}; return false; }
    }
    monsterSteps=std::move(pending); return true;
}
bool loadAraneaSamples(const std::string& directory) {
    AraneaSamples pending;
    const std::array<const char*,4> names={"walk","run","notice","roar"};
    const std::array<std::array<std::vector<std::int16_t>,4>*,4> banks={
        &pending.walk,&pending.run,&pending.notice,&pending.roar};
    for(std::size_t kind=0;kind<banks.size();++kind)for(std::size_t variant=0;variant<4;++variant) {
        auto& clip=(*banks[kind])[variant];
        clip=readFootstepWav(directory+"/"+names[kind]+"-0"+std::to_string(variant+1)+".wav");
        if(clip.empty()){araneaSamples={};return false;}
    }
    araneaSamples=std::move(pending);return true;
}
bool loadVerityPickupSample(const std::string& path) {
    verityPickup=readFootstepWav(path);return !verityPickup.empty();
}
bool loadPartygoerJumpscareSample(const std::string& path) {
    partygoerScream=readFootstepWav(path);return !partygoerScream.empty();
}
bool loadJumpscareSample(const std::string& path) {
    recordedScream=readFootstepWav(path); return !recordedScream.empty();
}

bool loadFootstepSamples(const std::string& directory) {
    std::array<std::vector<std::int16_t>,FOOTSTEP_COUNT> pending;
    for (std::size_t i=0;i<FOOTSTEP_COUNT;++i) {
        pending[i]=readFootstepWav(directory+"/step-0"+std::to_string(i+1)+".wav");
        if (pending[i].empty()) { recordedSteps={}; loadedSteps=0; return false; }
    }
    recordedSteps=std::move(pending); loadedSteps=FOOTSTEP_COUNT;
    return true;
}
std::size_t recordedFootstepCount() { return loadedSteps; }

std::vector<std::int16_t> synthesizeSound(const SoundEvent& input) {
    SoundEvent event=input;
    if(cinematicSound(event.kind))return cinematicClip(event);
    if(event.kind==SoundKind::VentRattle||event.kind==SoundKind::DistantFootsteps||event.kind==SoundKind::ThreatRustle)
        return atmosphereSound(event);
    if(event.kind==SoundKind::DarknessJumpscare) {
        // Original electrical shriek for the garage: bass impact, dissonant
        // falling tones and a torn, band-limited hiss. No borrowed scream.
        constexpr double duration=1.7;
        std::vector<std::int16_t> clip(std::size_t(duration*AUDIO_SAMPLE_RATE));
        std::uint32_t state=event.variation*747796405u+2891336453u;
        double low=0,body=0,phase=0;
        const double pitch=.97+.02*(event.variation%4);
        for(std::size_t i=0;i<clip.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE,n=noise(state);
            low+=.24*(n-low);body+=.025*(n-body);
            phase+=2*PI*pitch*(170+930*std::exp(-t*3.8))/AUDIO_SAMPLE_RATE;
            const double attack=std::min(1.,t/.004);
            const double release=std::clamp((duration-t)/.18,0.,1.);
            const double impact=.43*std::sin(2*PI*(82*t-16*t*t))*std::exp(-t*13);
            const double torn=(.62+.38*std::pow(std::sin(t*81),2))*std::exp(-t*2.6);
            const double shriek=(.23*std::sin(phase)+.14*std::sin(phase*1.413))*std::exp(-t*3.1);
            const double hiss=.50*(low-body)*torn+.07*(n-low)*std::exp(-t*9);
            clip[i]=pcm(.72*std::tanh(2.1*(impact+shriek+hiss))*attack*release);
        }
        clip.front()=clip.back()=0;return clip;
    }
    if(event.kind==SoundKind::PresentOpen) {
        // Low-pass paper rustle and a padded cardboard lid, without sharp hiss.
        std::vector<std::int16_t> clip(std::size_t(.58*AUDIO_SAMPLE_RATE));
        std::uint32_t state=event.variation*747796405u+2891336453u;
        double low=0,soft=0;
        for(std::size_t i=0;i<clip.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE;
            low+=.06*(noise(state)-low);soft+=.09*(low-soft);
            const double paper=std::exp(-std::pow((t-.20)/.14,2));
            const double lid=.08*std::sin(2*PI*125*t)*std::exp(-std::pow((t-.33)/.07,2));
            const double edge=std::min({1.,t/.025,(.58-t)/.05});
            clip[i]=pcm((.65*soft*paper+lid)*edge);
        }
        clip.front()=clip.back()=0;return clip;
    }
    if(event.kind==SoundKind::MouseAmbient) {
        std::vector<std::int16_t> clip(std::size_t(.65*AUDIO_SAMPLE_RATE));
        std::uint32_t state=event.variation*747796405u+2891336453u;
        double phase=0,soft=0;
        for(std::size_t i=0;i<clip.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE;
            phase+=2*PI*(1850+350*std::sin(t*12)+45*(event.variation%4))/AUDIO_SAMPLE_RATE;
            soft+=.24*(noise(state)-soft);
            const double chirp=std::exp(-std::pow((t-.20)/.045,2));
            const double rustle=std::exp(-std::pow((t-.42)/.13,2));
            const double edges=std::min({1.,t/.015,(.65-t)/.04});
            clip[i]=pcm((.14*std::sin(phase)*chirp+.075*soft*rustle)*edges);
        }
        clip.front()=clip.back()=0;return clip;
    }
    if(event.kind==SoundKind::MouseSqueak) {
        std::vector<std::int16_t> clip(std::size_t(.48*AUDIO_SAMPLE_RATE));
        double phase=0;
        for(std::size_t i=0;i<clip.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE,local=std::fmod(t,.24);
            const double envelope=local<.18?std::pow(std::sin(PI*local/.18),2):0;
            const double frequency=2400+900*std::sin(PI*local/.24)+35*(event.variation%4);
            phase+=2*PI*frequency/AUDIO_SAMPLE_RATE;
            clip[i]=pcm(.35*envelope*(std::sin(phase)+.18*std::sin(phase*2)));
        }
        clip.front()=clip.back()=0;return clip;
    }
    if(event.kind==SoundKind::VerityPickup)return verityPickup;
    if(event.kind==SoundKind::PartygoerJumpscare) {
        if(!partygoerScream.empty())return partygoerScream;
        return threatSound({SoundKind::Jumpscare});
    }
    if(event.kind==SoundKind::PartygoerStep) {
        // Recorded shoes with a softened attack and carpet-like high-frequency
        // absorption. No pitched oscillators: these must read as footsteps.
        constexpr double duration=.22;
        std::vector<std::int16_t> clip(std::size_t(duration*AUDIO_SAMPLE_RATE));
        const auto& source=recordedSteps[event.variation%FOOTSTEP_COUNT];
        std::size_t onset=0;
        if(!source.empty()) {
            int peak=0;for(auto sample:source)peak=std::max(peak,std::abs(int(sample)));
            while(onset<source.size() && std::abs(int(source[onset]))<peak*.035)++onset;
            onset=onset>44?onset-44:0;
        }
        std::uint32_t state=event.variation*747796405u+2891336453u;
        double squareSum=0,soft=0,body=0;
        for(std::size_t i=0;i<clip.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE;
            const double input=source.empty()?noise(state)*std::exp(-t*22):
                onset+i<source.size()?source[onset+i]/32768.:0;
            soft+=.22*(input-soft);body+=.045*(soft-body);
            const double fade=std::min({1.,t/.006,(duration-t)/.04});
            clip[i]=pcm((soft*.8+body*.2)*fade);
            squareSum+=double(clip[i])*clip[i];
        }
        // Match loudness across variants while retaining mixer headroom.
        const double rms=std::sqrt(squareSum/clip.size());
        const double gain=rms>0?8500/rms:1;
        for(auto& value:clip)value=pcm(std::clamp(value*gain/32768.,-.9,.9));
        clip.front()=clip.back()=0;return clip;
    }
    if(event.kind==SoundKind::Footstep) event.surface=Surface::Carpet;
    if (event.kind==SoundKind::ThreatNotice || event.kind==SoundKind::ThreatStep || event.kind==SoundKind::ThreatRunStep || event.kind==SoundKind::Jumpscare || event.kind==SoundKind::ThreatRoar)
        return threatSound(event);
    if (event.kind!=SoundKind::Footstep && event.kind!=SoundKind::ClothRustle)
        return environmentalSound(event);
    if (event.kind==SoundKind::Footstep && loadedSteps==FOOTSTEP_COUNT) return recordedSound(event);
    const bool rustle=event.kind==SoundKind::ClothRustle;
    const bool crouch=event.gait==Gait::Crouch;
    const bool sprint=event.gait==Gait::Sprint;
    const bool hard=event.surface==Surface::HardFloor;
    const double duration=rustle?.22:(crouch?.34:(sprint?.30:.36));
    const int count=static_cast<int>(duration*AUDIO_SAMPLE_RATE);
    std::vector<std::int16_t> result(static_cast<std::size_t>(count));
    std::uint32_t state=event.variation*747796405u+2891336453u;
    if (!state) state=1;
    double smooth=0,slow=0;
    const double pitch=.94+.12*double((event.variation*37u)%101)/100;
    const double amplitude=rustle?.046:(crouch?.070:(sprint?.33:.24));
    for (int i=0;i<count;++i) {
        const double t=double(i)/AUDIO_SAMPLE_RATE;
        const double white=noise(state);
        smooth+=(hard?.30:(event.surface==Surface::DampCarpet?.065:.10))*(white-smooth);
        slow+=.025*(white-slow);
        const double attack=std::min(1.0,t/(rustle?.015:.006));
        const double release=std::min(1.0,(duration-t)/.018);
        double value;
        if (rustle) {
            const double envelope=std::pow(std::sin(PI*t/duration),1.7);
            value=amplitude*(smooth-slow)*envelope*2.2;
        } else {
            // A low heel impact carries weight, followed by a quiet sole drag.
            // A soft attack and long decay avoid sharp, rapid-fire clicks.
            const double decay=std::exp(-t*(crouch?21:18));
            const double body=std::sin(2*PI*pitch*((hard?86:57)*t-28*t*t));
            const double heel=std::sin(2*PI*690*pitch*t)*std::exp(-t*105);
            const double scuff=std::exp(-std::pow((t-.145)/.070,2));
            value=amplitude*((hard?.56:.82)*body*decay
                +(hard?.42:.37)*smooth*(decay+.65*scuff)+(hard?.19:.018)*heel);
            if (event.surface==Surface::DampCarpet)
                value+=amplitude*.65*smooth*std::exp(-std::pow((t-.075)/.06,2));
        }
        result[static_cast<std::size_t>(i)]=pcm(value*attack*release);
    }
    result.front()=0; result.back()=0;
    return result;
}

void ThreatVocalCadence::reset() { *this={}; }
void ThreatVocalCadence::keyCollected() { pendingKeyRoar_=true;roarCooldown_=7; }
std::vector<SoundKind> ThreatVocalCadence::update(float dt,bool active,bool visible,bool enraged,bool paused,bool alive) {
    if(paused)return {};
    if(!alive){wasVisible_=pendingNotice_=pendingKeyRoar_=false;visibleRepeat_=7;return {};}
    dt=std::isfinite(dt)?std::max(0.f,dt):0.f;
    noticeCooldown_=std::max(0.f,noticeCooldown_-dt);
    vocalCooldown_=std::max(0.f,vocalCooldown_-dt);
    if(enraged)roarCooldown_=std::max(0.f,roarCooldown_-dt);
    if(!active){wasVisible_=pendingNotice_=false;visibleRepeat_=7;return {};}
    // Reacquisition inside the cooldown stays queued while sight is held.
    // Before the key, sustained pursuit also gets a call every seven seconds;
    // after the key the existing rage roar supplies that periodic cue instead.
    if(visible&&!wasVisible_)pendingNotice_=true;
    if(visible)visibleRepeat_=std::max(0.f,visibleRepeat_-dt);
    else { pendingNotice_=false;visibleRepeat_=7; }
    wasVisible_=visible;
    if(vocalCooldown_>0)return {};
    if(visible&&noticeCooldown_<=0&&(pendingNotice_||(!enraged&&visibleRepeat_<=0))) {
        pendingNotice_=false;noticeCooldown_=5;visibleRepeat_=7;vocalCooldown_=2;
        return {SoundKind::ThreatNotice};
    }
    if(pendingKeyRoar_||(enraged&&roarCooldown_<=0)) {
        pendingKeyRoar_=false;roarCooldown_=7;vocalCooldown_=2;
        return {SoundKind::ThreatRoar};
    }
    return {};
}

void FootstepCadence::reset(bool crouched) {
    distance_=0; crouched_=crouched; walking_=false;
    previousGait_=crouched?Gait::Crouch:Gait::Walk; variation_=1;
}

std::vector<SoundEvent> FootstepCadence::update(float dt,float actualDistance,
                                              bool crouched,bool sprinting,
                                              Surface surface,bool paused) {
    std::vector<SoundEvent> events;
    const Gait gait=crouched?Gait::Crouch:(sprinting?Gait::Sprint:Gait::Walk);
    if (paused || !std::isfinite(dt) || dt<=0) {
        distance_=0; walking_=false; crouched_=crouched; previousGait_=gait;
        return events;
    }
    if (crouched!=crouched_) {
        events.push_back({SoundKind::ClothRustle,gait,surface,variation_++});
        crouched_=crouched;
    }
    if (!std::isfinite(actualDistance) || actualDistance<=.00001f) {
        distance_=0; walking_=false; previousGait_=gait;
        return events;
    }
    // A teleport/reset is not a footstep, and long frames cannot queue a burst.
    if (actualDistance>std::max(.5f,dt*12.0f)) {
        distance_=0; walking_=false; previousGait_=gait;
        return events;
    }
    if (previousGait_!=gait) distance_*=stride(gait)/stride(previousGait_);
    previousGait_=gait;
    distance_+=actualDistance;
    double needed=walking_?stride(gait):.20;
    int steps=0;
    while (distance_>=needed && steps<3) {
        distance_-=needed; walking_=true; needed=stride(gait);
        events.push_back({SoundKind::Footstep,gait,surface,variation_++});
        ++steps;
    }
    if (steps==3) distance_=std::fmod(distance_,stride(gait));
    return events;
}

StereoGain positionalGains(const AudioScene& scene,AudioPosition source,float nearDistance,float farDistance) {
    const double dx=source.x-scene.listener.x,dy=source.y-scene.listener.y,dz=source.z-scene.listener.z;
    const double distance=std::sqrt(dx*dx+dy*dy+dz*dz);
    if (!std::isfinite(distance) || distance>=farDistance || nearDistance<=0 || farDistance<=nearDistance) return {};
    const double horizontal=std::hypot(scene.forward.x,scene.forward.z);
    const double fx=horizontal>1e-6?scene.forward.x/horizontal:0;
    const double fz=horizontal>1e-6?scene.forward.z/horizontal:-1;
    const double pan=distance>1e-6?std::clamp((-fz*dx+fx*dz)/distance,-1.0,1.0):0;
    const double facing=distance>1e-6?(dx*fx+dz*fz)/distance:1;
    const double gain=(nearDistance/std::max(double(nearDistance),distance))
        *(1-distance*distance/(farDistance*farDistance))*(facing<0?.86:1.0);
    return {float(gain*std::sqrt((1-pan)*.5)),float(gain*std::sqrt((1+pan)*.5))};
}

struct AudioMixer::Impl {
    // Fixed voice slots and pre-generated clips bound CPU/memory use at runtime.
    static constexpr std::size_t MAX_VOICES=12;
    struct Voice {
        const std::vector<std::int16_t>* clip=nullptr;
        std::size_t cursor=0;
        AudioPosition position{};
        bool positional=false;
        float gain=1;
        bool occluded=false;
        float nearDistance=3;
        unsigned priority=0;
        std::uint64_t started=0;
        float transmission=1;
        bool atmosphere=false;
    };
    std::array<std::array<std::array<std::vector<std::int16_t>,6>,3>,3> steps;
    std::array<std::vector<std::int16_t>,3> cloth;
    std::array<std::array<std::vector<std::int16_t>,4>,POSITIONED_CLIP_COUNT> ambience;
    std::array<float,AUDIO_SAMPLE_RATE> humTable{},droneTable{};
    std::array<Voice,MAX_VOICES> voices{};
    AudioScene scene{};
    std::uint32_t random=1,seed=1,variation=0;
    std::array<std::uint32_t,2> monsterVariation{};
    std::uint64_t sample=0,nextAmbient=0,lastStinger=0,voiceSequence=0;
    float volume=1,currentVolume=1,humLeft=0,humRight=0,currentTension=0;
    float atmosphereGain=1;
    bool paused=false,tensionArmed=true,captureOnly=false;

    Impl() {
        for (std::size_t gait=0;gait<3;++gait) {
            cloth[gait]=synthesizeSound({SoundKind::ClothRustle,Gait(gait),Surface::Carpet,1});
            for (std::size_t surface=0;surface<3;++surface)
                for (std::size_t index=0;index<6;++index)
                    steps[gait][surface][index]=synthesizeSound({SoundKind::Footstep,Gait(gait),Surface(surface),std::uint32_t(index+1)});
        }
        for (std::size_t kind=0;kind<ambience.size();++kind)
            for (std::size_t index=0;index<4;++index)
                ambience[kind][index]=synthesizeSound({SoundKind(int(SoundKind::AmbientCreak)+int(kind)),Gait::Walk,Surface::Carpet,std::uint32_t(index+1)});
        if(!recordedScream.empty()) {
            const auto index=std::size_t(SoundKind::Jumpscare)-std::size_t(SoundKind::AmbientCreak);
            for(std::size_t variant=0;variant<4;++variant) {
                const double pitch=.96+.026*variant;
                auto& clip=ambience[index][variant];
                clip.resize(std::size_t(2*AUDIO_SAMPLE_RATE));
                for(std::size_t i=0;i<clip.size();++i) {
                    const double t=double(i)/AUDIO_SAMPLE_RATE, position=i*pitch;
                    const auto a=std::size_t(position);
                    double roar=0;
                    if(a+1<recordedScream.size()) roar=(recordedScream[a]*(1-(position-a))+recordedScream[a+1]*(position-a))/32767.;
                    const double impact=.40*std::cos(2*PI*(58*t-12*t*t))*std::exp(-t*11);
                    clip[i]=pcm((roar*1.4+impact)*std::min(1.,(2-t)/.07));
                }
            }
        }
        for (std::size_t i=0;i<humTable.size();++i) {
            const double t=double(i)/AUDIO_SAMPLE_RATE;
            humTable[i]=float(.017*std::sin(2*PI*60*t)+.008*std::sin(2*PI*120*t)+.004*std::sin(2*PI*240*t));
            droneTable[i]=float(.015*std::sin(2*PI*37*t)+.011*std::sin(2*PI*41*t)+.005*std::sin(2*PI*73*t));
        }
        restart(1);
    }
    double randomUnit() { return (noise(random)+1)*.5; }
    void restart(std::uint32_t value) {
        seed=value?value:1; random=seed; variation=0; monsterVariation={}; sample=0;
        voices={}; voiceSequence=0; captureOnly=false; humLeft=humRight=currentTension=0;
        atmosphereGain=scene.atmosphereQuiet?0.f:1.f;
        currentVolume=volume; tensionArmed=scene.tension<.65f; lastStinger=0;
        nextAmbient=std::uint64_t((5+randomUnit()*5)*AUDIO_SAMPLE_RATE);
    }
    void add(const std::vector<std::int16_t>& clip,AudioPosition position,bool positional,float gain,
             bool occluded=false,float nearDistance=3,unsigned priority=0,float transmission=1,bool atmosphere=false) {
        if (paused || clip.empty() || !std::isfinite(gain) || gain<=0 || !std::isfinite(transmission) || transmission<=0) return;
        Voice* slot=nullptr;
        for (auto& voice:voices) if (!voice.clip) {
            slot=&voice;break;
        }
        // Calls cannot disappear behind a full queue of feet and ambience.
        // Equal/higher priority voices stay intact; capture owns its own mix.
        if(!slot)for(auto& voice:voices)if(voice.priority<priority&&
            (!slot||voice.priority<slot->priority||
             (voice.priority==slot->priority&&voice.started<slot->started)))slot=&voice;
        if(slot)*slot={&clip,0,position,positional,std::min(gain,1.0f),occluded,
                       nearDistance,priority,voiceSequence++,std::min(transmission,1.f),atmosphere};
    }
    void environmental(SoundKind kind,AudioPosition position,float gain,float transmission=1) {
        const auto index=int(kind)-int(SoundKind::AmbientCreak);
        if (index<0 || std::size_t(index)>=ambience.size() || paused || !std::isfinite(gain) || gain<=0 ||
            !std::isfinite(transmission) || transmission<=0) return;
        if(scene.cinematic && !cinematicSound(kind) && kind!=SoundKind::Door &&
           kind!=SoundKind::ElectricalStinger) return;
        const bool capture=captureSound(kind);
        // Catch feedback must not be dropped by a full ambience queue, nor pile
        // on top of twelve existing sources. It belongs at the camera.
        if(captureOnly && !capture) return;
        if (capture) { voices={}; captureOnly=true; humLeft=humRight=currentTension=0; currentVolume=volume; }
        const bool partyStep=kind==SoundKind::PartygoerStep;
        // Partygoers usually approach out of sight, behind several maze walls.
        // A direct ray can fall to .38^4: retain a diffuse footfall contribution
        // so the threat can be heard while hidden, with distance and panning intact.
        if(partyStep){transmission=.4f+.6f*std::clamp(transmission,0.f,1.f);gain*=.6f;}
        const bool pickup=kind==SoundKind::VerityPickup;
        const bool monsterStep=kind==SoundKind::ThreatStep||kind==SoundKind::ThreatRunStep||kind==SoundKind::PartygoerStep;
        const bool vocal=kind==SoundKind::ThreatRoar||kind==SoundKind::ThreatNotice;
        const bool distant=kind==SoundKind::VentRattle||kind==SoundKind::DistantFootsteps||kind==SoundKind::ThreatRustle;
        const bool atmosphere=distant||(scene.level0Atmosphere&&(kind==SoundKind::AmbientCreak||kind==SoundKind::WaterDrip));
        const std::size_t variant=capture?std::min(std::size_t(randomUnit()*4),std::size_t(3)):
            monsterStep?monsterVariation[kind==SoundKind::ThreatRunStep?1:0]++%4:variation++%4;
        add(ambience[std::size_t(index)][variant],position,!capture&&!cinematicSound(kind)&&!pickup,gain,
            (monsterStep&&!partyStep)||vocal,partyStep?12.f:vocal?7.f:(distant?6.f:3.f),cinematicSound(kind)||pickup?2u:(vocal||partyStep?1u:0u),transmission,atmosphere);
    }
    void schedule() {
        if(captureOnly||scene.cinematic||scene.level0Atmosphere) return;
        if (sample>=nextAmbient) {
            const double angle=randomUnit()*PI*2,distance=14+randomUnit()*18;
            const AudioPosition source{scene.listener.x+std::cos(angle)*distance,
                .25+randomUnit()*3,scene.listener.z+std::sin(angle)*distance};
            environmental(randomUnit()<.52?SoundKind::AmbientCreak:SoundKind::WaterDrip,source,.8f);
            nextAmbient=sample+std::uint64_t((9+randomUnit()*13)*AUDIO_SAMPLE_RATE);
        }
        if (scene.tension<.38f) tensionArmed=true;
        // Hysteresis and a 14-second cooldown prevent repeatedly firing on flicker.
        if (tensionArmed && scene.tension>.68f && sample>2*AUDIO_SAMPLE_RATE
            && (lastStinger==0 || sample-lastStinger>14*AUDIO_SAMPLE_RATE)) {
            environmental(SoundKind::ElectricalStinger,scene.fluorescent,.72f);
            tensionArmed=false; lastStinger=sample;
        }
    }
};

AudioMixer::AudioMixer():impl_(std::make_unique<Impl>()) {}
AudioMixer::~AudioMixer()=default;
void AudioMixer::setScene(const AudioScene& scene) {
    if(scene.cinematic!=impl_->scene.cinematic) {
        impl_->voices={};impl_->captureOnly=false;
        impl_->humLeft=impl_->humRight=impl_->currentTension=0;
    }
    impl_->scene=scene;
    auto bounded=[](float value) { return std::isfinite(value)?std::clamp(value,0.f,1.f):0.f; };
    impl_->scene.lightPower=bounded(scene.lightPower);
    impl_->scene.lightInstability=bounded(scene.lightInstability);
    impl_->scene.tension=bounded(scene.tension);
    impl_->scene.threatTransmission=bounded(scene.threatTransmission);
}
void AudioMixer::setVolume(float volume) {
    impl_->volume=std::isfinite(volume)?std::clamp(volume,0.f,1.f):0;
}
void AudioMixer::reset(std::uint32_t seed) { impl_->restart(seed); }
void AudioMixer::setPaused(bool paused) {
    if (paused!=impl_->paused) {
        // The finale clock pauses too: preserve its motor/door cue cursor.
        if(!impl_->scene.cinematic)impl_->voices={};
        impl_->humLeft=impl_->humRight=impl_->currentTension=0;
        // Do not treat a high tension value preserved across a menu as a new event.
        impl_->tensionArmed=impl_->scene.tension<.65f;
    }
    impl_->paused=paused;
}
void AudioMixer::enqueue(const SoundEvent& event) {
    if(impl_->captureOnly) return;
    const auto gait=std::min<std::size_t>(std::size_t(event.gait),2);
    const auto surface=std::min<std::size_t>(std::size_t(event.surface),2);
    if (event.kind==SoundKind::Footstep)
        impl_->add(impl_->steps[gait][surface][(event.variation-1)%6],{},false,1);
    else if (event.kind==SoundKind::ClothRustle) impl_->add(impl_->cloth[gait],{},false,1);
}
void AudioMixer::play(SoundKind kind,AudioPosition position,float gain,float transmission) { impl_->environmental(kind,position,gain,transmission); }
std::size_t AudioMixer::activeVoices() const {
    std::size_t result=0;
    for (const auto& voice:impl_->voices) result+=voice.clip!=nullptr;
    return result;
}
void AudioMixer::render(std::int16_t* output,std::size_t frames) {
    if (!output) return;
    auto& m=*impl_;
    if (m.paused) { std::fill_n(output,frames*2,0); return; }
    // Gains are block constants; hum and volume ease over samples to avoid clicks
    // when the nearest fixture or settings change. Footsteps remain listener-local.
    const auto humGain=positionalGains(m.scene,m.scene.fluorescent,2,28);
    for (std::size_t offset=0;offset<frames;) {
        // Fixed control ticks make event scheduling independent of render block size.
        if (m.sample%128==0) m.schedule();
        const std::size_t count=std::min(frames-offset,std::size_t(128-m.sample%128));
        std::array<StereoGain,Impl::MAX_VOICES> gains{};
        for (std::size_t voice=0;voice<m.voices.size();++voice)
            if (m.voices[voice].clip) gains[voice]=m.voices[voice].positional
                ?positionalGains(m.scene,m.voices[voice].position,m.voices[voice].nearDistance,64):StereoGain{.70710678f,.70710678f};
        const double time=double(m.sample-m.sample%128)/AUDIO_SAMPLE_RATE;
        const float electrical=float(.91+.09*std::sin(time*2*PI*.27));
        const float dying=float(1-m.scene.lightInstability*(.30+.26*std::sin(time*2*PI*7.3)));
        const float humPower=m.scene.lightPower*electrical*dying;
        for (std::size_t i=0;i<count;++i) {
            m.humLeft+=.001f*(humGain.left*humPower-m.humLeft);
            m.humRight+=.001f*(humGain.right*humPower-m.humRight);
            m.currentTension+=.000018f*(m.scene.tension-m.currentTension);
            m.currentVolume+=.001f*(m.volume-m.currentVolume);
            // ~120ms to duck an already-playing hint by95%; release is
            // gentler. Muted voices still advance and cannot replay later.
            m.atmosphereGain+=(m.scene.atmosphereQuiet?.00114f:.0005f)*
                ((m.scene.atmosphereQuiet?0.f:1.f)-m.atmosphereGain);
            const auto table=std::size_t(m.sample%AUDIO_SAMPLE_RATE);
            const double drone=m.droneTable[table]*m.currentTension*m.currentTension;
            double left=m.humTable[table]*m.humLeft+drone;
            double right=m.humTable[table]*m.humRight+drone;
            if(m.captureOnly||m.scene.cinematic) left=right=0;
            for (std::size_t v=0;v<m.voices.size();++v) {
                auto& voice=m.voices[v];
                if (!voice.clip) continue;
                const double sound=(*voice.clip)[voice.cursor++]/32767.0*voice.gain*
                    (voice.atmosphere?m.atmosphereGain:1.f);
                const float occlusion=voice.transmission*(voice.occluded?m.scene.threatTransmission:1.f);
                left+=sound*gains[v].left*occlusion; right+=sound*gains[v].right*occlusion;
                if (voice.cursor==voice.clip->size()) voice.clip=nullptr;
            }
            // Soft limiting preserves headroom even when several sources overlap.
            auto limited=[&](double value) { return pcm(.88*value/(1+std::abs(value)) * m.currentVolume); };
            output[(offset+i)*2]=limited(left); output[(offset+i)*2+1]=limited(right);
            ++m.sample;
        }
        offset+=count;
    }
}

bool writeAudioPreview(const std::string& path) {
    // 0-3s carpet walk, 3-6s damp crouch, 6-9s tile sprint, then environmental
    // tension. Left/right creaks and drips make stereo position easy to audition.
    constexpr int seconds=12;
    std::vector<std::int16_t> output(seconds*AUDIO_SAMPLE_RATE*2);
    AudioMixer mixer;
    AudioScene scene;
    scene.listener={0,1.65,0}; scene.fluorescent={-2,3.2,-2};
    mixer.setScene(scene); mixer.reset(1989);
    FootstepCadence cadence;
    std::size_t cursor=0;
    for (int frame=0;frame<seconds*60;++frame) {
        const double time=frame/60.0;
        const bool crouched=time>=3 && time<6,sprinting=time>=6 && time<9;
        const auto surface=time<3?Surface::Carpet:(time<6?Surface::DampCarpet:Surface::HardFloor);
        const float speed=time>=9?0:(crouched?CROUCH_SPEED:(sprinting?SPRINT_SPEED:WALK_SPEED));
        for (const auto& event:cadence.update(1.f/60,speed/60,crouched,sprinting,surface)) mixer.enqueue(event);
        if (frame==60) mixer.play(SoundKind::AmbientCreak,{-4,1,-1});
        if (frame==210) mixer.play(SoundKind::WaterDrip,{4,1,-1});
        if (frame==510) mixer.play(SoundKind::Door,{3,1,-2});
        if (frame==540) { scene.tension=.95f; scene.lightInstability=1; mixer.setScene(scene); }
        const auto end=std::size_t((frame+1)*AUDIO_SAMPLE_RATE/60);
        mixer.render(output.data()+cursor*2,end-cursor); cursor=end;
    }
    std::ofstream file(path,std::ios::binary);
    if (!file) return false;
    const auto bytes=static_cast<std::uint32_t>(output.size()*2);
    file.write("RIFF",4); little32(file,36+bytes); file.write("WAVEfmt ",8);
    little32(file,16); little16(file,1); little16(file,2);
    little32(file,AUDIO_SAMPLE_RATE); little32(file,AUDIO_SAMPLE_RATE*4);
    little16(file,4); little16(file,16); file.write("data",4); little32(file,bytes);
    for (std::size_t i=0;i<output.size();++i) {
        const double fade=std::min(1.0,double(output.size()-1-i)/(AUDIO_SAMPLE_RATE*.2));
        little16(file,static_cast<std::uint16_t>(std::int16_t(std::lround(output[i]*fade))));
    }
    return static_cast<bool>(file);
}

struct Audio::Impl {
    AudioMixer mixer;
    bool cinematic=false;
    bool paused=false;
#ifdef _WIN32
    static constexpr std::size_t BUFFER_COUNT=4,BUFFER_FRAMES=512;
    struct Buffer {
        WAVEHDR header{};
        std::array<std::int16_t,BUFFER_FRAMES*2> samples{};
        bool prepared=false,queued=false;
    };
    HWAVEOUT device=nullptr;
    std::array<Buffer,BUFFER_COUNT> buffers{};
    std::size_t nextBuffer=0;
    void flush() {
        if (!device) return;
        waveOutReset(device);
        for (auto& buffer:buffers) buffer.queued=false;
        nextBuffer=0;
    }
#endif
    bool service() {
#ifdef _WIN32
        if (!device) return false;
        for (std::size_t n=0;n<BUFFER_COUNT;++n) {
            auto& buffer=buffers[nextBuffer];
            if (buffer.queued && !(buffer.header.dwFlags&WHDR_DONE)) break;
            mixer.render(buffer.samples.data(),BUFFER_FRAMES);
            if (waveOutWrite(device,&buffer.header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR) return false;
            buffer.queued=true; nextBuffer=(nextBuffer+1)%BUFFER_COUNT;
        }
#endif
        return true;
    }
};

Audio::Audio():impl_(std::make_unique<Impl>()) {}
Audio::~Audio() { stop(); }
bool Audio::available() const {
#ifdef _WIN32
    return impl_->device!=nullptr;
#else
    return false;
#endif
}
void Audio::setScene(const AudioScene& scene) { impl_->cinematic=scene.cinematic;impl_->mixer.setScene(scene); }
void Audio::setVolume(float volume) { impl_->mixer.setVolume(volume); }
void Audio::setPaused(bool paused) {
    impl_->mixer.setPaused(paused);
    if(impl_->paused==paused)return;
#ifdef _WIN32
    impl_->flush();
#endif
    impl_->paused=paused;
}
void Audio::play(SoundKind kind,AudioPosition position,float gain,float transmission) {
    if (!available()) return;
    if(captureSound(kind)) {
        impl_->mixer.setPaused(false); impl_->paused=false;
#ifdef _WIN32
        impl_->flush(); // Remove queued ambience before the very first scream sample.
#endif
    }
    impl_->mixer.play(kind,position,gain,transmission);
    if(captureSound(kind) && !impl_->service()) stop();
}
void Audio::reset(std::uint32_t seed) {
    cadence_.reset(); impl_->mixer.reset(seed);
#ifdef _WIN32
    impl_->flush();
#endif
}
void Audio::start() {
#ifdef _WIN32
    if (impl_->device) return;
    WAVEFORMATEX format{};
    format.wFormatTag=WAVE_FORMAT_PCM; format.nChannels=2;
    format.nSamplesPerSec=AUDIO_SAMPLE_RATE; format.wBitsPerSample=16;
    format.nBlockAlign=4; format.nAvgBytesPerSec=AUDIO_SAMPLE_RATE*4;
    if (waveOutOpen(&impl_->device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR) {
        impl_->device=nullptr; return;
    }
    impl_->nextBuffer=0;
    for (auto& buffer:impl_->buffers) {
        buffer.header={}; buffer.queued=false;
        buffer.header.lpData=reinterpret_cast<char*>(buffer.samples.data());
        buffer.header.dwBufferLength=DWORD(buffer.samples.size()*sizeof(std::int16_t));
        if (waveOutPrepareHeader(impl_->device,&buffer.header,sizeof(WAVEHDR))!=MMSYSERR_NOERROR) {
            stop(); return;
        }
        buffer.prepared=true;
    }
    if (!impl_->service()) stop();
#endif
}
void Audio::stop() {
#ifdef _WIN32
    if (impl_->device) {
        waveOutReset(impl_->device);
        for (auto& buffer:impl_->buffers) {
            if (buffer.prepared) waveOutUnprepareHeader(impl_->device,&buffer.header,sizeof(WAVEHDR));
            buffer.prepared=false; buffer.queued=false; buffer.header={};
        }
        waveOutClose(impl_->device); impl_->device=nullptr;
    }
#endif
    cadence_.reset();
    if(impl_->cinematic)impl_->mixer.reset(); // Mute discards cues rather than replaying them late.
    impl_->mixer.setPaused(true); impl_->mixer.setPaused(impl_->paused);
}
void Audio::update(float dt,float actualDistance,bool crouched,bool sprinting,Surface surface,bool paused) {
    const auto events=cadence_.update(dt,actualDistance,crouched,sprinting,surface,paused);
    setPaused(paused);
    if (!available()) return;
    if (!paused) for (const auto& event:events) impl_->mixer.enqueue(event);
    if (!impl_->service()) stop();
}
}
