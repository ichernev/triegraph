#ifndef __UTIL_TIMER_H__
#define __UTIL_TIMER_H__

#include <chrono>

namespace triegraph {

template <
    typename clock_ = std::chrono::steady_clock,
    typename duration_ = std::chrono::milliseconds>
struct Timer {
    using clock = clock_;
    using instant = clock::time_point;
    using duration = duration_;
    using duration_rep = duration::rep;

    Timer(instant time = {}) : time(time) {}

    Timer(const Timer &) = default;
    Timer(Timer &&) = default;
    Timer &operator= (const Timer &) = default;
    Timer &operator= (Timer &&) = default;

    duration_rep operator- (const Timer &other) const {
        return std::chrono::duration_cast<duration>(time - other.time).count();
    }

    duration_rep elapsed() const { return Timer::now() - *this; }
    static Timer now() { return { clock::now() }; }

    instant time;
};

} /* namespace triegraph */

#endif /* __UTIL_TIMER_H__ */
