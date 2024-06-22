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
#include <iostream>

#include <format>

#include "core/uuid.hpp"

TEST_CASE("Basic UUID", "[UUID]")
{
    raoe::uuid uuid = raoe::make_random_uuid_v4();
    (void)uuid;
}

TEST_CASE("From String", "[UUID]")
{
    std::string test = "c940b5f2-0467-4005-8558-468f238b85db";
    raoe::uuid id;
    REQUIRE(raoe::from_string(test, id));

    std::string output = std::format("{}", id);

    REQUIRE(test == output);
}

TEST_CASE("Multiple Random are not the same", "[UUID]")
{
    raoe::uuid id1 = raoe::make_random_uuid_v4();
    raoe::uuid id2 = raoe::make_random_uuid_v4();
    REQUIRE(id1 != id2);
}