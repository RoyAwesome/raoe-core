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

#include <queue>
#include <set>
#include <unordered_set>

namespace raoe::engine::tile
{

    template<typename T, typename TInitializer>
    concept tile_map_initializer = std::is_constructible_v<T, const TInitializer&, flecs::world&, int64>;

    template<typename T, typename... TChunkDims>
    concept tile_map_settings =
        requires(T t, flecs::entity map_entity, chunk_position<TChunkDims...> pos,
                 chunk_position<TChunkDims...> range) {
            // a parameter pack of chunk_indexer for indexable dimensions
            // a function that meshes a chunk
            t.mesh_chunk(map_entity, pos);
            // a function that generates terrain for a given area
            t.generate_terrain(map_entity, pos, range);
        } &&
        std::is_default_constructible_v<T> && std::is_constructible_v<T, flecs::world&, int64> &&
        (chunk_indexer<TChunkDims> && ...);

    template<typename TTileMapSettings, chunk_indexer... TChunkDims>
        requires tile_map_settings<TTileMapSettings, TChunkDims...>
    struct tile_map
    {
        using chunk_storage = tile_storage_chunk<flecs::entity, TChunkDims...>;
        using tile_point = tile_position<TChunkDims...>;
        using chunk_point = chunk_position<TChunkDims...>;

        tile_map() = default;
        tile_map(const tile_map&) = default;
        tile_map& operator=(const tile_map&) = default;
        tile_map(tile_map&&) noexcept = default;
        tile_map& operator=(tile_map&&) noexcept = default;

        template<typename TTileMapInitializer>
        explicit tile_map(std::optional<tile_point> max_size, const TTileMapInitializer& initializer,
                          flecs::world& world, int64 seed)
            requires tile_map_initializer<TTileMapSettings, TTileMapInitializer>
            : m_max_size(max_size)
            , m_settings(initializer, world, seed)
        {
        }

        explicit tile_map(std::optional<tile_point> max_size, flecs::world& world, int64 seed)
            : m_max_size(max_size)
            , m_settings(world, seed)
        {
        }

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

        std::queue<chunk_point> m_generation_queue;
        std::queue<chunk_point> m_remeshing_queue;

        TTileMapSettings m_settings;
    };

    template<typename TTileMapSettings, chunk_indexer... TChunkDims>
        requires tile_map_settings<TTileMapSettings, TChunkDims...>
    struct tile_shared_module
    {
        using tile_map_t = tile_map<TTileMapSettings, TChunkDims...>;
        using chunk_storage = tile_map_t::chunk_storage;
        using map_observer = tile_map_t::map_observer;
        using chunk_point = tile_map_t::chunk_point;
        using tile_point = tile_map_t::tile_point;
        using chunk_become_observed_event = tile_map_t::chunk_become_observed_event;
        using observed_by_tag = tile_map_t::observed_by;
        using observing_tag = tile_map_t::observing;
        using tile_map_settings = TTileMapSettings;

        struct tile_shared_data
        {
            flecs::entity m_chunk_prefab;
        };

        struct tile_generation_queue_entry
        {
            chunk_point start_point = {};
            chunk_point size = {};
            flecs::ref<tile_map_t> tile_map = {};
        };

        // ReSharper disable once CppNonExplicitConvertingConstructor
        tile_shared_module(flecs::world& world)
        {
            world.component<tile_map_t>();
            world.component<chunk_storage>();
            world.component<map_observer>();
            world.component<chunk_point>();

            world.component<tile_shared_data>().add(flecs::Singleton);
            world.set<tile_shared_data>(tile_shared_data {
                .m_chunk_prefab = world.prefab().set<chunk_storage>({}).template set<chunk_point>({}),
            });

            world.observer<const transform_3d, map_observer>()
                .event(flecs::OnSet)
                .event(flecs::OnAdd)
                .yield_existing()
                .run(proces_tile_observation);

            world.system<const tile_generation_queue_entry>().kind(flecs::PreUpdate).run([](flecs::iter it) {
                while(it.next())
                {
                    // generate some data for the map at the given chunk positions
                    /*const tile_generation_queue_entry& task = it.field<const tile_generation_queue_entry>(0);
                    if(const tile_map_t* map = task.tile_map.try_get())
                    {
                    }*/
                }
            });

            // world.system<map_observer, const transform_3d>().kind(flecs::PreUpdate).run(proces_tile_observation);
        }

        template<uint64... TMapSizes>
        static tile_point make_map_size(decltype(TMapSizes)... sizes)
        {
            static_assert(sizeof...(TMapSizes) == sizeof...(TChunkDims),
                          "Number of sizes must match number of dimensions");
            return tile_point {sizes...};
        }

        template<typename TTileMapInitializer>
            requires tile_map_initializer<TTileMapSettings, TTileMapInitializer>
        static flecs::ref<tile_map_t> create_map(flecs::entity into_entity, const TTileMapInitializer& initializer,
                                                 int64 seed, std::optional<tile_point> max_size = {})
        {
            into_entity.set<tile_map_t>(tile_map_t(max_size, initializer, into_entity.world(), seed));

            into_entity.observe<chunk_become_observed_event>([into_entity](const chunk_become_observed_event& event) {
                on_chunk_observed_event(into_entity, event);
            });

            return {into_entity};
        }

        static flecs::ref<tile_map_t> create_map(flecs::entity into_entity, int64 seed,
                                                 std::optional<tile_point> max_size = {})
        {
            flecs::world w = into_entity.world();
            into_entity.set<tile_map_t>(tile_map_t(max_size, w, seed));

            into_entity.observe<chunk_become_observed_event>([into_entity](const chunk_become_observed_event& event) {
                on_chunk_observed_event(into_entity, event);
            });

            return {into_entity};
        }

        static void on_chunk_observed_event(flecs::entity map_entity, const chunk_become_observed_event& event)
        {
            tile_map_t* map = map_entity.try_get_mut<tile_map_t>();
            if(!map)
            {
                panic(
                    "Cannot process chunk observed event on entity that is not a tile map.  Map is {}, observer is: {}",
                    map_entity, event.observer.entity());
            }
            const tile_shared_data& module = map_entity.world().get<tile_shared_data>();

            // do we have a chunk for this?
            auto it = map->m_observed_chunks.find(event.position);
            if(it == map->m_observed_chunks.end())
            {
                // If we're becoming observed, create the chunk
                if(event.became_observed)
                {
                    flecs::entity chunk_entity = map_entity.world().entity().is_a(module.m_chunk_prefab);

                    chunk_entity.set<chunk_point>(event.position);

                    map->m_observed_chunks.emplace(event.position, flecs::ref<chunk_storage> {chunk_entity});
                    event.observer.entity().template add_second<observing_tag>(chunk_entity);
                    // TODO: Mark this chunk as needed to be generated.
                }
                else
                {
                    // We're being unobserved, but we don't have a chunk, so nothing to do
                    return;
                }
            }
            else
            {
                flecs::ref<chunk_storage>& chunk_ref = it->second;
                // if we're becoming unobserved, and we have a chunk, we may need to remove it
                if(!event.became_observed)
                {
                    // remove the observer from the chunk
                    event.observer.entity().template remove_second<observing_tag>(chunk_ref.entity());
                }
                // If we're becoming observed, and we already have a chunk, add the observer
                else
                {
                    event.observer.entity().template add_second<observing_tag>(chunk_ref.entity());
                }
            }
        }

        static void proces_tile_observation(flecs::iter it)
        {
            while(it.next())
            {
                flecs::entity observer_entity = it.entity(0);
                const auto& transform = it.field<const transform_3d>(0);
                const flecs::field<map_observer>& observer = it.field<map_observer>(1);

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

                observer_entity.each<observing_tag>([&](flecs::entity chunk_entity) {
                    if(const chunk_point* point = chunk_entity.try_get<chunk_point>())
                    {
                        // If this chunk is being observed already, and it's part of the differences, remove it
                        if(observed_difference.contains(*point))
                        {
                            map.entity().template enqueue<chunk_become_observed_event>(chunk_become_observed_event {
                                .observer = flecs::ref<map_observer> {observer_entity},
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
                        .observer = flecs::ref<map_observer> {observer_entity},
                        .position = new_chunk_pos,
                        .became_observed = true,
                    });
                }
            }
        }
    };

}
