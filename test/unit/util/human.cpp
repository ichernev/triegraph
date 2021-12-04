// SPDX-License-Identifier: MPL-2.0
/*
 * Copyright (c) 2021, Iskren Chernev
 */

#include "testlib/test.h"

#include "util/human.h"

using namespace triegraph;

int m = test::define_module(__FILE__, [] {

test::define_test("mem", [] {
    assert(to_human_mem(0) == "+0kb");
    assert(to_human_mem(50) == "+50kb");
    assert(to_human_mem(150) == "+150kb");
    assert(to_human_mem(950) == "+950kb");
    assert(to_human_mem(1000) == "+1.00mb");
    assert(to_human_mem(1250) == "+1.25mb");
    assert(to_human_mem(5050) == "+5.05mb");
    assert(to_human_mem(15050) == "+15.0mb");
    assert(to_human_mem(15550) == "+15.5mb");
    assert(to_human_mem(156000) == "+156mb");

    assert(to_human_mem(0u) == "0kb");
    assert(to_human_mem(50u) == "50kb");
    assert(to_human_mem(150u) == "150kb");
    assert(to_human_mem(950u) == "950kb");
    assert(to_human_mem(1000u) == "1.00mb");
    assert(to_human_mem(1250u) == "1.25mb");
    assert(to_human_mem(5050u) == "5.05mb");
    assert(to_human_mem(15050u) == "15.0mb");
    assert(to_human_mem(15550u) == "15.5mb");
    assert(to_human_mem(156000u) == "156mb");
});

test::define_test("time", [] {
    assert(to_human_time(0) == "+0ms");
    assert(to_human_time(50) == "+50ms");
    assert(to_human_time(150) == "+150ms");
    assert(to_human_time(950) == "+950ms");
    assert(to_human_time(1000) == "+1.00s");
    assert(to_human_time(1250) == "+1.25s");
    assert(to_human_time(5050) == "+5.05s");
    assert(to_human_time(15050) == "+15.0s");
    assert(to_human_time(15550) == "+15.5s");
    assert(to_human_time(156000) == "+156s");

    assert(to_human_time(0u) == "0ms");
    assert(to_human_time(50u) == "50ms");
    assert(to_human_time(150u) == "150ms");
    assert(to_human_time(950u) == "950ms");
    assert(to_human_time(1000u) == "1.00s");
    assert(to_human_time(1250u) == "1.25s");
    assert(to_human_time(5050u) == "5.05s");
    assert(to_human_time(15050u) == "15.0s");
    assert(to_human_time(15550u) == "15.5s");
    assert(to_human_time(156000u) == "156s");
});

test::define_test("number", [] {
    assert(to_human_number(0) == "+0");
    assert(to_human_number(50) == "+50");
    assert(to_human_number(150) == "+150");
    assert(to_human_number(950) == "+950");
    assert(to_human_number(1000) == "+1.00k");
    assert(to_human_number(1250) == "+1.25k");
    assert(to_human_number(5050) == "+5.05k");
    assert(to_human_number(15050) == "+15.0k");
    assert(to_human_number(15550) == "+15.5k");
    assert(to_human_number(156000) == "+156k");
    assert(to_human_number(156000123) == "+156m");
    assert(to_human_number(1234567890) == "+1.23b");

    assert(to_human_number(0u) == "0");
    assert(to_human_number(50u) == "50");
    assert(to_human_number(150u) == "150");
    assert(to_human_number(950u) == "950");
    assert(to_human_number(1000u) == "1.00k");
    assert(to_human_number(1250u) == "1.25k");
    assert(to_human_number(5050u) == "5.05k");
    assert(to_human_number(15050u) == "15.0k");
    assert(to_human_number(15550u) == "15.5k");
    assert(to_human_number(156000u) == "156k");
    assert(to_human_number(156000123u) == "156m");
    assert(to_human_number(1234567890u) == "1.23b");
});

});
