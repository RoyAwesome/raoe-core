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

#include "render/render.hpp"

#include "render/buffer.hpp"
#include "render/mesh.hpp"
#include "render/shader.hpp"
#include "render/types.hpp"

#include "render_private.hpp"

#include "glad/glad.h"

namespace raoe::render
{
    // create a cube
    static std::array cube_points = {
        simple_vertex {glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec2(0, 0)},
        simple_vertex {glm::vec3(1, 0, 0), glm::vec3(0, 0, -1), glm::vec2(1, 0)},
        simple_vertex {glm::vec3(1, 1, 0), glm::vec3(0, 0, -1), glm::vec2(1, 1)},
        simple_vertex {glm::vec3(0, 1, 0), glm::vec3(0, 0, -1), glm::vec2(0, 1)},
        simple_vertex {glm::vec3(0, 0, 1), glm::vec3(0, 0, 1),  glm::vec2(0, 0)},
        simple_vertex {glm::vec3(1, 0, 1), glm::vec3(0, 0, 1),  glm::vec2(1, 0)},
        simple_vertex {glm::vec3(1, 1, 1), glm::vec3(0, 0, 1),  glm::vec2(1, 1)},
        simple_vertex {glm::vec3(0, 1, 1), glm::vec3(0, 0, 1),  glm::vec2(0, 1)},
    };

    static std::array<uint8, 36> cube_indices = {
        0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 5, 4, 7, 7, 6, 5, 4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4,
    };

    static std::array triangle_points = {
        simple_vertex {glm::vec3(-1, -1, 0), glm::vec3(0, 0, -1), glm::vec2(0,   0)},
        simple_vertex {glm::vec3(1,  -1, 0), glm::vec3(0, 0, -1), glm::vec2(1,   0)},
        simple_vertex {glm::vec3(0,  1,  0), glm::vec3(0, 0, -1), glm::vec2(0.5, 1)},
    };

    static std::array plane_points = {
        simple_vertex {glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec2(0, 0)},
        simple_vertex {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec2(1, 0)},
        simple_vertex {glm::vec3(1, 0, 1), glm::vec3(0, 1, 0), glm::vec2(1, 1)},
        simple_vertex {glm::vec3(0, 0, 1), glm::vec3(0, 1, 0), glm::vec2(0, 1)},
    };

    static std::array<uint8, 6> plane_indices = {0, 1, 2, 2, 3, 0};

    static std::optional<render_context> _static_render_context;
    void set_render_context(const render_context& ctx)
    {
        check_if(ctx.error_shader != nullptr, "Error shader is null");
        check_if(ctx.error_texture != nullptr, "Error texture is null");
        check_if(ctx.generic_2d_shader != nullptr, "Generic 2D shader is null");
        check_if(!!ctx.load_callback, "Load callback is null");
        _static_render_context = ctx;
    }
    render_context& get_render_context()
    {
        check_if(!!_static_render_context, "Render context is not initialized");
        return *_static_render_context;
    }
    std::shared_ptr<texture_2d> generate_checkerboard_texture(const glm::ivec2& size, const glm::u8vec4& color1,
                                                              const glm::u8vec4& color2, const int square_size)
    {
        check_if(size.x > 0 && size.y > 0, "Texture size must be greater than 0");
        check_if(square_size > 0, "Square size must be greater than 0");
        std::vector<glm::u8vec4> data(size.x * size.y);
        for(int y = 0; y < size.y; ++y)
        {
            for(int x = 0; x < size.x; ++x)
            {
                const bool use_color1 = ((x / square_size) % 2) == ((y / square_size) % 2);
                data[y * size.x + x] = use_color1 ? color1 : color2;
            }
        }

        return std::make_shared<texture_2d>(data, size,
                                            texture_params {
                                                .wrap_u = texture_wrap::repeat,
                                                .wrap_v = texture_wrap::repeat,
                                                .filter_min = texture_filter::linear,
                                                .filter_mag = texture_filter::linear,
                                            });
    }

    std::tuple<int32, int32> get_size_and_gl_type(const renderer_type uniform_type)
    {
        switch(uniform_type)
        {
            case renderer_type::f32: return {1, GL_FLOAT};
            case renderer_type::f64: return {1, GL_DOUBLE};
            case renderer_type::i32: return {1, GL_INT};
            case renderer_type::u32: return {1, GL_UNSIGNED_INT};
            case renderer_type::vec2: return {2, GL_FLOAT};
            case renderer_type::vec3: return {3, GL_FLOAT};
            case renderer_type::vec4:
            case renderer_type::mat2: return {4, GL_FLOAT};
            case renderer_type::mat3: return {9, GL_FLOAT};
            case renderer_type::mat4: return {16, GL_FLOAT};
            case renderer_type::color: return {4, GL_UNSIGNED_BYTE};
            default: panic("Invalid renderer type for size and gl type: {}", underlying(uniform_type));
        }
    }

    uint32 get_or_create_vao(const std::span<const type_description> elements)
    {
        static std::unordered_map<uint32, uint32> vao_map;

        std::size_t hash = elements_hash(elements);

        if(const auto it = vao_map.find(hash); it != vao_map.end())
        {
            return it->second;
        }
        uint32 vao = 0;
        glCreateVertexArrays(1, &vao);

        for(int i = 0; i < elements.size(); i++)
        {
            auto& element = elements[i];
            glEnableVertexArrayAttrib(vao, i);

            auto [size, gl_type] = get_size_and_gl_type(element.type);
            glVertexArrayAttribFormat(vao, i, size, gl_type, GL_FALSE, element.offset);
            glVertexArrayAttribBinding(vao, i, 0);
        }

        vao_map.insert({hash, vao});
        return vao;
    }

    void render_mesh(const camera& camera, const mesh& mesh, const render_transform& render_transform)
    {
        // loop through each element of the mesh and render it
        for(auto&& [mesh_element, shader] : mesh.m_elements)
        {
            check_if(!!mesh_element, "Mesh element is null");
            // determine which shader we use
            shader::shader* s = shader ? shader.get() : get_render_context().error_shader.get();
            // write the uniforms
            s->use();
            (*s)["mvp"] = camera.get_camera_matrix() * render_transform.cached_world_transform;
            (*s)["tex"] = *get_render_context().error_texture;

            render_mesh_element(*mesh_element);
        }
    }
    void render_mesh_element(mesh_element& mesh_element)
    {
        // Generate any buffers if they are not already generated
        mesh_element.generate_buffers();

        if(!mesh_element.get_vertex_buffer())
        {
            return;
        }
        // bind the vao
        const uint32 vao = get_or_create_vao(mesh_element.vertex_element_type());

        glBindVertexArray(vao);

        // bind the mesh
        glVertexArrayVertexBuffer(vao, 0, mesh_element.get_vertex_buffer()->native_buffer(), 0,
                                  static_cast<GLsizei>(mesh_element.get_vertex_buffer()->element_stride()));
        const index_buffer* idx = mesh_element.get_index_buffer();
        if(idx)
        {
            glVertexArrayElementBuffer(vao, idx->native_buffer());
        }

        // Set the pipeline
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // glEnable(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        // draw the mesh
        if(idx)
        {
            GLenum mode = GL_NONE;
            switch(idx->elements()[0].type)
            {
                case renderer_type::u8: mode = GL_UNSIGNED_BYTE; break;
                case renderer_type::u16: mode = GL_UNSIGNED_SHORT; break;
                case renderer_type::u32: mode = GL_UNSIGNED_INT; break;
                default: panic("Invalid index buffer type");
            }
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(idx->element_count()), mode, nullptr);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh_element.get_vertex_buffer()->element_count()));
        }
    }

    void clear_surface(const glm::u8vec4 color)
    {
        glClearColor(static_cast<float>(color.r) / 255.f, static_cast<float>(color.g) / 255.f,
                     static_cast<float>(color.b) / 255.f, static_cast<float>(color.a) / 255.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

}
