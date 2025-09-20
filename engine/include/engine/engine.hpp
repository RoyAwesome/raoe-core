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
#include "render/render.hpp"
#include "spdlog/spdlog.h"
#include <format>

#include "flecs.h"

namespace raoe::engine
{
    enum class engine_flags : uint32
    {
        none = 0,
        headless = 1 << 0,  // Run without a window (no rendering)
        test_mode = 1 << 1, // Run in test mode (for unit tests, causing offline rendering)
    };
    struct engine_info_t
    {
        std::span<std::string_view> command_line_args;
        std::string app_name;
        std::string app_version;
        std::string org_name;
        std::string org_version;

        std::string engine_version = "0.1.0";

        engine_flags flags = engine_flags::none;
    };

    struct transform_3d
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        [[nodiscard]] glm::mat4 to_matrix() const noexcept
        {
            auto mat = glm::identity<glm::mat4>();
            mat = glm::translate(mat, position);
            mat *= glm::mat4_cast(rotation);
            mat = glm::scale(mat, scale);
            return mat;
        }
    };
    struct transform_2d
    {
        glm::vec2 position = glm::vec2(0.0f);
        float rotation = 0.0f; // in radians
        glm::vec2 scale = glm::vec2(1.0f);
        glm::vec2 origin = glm::vec2(0.0f);

        [[nodiscard]] glm::mat4 to_matrix() const noexcept
        {
            auto mat = glm::identity<glm::mat4>();
            // 2D transformations are in the XY plane, so we use a translation, rotation around Z, and scale
            mat = glm::translate(mat, glm::vec3(position + origin, 0.0f)); // Move to origin first for rotation
            mat = glm::rotate(mat, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            mat = glm::translate(mat, glm::vec3(-origin, 0.0f)); // Move back from origin
            mat = glm::scale(mat, glm::vec3(scale, 1.0f));
            return mat;
        }
    };

    // Returns a handle to the engine's ECS world.
    flecs::world world() noexcept;

    // Initializes the engine's ECS world and returns a handle to it.
    [[nodiscard]] flecs::world init_engine(int argc, char* argv[], std::string app_name, const std::string& org_name,
                                           engine_flags flags = engine_flags::none);

    // Shuts down the engine's ECS world and cleans up resources.
    void shutdown_engine() noexcept;

    // Returns the command line arguments passed to the engine.
    std::span<std::string_view> command_line_args();

    const engine_info_t& engine_info() noexcept;

    template<typename T>
        requires std::is_enum_v<T>
    void register_pipeline(flecs::world& world, flecs::entity_t initial_phase, T start, T end)
    {
        world.entity(static_cast<T>(start)).add(flecs::Phase).depends_on(initial_phase);
        for(auto i = std::underlying_type_t<T>(start) + 1; i <= std::underlying_type_t<T>(end); ++i)
        {
            world.entity(static_cast<T>(i)).add(flecs::Phase).depends_on(static_cast<T>(i - 1));
        }
    }

    namespace entities
    {
        enum class engine
        {
            main_window,
            main_camera,
            camera_2d,
            core_pack,
        };

        enum class startup
        {
            on_pre_init,

            on_window_start,
            on_render_start,

            on_script_init,
            on_script_post_init,
            on_engine_init,

            on_game_pre_start,
            on_game_start,
        };

        enum class render_tick
        {
            render_begin,
            poll_window,
            prepare_frame,
            draw,
            post_draw,
            present,
            render_end,
        };

        enum class engine_assets
        {
            error_texture,
            error_shader,
            default_material,
            immediate_mode_shader,
            engine_ubo,
            camera_ubo,
        };
    }

    struct scoped_world_defer_suspend
    {
        explicit scoped_world_defer_suspend(flecs::world_t* world)
            : world(world)
        {
            ecs_defer_suspend(world);
        }
        ~scoped_world_defer_suspend() { ecs_defer_resume(world); }
        flecs::world_t* world;
    };

}

RAOE_CORE_FLAGS_ENUM(raoe::engine::engine_flags)

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

RAOE_CORE_DECLARE_FORMATTER(
    flecs::entity, if(value.name().length() == 0) return format_to(ctx.out(), "flecs::entity(id: \'{}\')", value.id());
    else return format_to(ctx.out(), "flecs::entity(id: \'{}\', name: \'{}\')", value.id(), value.name().c_str());)