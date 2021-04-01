#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <iostream>
#include <chrono>
#include <vector>
#include <utility>

#include <iomanip>
#include <ctime>
#include <cstdlib> /* atexit */

namespace triegraph {

template <char sep=' ', typename... Args>
void print(std::ostream &os, Args&&... args) {
    auto x = {0, ((void) (os << sep << std::forward<Args>(args)), 0)... };
    (void) x;
    os << std::endl;
}

struct Logger {

    static inline Logger *instance = nullptr;

    static Logger& get() {
        if (instance == nullptr) {
            instance = new Logger();
            atexit(&Logger::free);
        }
        return *instance;
    }
    static void free() {
        delete instance;
    }

    using clock = std::chrono::steady_clock;
    using instant = clock::time_point;
    using tag_t = std::string;
    // using timer = std::tuple<instant, tag_t, bool>;
    struct Timer {
        instant begin;
        tag_t tag;
        bool expanded;
    };

    Logger() : os(std::cerr) {
        begin("main");
    }
    ~Logger() { while (!timers.empty()) end(); }

    // template <typename... Args>
    // void log(Args&&... args) {
    //     if (!timers.back().get<2>()) {
    //         os << std::endl;
    //         timers.back().get<2>() = true;
    //     }
    //     _ts();
    //     for (size_t i = 0; i < timers.size(); ++i) {
    //         os << '|';
    //     }
    //     print(os, std::forward<Args>(args)...);
    // }

    template <typename... Args>
    void log(Args&&... args) {
        if (!timers.back().expanded) {
            os << std::endl;
            timers.back().expanded = true;
        }
        _ts();
        for (size_t i = 1; i < timers.size(); ++i) {
            os << '|';
        }
        os << ":- "; _time_diff(timers.back().begin);
        print(os, std::forward<Args>(args)...);
    }

    void begin(const std::string &tag) {
        if (!timers.empty() && !timers.back().expanded) {
            os << std::endl;
            timers.back().expanded = true;
        }
        _ts();
        for (size_t i = 0; i < timers.size(); ++i) {
            os << '|';
        }
        timers.emplace_back(clock::now(), tag, false);
        os << ",- " << tag;
    }

    struct ScopedTimer {
        Logger &log;
        size_t level;
        ScopedTimer(Logger &log, const std::string &tag)
            : log(log), level(log.timers.size())
        {
            log.begin(tag);
        }
        ~ScopedTimer() { while (log.timers.size() > level) log.end(); }
    };

    ScopedTimer begin_scoped(const std::string &tag) {
        return ScopedTimer(*this, tag);
    }

    void end() {
        auto elem = timers.back(); timers.pop_back();
        if (!elem.expanded) {
            os << " [";
            _time_diff(elem.begin);
            os << "]" << std::endl;
        } else {
            _ts();
            for (size_t i = 0; i < timers.size(); ++i) {
                os << '|';
            }
            os << "`- " << elem.tag << " finished ["; _time_diff(elem.begin);
            os << "]" << std::endl;
        }
    }

private:
    void _time_diff(instant a) {
        _time_diff(a, clock::now());
    }

    void _time_diff(instant a, instant b) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                b - a).count();
        if (ms < 1000) {
            os << ms << "ms";
        } else {
            auto s = ms / 1000;
            if (s < 10) {
                os << s << "." << ms / 100 << (ms / 10 % 10) << "s";
            } else if (s < 100) {
                os << s << "." << ms / 100 << "s";
            } else {
                os << s << "s";
            }
        }
    }
    void _ts()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tmx;
        localtime_r(&in_time_t, &tmx);
        os << std::put_time(&tmx, "%Y-%m-%d %X");


        if (timers.size() == 0) {
            os << ".000";
        } else {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    clock::now() - timers[0].begin).count();
            ms %= 1000;
            os << "." << ms / 100 << (ms / 10) % 10 << ms % 10;
        }
    }


    std::ostream &os;
    std::vector<Timer> timers;
};

} /* namespace triegraph */

#endif /* __LOGGER_H__ */
