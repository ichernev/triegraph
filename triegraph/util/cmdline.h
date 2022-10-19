// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#ifndef __UTIL_CMDLINE_H__
#define __UTIL_CMDLINE_H__

#include <map>
#include <string>
#include <vector>
#include <algorithm>

namespace triegraph {

struct MapCfg {
    std::map<std::string, std::string> flags;

    MapCfg() {}
    MapCfg(std::initializer_list<std::string> items) {
        auto bp = items.begin();
        for (size_t i = 0; i < items.size(); i += 2) {
            flags[bp[i]] = bp[i+1];
        }
    }
    MapCfg(std::map<std::string, std::string> &&flags) : flags(std::move(flags)) {
    }

    template <typename T=std::string>
    T get(const std::string &key) const {
        if constexpr (std::is_same_v<T, bool>) {
            return flags.contains(key);
        }
        if (!flags.contains(key)) { throw "unknown key"; }

        if constexpr (std::is_same_v<T, std::string>) {
            return val_for_key(key);
        }
        std::istringstream is(val_for_key(key));
        T val_x;
        is >> val_x;
        return val_x;
    }

    template <typename T=std::string>
    T get_or(const std::string &key, T dfl) const {
        if constexpr (std::is_same_v<T, bool>) {
            return flags.contains(key);
        }
        if (!flags.contains(key)) { return dfl; }

        if constexpr (std::is_same_v<T, std::string>) {
            return val_for_key(key);
        }
        std::istringstream is(val_for_key(key));
        T val_x;
        is >> val_x;
        return val_x;
    }

    const std::string &val_for_key(const std::string &key) const {
        return flags.find(key)->second;
    }

    bool has(const std::string &key) const {
        return flags.contains(key);
    }

    MapCfg subset(const std::vector<std::string> &keys) {
        std::map<std::string, std::string> new_flags;
        for (const auto &[k, v] : flags) {
            if (std::find(keys.begin(), keys.end(), k) != keys.end()) {
                new_flags[k] = v;
            }
        }
        return {std::move(new_flags)};
    }
};

struct CmdLine : public MapCfg {
    std::vector<std::string> positional;

    CmdLine(int argc, const char *argv[]) {
        using std::string_literals::operator""s;

        bool force_positional = false;
        for (int i = 1; i < argc; ++i) {
            if (force_positional) {
                positional.emplace_back(argv[i]);
            } else {
                auto sv = std::string_view(argv[i]);
                if (sv == "--") {
                    force_positional = true;
                } else if (sv.starts_with("--")) {
                    auto rr = std::ranges::find(sv, '=');
                    if (rr != sv.end()) {
                        auto flag_key = std::string(sv.begin()+2, rr);
                        auto flag_val = std::string(rr+1, sv.end());
                        flags[flag_key] = flag_val;
                    } else {
                        flags[std::string(sv.begin() + 2, sv.end())] = "";
                    }
                } else {
                    positional.emplace_back(argv[i]);
                }
            }
        }
    }

    void debug() {
        for (const auto &f : flags) {
            std::cerr << f.first << " = " << f.second << std::endl;;
        }
        for (const auto &p : positional) {
            std::cerr << p << std::endl;
        }
    }
};


} /* namespace triegraph */

#endif /* __UTIL_CMDLINE_H__ */
