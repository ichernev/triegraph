#ifndef __UTIL_MEMORY_H__
#define __UTIL_MEMORY_H__

#include "util/human.h"

#include <unistd.h> /* getpid */
#include <iostream>
#include <sstream>
#include <fstream>

namespace triegraph {

struct Memory {
    i64 vmpeak = 0;
    i64 vmsize = 0;
    i64 vmhwm = 0;
    i64 vmrss = 0;
    i64 vmswap = 0;

    Memory() {}
    Memory(const Memory &) = default;
    Memory(Memory &&) = default;
    Memory &operator= (const Memory &) = default;
    Memory &operator= (Memory &&) = default;

    Memory &operator-= (const Memory &other) {
        vmpeak -= other.vmpeak;
        vmsize -= other.vmsize;
        vmhwm -= other.vmhwm;
        vmrss -= other.vmrss;
        vmswap -= other.vmswap;
        return *this;
    }

    Memory &operator+= (const Memory &other) {
        vmpeak += other.vmpeak;
        vmsize += other.vmsize;
        vmhwm += other.vmhwm;
        vmrss += other.vmrss;
        vmswap += other.vmswap;
        return *this;
    }

    Memory operator- (const Memory &other) const {
        Memory tmp = *this;
        return tmp -= other;
    }

    Memory operator+ (const Memory &other) const {
        Memory tmp = *this;
        return tmp += other;
    }

    static Memory current() {
        std::ostringstream fn;
        fn << "/proc/" << getpid() << "/status";
        std::ifstream ifs { fn.str() };

        Memory res;
        std::string line;
        while (std::getline(ifs, line)) {
            std::istringstream lines(line);
            std::string label;
            lines >> label;
            if (label == "VmPeak:") { lines >> res.vmpeak; }
            if (label == "VmSize:") { lines >> res.vmsize; }
            if (label == "VmHWM:") { lines >> res.vmhwm; }
            if (label == "VmRSS:") { lines >> res.vmrss; }
            if (label == "VmSwap:") { lines >> res.vmswap; }
        }
        return res;
    }


    friend std::ostream &operator<< (std::ostream &os, const Memory &res) {
        return os
            << "VmPeak: " << to_human_mem(res.vmpeak) << '\n'
            << "VmSize: " << to_human_mem(res.vmsize) << '\n'
            << "VmHWM:  " << to_human_mem(res.vmhwm) << '\n'
            << "VmRSS:  " << to_human_mem(res.vmrss) << '\n'
            << "VmSwap: " << to_human_mem(res.vmswap);
    }
};

} /* namespace triegraph */

#endif /* __UTIL_MEMORY_H__ */
