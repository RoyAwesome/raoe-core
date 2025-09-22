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

#include "render/immediate.hpp"

#include "render/mesh_builder.hpp"
#include "render_private.hpp"

#include <concepts>
#include <vector>
struct render_batch
{
    render_batch& add_quad(const glm::vec2 min, const glm::vec2 max, const glm::vec2 uv_min, const glm::vec2 uv_max,
                           const glm::u8vec4& color = raoe::render::colors::white)
    {
        // Push back 6 verts, 2 triangles, with the positions transformed by the next transform and the z coord the next
        // depth
        const glm::vec3 pos_min(min, static_cast<float>(next_depth) / 20000.0f);
        const glm::vec3 pos_max(max, static_cast<float>(next_depth) / 20000.0f);

        mesh_builder.add_quad(next_transform * glm::vec4(pos_min, 1.0f), next_transform * glm::vec4(pos_max, 1.0f),
                              raoe::render::quad_builder::face_direction::z_plus, uv_min, uv_max, color);

        return *this;
    }

    render_batch& push_rotation_rad(const float rotation = 0.0f, const glm::vec2& origin = glm::vec2(0.0f, 0.0f))
    {
        next_transform = glm::identity<glm::mat4>();
        next_transform = glm::rotate(next_transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        next_transform = glm::translate(next_transform, glm::vec3(origin, 0.0f));
        return *this;
    }

    render_batch& pop_transform()
    {
        next_transform = glm::identity<glm::mat4>();
        return *this;
    }

    render_batch& push_depth(const int32 depth)
    {
        next_depth = depth;
        return *this;
    }
    render_batch& pop_depth()
    {
        next_depth = 0;
        return *this;
    }

    raoe::render::mesh_element_builder<raoe::render::vertex_pos_uv_color_normal> mesh_builder;
    raoe::render::generic_handle<raoe::render::shader::material> material;
    glm::mat4 next_transform = glm::identity<glm::mat4>();
    int32 next_depth = 0;
};

struct immediate_render_data
{

    template<std::invocable<render_batch&> TBatchBuilder>
    immediate_render_data& begin_batch(const raoe::render::generic_handle<raoe::render::shader::material>& material,
                                       TBatchBuilder&& batch_builder)
    {
        auto it = std::find_if(batches.begin(), batches.end(),
                               [&material](const render_batch& batch) { return batch.material == material; });

        if(it == batches.end())
        {
            batches.push_back(render_batch {.mesh_builder = {}, .material = material});
            it = batches.end() - 1;
        }

        it->push_depth(depth++); // Increment depth for this batch
        batch_builder(*it);

        return *this;
    }

    std::vector<render_batch> batches = {};
    int32 depth = 0; // Depth for rendering, can be used to sort batches if needed
};
static immediate_render_data immediate_data;

raoe::render::generic_handle<raoe::render::shader::material> create_material_for_texture(
    const raoe::render::generic_handle<raoe::render::texture_2d>& texture)
{
    using namespace raoe::render;
    auto material = std::make_shared<shader::material>(generic_handle(get_render_context().generic_2d_shader));
    material->set_uniform("texture0", texture);
    return {material};
}

void raoe::render::draw_2d_texture_rect(const glm::vec2 rect_min, const glm::vec2 rect_max,
                                        const generic_handle<texture_2d>& texture, const glm::vec2 uv_min,
                                        const glm::vec2 uv_max, const glm::u8vec4& color, const float rotation,
                                        const glm::vec2& origin)
{
    immediate_data.begin_batch(create_material_for_texture(texture), [&](render_batch& batch) {
        batch.push_rotation_rad(rotation, origin).add_quad(rect_min, rect_max, uv_min, uv_max, color).pop_transform();
    });
}
void raoe::render::draw_2d_rect(const glm::vec2& rect_min, const glm::vec2& rect_max, const glm::u8vec4& color,
                                const float rotation, const glm::vec2& origin)
{
    immediate_data.begin_batch(get_internal_render_assets().white_material, [&](render_batch& batch) {
        batch.push_rotation_rad(rotation, origin)
            .add_quad(rect_min, rect_max, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), color)
            .pop_transform();
    });
}

void raoe::render::immediate::begin_immediate_batch()
{
    // Reset the immediate data for a new batch
    immediate_data = {};
}

void raoe::render::immediate::draw_immediate_batch(const uniform_buffer& engine_ubo, const uniform_buffer& camera_ubo)
{
    shader::material* current_material = nullptr;
    for(const auto& [mesh_builder, material, next_transform, next_depth] : immediate_data.batches)
    {
        if(material.get() != current_material)
        {
            current_material = material.get();
            current_material->use();
            current_material->shader_handle()->uniform_blocks()[0] = engine_ubo; // Engine UBO is at binding point 0
            current_material->shader_handle()->uniform_blocks()[1] = camera_ubo; // Camera UBO is at binding point 1
        }

        auto mesh = mesh_builder.build();
        render_mesh_element(mesh);
    }
}