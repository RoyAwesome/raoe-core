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
#include "render/colors.hpp"
#include "render/shader.hpp"
#include "render/texture.hpp"

namespace raoe::render
{
    class mesh_element;
    namespace shader
    {
        class shader;
    }

    struct mesh;

    struct render_transform
    {
        glm::mat4 cached_local_transform = glm::identity<glm::mat4>();
        glm::mat4 cached_world_transform = glm::identity<glm::mat4>();
    };

    class camera
    {
      public:
        camera() = default;

        explicit camera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
            : camera_matrix(glm::lookAt(position, target, up))
        {
        }

        explicit camera(const glm::mat4& matrix)
            : camera_matrix(matrix)
        {
        }

        void set_projection_matrix(const glm::mat4& matrix) { projection_matrix = matrix; }
        [[nodiscard]] const glm::mat4& get_projection_matrix() const { return projection_matrix; }

        void set_camera_matrix(const glm::mat4& matrix) { camera_matrix = matrix; }
        [[nodiscard]] const glm::mat4& get_camera_matrix() const { return camera_matrix; }

        camera& look_at(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
        {
            camera_matrix = glm::lookAt(position, target, up);
            return *this;
        }
        camera& translate(const glm::vec3& translation)
        {
            camera_matrix = glm::translate(camera_matrix, translation);
            return *this;
        }
        camera& rotate(const float angle, const glm::vec3& axis)
        {
            camera_matrix = glm::rotate(camera_matrix, angle, axis);
            return *this;
        }

        camera& with_orthographic(const float left, const float right, const float bottom, const float top,
                                  const float near = -1.0f, const float far = 1.0f)
        {
            projection_matrix = glm::ortho(left, right, bottom, top, near, far);
            return *this;
        }
        camera& with_perspective(const float fov_y, const float aspect_ratio, const float near, const float far)
        {
            projection_matrix = glm::perspective(fov_y, aspect_ratio, near, far);
            return *this;
        }

        [[nodiscard]] glm::mat4 get_view_projection_matrix() const { return projection_matrix * camera_matrix; }

      private:
        glm::mat4 camera_matrix = glm::identity<glm::mat4>();
        glm::mat4 projection_matrix = glm::identity<glm::mat4>();
    };

    struct render_context
    {
        std::shared_ptr<shader::shader> error_shader;
        std::shared_ptr<shader::shader> generic_2d_shader;
        std::shared_ptr<texture_2d> error_texture;
        glm::ivec2 surface_size;
        shader::glsl::file_load_callback_t load_callback;
    };

    void set_render_context(const render_context& ctx);
    render_context& get_render_context();

    std::shared_ptr<texture_2d> generate_checkerboard_texture(const glm::ivec2& size, const glm::u8vec4& color1,
                                                              const glm::u8vec4& color2, const int square_size);

    void render_mesh(const camera& camera, const mesh& mesh, const render_transform& render_transform);
    void render_mesh_element(mesh_element& mesh_element);
    void clear_surface(glm::u8vec4 color);
}