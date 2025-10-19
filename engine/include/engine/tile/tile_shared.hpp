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
#pragma once

#include "engine/engine.hpp"

#include "engine/tile/tile_shared.hpp"
#include "engine/tile/tile_storage.hpp"

#include <set>
#include <unordered_set>

namespace raoe::engine::tile
{
    template<chunk_indexer... TChunkDims>
    struct tile_map
    {
        tile_map() = default;
        tile_map(const tile_map&) = default;
        tile_map& operator=(const tile_map&) = default;
        tile_map(tile_map&&) noexcept = default;
        tile_map& operator=(tile_map&&) noexcept = default;

        using chunk_storage = tile_storage_chunk<flecs::entity, TChunkDims...>;
        using tile_point = tile_position<TChunkDims...>;
        using chunk_point = chunk_position<TChunkDims...>;

        struct data_change_event
        {
        };
        struct needs_remesh_event
        {
        };
        struct parent_map
        {
        };

        struct observed_by
        {
        };
        struct observing
        {
        };

        struct map_observer
        {
            int32 m_range = 8; // Range in chunks to observe around the origin
            flecs::ref<tile_map> observed_map = {};
            std::unordered_set<chunk_point> m_currently_observing;
        };

        struct chunk_become_observed_event
        {
            flecs::ref<map_observer> observer = {};
            chunk_point position = {};
            bool became_observed = false;
        };

        template<typename Func>
            requires std::invocable<Func, chunk_point>
        static void for_each_chunk_in_range(int64 range, Func&& func)
        {
            constexpr int32 dims = sizeof...(TChunkDims);
            // Generate all combinations of coordinates in the given range
            std::array<int64, dims> indices {};
            std::array<int64, dims> limits {};
            for(int32 i = 0; i < dims; ++i)
            {
                limits[i] = range * 2 + 1;
            }
            bool done = false;
            while(!done)
            {
                // Create chunk_point from indices
                chunk_point point;
                for(int32 i = 0; i < dims; ++i)
                {
                    point.m_data[i] = indices[i] - range;
                }
                // Call the provided function with the generated point
                func(point);

                // Increment indices
                for(int32 i = 0; i < dims; ++i)
                {
                    ++indices[i];
                    if(indices[i] < limits[i])
                    {
                        break;
                    }
                    if(i == dims - 1)
                    {
                        done = true;
                        break;
                    }
                    indices[i] = 0;
                }
            }
        }

        // Maximum tile map size, in tiles.  Can be none, which means infinite world
        std::optional<tile_point> m_max_size;

        // a struct that contains the observer information for a stored chunk
        std::unordered_map<chunk_point, flecs::ref<chunk_storage>> m_observed_chunks;
    };

    template<chunk_indexer... TChunkDims>
    struct tile_shared_module
    {
        using tile_map_t = tile_map<TChunkDims...>;
        using chunk_storage = tile_map_t::chunk_storage;
        using map_observer = tile_map_t::map_observer;
        using chunk_point = tile_map_t::chunk_point;
        using tile_point = tile_map_t::tile_point;
        using chunk_become_observed_event = tile_map_t::chunk_become_observed_event;
        using observed_by_tag = tile_map_t::observed_by;
        using observing_tag = tile_map_t::observing;

        flecs::entity m_chunk_prefab;
        // ReSharper disable once CppNonExplicitConvertingConstructor
        tile_shared_module(flecs::world& world)
        {
            world.component<tile_map_t>();
            world.component<chunk_storage>();
            world.component<map_observer>();

            m_chunk_prefab = world.prefab().set<chunk_storage>({}).template set<chunk_point>({});

            world.system<map_observer, const transform_3d>().kind(flecs::PreUpdate).run([](flecs::iter it) {
                while(it.next())
                {

                    flecs::entity e = it.entity(0);
                    const flecs::field<map_observer>& observer = it.field<map_observer>(1);
                    const auto& transform = it.field<const transform_3d>(2);

                    flecs::ref<tile_map_t> map = observer->observed_map;

                    if(!map)
                    {
                        continue;
                    }

                    // When an observer is added, we need to find all chunks within range and add this observer to
                    // them
                    std::unordered_set<chunk_point> observed_chunks;
                    chunk_point origin_chunk(*transform);
                    map->for_each_chunk_in_range(observer->m_range, [&](const chunk_point& offset) {
                        observed_chunks.emplace(offset + origin_chunk);
                    });

                    std::unordered_set<chunk_point> observed_difference;
                    std::copy_if(
                        observed_chunks.begin(), observed_chunks.end(),
                        std::inserter(observed_difference, observed_difference.end()),
                        [&](const chunk_point& point) { return !observer->m_currently_observing.contains(point); });

                    e.children<observing_tag>([&](flecs::entity chunk_entity) {
                        if(const chunk_point* point = chunk_entity.try_get<chunk_point>())
                        {
                            // If this chunk is being observed already, and it's part of the differences, remove it
                            if(observed_difference.contains(*point))
                            {
                                map.entity().template enqueue<chunk_become_observed_event>(chunk_become_observed_event {
                                    .observer = flecs::ref<map_observer> {e},
                                    .position = *point,
                                    .became_observed = false,
                                });
                                observed_difference.erase(*point);
                            }
                        }
                    });

                    // anything left over is new
                    for(const auto& new_chunk_pos : observed_difference)
                    {
                        map.entity().template enqueue<chunk_become_observed_event>(chunk_become_observed_event {
                            .observer = flecs::ref<map_observer> {e},
                            .position = new_chunk_pos,
                            .became_observed = true,
                        });
                    }
                }
            });
        }

        template<uint64... TMapSizes>
        static tile_point make_map_size(decltype(TMapSizes)... sizes)
        {
            static_assert(sizeof...(TMapSizes) == sizeof...(TChunkDims),
                          "Number of sizes must match number of dimensions");
            return tile_point {sizes...};
        }

        static flecs::ref<tile_map_t> create_map(flecs::entity into_entity, std::optional<tile_point> max_size = {})
        {
            into_entity.set<tile_map_t>(tile_map_t {
                .m_max_size = max_size,
            });

            return {into_entity};
        }
    };

}
