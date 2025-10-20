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

#include "engine/asset.hpp"
#include "engine/sys/pack.hpp"

template<>
struct raoe::engine::asset_loader<int>
{
    static int load_asset(const asset_load_params&) { return 42; }
};

TEST_CASE("Strong Asset Handle Test", "[ENGINE][ASSET]")
{
    flecs::world world;

    flecs::entity e = world.entity().set<int>(42);

    {
        flecs::ref<int> r = {e};
        raoe::engine::asset_handle handle1(r);
        {
            auto handle2 = handle1;
            REQUIRE(!!handle2);
            REQUIRE(*handle2 == 42);
        }
        REQUIRE(!!handle1);
        REQUIRE(*handle1 == 42);
    }

    REQUIRE(!e.is_alive());
}

TEST_CASE("Test Weak Asset Handles", "[ENGINE][ASSET]")
{
    flecs::world world;

    flecs::entity e = world.entity().set<int>(42);

    raoe::engine::asset_handle<int, raoe::engine::asset_handle_type::weak> weak_handle;
    {
        flecs::ref<int> r = {e};
        raoe::engine::asset_handle<int, raoe::engine::asset_handle_type::strong> strong_handle(r);
        weak_handle = strong_handle;

        {
            auto weak_handle2 = weak_handle;
            REQUIRE(!!weak_handle2);
            REQUIRE(*weak_handle2 == 42);
        }
        REQUIRE(!!weak_handle);
        REQUIRE(*weak_handle == 42);
    }

    REQUIRE(!e.is_alive());
    REQUIRE(!weak_handle);
}

TEST_CASE("Test loading strings", "[ENGINE][ASSET] ")
{
    using namespace raoe::engine;
    raoe::fs::permit_symlinks(true);
    raoe::fs::init_fs("asdf", "", "app_name", "org_name");
    flecs::world world;
    world.import <sys::pack_module>();
    sys::load_pack(world.entity(entities::engine::core_pack), "packs/core",
                   sys::pack_flags::system | sys::pack_flags::game);
    world.progress(); // Make sure the engine systems are initialized

    const auto handle_result = raoe::engine::load_asset<std::string>(world, raoe::fs::path("core.toml"));

    REQUIRE(handle_result.has_value());

    auto& handle = *handle_result;
    REQUIRE(handle.valid());
    REQUIRE(!handle->empty());
}