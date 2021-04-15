#ifndef __UTIL_CMDLINE_H__
#define __UTIL_CMDLINE_H__

#include <map>
#include <string>
#include <vector>

namespace triegraph {

struct CmdLine {
    std::map<std::string, std::string> flags;
    std::vector<std::string> positional;

    CmdLine(int argc, char *argv[]) {
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

    template <typename T=std::string>
    T get(std::string key) {
        if constexpr (std::is_same_v<T, bool>) {
            return flags.contains(key);
        }
        if (!flags.contains(key)) { throw "unknown key"; }

        if constexpr (std::is_same_v<T, std::string>) {
            return flags[key];
        }
        std::istringstream is(flags[key]);
        T val_x;
        is >> val_x;
        return val_x;
    }

    template <typename T=std::string>
    T get_or(std::string key, T dfl) {
        if constexpr (std::is_same_v<T, bool>) {
            return flags.contains(key);
        }
        if (!flags.contains(key)) { return dfl; }

        if constexpr (std::is_same_v<T, std::string>) {
            return flags[key];
        }
        std::istringstream is(flags[key]);
        T val_x;
        is >> val_x;
        return val_x;
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
