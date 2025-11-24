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

#include "engine/engine.hpp"

#include "engine/render.hpp"

#include <ranges>
#include <utility>

#include "fs/filesystem.hpp"

#include "engine/sys/pack.hpp"
#include "engine/sys/window.hpp"

namespace raoe::engine
{
    static std::vector<std::string_view> _command_line_args;
    static std::optional<flecs::world> _world;

    void load_important_assets(flecs::iter itr)
    {
        spdlog::info("Begin Load Important Assets");
        // Mount the core pack
        sys::load_pack(itr.world().entity(entities::engine::core_pack), "packs/core",
                       sys::pack_flags::system | sys::pack_flags::game);
        // TODO: Load the configuration .tomls
    }

    struct engine_module
    {
        flecs::entity coro_prefab;
        explicit engine_module(flecs::world& world)
        {
            register_pipeline(world, flecs::OnStart, entities::startup::before_render_context_created,
                              entities::startup::on_game_start);
            register_pipeline(world, flecs::OnUpdate, entities::render_tick::render_begin,
                              entities::render_tick::render_end);

            world.component<transform_2d>();
            world.component<transform_3d>();
            world.component<coro>();

            if(has_any_flags(world.get<engine_info_t>().flags, engine_flags::rendering))
            {
                world.import <sys::window_module>();
                world.import <render_module>();
            }
            else
            {
                spdlog::info("Running in headless mode, window and rendering systems will not be initialized.");
            }

            world.import <sys::pack_module>();
            world.system("Load Important Assets")
                .kind(entities::startup::on_pre_init)
                .immediate()
                .run(load_important_assets);

            coro_prefab = world.prefab("coro").auto_override<std::shared_ptr<coro>>();

            // Execute the coroutines every frame.
            world.system<std::shared_ptr<coro>>().kind(flecs::PreFrame).run([](flecs::iter itr) {
                while(itr.next())
                {
                    auto entity = itr.entity(0);
                    auto coroutine = itr.field<std::shared_ptr<coro>>(0);
                    if(std::shared_ptr<coro>& coro = *coroutine; coro && *coro)
                    {
                        (*coro)();

                        if(!coro->valid())
                        {
                            spdlog::trace("Coro Executor: destructing coro {} because it's done",
                                          coro->debug_info().coro_location.function_name());
                            entity.destruct();
                        }
                    }
                }
            });
        }
    };

    void log_ecs(const int32 level, const char* file, const int32 line, const char* msg)
    {
        spdlog::level::level_enum lvl = spdlog::level::info;
        if(level > 0)
        {
            lvl = spdlog::level::debug;
        }
        else if(level == -2)
        {
            lvl = spdlog::level::warn;
        }
        else if(level == -3)
        {
            lvl = spdlog::level::err;
        }
        else if(level == -4)
        {
            lvl = spdlog::level::critical;
        }

        std::string indentation;
        for(int32 i = 0; i < ecs_os_api.log_indent_; i++)
        {
            indentation += "\t -";
        }

        spdlog::log(lvl, "|{} ({}:{}) {} ", indentation, file, line, msg);
    }

    flecs::world world() noexcept
    {
        return flecs::world(_world->world_);
    }

    flecs::world init_engine(int argc, char* argv[], std::string app_name, const std::string& org_name,
                             const engine_flags flags)
    {
        check_if(argv != nullptr, "argv must be your program's argv");
        _command_line_args = std::vector<std::string_view>(argv, argv + argc);

        spdlog::info("Starting RAOE with game {}", app_name);
        spdlog::info("Command line arguments: '{}'", raoe::string::join(_command_line_args, ", "));

        std::set_terminate(on_terminate);

        spdlog::set_level(spdlog::level::debug);

        fs::permit_symlinks(true);
        fs::init_fs(argv[0], "", app_name, org_name);

        ecs_os_set_api_defaults();
        ecs_os_api_t api = ecs_os_api;

        api.log_ = log_ecs;
        api.flags_ |= EcsOsApiLogWithColors;
        api.abort_ = on_terminate;
        ecs_os_set_api(&api);

        ecs_log_set_level(0);

        _world.emplace(argc, argv);
        _world->component<engine_info_t>().add(flecs::Singleton);
        _world->set<engine_info_t>(engine_info_t {.command_line_args = std::span(_command_line_args),
                                                  .app_name = app_name,
                                                  .app_version = "0.1.0",
                                                  .org_name = org_name,
                                                  .org_version = "0.1.0",
                                                  .flags = flags});
        _world->import <engine_module>();

        return flecs::world(_world->world_);
    }

    void shutdown_engine() noexcept
    {
        _world.reset();
    }

    std::weak_ptr<const coro> start_coro(flecs::world& world, coro&& coroutine, std::source_location start_location)
    {
        auto c = std::make_shared<coro>(std::forward<coro>(coroutine));
        c->set_call_location(start_location);

        const auto& engine = world.get<engine_module>();
        world.entity().is_a(engine.coro_prefab).set<std::shared_ptr<coro>>(c);
        spdlog::trace("start_coro: Starting Coro {} called from {}", c->debug_info().coro_location.function_name(),
                      c->debug_info().call_location.function_name());
        return c;
    }
    const engine_info_t& engine_info() noexcept
    {
        return _world->get<engine_info_t>();
    }
}