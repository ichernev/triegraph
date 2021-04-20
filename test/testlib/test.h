#include <assert.h>
#include <functional>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>

#include "util/logger.h"
#include "util/human.h"
#include "util/timer.h"
#include "util/memory.h"

std::string test_module_name;
std::string prefix;
std::vector<std::pair<std::string, std::function<void()>>> tests;

enum struct ResType { NORMAL, CHILD_FAIL, ERROR };
std::ostream &operator<< (std::ostream &os, const ResType &rt) {
    const char *name;
    switch (rt) {
        case ResType::NORMAL: name = "NORMAL"; break;
        case ResType::CHILD_FAIL: name = "CHILD_FAIL"; break;
        case ResType::ERROR: name = "ERROR"; break;
        default: name = "UNKNOWN"; break;
    }
    return os << name;
}

namespace {
    template <typename F, typename... Args>
    std::pair<ResType, typename F::result_type> process_executor(F f, Args&&... args) {
        using R = F::result_type;
        int fd[2];
        if (pipe(fd) == -1) {
            std::cerr << "pipe() failed" << std::endl;
            return {};
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "fork() failed" << std::endl;
            return {ResType::ERROR, {}};
        }
        if (pid == 0) {
            // in child
            close(fd[0]); // close read end
            auto res = f(std::forward<Args>(args)...);
            // auto data = res.data();
            if (write(fd[1], &res, sizeof(res)) != sizeof(res)) {
                std::cerr << "write() failed" << std::endl;

            }
            close(fd[1]);
            exit(0);
        } else {
            // in parent
            // write args;
            siginfo_t info;
            if (waitid(P_PID, pid, &info, WEXITED) != 0) {
                std::cerr << "waitid failed" << std::endl;
                return {ResType::ERROR, {}};
            }
            if (!(info.si_code == CLD_EXITED && info.si_status == 0)) {
                std::cerr << "child exited abnormally" << std::endl;
                return {ResType::CHILD_FAIL, {}};
            }
            R res;
            close(fd[1]); // close write end
            if (read(fd[0], &res, sizeof(res)) != sizeof(res)) {
                std::cerr << "read() failed" << std::endl;
                return {ResType::ERROR, {}};
            }
            if (close(fd[0]) != 0) {
                std::cerr << "close() failed" << std::endl;
                return {ResType::ERROR, {}};
            }

            return {ResType::NORMAL, res};
        }
    }
}

namespace test {

    template <std::ranges::input_range R,
             typename T = std::ranges::range_value_t<R>,
             typename Cmp = std::less<T>>
    static std::vector<T> sorted(R &&range) {
        std::vector<T> res;
        std::ranges::copy(range, std::back_inserter(res));
        std::ranges::sort(res, Cmp {});
        return res;
    }

    static bool equal_sorted(
            std::ranges::input_range auto&& range1,
            std::ranges::input_range auto&& range2) {
        return std::ranges::equal(sorted(range1), range2);
    }

    static int define_module(const char *filename, std::function<void()> &&fn) {
        test_module_name = filename;
        fn();
        return 0;
    }

    static void define_test(const char *test, std::function<void()> &&fn) {
        tests.emplace_back(prefix + test, std::move(fn));
    }

    template <typename TestClass>
    static void register_test_class(const char *prefix) {
        // this pointer is leaked for now, it shouldn't hurt
        TestClass *ptr = new TestClass();
        ::prefix = std::string() + prefix + "::";
        ptr->define_tests();
        ::prefix = "";
    }
}


int main(int argc, char *argv[]) {
    using Timer = triegraph::Timer<>;
    triegraph::Logger::disable();

    bool measure_resources = test_module_name.starts_with("slow");
    std::cerr << "TEST MODULE " << test_module_name << std::endl;
    for (const auto &test : tests) {
        std::cerr << "  TEST " << test.first << " ";

        if (!measure_resources) {
            test.second();
            std::cerr << "[OK]" << std::endl;
        } else {
            using Res = std::pair<
                typename Timer::duration_rep,
                triegraph::Memory>;
            auto res = process_executor(std::function<Res()>([test_fn=test.second] {
                auto start_time = Timer::now();
                auto start_mem = triegraph::Memory::current();
                test_fn();
                auto end_mem = triegraph::Memory::current();
                auto end_time = Timer::now();
                return std::make_pair(end_time - start_time, end_mem - start_mem);
            }));
            if (res.first == ResType::NORMAL) {
                std::cerr << "[OK]"
                    << " " << triegraph::to_human_time(res.second.first, false)
                    << " " << triegraph::to_human_mem(res.second.second.vmhwm, false) << '\n'
                    << std::flush;
            }
        }
        // std::cerr
        //     << " " << triegraph::to_human_time(resources.time_ms)
        //     << " " << triegraph::to_human_mem(resources.mem.vmsize)
        //     << std::endl;;
    }

    return 0;
}
