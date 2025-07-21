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

struct GLFWwindow;

namespace raoe::engine::sys
{
    struct window_module
    {
        explicit window_module(flecs::world& world);
    };

    enum window_mode_flags
    {
        none = 0,
        fullscreen = 1 << 0,
        borderless = 1 << 1,
        resizable = 1 << 2,
        maximized = 1 << 3,
        minimized = 1 << 4,
    };

    class window
    {
      public:
        window() = default;

        [[nodiscard]] glm::ivec2 size() const noexcept { return m_size; }
        [[nodiscard]] window_mode_flags mode() const noexcept { return m_mode; }
        [[nodiscard]] std::string title() const noexcept;

        void set_title(const std::string& new_title) const;
        void set_size(glm::ivec2 new_size) noexcept;
        void set_mode(window_mode_flags new_mode) noexcept;

        [[nodiscard]] GLFWwindow* native_handle() const noexcept { return m_window.get(); }

      private:
        friend flecs::ref<window> create_window(flecs::entity into_entity, std::string_view title, glm::ivec2 size,
                                                window_mode_flags mode);
        explicit window(std::shared_ptr<GLFWwindow> in_window, const glm::ivec2 in_size,
                        const window_mode_flags in_flags)
            : m_mode(in_flags)
            , m_size(in_size)
            , m_window(std::move(in_window))
        {
        }
        struct glfw_deleter
        {
            void operator()(GLFWwindow* window) const;
        };
        window_mode_flags m_mode = none;
        glm::ivec2 m_size = {};
        std::shared_ptr<GLFWwindow> m_window;
    };

    flecs::ref<window> create_window(flecs::entity into_entity, std::string_view title, glm::ivec2 size,
                                     window_mode_flags mode);
}

RAOE_CORE_FLAGS_ENUM(raoe::engine::sys::window_mode_flags);