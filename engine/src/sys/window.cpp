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

#include "engine/sys/window.hpp"
#include "engine/engine.hpp"

#include "glad/glad.h"

#include "GLFW/glfw3.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

namespace raoe::engine::sys
{
    void gl_error_callback(const GLenum source, const GLenum type, GLuint id, const GLenum severity, GLsizei length,
                           const GLchar* message, const void* userParam)
    {
        using namespace std::literals::string_view_literals;
        auto severity_str = ""sv;
        switch(severity)
        {
            case GL_DEBUG_SEVERITY_HIGH: severity_str = "HIGH"sv; break;
            case GL_DEBUG_SEVERITY_MEDIUM: severity_str = "MEDIUM"sv; break;
            case GL_DEBUG_SEVERITY_LOW: severity_str = "LOW"sv; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "NOTIFICATION"sv; break;
            default:;
        }

        auto type_str = ""sv;
        switch(type)
        {
            case GL_DEBUG_TYPE_ERROR: type_str = "GL_DEBUG_TYPE_ERROR"sv; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"sv; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"sv; break;
            case GL_DEBUG_TYPE_PORTABILITY: type_str = "GL_DEBUG_TYPE_PORTABILITY"sv; break;
            case GL_DEBUG_TYPE_PERFORMANCE: type_str = "GL_DEBUG_TYPE_PERFORMANCE"sv; break;
            case GL_DEBUG_TYPE_MARKER: type_str = "GL_DEBUG_TYPE_MARKER"sv; break;
            case GL_DEBUG_TYPE_PUSH_GROUP: type_str = "GL_DEBUG_TYPE_PUSH_GROUP"sv; break;
            case GL_DEBUG_TYPE_POP_GROUP: type_str = "GL_DEBUG_TYPE_POP_GROUP"sv; break;
            case GL_DEBUG_TYPE_OTHER: type_str = "GL_DEBUG_TYPE_OTHER"sv; break;
            default:;
        }

        auto src_str = ""sv;
        switch(source)
        {
            case GL_DEBUG_SOURCE_API: src_str = "GL_DEBUG_SOURCE_API"sv; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM: src_str = "GL_DEBUG_SOURCE_WINDOW_SYSTEM"sv; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: src_str = "GL_DEBUG_SOURCE_SHADER_COMPILER"sv; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY: src_str = "GL_DEBUG_SOURCE_THIRD_PARTY"sv; break;
            case GL_DEBUG_SOURCE_APPLICATION: src_str = "GL_DEBUG_SOURCE_APPLICATION"sv; break;
            case GL_DEBUG_SOURCE_OTHER: src_str = "GL_DEBUG_SOURCE_OTHER"sv; break;
            default:;
        }

        spdlog::level::level_enum log_level = spdlog::level::info;
        switch(severity)
        {
            case GL_DEBUG_SEVERITY_HIGH: log_level = spdlog::level::err; break;
            case GL_DEBUG_SEVERITY_MEDIUM: log_level = spdlog::level::warn; break;
            case GL_DEBUG_SEVERITY_LOW: log_level = spdlog::level::info; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: log_level = spdlog::level::trace; break;
            default:;
        }

        spdlog::log(log_level, "OpenGL [{}]: {} type={} Message:\"{}\"", severity_str, src_str, type_str, message);
        debug::debug_break_if(type == GL_DEBUG_TYPE_ERROR);
    }
    static void glfw_error_callback(int error_code, const char* description)
    {
        ensure(!error_code, "GLFW Error: {} {}", error_code, description);
    }

    void init_glfw(flecs::iter it)
    {
        it.world().defer_suspend();
        using namespace std::literals::string_view_literals;

        glfwSetErrorCallback(glfw_error_callback);

        check_if(glfwInit(), "Error initializing glfw");

        glfwWindowHintString(GLFW_WAYLAND_APP_ID, engine_info().app_name.c_str());

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        flecs::entity main_window = it.world().entity(entities::engine::main_window);
        // TODO: Configuration - Default Window Size should be configurable
        constexpr glm::ivec2 default_window_size = {1280, 720};

        auto window = create_window(main_window, engine_info().app_name, default_window_size, none);

        glfwMakeContextCurrent(window->native_handle());
        gladLoadGL();
        glfwSwapInterval(1);

        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(gl_error_callback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        glViewport(0, 0, default_window_size.x, default_window_size.y);

        IMGUI_CHECKVERSION();

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        spdlog::info("Initializing ImGui for window: {}", main_window);
        ImGui_ImplGlfw_InitForOpenGL(window->native_handle(), true);
        ImGui_ImplOpenGL3_Init();
        it.world().defer_resume();
    }

    void shutdown_glfw(flecs::iter it)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwTerminate();
    }

    void poll_glfw_events(flecs::iter it)
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void handle_glfw_events(flecs::iter it)
    {
        while(it.next())
        {
            if(const auto w = it.field<window>(0); glfwWindowShouldClose(w->native_handle()))
            {
                it.entity(0).destruct();
            }
        }
    }

    void check_for_any_windows_open(flecs::iter it)
    {
        // If there is only one window left, and we're in the OnRemove handler, exit the program.
        if(it.world().count<window>() <= 1)
        {
            it.world().quit();
        }
    }

    void present_glfw_window(flecs::iter it)
    {

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        while(it.next())
        {
            const auto handle = it.field<window>(0)->native_handle();
            glfwSwapBuffers(handle);
        }
    }

    window_module::window_module(flecs::world& world)
    {
        world.component<window>();

        world.system().kind(entities::startup::on_window_start).immediate().run(init_glfw);
        world.system().kind(flecs::PreFrame).run(poll_glfw_events);
        world.system<window>().kind(entities::render_tick::poll_window).run(handle_glfw_events);
        world.system<window>().kind(entities::render_tick::present).run(present_glfw_window);

        world.observer<window_module>().event(flecs::OnRemove).run(shutdown_glfw);
        world.observer<window>().event(flecs::OnRemove).run(check_for_any_windows_open);
    }

    std::string window::title() const noexcept
    {
        return glfwGetWindowTitle(m_window.get());
    }
    void window::set_title(const std::string& new_title) const
    {
        glfwSetWindowTitle(m_window.get(), new_title.data());
    }
    void window::set_size(const glm::ivec2 new_size) noexcept
    {
        m_size = new_size;
        glfwSetWindowSize(m_window.get(), new_size.x, new_size.y);
    }
    void window::set_mode(const window_mode_flags new_mode) noexcept
    {
        m_mode = new_mode;
        if(has_any_flags(m_mode, fullscreen))
        {
            auto* monitor = glfwGetPrimaryMonitor();
            const auto* vid_mode = glfwGetVideoMode(monitor);

            glfwSetWindowMonitor(m_window.get(), monitor, 0, 0, vid_mode->width, vid_mode->height,
                                 vid_mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor(m_window.get(), nullptr, 0, 0, m_size.x, m_size.y, GLFW_DONT_CARE);
        }

        glfwSetWindowAttrib(m_window.get(), GLFW_DECORATED, has_any_flags(m_mode, borderless) ? GLFW_FALSE : GLFW_TRUE);
        glfwSetWindowAttrib(m_window.get(), GLFW_RESIZABLE, has_any_flags(m_mode, resizable) ? GLFW_TRUE : GLFW_FALSE);
    }

    void window::glfw_deleter::operator()(GLFWwindow* window) const
    {
        glfwDestroyWindow(window);
    }
    flecs::ref<window> create_window(flecs::entity into_entity, const std::string_view title, const glm::ivec2 size,
                                     const window_mode_flags mode)
    {
        GLFWmonitor* monitor = nullptr;
        if(has_any_flags(mode, fullscreen))
        {
            monitor = glfwGetPrimaryMonitor();
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }
        else if(has_any_flags(mode, borderless))
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        }
        else if(has_any_flags(mode, resizable))
        {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        }
        else // classic windowed mode
        {
            glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }
        window new_window(std::shared_ptr<GLFWwindow>(glfwCreateWindow(size.x, size.y, title.data(), monitor, nullptr),
                                                      window::glfw_deleter {}),
                          size, mode);

        into_entity.set<window>(std::move(new_window));

        return {into_entity};
    }
}