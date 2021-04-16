#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "util/util.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <utility>

#include <iomanip>
#include <ctime>
#include <cstdlib> /* atexit */
#include <unistd.h> /* getpid */

namespace triegraph {

// template <char sep=' ', typename... Args>
// void print(std::ostream &os, Args&&... args) {
//     auto x = {0, ((void) (os << sep << std::forward<Args>(args)), 0)... };
//     (void) x;
//     os << std::endl;
// }

struct MemRes {
    i64 vmpeak;
    i64 vmsize;
    i64 vmrss;
    i64 vmswap;

    MemRes() {}
    MemRes(const MemRes &) = default;
    MemRes(MemRes &&) = default;
    MemRes &operator= (const MemRes &) = default;
    MemRes &operator= (MemRes &&) = default;

    MemRes &operator-= (const MemRes &other) {
        vmpeak -= other.vmpeak;
        vmsize -= other.vmsize;
        vmrss -= other.vmrss;
        vmswap -= other.vmswap;
        return *this;
    }

    MemRes operator- (const MemRes &other) const {
        MemRes tmp = *this;
        return tmp -= other;
    }

    static MemRes current() {
        std::ostringstream fn;
        fn << "/proc/" << getpid() << "/status";
        std::ifstream ifs { fn.str() };

        MemRes res;
        std::string line;
        while (std::getline(ifs, line)) {
            std::istringstream lines(line);
            std::string label;
            lines >> label;
            if (label == "VmPeak:") { lines >> res.vmpeak; }
            if (label == "VmSize:") { lines >> res.vmsize; }
            if (label == "VmRSS:") { lines >> res.vmrss; }
            if (label == "VmSwap:") { lines >> res.vmswap; }
        }
        return res;
    }

    static std::string human_mem(i64 kb, bool add_sign = true) {
        auto suffix = std::vector { "kb", "mb", "gb" };

        char sign = kb >= 0 ? '+' : '-';
        u32 sid = 0;
        u64 amt = std::abs(kb); u32 rem = 0;
        while (sid + 1 < suffix.size() && amt >= 1000) {
            rem = amt % 1000;
            amt /= 1000;
            sid += 1;
        }

        std::ostringstream res;
        if (add_sign) res << sign;
        if (amt < 10) {
            res << amt << "." << (rem / 100) << (rem / 10) % 10 << suffix[sid];
        } else if (amt < 100) {
            res << amt << "." << (rem / 100) << suffix[sid];
        } else {
            res << amt << suffix[sid];
        }
        return res.str();
    }
};

struct Logger {

    enum separator { NONE, SPACE, NEWLINE };
    static inline Logger *instance = nullptr;
    static inline bool enabled = true;

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
    static void enable() { enabled = true; }
    static void disable() { enabled = false; }

    using clock = std::chrono::steady_clock;
    using instant = clock::time_point;
    using tag_t = std::string;
    // using timer = std::tuple<instant, tag_t, bool>;
    struct Timer {
        instant begin;
        tag_t tag;
        bool expanded;
        MemRes mem;
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
            print_ln(os);
            timers.back().expanded = true;
        }
        _ts();
        for (size_t i = 1; i < timers.size(); ++i) {
            print<NONE>(os, '|');
        }
        print<NONE>(os, ":- "); /* _time_diff(timers.back().begin); */
        print_ln(os, std::forward<Args>(args)...);
    }

    Logger &begin(const std::string &tag) {
        if (!timers.empty() && !timers.back().expanded) {
            print_ln(os);
            timers.back().expanded = true;
        }
        _ts();
        for (size_t i = 0; i < timers.size(); ++i) {
            print<NONE>(os, '|');
        }
        timers.emplace_back(clock::now(), tag, false, MemRes::current());
        print<NONE>(os, ",- ", tag);

        return *this;
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

    Logger &end() {
        auto elem = timers.back(); timers.pop_back();
        if (!elem.expanded) {
            print(os, '[');
            _time_diff(elem.begin);
            print<NONE>(os, "] ");
            _mem_diff(elem.mem);
            print_ln<NONE>(os);
        } else {
            _ts();
            for (size_t i = 0; i < timers.size(); ++i) {
                print<NONE>(os, '|');
            }
            print<NONE>(os, "`- ", elem.tag, " finished [");
            _time_diff(elem.begin);
            print<NONE>(os, "] ");
            _mem_diff(elem.mem);
            print_ln<NONE>(os);
        }
        return *this;
    }

private:

    template <int sep=SPACE, typename... Args>
    void print(std::ostream &os, Args&&... args) {
        if (enabled) {
            auto x = {0, ((void) (os
                        << (sep == NONE ? "" : sep == SPACE ? " " : "\n")
                        << std::forward<Args>(args)), 0)... };
            (void) x;
        }
    }

    template <int sep=SPACE, typename... Args>
    void print_ln(std::ostream &os, Args&&... args) {
        // auto x = {0, ((void) (os << sep << std::forward<Args>(args)), 0)... };
        // (void) x;
        print<sep>(os, std::forward<Args>(args)...);
        if (enabled) os << std::endl;
    }

    void _time_diff(instant a) {
        _time_diff(a, clock::now());
    }

    void _time_diff(instant a, instant b) {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                b - a).count();
        if (ms < 1000) {
            print<NONE>(os, ms, "ms");
        } else {
            auto s = ms / 1000;
            if (s < 10) {
                print<NONE>(os, s, '.', ms / 100, ms / 10 % 10, "s");
            } else if (s < 100) {
                print<NONE>(os, s, '.', ms / 100, "s");
            } else {
                print<NONE>(os, s, "s");
            }
        }
    }

    void _ts()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tmx;
        localtime_r(&in_time_t, &tmx);
        print<NONE>(os, std::put_time(&tmx, "%Y-%m-%d %X"));


        if (timers.size() == 0) {
            print<NONE>(os, ".000");
        } else {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    clock::now() - timers[0].begin).count();
            ms %= 1000;
            print<NONE>(os, '.', ms / 100, (ms / 10) % 10, ms % 10);
        }
    }

    void _mem_diff(MemRes old) {
        MemRes nu = MemRes::current();
        print<SPACE>(os,
                MemRes::human_mem((nu - old).vmsize),
                "total", MemRes::human_mem(nu.vmsize, false));
    }

    std::ostream &os;
    std::vector<Timer> timers;
};

} /* namespace triegraph */

#endif /* __LOGGER_H__ */
