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

#include "catch2/benchmark/catch_benchmark.hpp"
#include "core/core.hpp"
#include "spdlog/spdlog.h"

#include "engine/tile/tile_shared.hpp"

#include <catch2/catch_test_macros.hpp>
#include <unordered_set>

template<raoe::engine::tile::chunk_indexer... TChunkDims>
struct test_tilemap_settings
{
    test_tilemap_settings() = default;
    test_tilemap_settings(flecs::world& world, int64 seed) {}
    // a parameter pack of chunk_indexer for indexable dimensions
    // a function that meshes a chunk
    void mesh_chunk(flecs::entity entity, raoe::engine::tile::chunk_position<TChunkDims...> position) {}
    // a function that generates terrain for a given area
    void generate_terrain(flecs::entity entity, raoe::engine::tile::chunk_position<TChunkDims...> position,
                          raoe::engine::tile::chunk_position<TChunkDims...> range)
    {
    }
};

TEST_CASE("Test Iteration Set", "[ENGINE][TILE]")
{
    // Generate a set of chunk positions in a range and verify correctness
    using tm_settings =
        test_tilemap_settings<raoe::engine::tile::integral_dimension<32>, raoe::engine::tile::integral_dimension<32>>;
    using tile_map = raoe::engine::tile::tile_map<tm_settings, raoe::engine::tile::integral_dimension<32>,
                                                  raoe::engine::tile::integral_dimension<32>>;
    using chunk_point = tile_map::chunk_point;

    std::unordered_set<chunk_point> points;
    tile_map::for_each_chunk_in_range(5, [&](const chunk_point& point) { points.insert(point); });
    REQUIRE(points.size() == std::pow((2 * 5 + 1), 2));

    for(int x = -5; x <= 5; x++)
    {
        for(int y = -5; y <= 5; y++)
        {
            chunk_point p {x, y};
            REQUIRE(points.contains(p));
        }
    }
}

TEST_CASE("Benchmarks", "[ENGINE][TILE]")
{
    BENCHMARK("2d Iteration, 8 chunks observation range")
    {
        using tm_settings = test_tilemap_settings<raoe::engine::tile::integral_dimension<32>,
                                                  raoe::engine::tile::integral_dimension<32>>;
        using tile_map = raoe::engine::tile::tile_map<tm_settings, raoe::engine::tile::integral_dimension<32>,
                                                      raoe::engine::tile::integral_dimension<32>>;
        using chunk_point = tile_map::chunk_point;
        tile_map::for_each_chunk_in_range(8, [&](const chunk_point&) {});
    };

    BENCHMARK("3d Iteration, 8 chunks observation range")
    {
        using tm_settings = test_tilemap_settings<raoe::engine::tile::integral_dimension<32>,
                                                  raoe::engine::tile::integral_dimension<32>,
                                                  raoe::engine::tile::integral_dimension<32>>;
        using tile_map = raoe::engine::tile::tile_map<tm_settings, raoe::engine::tile::integral_dimension<32>,
                                                      raoe::engine::tile::integral_dimension<32>,
                                                      raoe::engine::tile::integral_dimension<32>>;
        using chunk_point = tile_map::chunk_point;
        tile_map::for_each_chunk_in_range(8, [&](const chunk_point&) {});
    };
}