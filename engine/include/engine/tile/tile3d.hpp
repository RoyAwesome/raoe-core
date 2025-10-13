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

namespace raoe::engine::tile
{
    struct tile_3d_module : tile_shared_module<integral_dimension<32>, integral_dimension<32>, integral_dimension<32>>
    {
        using Base = tile_shared_module;

        // ReSharper disable once CppNonExplicitConvertingConstructor
        tile_3d_module(flecs::world& world);
    };

}