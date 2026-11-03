#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace br {
// Fixed-size histogram: no frame history grows with session duration.
// Percentiles are upper bucket bounds (0.25 ms resolution), max is exact.
class TimingStatistics {
public:
    static constexpr double BIN_MS = .25;
    static constexpr std::size_t BINS = 4001;
    void add(double ms) {
        if (!std::isfinite(ms) || ms < 0) return;
        ++count_; sum_ += ms; maximum_ = std::max(maximum_, ms);
        ++bins_[ms >= 1000 ? BINS-1 : std::size_t(ms/BIN_MS)];
    }
    std::uint64_t count() const { return count_; }
    double mean() const { return count_ ? sum_/double(count_) : 0; }
    double maximum() const { return maximum_; }
    double percentile(double fraction) const {
        if (!count_) return 0;
        if (!std::isfinite(fraction)) return 0;
        const auto target=std::max<std::uint64_t>(1, std::uint64_t(std::ceil(std::clamp(fraction,0.,1.)*double(count_))));
        std::uint64_t seen=0;
        for(std::size_t i=0;i<BINS;++i) {
            seen+=bins_[i];
            if(seen>=target) return i==BINS-1 ? maximum_ : std::min(maximum_,(i+1)*BIN_MS);
        }
        return maximum_;
    }
private:
    std::array<std::uint64_t,BINS> bins_{};
    std::uint64_t count_=0;
    double sum_=0,maximum_=0;
};
struct SlowFrame {
    std::uint64_t frame;
    double frameMs,workMs,budgetMs;
    int level;
    bool cinematic;
};
class FrameStatistics {
public:
    static constexpr std::size_t MAX_SLOW_FRAMES=16;
    explicit FrameStatistics(double threshold=50) : threshold_(threshold) {
        if(!std::isfinite(threshold_) || threshold_<=0) threshold_=50;
        worst_.reserve(MAX_SLOW_FRAMES+1);
    }
    void add(std::uint64_t frame,double ms,double work,double budget,int level,bool cinematic) {
        if(!std::isfinite(ms)||!std::isfinite(work)||ms<0||work<0)return;
        if(!std::isfinite(budget)||budget<0)budget=0;
        frames_.add(ms);work_.add(work);
        // A steady 30-FPS menu is not a stutter. Uncapped frames use the fixed threshold.
        if(ms<=std::max(threshold_,budget*2))return;
        ++slowCount_;
        const SlowFrame sample{frame,ms,work,budget,level,cinematic};
        const auto where=std::lower_bound(worst_.begin(),worst_.end(),ms,
            [](const SlowFrame& a,double b){return a.frameMs>b;});
        if(where!=worst_.end() || worst_.size()<MAX_SLOW_FRAMES)worst_.insert(where,sample);
        if(worst_.size()>MAX_SLOW_FRAMES)worst_.pop_back();
    }
    const TimingStatistics& frames() const { return frames_; }
    const TimingStatistics& work() const { return work_; }
    const std::vector<SlowFrame>& worst() const { return worst_; }
    std::uint64_t slowCount() const { return slowCount_; }
    double threshold() const { return threshold_; }
private:
    TimingStatistics frames_,work_;
    std::vector<SlowFrame> worst_;
    std::uint64_t slowCount_=0;
    double threshold_;
};
} // namespace br
