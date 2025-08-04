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

namespace raoe::render
{

    enum class texture_type;
    class texture;
    template<texture_type TTextureType>
    class typed_texture;

    enum class renderer_type
    {
        none = 0,
        i8,
        i16,
        i32,
        u8,
        u16,
        u32,
        f32,
        f64,
        vec2,
        vec3,
        vec4,
        mat2,
        mat3,
        mat4,
        color,
        texture1d,
        texture2d,
        texture3d,
        texture_cube,
        texture1d_array,
        texture2d_array,
        texture_cube_array,

        any_texture, // any texture type, used for compile time uniform type checking, but will be checked at runtime if
                     // the sizes match up with a sampler in the shader

        custom,
        count,
    };

    constexpr bool is_texture_type(const renderer_type type)
    {
        return type == renderer_type::texture1d || type == renderer_type::texture2d ||
               type == renderer_type::texture3d || type == renderer_type::texture_cube ||
               type == renderer_type::texture1d_array || type == renderer_type::texture2d_array ||
               type == renderer_type::texture_cube_array || type == renderer_type::any_texture;
    }

    constexpr bool is_valid_renderer_type(const renderer_type type)
    {
        return type != renderer_type::none && type != renderer_type::custom && type != renderer_type::count;
    }

    enum class type_hint : uint8
    {
        none = 0,
        position,
        normal,
        uv,
        color,
        tangent,
        bitangent,
        count
    };

    struct type_description
    {
        renderer_type type = renderer_type::none;
        std::size_t offset = 0;
        std::size_t size = 0;
        type_hint hint = type_hint::none;
    };

    /*  Renderer Type Of
        This is a template struct that is used to describe the layout of a vertex buffer.
        It is specialized for each type that is used in a vertex buffer.

        assuming you have some vertex struct like
        struct simple_vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 uv;
        };

        you must specialize renderer_type_of for simple_vertex like so
        template<>
        struct renderer_type_of<simple_vertex>
        {
            static std::span<const type_description> elements()
            {
                static const std::array<const type_description, 3> ele = {
                    type_description {renderer_type::vec3, offsetof(simple_vertex, position), sizeof(glm::vec3)},
                    type_description {renderer_type::vec3, offsetof(simple_vertex, normal),   sizeof(glm::vec3)},
                    type_description {renderer_type::vec2, offsetof(simple_vertex, uv),       sizeof(glm::vec2)},
                };
                return ele;
            }
        };

        This will allow the renderer to know how to layout the vertex buffer for the simple_vertex type.

        TODO: This allows the type to be used as a uniform in shaders.
    */
    template<typename T>
    struct renderer_type_of
    {
        static_assert(false, "Specialization required for vertex_layout_of");
    };

    struct simple_vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    template<>
    struct renderer_type_of<simple_vertex>
    {
        static std::span<const type_description> elements()
        {
            static constexpr std::array<const type_description, 3> ele = {
                type_description {renderer_type::vec3, offsetof(simple_vertex, position), sizeof(glm::vec3),
                                  type_hint::position                                                                     },
                type_description {renderer_type::vec3, offsetof(simple_vertex, normal),   sizeof(glm::vec3),
                                  type_hint::normal                                                                       },
                type_description {renderer_type::vec2, offsetof(simple_vertex, uv),       sizeof(glm::vec2), type_hint::uv},
            };
            return ele;
        }
    };

    struct vertex_pos_uv_color_normal
    {
        glm::vec3 position;
        glm::vec2 uv;
        glm::u8vec4 color;
        glm::vec3 normal;
    };
    template<>
    struct renderer_type_of<vertex_pos_uv_color_normal>
    {
        static std::span<const type_description> elements()
        {
            static constexpr std::array<const type_description, 4> ele = {
                type_description {renderer_type::vec3,  offsetof(vertex_pos_uv_color_normal, position),
                                  sizeof(glm::vec3),                                                                       type_hint::position},
                type_description {renderer_type::vec2,  offsetof(vertex_pos_uv_color_normal, uv),       sizeof(glm::vec2),
                                  type_hint::uv                                                                                               },
                type_description {renderer_type::color, offsetof(vertex_pos_uv_color_normal, color),
                                  sizeof(glm::u8vec4),                                                                     type_hint::color   },
                type_description {renderer_type::vec3,  offsetof(vertex_pos_uv_color_normal, normal),   sizeof(glm::vec3),
                                  type_hint::normal                                                                                           },
            };
            return ele;
        }
    };

    constexpr std::size_t elements_hash(const std::span<const type_description> elements)
    {
        std::size_t hash = 0;
        for(const auto& [type, offset, size, hint] : elements)
        {
            hash = hash_combine(hash, underlying(type));
            hash = hash_combine(hash, offset);
            hash = hash_combine(hash, size);
            hash = hash_combine(hash, underlying(hint));
        }
        return hash;
    }

    template<typename T>
    concept type_described = requires(T t) {
        { renderer_type_of<std::remove_cv_t<T>>::elements() } -> std::convertible_to<std::span<const type_description>>;
    };

    template<type_described T>
    inline constexpr auto renderer_type_hash_v = elements_hash(renderer_type_of<T>::elements());

    template<typename T>
    inline constexpr auto shader_uniform_type_v = renderer_type::none;

    template<>
    inline constexpr auto shader_uniform_type_v<int8> = renderer_type::i8;
    template<>
    inline constexpr auto shader_uniform_type_v<uint8> = renderer_type::u8;
    template<>
    inline constexpr auto shader_uniform_type_v<int16> = renderer_type::i16;
    template<>
    inline constexpr auto shader_uniform_type_v<uint16> = renderer_type::u16;
    template<>
    inline constexpr auto shader_uniform_type_v<int32> = renderer_type::i32;
    template<>
    inline constexpr auto shader_uniform_type_v<uint32> = renderer_type::u32;
    template<>
    inline constexpr auto shader_uniform_type_v<float> = renderer_type::f32;
    template<>
    inline constexpr auto shader_uniform_type_v<double> = renderer_type::f64;
    template<>
    inline constexpr auto shader_uniform_type_v<glm::vec2> = renderer_type::vec2;
    template<>
    inline constexpr auto shader_uniform_type_v<glm::vec3> = renderer_type::vec3;
    template<>
    inline constexpr auto shader_uniform_type_v<glm::vec4> = renderer_type::vec4;
    template<>
    inline constexpr auto shader_uniform_type_v<glm::mat2> = renderer_type::mat2;
    template<>
    inline constexpr auto shader_uniform_type_v<glm::mat3> = renderer_type::mat3;
    template<>
    inline constexpr auto shader_uniform_type_v<glm::mat4> = renderer_type::mat4;

    template<typename T>
    concept index_buffer_type =
        std::is_same_v<std::remove_cv_t<T>, uint8> || std::is_same_v<std::remove_cv_t<T>, uint16> ||
        std::is_same_v<std::remove_cv_t<T>, uint32>;

    template<index_buffer_type T>
    struct renderer_type_of<T>
    {
        static std::span<const type_description> elements()
        {
            static const std::array<const type_description, 1> ele = {
                type_description {shader_uniform_type_v<T>, 0, sizeof(T)}
            };

            return ele;
        }
    };

}

RAOE_CORE_DECLARE_FORMATTER(
    raoe::render::renderer_type, switch(value) {
        case raoe::render::renderer_type::none: return format_to(ctx.out(), "renderer_type::none");
        case raoe::render::renderer_type::i8: return format_to(ctx.out(), "renderer_type::i8");
        case raoe::render::renderer_type::i16: return format_to(ctx.out(), "renderer_type::i16");
        case raoe::render::renderer_type::i32: return format_to(ctx.out(), "renderer_type::i32");
        case raoe::render::renderer_type::u8: return format_to(ctx.out(), "renderer_type::u8");
        case raoe::render::renderer_type::u16: return format_to(ctx.out(), "renderer_type::u16");
        case raoe::render::renderer_type::u32: return format_to(ctx.out(), "renderer_type::u32");
        case raoe::render::renderer_type::f32: return format_to(ctx.out(), "renderer_type::f32");
        case raoe::render::renderer_type::f64: return format_to(ctx.out(), "renderer_type::f64");
        case raoe::render::renderer_type::vec2: return format_to(ctx.out(), "renderer_type::vec2");
        case raoe::render::renderer_type::vec3: return format_to(ctx.out(), "renderer_type::vec3");
        case raoe::render::renderer_type::vec4: return format_to(ctx.out(), "renderer_type::vec4");
        case raoe::render::renderer_type::mat2: return format_to(ctx.out(), "renderer_type::mat2");
        case raoe::render::renderer_type::mat3: return format_to(ctx.out(), "renderer_type::mat3");
        case raoe::render::renderer_type::mat4: return format_to(ctx.out(), "renderer_type::mat4");
        case raoe::render::renderer_type::color: return format_to(ctx.out(), "renderer_type::color");
        case raoe::render::renderer_type::texture1d: return format_to(ctx.out(), "renderer_type::texture1d");
        case raoe::render::renderer_type::texture2d: return format_to(ctx.out(), "renderer_type::texture2d");
        case raoe::render::renderer_type::texture3d: return format_to(ctx.out(), "renderer_type::texture3d");
        case raoe::render::renderer_type::texture_cube: return format_to(ctx.out(), "renderer_type::texture_cube");
        case raoe::render::renderer_type::texture1d_array:
            return format_to(ctx.out(), "renderer_type::texture1d_array");
        case raoe::render::renderer_type::texture2d_array:
            return format_to(ctx.out(), "renderer_type::texture2d_array");
        case raoe::render::renderer_type::texture_cube_array:
            return format_to(ctx.out(), "renderer_type::texture_cube_array");
        case raoe::render::renderer_type::any_texture: return format_to(ctx.out(), "renderer_type::any_texture");
        case raoe::render::renderer_type::custom: return format_to(ctx.out(), "renderer_type::custom");
        case raoe::render::renderer_type::count: return format_to(ctx.out(), "renderer_type::count");
        default: return format_to(ctx.out(), "renderer_type::unknown");
    };)