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

#include "engine/tile/tile_storage.hpp"

namespace raoe::engine::tile
{

    template<chunk_indexer... TChunkDims>
    struct tile_map
    {
        using chunk_storage = tile_storage_chunk<flecs::entity, TChunkDims...>;
        using tile_point_t = std::array<int64, sizeof...(TChunkDims)>;

        struct tile_point_base
        {
            tile_point_base()
                : m_data {}
            {
            }
            template<int64... TIndices>
            tile_point_base(decltype(TIndices)... indices)
                : m_data {{indices...}}
            {
                static_assert(sizeof...(TIndices) == sizeof...(TChunkDims),
                              "Number of indices must match number of dimensions");
            }

            tile_point_base(const tile_point_base& other) = default;
            tile_point_base& operator=(const tile_point_base& other) = default;
            tile_point_base(tile_point_base&& other) noexcept = default;
            tile_point_base& operator=(tile_point_base&& other) noexcept = default;

            tile_point_t m_data;
        };

        struct tile_position final : public tile_point_base
        {
        };

        struct chunk_position final : public tile_point_base
        {
            chunk_position()
                : tile_point_base()
            {
            }
            template<int64... TIndices>
            explicit chunk_position(decltype(TIndices)... indices)
                : tile_point_base(indices...)
            {
                static_assert(sizeof...(TIndices) == sizeof...(TChunkDims),
                              "Number of indices must match number of dimensions");
            }
            explicit chunk_position(tile_position point)
                : tile_point_base()
            {
                for(size_t i = 0; i < sizeof...(TChunkDims); i++)
                {
                    tile_point_base::m_data[i] = point.m_data[i] >= 0
                                                     ? point.m_data[i] / strides_array<TChunkDims...>()[i]
                                                     : (point.m_data[i] - (strides_array<TChunkDims...>()[i] - 1)) /
                                                           strides_array<TChunkDims...>()[i];
                }
            }

            chunk_position(const chunk_position& other) = default;
            chunk_position& operator=(const chunk_position& other) = default;
            chunk_position(chunk_position&& other) noexcept = default;
            chunk_position& operator=(chunk_position&& other) noexcept = default;

            chunk_position operator+(const chunk_position& other) const
            {
                chunk_position result = *this;
                for(size_t i = 0; i < sizeof...(TChunkDims); i++)
                {
                    result.m_data[i] += other.m_data[i];
                }
                return result;
            }
            chunk_position operator-(const chunk_position& other) const
            {
                chunk_position result = *this;
                for(size_t i = 0; i < sizeof...(TChunkDims); i++)
                {
                    result.m_data[i] -= other.m_data[i];
                }
                return result;
            }

            template<std::size_t N>
            int64 get() const
            {
                static_assert(N < sizeof...(TChunkDims), "Index out of range");
                return tile_point_base::m_data[N];
            }

            tile_position to_tile_position() const
            {
                tile_position pos;
                for(size_t i = 0; i < sizeof...(TChunkDims); i++)
                {
                    pos.m_data[i] = tile_point_base::m_data[i] * strides_array<TChunkDims...>()[i];
                }
                return pos;
            }
        };

        struct data_change_event
        {
        };
        struct needs_remesh_event
        {
        };
        struct parent_map
        {
        };

        struct map_observer
        {
            int32 m_range = 8; // Range in chunks to observe around the origin
        };

        // Maximum tile map size, in tiles.  Can be none, which means infinite world
        std::optional<tile_point_t> m_max_size;
    };

    template<chunk_indexer... TChunkDims>
    struct tile_shared_module
    {
        using TileMapType = tile_map<TChunkDims...>;
        using TileMapChunkStorageType = TileMapType::chunk_storage;

        flecs::entity m_chunk_prefab;
        // ReSharper disable once CppNonExplicitConvertingConstructor
        tile_shared_module(flecs::world& world)
        {
            world.component<TileMapType>();
            world.component<TileMapChunkStorageType>();

            m_chunk_prefab =
                world.prefab().set<TileMapChunkStorageType>({}).template set<typename TileMapType::chunk_position>({});
        }

        template<uint64... TMapSizes>
        static TileMapType::tile_point_t make_map_size(decltype(TMapSizes)... sizes)
        {
            static_assert(sizeof...(TMapSizes) == sizeof...(TChunkDims),
                          "Number of sizes must match number of dimensions");
            return typename TileMapType::tile_point_t {sizes...};
        }

        static flecs::ref<TileMapType> create_map(flecs::entity into_entity,
                                                  std::optional<typename TileMapType::tile_point_t> max_size = {})
        {

            into_entity.set<TileMapType>(TileMapType {
                .m_max_size = max_size,
            });

            return {into_entity};
        }
    };
}