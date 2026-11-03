#pragma once
#include "common/performance.h"
#include "common/gl_resource.h"
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace br::app {
struct ProcessMemory {
    std::uint64_t resident=0,privateBytes=0,peakResident=0;
    bool residentKnown=false,privateKnown=false;
};
ProcessMemory processMemory();
struct PerformanceSample {
    double seconds=0;
    int level=0;
    std::size_t chunks=0,prepared=0,gpuChunks=0;
    ProcessMemory memory;
    gl::Snapshot resources;
    std::string event;
};
// Declare before GPU members: destruction writes the final counter snapshot
// after all GL owners have unwound, while the external context remains alive.
class PerformanceMonitor {
public:
    PerformanceMonitor(const std::filesystem::path& output,double slowThreshold=50);
    ~PerformanceMonitor() noexcept;
    PerformanceMonitor(const PerformanceMonitor&)=delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&)=delete;
    void record(std::uint64_t frame,double ms,double workMs,double budgetMs,
                int level,bool cinematic,std::size_t chunks,std::size_t prepared,std::size_t gpuChunks);
    void mark(const std::string& event,int level,std::size_t chunks,std::size_t prepared,std::size_t gpuChunks);
    const FrameStatistics& statistics() const { return stats_; }
    const ProcessMemory& memory() const { return currentMemory_; }
    std::uint64_t liveResources() const { return gl::snapshot().totalAlive(); }
    std::string summary() const;
private:
    static constexpr std::size_t SAMPLE_LIMIT=720;
    std::filesystem::path output_;
    FrameStatistics stats_;
    std::chrono::steady_clock::time_point start_=std::chrono::steady_clock::now();
    double lastSample_=-1;
    ProcessMemory currentMemory_;
    std::uint64_t peakResident_=0,peakPrivate_=0,totalSamples_=0,peakResources_=0;
    std::vector<PerformanceSample> samples_;
    std::size_t nextSample_=0;
    void sample(const std::string& event,int level,std::size_t chunks,std::size_t prepared,std::size_t gpuChunks,bool force);
    void writeReport() const;
};
} // namespace br::app
