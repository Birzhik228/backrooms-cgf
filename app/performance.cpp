#include "performance.h"
#include "diagnostics.h"
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

namespace br::app {
ProcessMemory processMemory() {
    ProcessMemory result;
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb=sizeof(counters);
    if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters))) {
        result.resident=counters.WorkingSetSize;result.peakResident=counters.PeakWorkingSetSize;
        result.privateBytes=counters.PrivateUsage;result.residentKnown=result.privateKnown=true;
    }
#elif defined(__linux__)
    std::ifstream status("/proc/self/status");std::string line;
    while(std::getline(status,line)) {
        std::istringstream field(line);std::string key;std::uint64_t kb;
        if(field>>key>>kb) {
            if(key=="VmRSS:"){result.resident=kb*1024;result.residentKnown=true;}
            if(key=="VmHWM:")result.peakResident=kb*1024;
        }
    }
    // Private resident pages on Linux, private committed bytes on Windows.
    // Both are explicitly labelled in the JSON rather than confused with VRAM.
    std::ifstream rollup("/proc/self/smaps_rollup");
    while(std::getline(rollup,line)) {
        std::istringstream field(line);std::string key;std::uint64_t kb;
        if((field>>key>>kb)&&(key=="Private_Clean:"||key=="Private_Dirty:"||key=="Private_Hugetlb:")) {
            result.privateBytes+=kb*1024;result.privateKnown=true;
        }
    }
#endif
    return result;
}
PerformanceMonitor::PerformanceMonitor(const std::filesystem::path& output,double slowThreshold)
    : output_(output),stats_(slowThreshold) {
    samples_.reserve(SAMPLE_LIMIT);
    if(!output_.empty()) {
        if(output_.has_parent_path())std::filesystem::create_directories(output_.parent_path());
        std::ofstream probe(output_);
        if(!probe)throw std::runtime_error("Cannot open performance report: "+output_.string());
    }
}
PerformanceMonitor::~PerformanceMonitor() noexcept {
    if(output_.empty())return;
    try {writeReport();log("Performance report saved: "+output_.string());}
    catch(const std::exception& e){try{log(std::string("Performance report failed: ")+e.what());}catch(...){}}
}
void PerformanceMonitor::record(std::uint64_t frame,double ms,double workMs,double budgetMs,
                               int level,bool cinematic,std::size_t chunks,std::size_t prepared,std::size_t gpuChunks) {
    stats_.add(frame,ms,workMs,budgetMs,level,cinematic);
    peakResources_=std::max(peakResources_,gl::snapshot().totalAlive());
    sample(cinematic?"cinematic":"tick",level,chunks,prepared,gpuChunks,false);
}
void PerformanceMonitor::mark(const std::string& event,int level,std::size_t chunks,std::size_t prepared,std::size_t gpuChunks) {
    sample(event,level,chunks,prepared,gpuChunks,true);
}
void PerformanceMonitor::sample(const std::string& event,int level,std::size_t chunks,std::size_t prepared,std::size_t gpuChunks,bool force) {
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start_).count();
    if(!force&&lastSample_>=0&&seconds-lastSample_<1)return;
    lastSample_=seconds;
    currentMemory_=processMemory();
    peakResident_=std::max({peakResident_,currentMemory_.resident,currentMemory_.peakResident});
    peakPrivate_=std::max(peakPrivate_,currentMemory_.privateBytes);
    const auto resources=gl::snapshot();peakResources_=std::max(peakResources_,resources.totalAlive());
    PerformanceSample value{seconds,level,chunks,prepared,gpuChunks,currentMemory_,resources,event};
    ++totalSamples_;
    if(samples_.size()<SAMPLE_LIMIT)samples_.push_back(std::move(value));
    else{samples_[nextSample_]=std::move(value);nextSample_=(nextSample_+1)%SAMPLE_LIMIT;}
}
std::string PerformanceMonitor::summary() const {
    std::ostringstream out;out<<std::fixed<<std::setprecision(2);
    out<<"Performance: frames "<<stats_.frames().count()<<" / p95 "<<stats_.frames().percentile(.95)
       <<" ms / p99 "<<stats_.frames().percentile(.99)<<" ms / max "<<stats_.frames().maximum()
       <<" ms / slow "<<stats_.slowCount()<<" / peak process RAM "<<peakResident_/1048576.0<<" MiB"
       <<" / peak tracked GL objects "<<peakResources_;
    return out.str();
}
namespace {
void quoted(std::ostream& out,const std::string& value) {
    out<<'"';
    for(unsigned char c:value) {
        if(c=='"'||c=='\\')out<<'\\'<<c;
        else if(c<' ')out<<"\\u00"<<"0123456789abcdef"[c>>4]<<"0123456789abcdef"[c&15];
        else out<<c;
    }
    out<<'"';
}
void numbers(std::ostream& out,const std::array<std::uint64_t,7>& values) {
    out<<'[';for(std::size_t i=0;i<values.size();++i){if(i)out<<',';out<<values[i];}out<<']';
}
void timing(std::ostream& out,const TimingStatistics& t) {
    out<<"{\"mean\":"<<t.mean()<<",\"p50\":"<<t.percentile(.5)<<",\"p95\":"<<t.percentile(.95)
       <<",\"p99\":"<<t.percentile(.99)<<",\"max\":"<<t.maximum()<<'}';
}
}
void PerformanceMonitor::writeReport() const {
    std::ofstream out(output_);if(!out)throw std::runtime_error("Report file became unavailable");
    out.imbue(std::locale::classic());
    out<<std::fixed<<std::setprecision(4);
    const auto final=gl::snapshot();
    out<<"{\n\"schema\":1,\n\"frames\":"<<stats_.frames().count()<<",\n\"duration_seconds\":"
       <<std::chrono::duration<double>(std::chrono::steady_clock::now()-start_).count()<<",\n\"frame_ms\":";
    timing(out,stats_.frames());out<<",\n\"work_ms\":";timing(out,stats_.work());
    out<<",\n\"slow_threshold_ms\":"<<stats_.threshold()<<",\n\"slow_frames\":"<<stats_.slowCount()
       <<",\n\"peak_resident_bytes\":"<<peakResident_<<",\n\"peak_sampled_private_bytes\":"<<peakPrivate_
       <<",\n\"private_memory_kind\":\""
#ifdef _WIN32
       <<"committed"
#else
       <<"resident"
#endif
       <<"\",\n\"peak_observed_gl_objects\":"<<peakResources_
       <<",\n\"resource_kinds\":[\"buffer\",\"vertex_array\",\"texture\",\"framebuffer\",\"program\",\"shader\",\"sync\"]"
       <<",\n\"final_alive\":";numbers(out,final.alive);
    out<<",\n\"created\":";numbers(out,final.created);out<<",\n\"destroyed\":";numbers(out,final.destroyed);
    out<<",\n\"samples_total\":"<<totalSamples_<<",\n\"samples_retained\":"<<samples_.size()<<",\n\"samples\":[";
    for(std::size_t i=0;i<samples_.size();++i) {
        const auto& s=samples_[(nextSample_+i)%samples_.size()];if(i)out<<',';
        out<<"\n{\"seconds\":"<<s.seconds<<",\"level\":"<<s.level<<",\"event\":";quoted(out,s.event);
        out<<",\"resident_bytes\":"<<s.memory.resident<<",\"private_bytes\":"<<s.memory.privateBytes
           <<",\"resident_known\":"<<(s.memory.residentKnown?"true":"false")<<",\"private_known\":"<<(s.memory.privateKnown?"true":"false")
           <<",\"chunks\":"<<s.chunks<<",\"prepared\":"<<s.prepared<<",\"gpu_chunks\":"<<s.gpuChunks<<",\"alive\":";
        numbers(out,s.resources.alive);out<<'}';
    }
    out<<"\n],\n\"worst_frames\":[";
    for(std::size_t i=0;i<stats_.worst().size();++i){const auto& f=stats_.worst()[i];if(i)out<<',';
        out<<"{\"frame\":"<<f.frame<<",\"frame_ms\":"<<f.frameMs<<",\"work_ms\":"<<f.workMs
           <<",\"budget_ms\":"<<f.budgetMs<<",\"level\":"<<f.level<<",\"cinematic\":"<<(f.cinematic?"true":"false")<<'}';}
    out<<"]\n}\n";if(!out)throw std::runtime_error("Performance report write failed");
}
} // namespace br::app
