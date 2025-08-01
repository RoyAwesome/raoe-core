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

struct scoped_world_defer_suspend
{
    explicit scoped_world_defer_suspend(flecs::world_t* world)
        : world(world)
    {
        ecs_defer_suspend(world);
    }
    ~scoped_world_defer_suspend() { ecs_defer_resume(world); }
    flecs::world_t* world;
};

void init_render(const flecs::iter it)
{
    auto _ = scoped_world_defer_suspend(it.world());

    auto assets = raoe::render::init_renderer();
    it.world().set<raoe::render::render_assets>(std::move(assets));

    it.world().entity(raoe::engine::entities::engine::main_camera).add<raoe::render::camera>();
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
}

void draw_frame(flecs::iter itr)
{
    // Get the global infos
    const auto& render_assets = itr.world().get<raoe::render::render_assets>();
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

        raoe::render::render_mesh(*camera.get(), *render_info->mesh, *render_transform, render_assets);
    }
}

raoe::engine::render_module::render_module(const flecs::world& world)
{
    world.component<render::render_transform>();
    world.component<render::camera>();
    world.component<render::render_assets>().add(flecs::Singleton);

    world.system().kind(entities::startup::on_render_start).immediate().run(init_render);
    world.system<render::render_transform, const transform_3d>()
        .kind(entities::render_tick::render_begin)
        .each(compute_render_transform_3d);

    world.system<render::render_transform, const transform_2d>()
        .kind(entities::render_tick::render_begin)
        .each(compute_render_transform_2d);

    world.system().kind(entities::render_tick::render_begin).run(prepare_frame);

    world.system<const render_info, const render::render_transform>().kind(entities::render_tick::draw).run(draw_frame);
}