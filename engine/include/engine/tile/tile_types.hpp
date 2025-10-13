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

}
