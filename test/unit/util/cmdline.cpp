#include "testlib/test.h"
#include "util/cmdline.h"
#include "util/util.h"

using namespace triegraph;

int m = test::define_module(__FILE__, [] {
test::define_test("empty MapCfg", [] {
    using std::string_literals::operator""s;
    auto x = MapCfg {};

    assert(test::throws_ccp([&x] { x.get<std::string>("foo"); }, "unknown key"));
    assert(x.get_or<std::string>("foo", "bar") == "bar"s);
});

test::define_test("init_list MapCfg", [] {
    using std::string_literals::operator""s;
    auto x = MapCfg {
        "foo", "bar",
        "trie-depth", "5",
        "empty", "",
    };

    assert(test::throws_ccp([&x] { x.get<std::string>("foo-2"); }, "unknown key"));
    assert(x.get<std::string>("foo") == "bar"s);
    assert(x.get<std::string>("trie-depth") == "5"s);
    assert(x.get<u32>("trie-depth") == 5u);
    assert(x.get<i32>("trie-depth") == 5);
    assert(x.get<bool>("empty") == true);
});

});

