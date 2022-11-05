// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include "triegraph/util/util.h"
#include "triegraph/util/memory.h"
#include "triegraph/util/timer.h"
#include "triegraph/util/human.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <vector>
#include <utility>

#include <iomanip>
#include <ctime>
#include <cstdlib> /* atexit */

namespace triegraph {

// template <char sep=' ', typename... Args>
// void print(std::ostream &os, Args&&... args) {
//     auto x = {0, ((void) (os << sep << std::forward<Args>(args)), 0)... };
//     (void) x;
//     os << std::endl;
// }

struct Logger {
    using Timer = triegraph::Timer<>;
    using tag_t = std::string;

    struct Resources {
        using duration_t = std::chrono::milliseconds::rep;

        typename Timer::duration_rep time_ms;
        Memory mem;
    };

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

    // using clock = std::chrono::steady_clock;
    // using instant = clock::time_point;
    // using timer = std::tuple<instant, tag_t, bool>;
    struct Snapshot {
        Timer begin;
        tag_t tag;
        bool expanded;
        Memory mem;
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
        timers.emplace_back(Timer::now(), tag, false, Memory::current());
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

    Logger &end() { Resources res; return end(res); }

    Logger &end(Resources &res) {
        auto elem = timers.back(); timers.pop_back();
        if (!elem.expanded) {
            print(os, '[');
            res.time_ms = elem.begin.elapsed();
            print<NONE>(os, to_human_time(res.time_ms));
            print<NONE>(os, "] ");
            res.mem = _mem_diff(elem.mem);
            print_ln<NONE>(os);
        } else {
            _ts();
            for (size_t i = 0; i < timers.size(); ++i) {
                print<NONE>(os, '|');
            }
            print<NONE>(os, "`- ", elem.tag, " finished [");
            res.time_ms = elem.begin.elapsed();
            print<NONE>(os, to_human_time(res.time_ms));
            print<NONE>(os, "] ");
            res.mem = _mem_diff(elem.mem);
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

    // typename Timer::duration_rep _time_diff(Timer a) {
    //     return _time_diff(a, Timer::now());
    // }

    // typename Timer::duration_rep _time_diff(Timer a, Timer b) {
    //     auto ms = b - a;
    //     if (ms < 1000) {
    //         print<NONE>(os, ms, "ms");
    //     } else {
    //         auto s = ms / 1000;
    //         if (s < 10) {
    //             print<NONE>(os, s, '.', ms / 100, ms / 10 % 10, "s");
    //         } else if (s < 100) {
    //             print<NONE>(os, s, '.', ms / 100, "s");
    //         } else {
    //             print<NONE>(os, s, "s");
    //         }
    //     }
    //     return ms;
    // }

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
            auto ms = timers[0].begin.elapsed();
            ms %= 1000;
            print<NONE>(os, '.', ms / 100, (ms / 10) % 10, ms % 10);
        }
    }

    Memory _mem_diff(Memory old) {
        auto nu = Memory::current();
        auto res = nu - old;
        print<SPACE>(os,
                to_human_mem(res.vmpeak),
                "total", to_human_mem(nu.vmpeak, false));
        return res;
    }

    std::ostream &os;
    std::vector<Snapshot> timers;
};

} /* namespace triegraph */

#endif /* __LOGGER_H__ */
