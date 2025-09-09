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

#include "glm/ext.hpp"
#include "spdlog/spdlog.h"
#include <format>

#include "render/texture.hpp"

namespace raoe::render
{
    struct internal_render_assets
    {
        std::shared_ptr<texture_2d> white_texture;
    };

    internal_render_assets& get_internal_render_assets();
}

RAOE_CORE_DECLARE_FORMATTER(glm::uvec2, return format_to(ctx.out(), "({}, {})", value.x, value.y);)
RAOE_CORE_DECLARE_FORMATTER(glm::uvec3, return format_to(ctx.out(), "({}, {}, {})", value.x, value.y, value.z);)
RAOE_CORE_DECLARE_FORMATTER(glm::uvec4,
                            return format_to(ctx.out(), "({}, {}, {}, {})", value.x, value.y, value.z, value.w);)
RAOE_CORE_DECLARE_FORMATTER(glm::ivec2, return format_to(ctx.out(), "({}, {})", value.x, value.y);)
RAOE_CORE_DECLARE_FORMATTER(glm::ivec3, return format_to(ctx.out(), "({}, {}, {})", value.x, value.y, value.z);)
RAOE_CORE_DECLARE_FORMATTER(glm::ivec4,
                            return format_to(ctx.out(), "({}, {}, {}, {})", value.x, value.y, value.z, value.w);)
RAOE_CORE_DECLARE_FORMATTER(glm::bvec2, return format_to(ctx.out(), "({}, {})", value.x, value.y);)
RAOE_CORE_DECLARE_FORMATTER(glm::bvec3, return format_to(ctx.out(), "({}, {}, {})", value.x, value.y, value.z);)
RAOE_CORE_DECLARE_FORMATTER(glm::bvec4,
                            return format_to(ctx.out(), "({}, {}, {}, {})", value.x, value.y, value.z, value.w);)
RAOE_CORE_DECLARE_FORMATTER(glm::vec2, return format_to(ctx.out(), "({}, {})", value.x, value.y);)
RAOE_CORE_DECLARE_FORMATTER(glm::vec3, return format_to(ctx.out(), "({}, {}, {})", value.x, value.y, value.z);)
RAOE_CORE_DECLARE_FORMATTER(glm::vec4,
                            return format_to(ctx.out(), "({}, {}, {}, {})", value.x, value.y, value.z, value.w);)
RAOE_CORE_DECLARE_FORMATTER(
    glm::mat4, return format_to(ctx.out(), "\n[{}, {}, {}, {}]\n[{}, {}, {}, {}]\n[{}, {}, {}, {}]\n[{}, {}, {}, {}]",
                                value[0][0], value[1][0], value[2][0], value[3][0], value[0][1], value[1][1],
                                value[2][1], value[3][1], value[0][2], value[1][2], value[2][2], value[3][2],
                                value[0][3], value[1][3], value[2][3], value[3][3]);)
RAOE_CORE_DECLARE_FORMATTER(glm::mat3, return format_to(ctx.out(), "\n[{}, {}, {}]\n[{}, {}, {}]\n[{}, {}, {}]",
                                                        value[0][0], value[1][0], value[2][0], value[0][1], value[1][1],
                                                        value[2][1], value[0][2], value[1][2], value[2][2]);)
RAOE_CORE_DECLARE_FORMATTER(glm::mat2, return format_to(ctx.out(), "\n[{}, {}]\n[{}, {}]", value[0][0], value[1][0],
                                                        value[0][1], value[1][1]);)
RAOE_CORE_DECLARE_FORMATTER(glm::quat,
                            return format_to(ctx.out(), "({}, {}, {}, {})", value.x, value.y, value.z, value.w);)