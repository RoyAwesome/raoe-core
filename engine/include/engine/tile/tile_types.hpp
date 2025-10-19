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

#include <concepts>
#include <type_traits>

namespace raoe::engine::tile
{
    template<typename E>
    concept indexable_enum =
        std::is_scoped_enum_v<E> && std::convertible_to<std::underlying_type_t<E>, size_t> && requires(E e) {
            { E::MAXVALUE } -> std::convertible_to<std::underlying_type_t<E>>;
        };

    template<typename T>
    concept dimension_type = requires(T t) {
        { t.stride() } -> std::convertible_to<size_t>;
        { t.index() } -> std::convertible_to<size_t>;
    };

    template<typename T>
    concept chunk_indexer = indexable_enum<T> || dimension_type<T>;

    template<indexable_enum T>
    constexpr size_t stride_of()
    {
        return static_cast<size_t>(T::MAXVALUE);
    }
    template<dimension_type T>
    constexpr size_t stride_of()
    {
        return T::stride();
    }

    template<indexable_enum T>
    constexpr size_t index_of(T indexer)
    {
        return static_cast<size_t>(indexer);
    }
    template<dimension_type T>
    constexpr size_t index_of(T indexer)
    {
        return indexer.index();
    }

    template<chunk_indexer... Dims>
    constexpr size_t total_stride()
    {
        return (stride_of<Dims>() * ...);
    }

    template<chunk_indexer... Dims>
    constexpr std::array<size_t, sizeof...(Dims)> strides_array()
    {
        return {stride_of<Dims>()...};
    }

    template<chunk_indexer... TChunkDims>
    struct tile_point_base
    {
        using tile_point_t = std::array<int64, sizeof...(TChunkDims)>;
        tile_point_base()
            : m_data {}
        {
        }

        template<std::integral... TIndices>
        explicit tile_point_base(TIndices... indices)
            : m_data {{static_cast<int64>(indices)...}}
        {
            static_assert(sizeof...(TIndices) == sizeof...(TChunkDims),
                          "Number of indices must match number of dimensions");
        }

        tile_point_base(const tile_point_base& other) = default;
        tile_point_base& operator=(const tile_point_base& other) = default;
        tile_point_base(tile_point_base&& other) noexcept = default;
        tile_point_base& operator=(tile_point_base&& other) noexcept = default;

        bool operator==(const tile_point_base& other) const noexcept
        {
            for(size_t i = 0; i < sizeof...(TChunkDims); i++)
            {
                if(m_data[i] != other.m_data[i])
                {
                    return false;
                }
            }
            return true;
        }

        tile_point_t m_data;
    };

    template<chunk_indexer... TChunkDims>
    struct tile_position final : public tile_point_base<TChunkDims...>
    {
    };

    template<chunk_indexer... TChunkDims>
    struct chunk_position final : public tile_point_base<TChunkDims...>
    {
        using base = tile_point_base<TChunkDims...>;
        chunk_position()
            : base()
        {
        }

        template<std::integral... TIndices>
        explicit chunk_position(TIndices... indices)
            : base(indices...)
        {
            static_assert(sizeof...(TIndices) == sizeof...(TChunkDims),
                          "Number of indices must match number of dimensions");
        }
        explicit chunk_position(tile_position<TChunkDims...> point)
            : base()
        {
            for(size_t i = 0; i < sizeof...(TChunkDims); i++)
            {
                base::m_data[i] = point.m_data[i] >= 0 ? point.m_data[i] / strides_array<TChunkDims...>()[i]
                                                       : (point.m_data[i] - (strides_array<TChunkDims...>()[i] - 1)) /
                                                             strides_array<TChunkDims...>()[i];
            }
        }

        explicit chunk_position(const transform_3d& transform)
            : base()
        {
            if constexpr(sizeof...(TChunkDims) >= 1)
            {
                base::m_data[0] = transform.position.x;
            }
            if constexpr(sizeof...(TChunkDims) >= 2)
            {
                base::m_data[1] = transform.position.y;
            }
            if constexpr(sizeof...(TChunkDims) >= 3)
            {
                base::m_data[1] = transform.position.z;
            }
            if constexpr(sizeof...(TChunkDims) >= 4)
            {
                for(int i = 2; i < sizeof...(TChunkDims); i++)
                {
                    base::m_data[i] = 0;
                }
            }
        }

        explicit chunk_position(const transform_2d& transform)
            : base()
        {
            if constexpr(sizeof...(TChunkDims) >= 1)
            {
                base::m_data[0] = transform.position.x;
            }
            if constexpr(sizeof...(TChunkDims) >= 2)
            {
                base::m_data[1] = transform.position.y;
            }
            if constexpr(sizeof...(TChunkDims) >= 3)
            {
                for(int i = 2; i < sizeof...(TChunkDims); i++)
                {
                    base::m_data[i] = 0;
                }
            }
        }

        static chunk_position init_from_integral(std::integral auto ind)
        {
            chunk_position result;
            result.m_data.fill(static_cast<int64>(ind));
            return result;
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
            return base::m_data[N];
        }

        tile_position<TChunkDims...> to_tile_position() const
        {
            tile_position pos;
            for(size_t i = 0; i < sizeof...(TChunkDims); i++)
            {
                pos.m_data[i] = base::m_data[i] * strides_array<TChunkDims...>()[i];
            }
            return pos;
        }
    };

}

template<raoe::engine::tile::chunk_indexer... TChunkDims>
struct std::hash<raoe::engine::tile::chunk_position<TChunkDims...>>
{
    std::size_t operator()(const raoe::engine::tile::chunk_position<TChunkDims...>& k) const
    {
        std::size_t h = 0;
        for(size_t i = 0; i < sizeof...(TChunkDims); i++)
        {
            h = raoe::hash_combine(h, k.m_data[i]);
        }
        return h;
    }
};