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

#include "core/core.hpp"

#include "engine/tile/tile_types.hpp"

#include <array>
#include <vector>

namespace raoe::engine::tile
{
    template<std::equality_comparable StorageType, chunk_indexer... Dims>
    class tile_storage_chunk
    {
      public:
        tile_storage_chunk() = default;
        explicit tile_storage_chunk(StorageType&& default_value)
            : tile_storage_chunk()
        {
            m_palette.emplace_back(default_value);
            m_data.fill(1);
        }

        [[nodiscard]] std::optional<StorageType> get(decltype(Dims {})... ind) const noexcept
        {
            size_t linear_index = linearize(ind...);
            if(linear_index >= m_data.size())
            {
                return std::nullopt; // Out of bounds
            }

            uint16 pallet_index = m_data[linear_index];
            if(pallet_index == 0 || pallet_index > m_palette.size())
            {
                return std::nullopt; // Invalid index or no value set
            }
            return m_palette[pallet_index - 1]; // Convert to zero-based index
        }

        [[nodiscard]] std::optional<StorageType> operator[](decltype(Dims {})... ind) const noexcept
        {
            return get(ind...);
        }

        [[nodiscard]] StorageType get_checked(decltype(Dims {})... ind) const
        {
            size_t linear_index = linearize(ind...);
            check_if(linear_index < m_data.size(), "Index out of range for tile storage chunk");

            uint16 pallet_index = m_data[linear_index];
            check_if(pallet_index != 0 && pallet_index <= m_palette.size(), "No value set for given index");

            return m_palette[pallet_index - 1]; // Convert to zero-based index
        }

        void set(StorageType value, decltype(Dims {})... ind) noexcept
        {
            size_t linear_index = linearize(ind...);
            raoe::check_if(linear_index < m_data.size(), "Index out of range for tile storage chunk");

            auto pallet_itr = std::ranges::find(m_palette, value);
            if(pallet_itr != m_palette.end())
            {
                // Value already exists in the palette, use its index
                m_data[linear_index] = static_cast<uint16>(std::distance(m_palette.begin(), pallet_itr) + 1);
            }
            else
            {
                // New value, add to palette
                m_palette.push_back(value);
                m_data[linear_index] = static_cast<uint16>(m_palette.size());
            }
        }

        template<size_t DimIndex>
        static bool index_in_range(chunk_indexer auto indexer)
        {
            static_assert(DimIndex < sizeof...(Dims), "DimIndex must be less than the number of dimensions");
            return index_of(indexer) < stride_of<decltype(indexer)>();
        }

        template<size_t DimIndex>
        static constexpr size_t stride_of_dim()
        {
            static_assert(DimIndex < sizeof...(Dims), "DimIndex must be less than the number of dimensions");
            // TODO: cpp26 pack indexing:
            //  return stride_of<Dims[DimIndex]>();
            return std::get<DimIndex>(std::tuple {stride_of<decltype(Dims {})>()...});
        }

      private:
        static constexpr size_t data_size = total_stride<Dims...>();
        std::vector<StorageType> m_palette;
        std::array<uint16, data_size> m_data;

        // Linearizes the indices of the dimensions into a single index
        // using template recursion (TODO: C++26 template-for and pack indexing will make this easier)
        template<chunk_indexer... Dims2>
        constexpr static size_t linearize(chunk_indexer auto indexer, Dims2&... dims)
        {
            constexpr size_t dim_index = sizeof...(Dims) - sizeof...(Dims2) - 1;
            raoe::check_if(index_in_range<dim_index>(indexer), "Index out of range for dimension {}", dim_index);
            return (index_of(indexer) * (stride_of<decltype(Dims2 {})>() * ...)) + linearize(dims...);
        }
        // Base case for recursion
        constexpr static size_t linearize(chunk_indexer auto indexer) { return index_of(indexer); }
    };

    template<size_t N>
    struct integral_dimension
    {
        static constexpr size_t stride() { return N; }

        constexpr integral_dimension(const size_t index = 0)
            : m_index(index)
        {
        }
        [[nodiscard]] constexpr size_t index() const { return m_index; }

        size_t m_index;
    };

}