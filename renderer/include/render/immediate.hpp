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

#include "render.hpp"

#include <any>

namespace raoe::render
{
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

      private:
        std::any m_handle;
        vtable m_vtable;
    };

    template<template<typename> typename TTextureHandle>
    void draw_2d_texture_rect(glm::vec2 rect_min, glm::vec2 rect_max, const TTextureHandle<texture_2d>& texture,
                              glm::vec2 uv_min = glm::vec2(0.0f, 0.0f), glm::vec2 uv_max = glm::vec2(1.0f, 1.0f),
                              const glm::u8vec4& color = colors::white, float rotation = 0.0f,
                              const glm::vec2& origin = glm::vec2(0.0f, 0.0f))
    {
        draw_2d_texture_rect(rect_min, rect_max, generic_handle(texture), uv_min, uv_max, color, rotation, origin);
    }

    void draw_2d_texture_rect(glm::vec2 rect_min, glm::vec2 rect_max, const generic_handle<texture_2d>& texture,
                              glm::vec2 uv_min = glm::vec2(0.0f, 0.0f), glm::vec2 uv_max = glm::vec2(1.0f, 1.0f),
                              const glm::u8vec4& color = colors::white, float rotation = 0.0f,
                              const glm::vec2& origin = glm::vec2(0.0f, 0.0f));

    void draw_2d_rect(const glm::vec2& rect_min, const glm::vec2& rect_max, const glm::u8vec4& color = colors::white,
                      float rotation = 0.0f, const glm::vec2& origin = glm::vec2(0.0f, 0.0f));

    namespace immediate
    {
        void begin_immediate_batch();
        void draw_immediate_batch(const uniform_buffer& engine_ubo, const uniform_buffer& camera_ubo);
    }

}

template<typename T>
struct std::hash<raoe::render::generic_handle<T>>
{
    std::size_t operator()(const raoe::render::generic_handle<T>& handle) const noexcept
    {
        return std::hash<T*>()(handle.get());
    }
};