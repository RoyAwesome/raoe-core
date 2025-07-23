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

#include "engine/engine.hpp"

#include "fs/filesystem.hpp"

namespace raoe::engine::sys
{
    struct pack_module
    {
        explicit pack_module(flecs::world& world);
    };

    enum class pack_state : int32
    {
        mounted,
        unmounted
    };

    struct pack_state_change
    {
    };

    enum class pack_flags : int32
    {
        none = 0,
        system = 1 << 0,
        game = 1 << 1,
        dlc = 1 << 2,
        mod = 1 << 3,
        local = 1 << 4,
        downloaded = 1 << 5,

        always_mounted = system | game,
    };

    struct pack
    {
        std::filesystem::path m_path;
        std::string m_name;
        pack_state m_state;
        pack_flags m_flags = pack_flags::none;
        struct pack_manifest
        {
            std::string m_name;
            int32 m_version;
            std::string m_author;
            std::string m_description;
            struct pack_dependency
            {
                std::string m_name;
                int32 m_version;
            };
            std::vector<pack_dependency> m_dependencies;

            std::vector<raoe::fs::path> m_init_scripts;
            std::vector<raoe::fs::path> m_game_scripts;
            std::vector<raoe::fs::path> m_editor_scripts;
        };
        pack_manifest m_manifest;
    };

    flecs::ref<pack> load_pack(flecs::entity into_entity, const std::filesystem::path& path,
                               pack_flags flags = pack_flags::none);

    inline flecs::ref<pack> load_pack(flecs::world& world, const std::string& name, const std::filesystem::path& path,
                                      const pack_flags flags = pack_flags::none)
    {
        return load_pack(world.entity(name.c_str()), path, flags);
    }

    bool mount_pack(flecs::ref<pack> pack_ref);
}

RAOE_CORE_FLAGS_ENUM(raoe::engine::sys::pack_flags);