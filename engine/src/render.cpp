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

#include "engine/render.hpp"

#include "render/render.hpp"

#include "engine/sys/pack.hpp"
#include "engine/sys/window.hpp"
#include "render/immediate.hpp"

template<typename T>
void fill_injections_for_vertex_type(std::unordered_map<std::string, std::string>& injections)
{
}

struct engine_uniforms
{
    glm::vec2 screen_size = {0, 0};
    float time = 0.0f;
};

template<>
struct raoe::render::renderer_type_of<engine_uniforms>
{
    static std::span<const type_description> elements()
    {
        static constexpr std::array<const type_description, 2> ele = {
            type_description {
                              .type = renderer_type::vec2,
                              .offset = offsetof(engine_uniforms, screen_size),
                              },
            type_description {
                              .type = renderer_type::f32,
                              .offset = offsetof(engine_uniforms,                                                                 time),
                              },
        };
        return ele;
    }
};

struct camera_uniform
{
    glm::mat4 camera_matrix = glm::identity<glm::mat4>();
    glm::mat4 projection_matrix = glm::identity<glm::mat4>();
    glm::mat4 proj_camera = glm::identity<glm::mat4>();
};

template<>
struct raoe::render::renderer_type_of<camera_uniform>
{
    static std::span<const type_description> elements()
    {
        static constexpr std::array<const type_description, 3> ele = {
            type_description {
                              .type = renderer_type::mat4,
                              .offset = offsetof(camera_uniform,                      camera_matrix),
                              },
            type_description {
                              .type = renderer_type::mat4,
                              .offset = offsetof(camera_uniform,                                                                                     projection_matrix),

                              },
            type_description {
                              .type = renderer_type::mat4,
                              .offset = offsetof(camera_uniform,proj_camera),

                              },
        };
        return ele;
    }
};

struct ubo
{
    std::shared_ptr<raoe::render::uniform_buffer> m_uniform_buffer;
};

void init_render(const flecs::iter it)
{
    using namespace raoe::render::shader;

    spdlog::info("Initializing renderer");

    auto _ = raoe::engine::scoped_world_defer_suspend(it.world());
    const auto error_texture = raoe::render::generate_checkerboard_texture({64, 64}, raoe::render::colors::black,
                                                                           raoe::render::colors::magic_pink, 8);

    spdlog::info("Building Error Shader");
    const auto error_shader = glsl_builder("Error Shader")
                                  .with_file_loader(raoe::engine::sys::load_string_from_pack)
                                  .load_module<shader_type::vertex>("core/shaders/common.vert.glsl")
                                  .load_module<shader_type::fragment>("core/shaders/error.frag.glsl")
                                  .build_sync();

    spdlog::info("Building Generic 2D Shader");
    const auto generic_2d_shader = glsl_builder("Generic 2D Shader")
                                       .with_file_loader(raoe::engine::sys::load_string_from_pack)
                                       .load_module<shader_type::vertex>("core/shaders/common.vert.glsl")
                                       .load_module<shader_type::fragment>("core/shaders/generic_2d.frag.glsl")
                                       .build_sync();

    const glm::vec2 window_size =
        it.world().entity(raoe::engine::entities::engine::main_window).get<raoe::engine::sys::window>().size();

    const raoe::render::render_context ctx {.error_shader = error_shader,
                                            .generic_2d_shader = generic_2d_shader,
                                            .error_texture = error_texture,
                                            .surface_size = window_size,
                                            .load_callback =
                                                glsl::file_load_callback_t(raoe::engine::sys::load_string_from_pack)};

    spdlog::info("Setting render context");
    raoe::render::set_render_context(ctx);

    it.world()
        .entity(raoe::engine::entities::engine_assets::engine_ubo)
        .set<ubo>({std::make_shared<raoe::render::uniform_buffer>(engine_uniforms {})});

    it.world()
        .entity(raoe::engine::entities::engine_assets::camera_ubo)
        .set<ubo>({std::make_shared<raoe::render::uniform_buffer>(camera_uniform {})});

    it.world().entity(raoe::engine::entities::engine::main_camera).add<raoe::render::camera>();

    raoe::render::camera screen_space_orthographic_camera =
        raoe::render::camera().with_orthographic(0.0f, window_size.x, window_size.y, 0.0f, -1, 100.0f);
    it.world()
        .entity(raoe::engine::entities::engine::camera_2d)
        .add<raoe::render::camera>()
        .set(screen_space_orthographic_camera);

    it.world()
        .entity(raoe::engine::entities::engine_assets::error_texture)
        .set<std::shared_ptr<raoe::render::texture_2d>>(error_texture);
}

void compute_render_transform_3d(flecs::entity e, raoe::render::render_transform& rt,
                                 const raoe::engine::transform_3d& transform)
{
    spdlog::info("Computing render_transform for {}", e);
    rt.cached_local_transform = transform.to_matrix();
    rt.cached_world_transform = glm::identity<glm::mat4>();

    // compute the world transform for this entity
    flecs::entity transform_e = e;
    while(transform_e.is_valid())
    {
        if(transform_e.has<raoe::engine::transform_3d>())
        {
            rt.cached_world_transform =
                transform_e.get<raoe::engine::transform_3d>().to_matrix() * rt.cached_world_transform;
        }
        transform_e = transform_e.parent();
    }
}

void compute_render_transform_2d(const flecs::entity e, raoe::render::render_transform& rt,
                                 const raoe::engine::transform_2d& transform)
{
    rt.cached_local_transform = transform.to_matrix();
    rt.cached_world_transform = glm::identity<glm::mat4>();

    flecs::entity transform_e = e;
    while(transform_e.is_valid())
    {
        if(transform_e.has<raoe::engine::transform_2d>())
        {
            rt.cached_world_transform =
                transform_e.get<raoe::engine::transform_2d>().to_matrix() * rt.cached_world_transform;
        }
        transform_e = transform_e.parent();
    }
}

void prepare_frame(flecs::iter itr)
{
    raoe::render::clear_surface(raoe::render::colors::cornflower_blue);
    // Prepare the engine UBO
}

void draw_frame(flecs::iter itr)
{
    // Get the global infos
    flecs::ref<raoe::render::camera> main_camera = {itr.world().entity(raoe::engine::entities::engine::main_camera)};
    raoe::check_if(main_camera,
                   "No main camera found. Please ensure a camera is set as the main camera in the engine.");
    while(itr.next())
    {
        const auto render_info = itr.field<const raoe::engine::render_info>(0);
        const auto render_transform = itr.field<const raoe::render::render_transform>(1);

        if(!render_info->mesh)
        {
            spdlog::warn("No Mesh for entity {}. Skipping rendering.", itr.entity(0));
            continue;
        }

        flecs::ref<raoe::render::camera> camera = render_info->camera;
        if(!camera)
        {
            camera = main_camera;
        }

        raoe::render::render_mesh(*camera.get(), *render_info->mesh, *render_transform);
    }
}

void tick_start(flecs::iter itr)
{
    itr.world()
        .entity(raoe::engine::entities::engine_assets::engine_ubo)
        .get<ubo>()
        .m_uniform_buffer->set_data(
            engine_uniforms {.screen_size = glm::vec2(raoe::render::get_render_context().surface_size),
                             .time = itr.world().delta_time()});
    raoe::render::immediate::begin_immediate_batch();
}

void post_draw(const flecs::iter itr)
{
    const auto& camera = itr.world().entity(raoe::engine::entities::engine::camera_2d).get<raoe::render::camera>();
    const auto& [camera_ubo] = itr.world().entity(raoe::engine::entities::engine_assets::camera_ubo).get<ubo>();
    const auto& [engine_ubo] = itr.world().entity(raoe::engine::entities::engine_assets::engine_ubo).get<ubo>();

    raoe::check_if(!!camera_ubo, "Camera UBO is null");
    raoe::check_if(!!engine_ubo, "Engine UBO is null");

    camera_ubo->set_data(camera_uniform {
        .camera_matrix = camera.get_camera_matrix(),
        .projection_matrix = camera.get_projection_matrix(),
        .proj_camera = camera.get_view_projection_matrix(),
    });

    raoe::render::immediate::draw_immediate_batch(*engine_ubo, *camera_ubo);
}
raoe::engine::render_module::render_module(const flecs::world& world)
{
    world.component<render::render_transform>();
    world.component<render::camera>();
    world.component<ubo>();

    world.system().kind(entities::startup::on_render_start).immediate().run(init_render);
    world.system<render::render_transform, const transform_3d>()
        .kind(entities::render_tick::render_begin)
        .each(compute_render_transform_3d);

    world.system<render::render_transform, const transform_2d>()
        .kind(entities::render_tick::render_begin)
        .each(compute_render_transform_2d);

    world.system().kind(flecs::PreFrame).run(tick_start);
    world.system().kind(entities::render_tick::post_draw).run(post_draw);

    world.system().kind(entities::render_tick::render_begin).run(prepare_frame);
    world.system<const render_info, const render::render_transform>().kind(entities::render_tick::draw).run(draw_frame);
}