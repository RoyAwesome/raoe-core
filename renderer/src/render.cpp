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

    static std::array<uint8, 6> plane_indices = {0, 1, 2, 2, 3, 0};

    static std::optional<render_context> _static_render_context;
    static std::optional<internal_render_assets> _static_internal_render_assets;
    void set_render_context(const render_context& ctx)
    {
        check_if(ctx.error_shader != nullptr, "Error shader is null");
        check_if(ctx.error_texture != nullptr, "Error texture is null");
        check_if(ctx.generic_2d_shader != nullptr, "Generic 2D shader is null");
        check_if(ctx.generic_2d_shader->has_uniform("texture0"),
                 "Generic 2d shader missing 'texture0' uniform.  It needs it for 2D texture rendering");
        check_if(!!ctx.load_callback, "Load callback is null");
        _static_render_context = ctx;
        if(!_static_render_context->error_texture->has_gpu_data())
        {
            _static_render_context->error_texture->upload_to_gpu();
        }
        auto& [white_texture, white_material] = get_internal_render_assets(); // Ensure internal assets are created
        // Recreate the white material, in case the generic 2d shader changed
        white_material = std::make_shared<shader::material>(generic_handle(ctx.generic_2d_shader));
        white_material->set_uniform("texture0", generic_handle(white_texture));
    }
    void shutdown_renderer()
    {
        _static_render_context.reset();
        _static_internal_render_assets.reset();
    }
    render_context& get_render_context()
    {
        check_if(!!_static_render_context, "Render context is not initialized");
        return *_static_render_context;
    }

    internal_render_assets& get_internal_render_assets()
    {
        if(!_static_internal_render_assets)
        {
            _static_internal_render_assets.emplace();
            _static_internal_render_assets->white_texture =
                std::make_shared<texture_2d>(std::array {colors::white}, glm::ivec2 {1, 1},
                                             texture_params {
                                                 .filter_min = texture_filter::nearest,
                                                 .filter_mag = texture_filter::nearest,
                                             });
            _static_internal_render_assets->white_texture->upload_to_gpu();
        }

        return *_static_internal_render_assets;
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
                                                .filter_min = texture_filter::nearest,
                                                .filter_mag = texture_filter::nearest,
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

    bool type_normalized(const renderer_type uniform_type)
    {
        return uniform_type == renderer_type::color;
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
            glVertexArrayAttribFormat(vao, i, size, gl_type, type_normalized(element.type) ? GL_TRUE : GL_FALSE,
                                      element.offset);
            glVertexArrayAttribBinding(vao, i, 0);
        }

        vao_map.insert({hash, vao});
        return vao;
    }

    void render_mesh(const generic_handle<mesh>& mesh, const uniform_buffer& engine_ubo,
                     const uniform_buffer& camera_ubo)
    {
        for(auto&& [mesh_element, material] : mesh->m_elements)
        {
            if(mesh_element)
            {
                // if no material, use the error material.
                if(material)
                {
                    material->use();
                    material->shader_handle()->uniform_blocks()[0] = engine_ubo; // Engine UBO is at binding point 0
                    material->shader_handle()->uniform_blocks()[1] = camera_ubo; // Camera UBO is at binding point 1
                }

                render_mesh_element(*mesh_element.get());
            }
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
