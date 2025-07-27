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
#include "render/texture.hpp"

namespace raoe::render
{
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

        void set_camera_matrix(const glm::mat4& matrix) { camera_matrix = matrix; }
        [[nodiscard]] const glm::mat4& get_camera_matrix() const { return camera_matrix; }

      private:
        glm::mat4 camera_matrix = glm::identity<glm::mat4>();
    };

    struct render_assets
    {
        std::shared_ptr<shader::shader> error_shader;
        std::shared_ptr<texture::texture_2d> error_texture;
    };

    render_assets init_renderer();

    void render_mesh(const camera& camera, const mesh& mesh, const render_transform& render_transform,
                     const render_assets& render_assets);
}