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

#include "tag/tag.hpp"

#include <string_view>

#include <catch2/catch_test_macros.hpp>

using namespace std::literals::string_view_literals;

TEST_CASE("Construction", "[Tag]")
{
    raoe::tag tag("minecraft:dirt");
    REQUIRE(tag.prefix() == "minecraft"sv);
    REQUIRE(tag.identifier() == "dirt"sv);
    REQUIRE(tag == "minecraft:dirt"sv);
}

TEST_CASE("InvalidIdentifier", "[Tag]")
{
    raoe::tag tag("minecraft:");
    REQUIRE(tag.identifier() == ""sv);
    REQUIRE(tag == false);
}

TEST_CASE("InvalidPrefix", "[Tag]")
{
    raoe::tag tag(":test");
    REQUIRE(tag.prefix() == ""sv);
    REQUIRE(tag == false);
}

TEST_CASE("DefaultNamespace", "[Tag]")
{
    raoe::tag tag("dirt");
    REQUIRE(tag.prefix() == raoe::tag::default_prefix());
    REQUIRE(tag.identifier() == "dirt"sv);
    REQUIRE(tag == std::string_view(std::string(raoe::tag::default_prefix()) + ":dirt"));
}

TEST_CASE("InvalidCharInNamespace", "[Tag]")
{
    raoe::tag tag("voidcra/ft:dirt");
    REQUIRE(tag == false);
}

TEST_CASE("PathDirectory", "[Tag]")
{
    raoe::tag tag("minecraft:block/dirt");
    REQUIRE(tag.prefix() == "minecraft"sv);
    REQUIRE(tag.identifier() == "block/dirt"sv);
    REQUIRE(tag == "minecraft:block/dirt"sv);
}

TEST_CASE("InvalidCharInPath", "[Tag]")
{
    raoe::tag tag("voidcraft:di()rt");
    REQUIRE(tag == false);
}

TEST_CASE("Equality", "[Tag]")
{
    raoe::tag TagA("voidcraft:dirt");
    raoe::tag TagB("voidcraft:dirt");

    REQUIRE((TagA == TagB) == true);
}

TEST_CASE("JustColon", "[Tag]")
{
    raoe::tag tag(":");
    REQUIRE(tag.identifier() == ""sv);
    REQUIRE(tag.prefix() == ""sv);
    REQUIRE(tag == false);
}

TEST_CASE("Type", "[TAG]")
{
    raoe::tag tag("minecraft#tile:dirt");
    REQUIRE(tag.prefix() == "minecraft"sv);
    REQUIRE(tag.identifier() == "dirt"sv);
    REQUIRE(tag.type() == "tile"sv);
}

TEST_CASE("matches", "[TAG]")
{
    raoe::tag tag("minecraft#tile:dirt");
    raoe::tag tag2("minecraft:dirt");
    REQUIRE(tag.matches(tag2));
}

TEST_CASE("doesn't", "[TAG]")
{
    raoe::tag tag("minecraft#tile:dirt");
    raoe::tag tag2("minecraft#texture:dirt");
    REQUIRE(!tag.matches(tag2));
}

TEST_CASE("Move Constructor", "[TAG]")
{
    raoe::tag tag1("minecraft#tile:dirt");
    raoe::tag tag2 = std::move(tag1);

    REQUIRE(tag2.prefix() == "minecraft"sv);
    REQUIRE(tag2.identifier() == "dirt"sv);
    REQUIRE(tag2.type() == "tile"sv);
}