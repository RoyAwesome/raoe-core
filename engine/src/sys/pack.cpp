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

#include "engine/sys/pack.hpp"

#include "toml++/toml.hpp"

raoe::engine::sys::pack_module::pack_module(flecs::world& world)
{
    world.component<pack>();

    world.observer<pack>().event(flecs::OnSet).run([](flecs::iter& it) {
        while(it.next())
        {
            if(const auto pack = it.field<sys::pack>(0);
               has_any_flags(pack->m_flags, pack_flags::always_mounted) && pack->m_state != pack_state::mounted)
            {
                spdlog::info("sys::pack Loaded pack {}, but it's set to always mount.  Mounting it.", pack->m_name);
                mount_pack({it.entity(0)});
            }
            else
            {
                it.world().event<pack_state_change>().id<sys::pack>().entity(it.entity(0)).emit();
            }
        }
    });
}

void parse_manifest(raoe::engine::sys::pack::pack_manifest& manifest, const std::filesystem::path& manifest_file_path)
{
    auto table = toml::parse_file(manifest_file_path.string());
    if(!table)
    {
        raoe::panic("Unable to parse manifest file {}: {}", manifest_file_path.string(), table.error().description());
    }

    // Parse the manifest file into the pack_manifest structure
    manifest.m_name = table["name"].value_or("");
    manifest.m_version = table["version"].value_or(0);
    manifest.m_author = table["author"].value_or("");
    manifest.m_description = table["description"].value_or("");

    // Parse dependencies
    if(const auto deps = table["dependencies"].as_table())
    {
        for(const auto& [key, value] : *deps)
        {
            raoe::engine::sys::pack::pack_manifest::pack_dependency dependency;
            dependency.m_name = key.str();
            dependency.m_version = value.value_or(0);
            manifest.m_dependencies.push_back(std::move(dependency));
        }
    }

    if(auto scripts_table = table["scripts"].as_table())
    {
        auto array_into_vector = [&scripts_table](const std::string_view key,
                                                  std::vector<raoe::fs::path>& into_vector) {
            if(const auto scripts = scripts_table->at(key).as_array())
            {
                for(const auto& script : *scripts)
                {
                    if(const auto& script_str = script.as_string())
                    {
                        into_vector.emplace_back(script_str->get());
                    }
                }
            }
        };
        array_into_vector("init", manifest.m_init_scripts);
        array_into_vector("game", manifest.m_game_scripts);
        array_into_vector("editor", manifest.m_editor_scripts);
    }
}

flecs::ref<raoe::engine::sys::pack> raoe::engine::sys::load_pack(const flecs::entity into_entity,
                                                                 const std::filesystem::path& path,
                                                                 const pack_flags flags)
{
    if(!ensure(std::filesystem::is_directory(path), "Unable to load pack from {}, not a directory", path.string()))
    {
        return {};
    }

    // Check if the pack has a valid manifest file, and if so, load it.
    std::string manifest_filename = path.stem().string() + ".toml";
    if(!ensure(std::filesystem::exists(path / manifest_filename), "Unable to find manifest file {} in pack {}",
               manifest_filename, path.string()))
    {
        return {};
    }
    spdlog::info(" - Loading pack {}, manifest {}", path.string(), manifest_filename);
    pack loaded_pack {.m_path = path,
                      .m_name = path.filename().string(),
                      .m_state = pack_state::unmounted,
                      .m_flags = flags,
                      .m_manifest = {}};

    parse_manifest(loaded_pack.m_manifest, path / manifest_filename);

    // place the pack into the entity, and return a reference to it
    return into_entity.set(std::move(loaded_pack));
}
bool raoe::engine::sys::mount_pack(flecs::ref<pack> pack_ref)
{
    if(!ensure(pack_ref, "cow::sys::mount_pack - Attempting to load an invalid pack {}", pack_ref.entity()))
    {
        return false;
    }

    if(!ensure(std::filesystem::is_directory(pack_ref->m_path),
               "cow::sys::mount_pack - Pack file {} does not exist, cannot mount", pack_ref.entity()))
    {
        return false;
    }

    pack_ref->m_state = pack_state::mounted;
    fs::mount(pack_ref->m_path);
    pack_ref.entity().world().event<pack_state_change>().id<pack>().entity(pack_ref.entity()).emit();
    spdlog::info(" - Mounted pack {} at {}", pack_ref->m_name, pack_ref->m_path.string());
    return true;
}