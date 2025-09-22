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

#include <any>

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
        type_hint hint = type_hint::none;
        std::size_t array_size = 1;
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

        This will allow the renderer to know how to lay out the vertex buffer for the simple_vertex type.
    */
    template<typename>
    struct renderer_type_of
    {
        static_assert(false, "Specialization required for renderer_type_of");
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
                type_description {
                                  .type = renderer_type::vec3,
                                  .offset = offsetof(simple_vertex, position),
                                  .hint = type_hint::position,
                                  },
                type_description {
                                  .type = renderer_type::vec3,
                                  .offset = offsetof(simple_vertex,                                                                   normal),
                                  .hint = type_hint::normal,
                                  },
                type_description {
                                  .type = renderer_type::vec2,
                                  .offset = offsetof(simple_vertex,       uv),
                                  .hint = type_hint::uv,
                                  },
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
                type_description {
                                  .type = renderer_type::vec3,
                                  .offset = offsetof(vertex_pos_uv_color_normal, position),
                                  .hint = type_hint::position,
                                  },
                type_description {
                                  .type = renderer_type::vec2,
                                  .offset = offsetof(vertex_pos_uv_color_normal,                                                                                uv),
                                  .hint = type_hint::uv,
                                  },
                type_description {
                                  .type = renderer_type::color,
                                  .offset = offsetof(vertex_pos_uv_color_normal,                        color),
                                  .hint = type_hint::color,
                                  },
                type_description {
                                  .type = renderer_type::vec3,
                                  .offset = offsetof(vertex_pos_uv_color_normal, normal),
                                  .hint = type_hint::normal,
                                  },
            };
            return ele;
        }
    };

    constexpr std::size_t elements_hash(const std::span<const type_description> elements)
    {
        std::size_t hash = 0;
        for(const auto& [type, offset, hint, array_size] : elements)
        {
            hash = hash_combine(hash, underlying(type));
            hash = hash_combine(hash, offset);
            hash = hash_combine(hash, underlying(hint));
            hash = hash_combine(hash, array_size);
        }
        return hash;
    }

    constexpr bool elements_equal(const std::span<const type_description> a,
                                  const std::span<const type_description> b) noexcept
    {
        if(a.size() != b.size())
        {
            return false;
        }

        for(std::size_t i = 0; i < a.size(); i++)
        {
            if(a[i].type != b[i].type || a[i].offset != b[i].offset || a[i].hint != b[i].hint ||
               a[i].array_size != b[i].array_size)
            {
                return false;
            }
        }

        return true;
    }

    template<typename T>
    concept type_described = requires(T t) {
        {
            renderer_type_of<std::remove_cvref_t<T>>::elements()
        } -> std::convertible_to<std::span<const type_description>>;
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
    template<>
    inline constexpr auto shader_uniform_type_v<glm::u8vec4> = renderer_type::color;

    template<renderer_type TType>
    inline constexpr auto shader_uniform_size_v = 0;
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::u8> = sizeof(int8);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::i8> = sizeof(uint8);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::u16> = sizeof(uint16);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::i16> = sizeof(int16);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::u32> = sizeof(uint32);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::i32> = sizeof(int32);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::f32> = sizeof(float);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::f64> = sizeof(double);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::vec2> = sizeof(glm::vec2);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::vec3> = sizeof(glm::vec3);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::vec4> = sizeof(glm::vec4);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::mat2> = sizeof(glm::mat2);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::mat3> = sizeof(glm::mat3);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::mat4> = sizeof(glm::mat4);
    template<>
    inline constexpr auto shader_uniform_size_v<renderer_type::color> = sizeof(glm::u8vec4);

    constexpr std::size_t shader_uniform_size(const renderer_type type)
    {
        switch(type)
        {
            case renderer_type::u8: return shader_uniform_size_v<renderer_type::u8>;
            case renderer_type::i8: return shader_uniform_size_v<renderer_type::i8>;
            case renderer_type::u16: return shader_uniform_size_v<renderer_type::u16>;
            case renderer_type::i16: return shader_uniform_size_v<renderer_type::i16>;
            case renderer_type::u32: return shader_uniform_size_v<renderer_type::u32>;
            case renderer_type::i32: return shader_uniform_size_v<renderer_type::i32>;
            case renderer_type::f32: return shader_uniform_size_v<renderer_type::f32>;
            case renderer_type::f64: return shader_uniform_size_v<renderer_type::f64>;
            case renderer_type::vec2: return shader_uniform_size_v<renderer_type::vec2>;
            case renderer_type::vec3: return shader_uniform_size_v<renderer_type::vec3>;
            case renderer_type::vec4: return shader_uniform_size_v<renderer_type::vec4>;
            case renderer_type::mat2: return shader_uniform_size_v<renderer_type::mat2>;
            case renderer_type::mat3: return shader_uniform_size_v<renderer_type::mat3>;
            case renderer_type::mat4: return shader_uniform_size_v<renderer_type::mat4>;
            case renderer_type::color: return shader_uniform_size_v<renderer_type::color>;
            default: return 0;
        }
    }

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
                type_description {
                                  .type = shader_uniform_type_v<T>,
                                  }
            };

            return ele;
        }
    };

    template<template<typename> class THandle, typename T>
    concept asset_handle =
        requires(THandle<T> a) {
            { a.get() } -> std::convertible_to<T*>;
            { std::hash<THandle<T>> {}(a) } -> std::convertible_to<std::size_t>;
        } && std::is_move_constructible_v<THandle<T>> && std::is_move_assignable_v<THandle<T>> &&
        std::is_copy_assignable_v<THandle<T>> && std::is_copy_constructible_v<THandle<T>> &&
        std::convertible_to<THandle<T>, bool>;

    template<typename T>
    struct generic_handle
    {
        struct vtable
        {
            T* (*get)(const std::any& handle);
            bool (*is_valid)(const std::any& handle);
        };
        template<template<typename> class THandle>
        generic_handle(const THandle<T>& handle)
            : m_handle(handle)
        {
            m_vtable = vtable {
                .get = [](const std::any& h) -> T* { return std::any_cast<THandle<T>>(h).get(); },
                .is_valid = [](const std::any& h) -> bool { return static_cast<bool>(std::any_cast<THandle<T>>(h)); },
            };
        }

        template<typename U>
            requires std::derived_from<U, T>
        generic_handle(const generic_handle<U>& handle)
            : m_handle(handle)
        {
            m_vtable = vtable {
                .get = [](const std::any& h) -> T* { return std::any_cast<generic_handle<U>>(h).get(); },
                .is_valid = [](const std::any& h) -> bool {
                    return static_cast<bool>(std::any_cast<generic_handle<U>>(h));
                },
            };
        }

        generic_handle()
            : m_handle(std::any())
        {
            m_vtable = vtable {
                .get = [](const std::any&) -> T* { return nullptr; },
                .is_valid = [](const std::any&) -> bool { return false; },
            };
        }

        T* operator->() const noexcept { return get(); }
        T* get() const { return m_vtable.get(m_handle); }
        operator bool() const { return m_vtable.is_valid(m_handle); }

        bool operator==(const generic_handle& rhs) const { return get() == rhs.get(); }
        bool operator==(std::nullptr_t) const { return get() == nullptr; }

      private:
        std::any m_handle;
        vtable m_vtable;
    };
    // Deduction guide to convert shared ptr to generic_handle
    template<typename T>
    generic_handle(std::shared_ptr<T>) -> generic_handle<T>;

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

RAOE_CORE_DECLARE_FORMATTER(
    raoe::render::type_hint, switch(value) {
        case raoe::render::type_hint::none: return format_to(ctx.out(), "type_hint::none");
        case raoe::render::type_hint::position: return format_to(ctx.out(), "type_hint::position");
        case raoe::render::type_hint::normal: return format_to(ctx.out(), "type_hint::normal");
        case raoe::render::type_hint::uv: return format_to(ctx.out(), "type_hint::uv");
        case raoe::render::type_hint::color: return format_to(ctx.out(), "type_hint::color");
        case raoe::render::type_hint::tangent: return format_to(ctx.out(), "type_hint::tangent");
        case raoe::render::type_hint::bitangent: return format_to(ctx.out(), "type_hint::bitangent");
        case raoe::render::type_hint::count: return format_to(ctx.out(), "type_hint::count");
        default: return format_to(ctx.out(), "type_hint::unknown");
    };)

RAOE_CORE_DECLARE_FORMATTER(raoe::render::type_description,
                            return format_to(ctx.out(),
                                             "Type Description {{ type: {}, offset: {}, hint: {}, array_size: {} }}",
                                             value.type, value.offset, value.hint, value.array_size);)