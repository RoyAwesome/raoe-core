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

#include "render/mesh.hpp"
#include "render/render.hpp"
#include "render/shader.hpp"

#include <vector>

#include "glm/ext.hpp"

namespace raoe::render
{
    namespace quad_builder
    {
        enum class axis
        {
            x,
            y,
            z
        };
        enum class face_direction
        {
            x_plus,
            x_minus,
            y_plus,
            y_minus,
            z_plus,
            z_minus
        };
        enum class permutation
        {
            Xyz,
            Zxy,
            Yzx,
        };

        inline axis axis_from_direction(const face_direction direction)
        {
            switch(direction)
            {
                case face_direction::x_minus:
                case face_direction::x_plus: return axis::x;
                case face_direction::y_plus:
                case face_direction::y_minus: return axis::y;
                case face_direction::z_plus:
                case face_direction::z_minus: return axis::z;
            }
            return axis::x; // Default case, should not happen
        }

        inline std::tuple<axis, axis, axis> axes_from_permutation(const permutation perm)
        {
            switch(perm)
            {
                case permutation::Xyz: return {axis::x, axis::y, axis::z};
                case permutation::Zxy: return {axis::z, axis::x, axis::y};
                case permutation::Yzx: return {axis::y, axis::z, axis::x};
            }
            return {axis::x, axis::y, axis::z}; // Default case, should not happen
        }

        struct oriented_face
        {
            int32 sign;
            permutation perm;

            glm::vec3 n;
            glm::vec3 u;
            glm::vec3 v;
        };
        inline oriented_face face_from_direction(const face_direction direction)
        {
            const int32 sign =
                std::to_underlying(direction) % 2 == 0 ? 1 : -1; // Get the sign based on the face direction

            permutation perm;
            switch(direction)
            {
                case face_direction::x_minus:
                case face_direction::x_plus:
                default: perm = permutation::Xyz; break;
                case face_direction::y_plus:
                case face_direction::y_minus: perm = permutation::Yzx; break;
                case face_direction::z_plus:
                case face_direction::z_minus: perm = permutation::Zxy; break;
            }

            glm::vec3 n, u, v;
            switch(perm)
            {
                case permutation::Xyz:
                    n = glm::vec3(1, 0, 0);
                    u = glm::vec3(0, 1, 0);
                    v = glm::vec3(0, 0, 1);
                    break;
                case permutation::Zxy:
                    n = glm::vec3(0, 0, 1);
                    u = glm::vec3(1, 0, 0);
                    v = glm::vec3(0, 1, 0);
                    break;
                case permutation::Yzx:
                    n = glm::vec3(0, 1, 0);
                    u = glm::vec3(0, 0, 1);
                    v = glm::vec3(1, 0, 0);
            }
            return {.sign = sign, .perm = perm, .n = n, .u = u, .v = v};
        }
    }

    template<type_described TVertexFormat>
    class mesh_element_builder
    {
      public:
        [[nodiscard]] std::shared_ptr<mesh_element> build() const
        {
            auto element = std::make_shared<mesh_element>();
            element->set_data(vertices, indices);
            return element;
        }

        mesh_element_builder& add_vertex_position(glm::vec3 position)
        {
            vertices.push_back({});
            // TODO: With C++ reflection I can set the position field of the vertex format directly
            if constexpr(requires(TVertexFormat v) {
                             { v.position } -> std::convertible_to<glm::vec3>;
                         })
            {
                vertices.back().position = position;
            }
            else if constexpr(requires(TVertexFormat v) {
                                  { v.position } -> std::convertible_to<glm::vec2>;
                              })
            {
                vertices.back().position = glm::vec2(position.x, position.y);
            }
            else if constexpr(requires(TVertexFormat v) {
                                  { v.position } -> std::convertible_to<glm::vec4>;
                              })
            {
                vertices.back().position = glm::vec4(position.x, position.y, position.z, 0);
            }

            return *this;
        }
        mesh_element_builder& with_uv(glm::vec2 uv)
        {
            if constexpr(requires(TVertexFormat v) { v.uv; })
            {
                vertices.back().uv = uv;
            }

            return *this;
        }

        mesh_element_builder& with_color(glm::vec4 color)
        {
            if constexpr(requires(TVertexFormat v) { v.color; })
            {
                vertices.back().color = color;
            }
            return *this;
        }

        mesh_element_builder& with_normal(glm::vec3 normal)
        {
            if constexpr(requires(TVertexFormat v) { v.normal; })
            {
                vertices.back().normal = normal;
            }
            return *this;
        }

        mesh_element_builder& gen_normals()
        {
            if constexpr(requires(TVertexFormat v) {
                             v.normal;
                             { v.position } -> std::same_as<glm::vec3>;
                         })
            {
                if(vertices.size() > 3)
                {
                    // Generate normals for the last 3 vertices
                    auto& v1 = vertices[vertices.size() - 1];
                    auto& v2 = vertices[vertices.size() - 2];
                    auto& v3 = vertices[vertices.size() - 3];
                    glm::vec3 normal = glm::triangleNormal(v1.position, v2.position, v3.position);
                    v1.normal = normal;
                    v2.normal = normal;
                    v3.normal = normal;
                }
            }
            return *this;
        }

        mesh_element_builder& add_index(const uint16 index)
        {
            indices.push_back(index);
            return *this;
        }
        mesh_element_builder& add_indices(std::span<int16> indices_to_add)
        {
            indices.insert(indices.end(), indices_to_add.begin(), indices_to_add.end());
            return *this;
        }

        mesh_element_builder& extend_indices(const std::span<const uint16> indices_to_add)
        {
            const auto start_index = static_cast<uint16>(indices.size());
            // add the indices to the end of the current indices, but add start_index to each value
            for(const auto& index : indices_to_add)
            {
                indices.push_back(index + start_index);
            }

            return *this;
        }

        // Add a quad defined by two points (bottom-left and top-right) and a face direction.
        mesh_element_builder& add_quad(glm::vec3 pos_min, glm::vec3 pos_max,
                                       quad_builder::face_direction face_direction,
                                       glm::vec2 uv_min = glm::vec2(0.0f, 0.0f),
                                       glm::vec2 uv_max = glm::vec2(1.0f, 1.0f), glm::vec4 color = colors::white)
        {
            quad_builder::oriented_face face =
                quad_builder::face_from_direction(face_direction); // Get the face orientation based on the direction

            float width = pos_max.x - pos_min.x;
            float height = pos_max.y - pos_min.y;

            glm::vec3 u_vec = face.u * width;
            glm::vec3 v_vec = face.v * height;

            const glm::vec3 minu_minv = pos_min;
            const glm::vec3 minu_maxv = minu_minv + u_vec;
            const glm::vec3 maxu_minv = minu_minv + v_vec;
            const glm::vec3 maxu_maxv = minu_minv + u_vec + v_vec;

            add_vertex_position(minu_minv).with_uv(uv_min).with_color(color).with_normal(face.sign > 0 ? face.n
                                                                                                       : -face.n);
            add_vertex_position(minu_maxv)
                .with_uv(glm::vec2(uv_max.x, uv_min.y))
                .with_color(color)
                .with_normal(face.sign > 0 ? face.n : -face.n);
            add_vertex_position(maxu_minv)
                .with_uv(glm::vec2(uv_min.x, uv_max.y))
                .with_color(color)
                .with_normal(face.sign > 0 ? face.n : -face.n);
            add_vertex_position(maxu_maxv).with_uv(uv_max).with_color(color).with_normal(face.sign > 0 ? face.n
                                                                                                       : -face.n);

            if(face.sign <= 0)
            {
                extend_indices(std::array<uint16, 6> {0, 1, 2, 1, 3, 2}); // Clockwise order
            }
            else
            {
                extend_indices(std::array<uint16, 6> {0, 2, 1, 1, 2, 3}); // Counter-clockwise order
            }

            return *this;
        }

      private:
        std::vector<TVertexFormat> vertices;
        std::vector<uint16> indices;
    };

    class mesh_builder
    {
      public:
        mesh_builder& with_shader(const std::shared_ptr<shader::shader>& shader)
        {
            m_pending_shader = shader;
            return *this;
        }

        template<type_described TVertexFormat, std::invocable<void(mesh_element_builder<TVertexFormat>&)> TBuilderFunc>
        mesh_builder& add_element(TBuilderFunc&& builder_func)
        {
            mesh_element_builder<TVertexFormat> builder;
            builder_func(builder);
            m_elements.push_back({builder.build(), m_pending_shader});
            return *this;
        }

        std::shared_ptr<mesh> build() { return std::make_shared<mesh>(m_elements); }

      private:
        std::shared_ptr<shader::shader> m_pending_shader;
        using mesh_part = std::tuple<std::shared_ptr<mesh_element>, std::shared_ptr<shader::shader>>;
        std::vector<mesh_part> m_elements;
    };
}