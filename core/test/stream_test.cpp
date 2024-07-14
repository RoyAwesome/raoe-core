/*
Copyright 2022-2024 Roy Awesome's Open Engine (RAOE)

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

#include <catch2/catch_test_macros.hpp>

#include "core/stream.hpp"

#include <sstream>

TEST_CASE("Test String Stream", "[STREAM]")
{
    std::string test_words = "some words";
    std::stringstream strstream(test_words);

    std::vector<std::byte> container;
    raoe::stream::read_stream_into(std::back_inserter(container), strstream);

    REQUIRE(container.size() == test_words.size());
    for(int i = 0; i < container.size(); i++)
    {
        REQUIRE(container[i] == std::byte(test_words[i]));
    }
}

TEST_CASE("Test String View Case", "[STREAM]")
{
    std::string test_words = "some words";

    std::vector<std::byte> container;
    raoe::stream::read_stream_into(std::back_inserter(container), test_words);

    REQUIRE(container.size() == test_words.size());
    for(int i = 0; i < container.size(); i++)
    {
        REQUIRE(container[i] == std::byte(test_words[i]));
    }
}