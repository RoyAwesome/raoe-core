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
}