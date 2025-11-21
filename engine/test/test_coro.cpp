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

coro test_sequence(const int32 n)
{
    co_yield wait::for_next_tick();
    for(int32 i = 0; i < n; i++)
    {
        spdlog::info("i = {}", i);
        co_yield wait::for_next_tick();
    }
}

TEST_CASE("test basic sequencer", "[SEQUENCE]")
{
    auto seq = test_sequence(10);

    int i = 0;
    while(seq)
    {
        seq();
        i++;
    }

    REQUIRE(i == 10);
}

coro test_time()
{
    using namespace std::chrono_literals;

    const auto start = std::chrono::high_resolution_clock::now();
    co_yield wait::for_duration(1s);
    for(int i = 0; i < 10; i++)
    {
        auto duration = std::chrono::high_resolution_clock::now() - start;
        spdlog::info("i = {} time= {}", i, std::chrono::duration_cast<std::chrono::milliseconds>(duration));
        co_yield wait::for_duration(1s);
    }
}

TEST_CASE("test timed sequencer", "[SEQUENCE]")
{
    auto seq = test_time();
    while(seq)
    {
        seq();
    }
}