#include "music.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cwctype>
#include <limits>
#include <system_error>
#include <vector>
#if defined(BACKROOMS_USE_SDL_AUDIO)
#include <SDL.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <digitalv.h>
#endif

namespace br {
struct BackgroundMusic::Impl {
    MusicStatus state=MusicStatus::Missing;
    std::string message;
    float pollElapsed=0;
    std::uint32_t position=0,duration=0,loops=0;
    int appliedVolume=-1;
    bool started=false;
#if defined(BACKROOMS_USE_SDL_AUDIO)
    static constexpr int sampleRate=22050;
    static constexpr std::size_t channels=2,chunkFrames=512,queueChunks=4;
    static constexpr std::size_t frameBytes=channels*sizeof(std::int16_t);
    SDL_AudioDeviceID device=0;
    bool audioInitialized=false;
    std::vector<std::int16_t> samples;
    std::uint64_t submittedFrames=0;

    void closeDevice() {
        if(device) {
            SDL_CloseAudioDevice(device);
            device=0;
        }
        if(audioInitialized) {
            // SDL counts subsystem initializations. Closing the soundtrack must
            // not shut down the independently owned effects output device.
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            audioInitialized=false;
        }
        samples.clear();
        submittedFrames=0;
    }
    bool fail(const char* operation) {
        message=std::string(operation)+": "+SDL_GetError();
        closeDevice();
        state=MusicStatus::Failed;
        return false;
    }
    std::uint64_t cachePosition() {
        const auto queued=static_cast<std::uint64_t>(SDL_GetQueuedAudioSize(device)/frameBytes);
        const auto delivered=submittedFrames-std::min(submittedFrames,queued);
        const auto trackFrames=samples.size()/channels;
        if(trackFrames) {
            position=static_cast<std::uint32_t>((delivered%trackFrames)*1000/sampleRate);
            loops=static_cast<std::uint32_t>(std::min<std::uint64_t>(delivered/trackFrames,
                std::numeric_limits<std::uint32_t>::max()));
        }
        // This is the position delivered to the OS, within one device buffer
        // of the audible position, not a claim of hardware sample accuracy.
        return delivered;
    }
    void pauseAndDiscardPending() {
        SDL_PauseAudioDevice(device,1);
        submittedFrames=cachePosition();
        SDL_ClearQueuedAudio(device);
    }
    bool fillQueue(int level) {
        const auto trackFrames=samples.size()/channels;
        if(!device || !trackFrames)return false;
        std::array<std::int16_t,chunkFrames*channels> chunk{};
        auto queued=SDL_GetQueuedAudioSize(device);
        constexpr auto bytes=static_cast<Uint32>(chunkFrames*frameBytes);
        constexpr auto targetBytes=static_cast<Uint32>(queueChunks*chunkFrames*frameBytes);
        // At most four chunks per update, even if the device drains while we
        // are filling it. Music memory and per-frame work are both bounded.
        for(std::size_t count=0;count<queueChunks && queued<=targetBytes-bytes;++count) {
            for(std::size_t frame=0;frame<chunkFrames;++frame) {
                const auto source=static_cast<std::size_t>((submittedFrames+frame)%trackFrames)*channels;
                for(std::size_t channel=0;channel<channels;++channel)
                    chunk[frame*channels+channel]=static_cast<std::int16_t>(
                        static_cast<int>(samples[source+channel])*level/1000);
            }
            if(SDL_QueueAudio(device,chunk.data(),bytes)<0)return fail("Music queue failed");
            submittedFrames+=chunkFrames;
            queued+=bytes;
        }
        return true;
    }
#elif defined(_WIN32)
    MCIDEVICEID device=0;

    void closeDevice() {
        if (device) {
            // Closing stops playback and releases the file; never wait for the
            // whole track to finish and never send a command to all devices.
            mciSendCommandW(device,MCI_CLOSE,0,0);
            device=0;
        }
    }
    bool check(MCIERROR result,const char* operation) {
        if (!result) return true;
        wchar_t description[256]={};
        mciGetErrorStringW(result,description,256);
        message=std::string(operation)+" (Windows audio error "+std::to_string(result)+")";
        if (description[0]) {
            const int size=WideCharToMultiByte(CP_UTF8,0,description,-1,nullptr,0,nullptr,nullptr);
            if (size>1) {
                std::string detail(static_cast<std::size_t>(size),'\0');
                WideCharToMultiByte(CP_UTF8,0,description,-1,detail.data(),size,nullptr,nullptr);
                detail.pop_back();
                message+=": "+detail;
            }
        }
        closeDevice();
        state=MusicStatus::Failed;
        return false;
    }
    bool setVolume(int volume) {
        if (volume==appliedVolume) return true;
        MCI_DGV_SETAUDIO_PARMSW params{};
        params.dwItem=MCI_DGV_SETAUDIO_VOLUME;
        params.dwValue=static_cast<DWORD>(volume);
        if (!check(mciSendCommandW(device,MCI_SETAUDIO,
                    MCI_DGV_SETAUDIO_ITEM|MCI_DGV_SETAUDIO_VALUE,
                    reinterpret_cast<DWORD_PTR>(&params)),"Music volume failed")) return false;
        appliedVolume=volume;
        return true;
    }
    bool query(DWORD item,DWORD_PTR& value) {
        MCI_STATUS_PARMS params{};
        params.dwItem=item;
        if (!check(mciSendCommandW(device,MCI_STATUS,MCI_STATUS_ITEM,
                    reinterpret_cast<DWORD_PTR>(&params)),"Music status failed")) return false;
        value=params.dwReturn;
        return true;
    }
    bool cachePosition() {
        DWORD_PTR value=0;
        if (!query(MCI_STATUS_POSITION,value)) return false;
        position=static_cast<std::uint32_t>(value);
        return true;
    }
    bool play() {
        MCI_PLAY_PARMS params{};
        // Omitting MCI_FROM resumes from the current position after MCI_PAUSE.
        if (!check(mciSendCommandW(device,MCI_PLAY,0,
                    reinterpret_cast<DWORD_PTR>(&params)),"Music playback failed")) return false;
        started=true;
        state=MusicStatus::Playing;
        pollElapsed=0;
        return true;
    }
#else
    void closeDevice() {}
#endif
};

BackgroundMusic::BackgroundMusic():impl_(std::make_unique<Impl>()) {}
BackgroundMusic::~BackgroundMusic() { impl_->closeDevice(); }

void BackgroundMusic::stop() {
    impl_->closeDevice();
    impl_->state=MusicStatus::Missing;
    impl_->message.clear();
    impl_->position=impl_->duration=impl_->loops=0;
    impl_->pollElapsed=0;
    impl_->appliedVolume=-1;
    impl_->started=false;
}

bool BackgroundMusic::load(const std::filesystem::path& requested) {
    stop();
    auto& data=*impl_;
    auto path=requested;
    std::error_code fsError;
    if (!std::filesystem::is_regular_file(path,fsError)) {
        bool found=false;
#if defined(BACKROOMS_USE_SDL_AUDIO)
        const wchar_t* extensions[]={L".wav"};
#else
        const wchar_t* extensions[]={L".mp3",L".wav",L".m4a"};
#endif
        for (const auto* extension:extensions) {
            auto candidate=requested;
            candidate.replace_extension(extension);
            if (candidate==requested) continue;
            fsError.clear();
            if (std::filesystem::is_regular_file(candidate,fsError)) {
                path=std::move(candidate); found=true; break;
            }
        }
        if (!found) return false;
    }
    auto extension=path.extension().wstring();
    std::transform(extension.begin(),extension.end(),extension.begin(),
                   [](wchar_t character){return static_cast<wchar_t>(std::towlower(character));});
    if (extension!=L".mp3" && extension!=L".wav" && extension!=L".m4a") {
        data.message="Music needs an MP3, WAV, or Windows-supported M4A file.";
        data.state=MusicStatus::Failed;
        return false;
    }
#if defined(BACKROOMS_USE_SDL_AUDIO)
    if(extension!=L".wav") {
        data.message="The Mac soundtrack backend needs a WAV file; the packaged soundtrack is WAV.";
        data.state=MusicStatus::Failed;
        return false;
    }
    constexpr std::size_t maxPcmBytes=256u*1024u*1024u;
    const auto fileBytes=std::filesystem::file_size(path,fsError);
    if(fsError || fileBytes>maxPcmBytes) {
        data.message="The soundtrack could not be read or exceeds the 256 MiB audio limit.";
        data.state=MusicStatus::Failed;
        return false;
    }
    SDL_AudioSpec source{};
    Uint8* raw=nullptr;
    Uint32 rawBytes=0;
    const auto filename=path.u8string();
    if(!SDL_LoadWAV(filename.c_str(),&source,&raw,&rawBytes))return data.fail("Music WAV load failed");
    const std::unique_ptr<Uint8,decltype(&SDL_FreeWAV)> rawOwner(raw,SDL_FreeWAV);
    if(!rawBytes || rawBytes>maxPcmBytes) {
        data.message="The WAV soundtrack has no samples or exceeds the audio limit.";
        data.state=MusicStatus::Failed;
        return false;
    }
    SDL_AudioCVT conversion{};
    const int conversionNeeded=SDL_BuildAudioCVT(&conversion,source.format,source.channels,source.freq,
        AUDIO_S16SYS,static_cast<Uint8>(Impl::channels),Impl::sampleRate);
    if(conversionNeeded<0)return data.fail("Music format conversion failed");
    if(conversion.len_mult<=0 || static_cast<std::size_t>(rawBytes)>maxPcmBytes/static_cast<std::size_t>(conversion.len_mult)) {
        data.message="The converted soundtrack exceeds the audio limit.";
        data.state=MusicStatus::Failed;
        return false;
    }
    std::vector<Uint8> converted(static_cast<std::size_t>(rawBytes)*static_cast<std::size_t>(conversion.len_mult));
    std::memcpy(converted.data(),raw,rawBytes);
    conversion.buf=converted.data();
    conversion.len=static_cast<int>(rawBytes);
    if(conversionNeeded && SDL_ConvertAudio(&conversion)<0)return data.fail("Music sample conversion failed");
    const auto outputBytes=conversionNeeded?static_cast<std::size_t>(conversion.len_cvt):static_cast<std::size_t>(rawBytes);
    if(!outputBytes || outputBytes%Impl::frameBytes || outputBytes>converted.size()) {
        data.message="The WAV soundtrack does not contain complete stereo frames.";
        data.state=MusicStatus::Failed;
        return false;
    }
    data.samples.resize(outputBytes/sizeof(std::int16_t));
    std::memcpy(data.samples.data(),converted.data(),outputBytes);
    data.duration=static_cast<std::uint32_t>((data.samples.size()/Impl::channels)*1000/Impl::sampleRate);
    // GLFW owns the application entry point; no SDL_main wrapper is used.
    SDL_SetMainReady();
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0)return data.fail("Music audio initialization failed");
    data.audioInitialized=true;
    SDL_AudioSpec desired{};
    desired.freq=Impl::sampleRate;
    desired.format=AUDIO_S16SYS;
    desired.channels=static_cast<Uint8>(Impl::channels);
    desired.samples=static_cast<Uint16>(Impl::chunkFrames);
    // No callback: only the game thread reads track data and queues PCM.
    data.device=SDL_OpenAudioDevice(nullptr,0,&desired,nullptr,0);
    if(!data.device)return data.fail("Music output device could not be opened");
    data.state=MusicStatus::Ready; // Newly opened SDL devices are paused.
    return true;
#elif defined(_WIN32)
    const auto absolute=std::filesystem::absolute(path,fsError);
    if (fsError) {
        data.message="The local music path could not be resolved.";
        data.state=MusicStatus::Failed;
        return false;
    }
    auto filename=absolute.wstring();
    wchar_t deviceType[]=L"mpegvideo";
    MCI_DGV_OPEN_PARMSW open{};
    open.lpstrDeviceType=deviceType;
    open.lpstrElementName=filename.data();
    // Force the digital audio/video driver so per-device volume is available
    // for WAV as well as MP3. No alias strings, shell commands, or downloads.
    if (!data.check(mciSendCommandW(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT,
                    reinterpret_cast<DWORD_PTR>(&open)),"Music could not be opened")) return false;
    data.device=open.wDeviceID;
    MCI_SET_PARMS time{};
    time.dwTimeFormat=MCI_FORMAT_MILLISECONDS;
    if (!data.check(mciSendCommandW(data.device,MCI_SET,MCI_SET_TIME_FORMAT,
                    reinterpret_cast<DWORD_PTR>(&time)),"Music time format failed")) return false;
    DWORD_PTR duration=0;
    if (!data.query(MCI_STATUS_LENGTH,duration)) return false;
    if (duration==0) {
        data.closeDevice();
        data.message="The music file contains no playable audio duration.";
        data.state=MusicStatus::Failed;
        return false;
    }
    data.duration=static_cast<std::uint32_t>(duration);
    // A load is always silent, even before the first gameplay update.
    if (!data.setVolume(0)) return false;
    data.state=MusicStatus::Ready;
    return true;
#else
    data.message="Local soundtrack playback uses the Windows multimedia driver.";
    data.state=MusicStatus::Failed;
    return false;
#endif
}

void BackgroundMusic::update(bool paused,bool muted,float volume,float dt) {
    if (!available()) return;
#if defined(BACKROOMS_USE_SDL_AUDIO)
    (void)dt;
    auto& data=*impl_;
    if(!std::isfinite(volume))volume=0;
    const int level=static_cast<int>(std::lround(std::clamp(volume,0.0f,1.0f)*1000));
    if(paused || muted || level==0) {
        if(data.state==MusicStatus::Playing) {
            data.pauseAndDiscardPending();
            data.state=MusicStatus::Paused;
        }
        return;
    }
    const bool resume=data.state!=MusicStatus::Playing;
    const bool changeVolume=level!=data.appliedVolume;
    if(changeVolume) {
        data.pauseAndDiscardPending();
        data.appliedVolume=level;
    }
    if(!data.fillQueue(level))return;
    if(resume || changeVolume)SDL_PauseAudioDevice(data.device,0);
    data.started=true;
    data.state=MusicStatus::Playing;
    data.cachePosition();
#elif defined(_WIN32)
    auto& data=*impl_;
    if (!std::isfinite(volume)) volume=0;
    const int level=static_cast<int>(std::lround(std::clamp(volume,0.0f,1.0f)*1000));
    const bool suspend=paused||muted||level==0;
    if (!data.setVolume(suspend?0:level)) return;
    if (suspend) {
        if (data.state==MusicStatus::Playing) {
            if (!data.check(mciSendCommandW(data.device,MCI_PAUSE,0,0),"Music pause failed")) return;
            if (!data.cachePosition()) return;
            data.state=MusicStatus::Paused;
        }
        return;
    }
    if (data.state!=MusicStatus::Playing) {
        data.play();
        return;
    }
    if (std::isfinite(dt) && dt>0) data.pollElapsed+=std::min(dt,0.25f);
    if (data.pollElapsed<0.25f) return;
    data.pollElapsed=0;
    DWORD_PTR mode=0;
    if (!data.query(MCI_STATUS_MODE,mode) || !data.cachePosition()) return;
    if (mode==MCI_MODE_STOP && data.started) {
        MCI_SEEK_PARMS seek{};
        if (!data.check(mciSendCommandW(data.device,MCI_SEEK,MCI_SEEK_TO_START,
                        reinterpret_cast<DWORD_PTR>(&seek)),"Music loop failed")) return;
        data.position=0;
        ++data.loops;
        data.play();
    }
#else
    (void)paused; (void)muted; (void)volume; (void)dt;
#endif
}

bool BackgroundMusic::available() const {
    return impl_->state==MusicStatus::Ready || impl_->state==MusicStatus::Playing || impl_->state==MusicStatus::Paused;
}
MusicStatus BackgroundMusic::status() const { return impl_->state; }
const std::string& BackgroundMusic::error() const { return impl_->message; }
std::uint32_t BackgroundMusic::positionMilliseconds() const { return impl_->position; }
std::uint32_t BackgroundMusic::durationMilliseconds() const { return impl_->duration; }
std::uint32_t BackgroundMusic::loopCount() const { return impl_->loops; }
}
