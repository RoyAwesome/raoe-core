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

    template<template<typename> typename TMaterialHandle>
    void draw_material_rect(glm::vec2 rect_min, glm::vec2 rect_max, const TMaterialHandle<shader::material>& material,
                            glm::vec2 uv_min = glm::vec2(0.0f, 0.0f), glm::vec2 uv_max = glm::vec2(1.0f, 1.0f),
                            const glm::u8vec4& color = colors::white, float rotation = 0.0f,
                            const glm::vec2& origin = glm::vec2(0.0f, 0.0f))
    {
        draw_material_rect(rect_min, rect_max, generic_handle(material), uv_min, uv_max, color, rotation, origin);
    }

    void draw_material_rect(glm::vec2 rect_min, glm::vec2 rect_max, const generic_handle<shader::material>& material,
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