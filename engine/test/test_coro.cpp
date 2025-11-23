/*
    Copyright 2022-2025 Roy Awesome's Open Engine (RAOE)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

 */

#include "core/core.hpp"
#include "spdlog/spdlog.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "engine/coro.hpp"

using namespace raoe::engine;

coro test_sequence(const int32 n, int32& i)
{
    co_yield wait::for_next_tick();
    for(i = 0; i < n; i++)
    {
        spdlog::info("i = {}", i);
        co_yield wait::for_next_tick();
    }
}

TEST_CASE("test basic sequencer", "[CORO]")
{
    int i = 0;
    auto seq = test_sequence(10, i);

    while(seq)
    {
        seq();
    }

    REQUIRE(i == 10);
}

TEST_CASE("test debug info", "[CORO]")
{
    int i = 0;
    const auto seq = test_sequence(10, i);

    spdlog::info("src_location {}", seq.debug_info().coro_location.function_name());
}

coro test_time(int& i)
{
    using namespace std::chrono_literals;

    const auto start = std::chrono::high_resolution_clock::now();
    spdlog::info("Initial Wait For Duration");
    co_yield wait::for_duration(1s);
    for(i = 0; i < 10; i++)
    {
        auto duration = std::chrono::high_resolution_clock::now() - start;
        spdlog::info("i = {} time= {}", i, std::chrono::duration_cast<std::chrono::milliseconds>(duration));
        co_yield wait::for_duration(1s);
    }
}

TEST_CASE("test timed sequencer", "[CORO]")
{
    const auto start = std::chrono::high_resolution_clock::now();
    int counter = 0;
    auto seq = test_time(counter);
    while(seq)
    {
        seq();
    }

    REQUIRE(counter == 10);
    REQUIRE(std::chrono::high_resolution_clock::now() - start >= std::chrono::seconds(10));
}
