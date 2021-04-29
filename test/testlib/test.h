#ifndef __TESTLIB_TEST_H__
#define __TESTLIB_TEST_H__

#include <assert.h>
#include <functional>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>

#include "util/logger.h"
#include "util/human.h"
#include "util/timer.h"
#include "util/memory.h"
#include "util/cmdline.h"

// std::string test_module_name;
// std::string prefix;
// std::vector<std::pair<std::string, std::function<void()>>> tests;

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
            try {
                auto res = f(std::forward<Args>(args)...);
                if (write(fd[1], &res, sizeof(res)) != sizeof(res)) {
                    std::cerr << "write() failed" << std::endl;
                }
            } catch (const char *err) {
                std::cerr << "child ex: " << err << std::endl;
                exit(1);
            } catch (...) {
                std::cerr << "child unknown ex" << std::endl;
                exit(1);
            }
            // auto data = res.data();
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
                // std::cerr << "child exited abnormally" << std::endl;
                // std::cerr << info.si_code << " " << CLD_DUMPED << " " << info.si_status << std::endl;
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

    struct TestCaseBase;
    struct TestModule {
        TestModule(const std::string &name) : name(name) {}

        std::string name;
        std::string prefix;
        std::vector<std::unique_ptr<TestCaseBase>> tests;
    };
    std::vector<TestModule> modules;

    struct TestCaseBase {
        virtual void prepare() {};
        virtual void run() {};
        virtual ~TestCaseBase() {};

        TestCaseBase(const std::string &name) : name(name) {}
        TestCaseBase(std::string &&name) : name(std::move(name)) {}

        TestCaseBase(const TestCaseBase &) = delete;
        TestCaseBase &operator= (const TestCaseBase &) = delete;
        TestCaseBase(TestCaseBase &&) = default;
        TestCaseBase &operator= (TestCaseBase &&) = default;

        std::string name;
    };

    struct TestCaseSimple : public TestCaseBase {
        TestCaseSimple(std::string &&name,
                std::function<void()> &&runner)
            : TestCaseBase(std::move(name)),
              runner(std::move(runner)) {}
        virtual void run() {
            runner();
        }
        std::function<void()> runner;
    };

    template <std::ranges::input_range R,
             typename T = std::ranges::range_value_t<R>,
             typename Cmp = std::less<T>>
    std::vector<T> sorted(R &&range) {
        std::vector<T> res;
        std::ranges::copy(range, std::back_inserter(res));
        std::ranges::sort(res, Cmp {});
        return res;
    }

    bool equal_sorted(
            std::ranges::input_range auto&& range1,
            std::ranges::input_range auto&& range2) {
        return std::ranges::equal(sorted(range1), range2);
    }

    bool throws_ccp(std::function<void()> f, const char *expected) {
        try {
            f();
        } catch (const char *got) {
            assert(std::strcmp(got, expected) == 0);
            return true;
        }
        return false;
    }

    static int define_module(const char *filename, std::function<void()> &&fn) {
        modules.emplace_back(filename);
        fn();
        return 0;
    }

    void define_test(const char *test_name, std::function<void()> &&fn) {
        auto &m = modules.back();
        m.tests.emplace_back(std::make_unique<TestCaseSimple>(
                    m.prefix + test_name, std::move(fn)));
    }

    void add_test(std::unique_ptr<TestCaseBase> &&test) {
        auto &m = modules.back();
        m.tests.emplace_back(std::move(test));
    }

    template <typename TC, typename... Args>
    void add_test(Args&&... args) {
        add_test(std::make_unique<TC>(std::forward<Args>(args)...));
    }

    bool guard(std::function<void()> fn) {
        try {
            fn();
            return true;
        } catch (const char *err) {
            std::cerr << "Exception: " << err << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Other Exception." << std::endl;
            return false;
        }
    }
}


int main(int argc, char *argv[]) {
    auto cmdline = triegraph::CmdLine(argc, argv);
    using Timer = triegraph::Timer<>;
    if (!cmdline.get<bool>("leave-logger"))
        triegraph::Logger::disable();

    for (auto &mod : test::modules) {
        bool measure_resources = cmdline.get<bool>("measure") ?
            true : cmdline.get<bool>("no-measure") ?
                false :
                mod.name.starts_with("test/benchmark");
        std::cerr << "TEST MODULE " << mod.name << std::endl;
        for (auto &test : mod.tests) {
            std::cerr << "  TEST " << test->name << " " << std::flush;

            if (!measure_resources) {
                bool res = test::guard([testp=test.get()] {
                    testp->prepare();
                    testp->run();
                });
                std::cerr << (res ? "[OK]" : "[FAIL]") << std::endl;
            } else {
                using Res = std::pair<
                    typename Timer::duration_rep,
                    triegraph::Memory>;
                if (!test::guard([testp=test.get()] { testp->prepare(); })) {
                    std::cerr << "[FAIL]" << std::endl;
                    test.reset(nullptr);
                    continue;
                }
                auto res = process_executor(std::function<Res()>([testp=test.get()] {
                    auto start_time = Timer::now();
                    auto start_mem = triegraph::Memory::current();
                    testp->run();
                    auto end_mem = triegraph::Memory::current();
                    auto end_time = Timer::now();
                    return std::make_pair(end_time - start_time, end_mem - start_mem);
                }));
                if (res.first == ResType::NORMAL) {
                    std::cerr << "[OK]"
                        << " " << triegraph::to_human_time(res.second.first, false)
                        << " " << triegraph::to_human_mem(res.second.second.vmhwm, false) << '\n'
                        << std::flush;
                } else {
                    std::cerr << "[FAIL] " << res.first << std::endl;
                }
            }
            // free consumed memory
            test.reset(nullptr);
        }
    }

    return 0;
}

#endif /* __TESTLIB_TEST_H__ */
