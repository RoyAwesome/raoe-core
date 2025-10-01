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

#include "engine/asset.hpp"
#include "engine/sys/pack.hpp"

namespace raoe::engine
{
    render::shader::shader asset_loader<render::shader::shader>::load_asset(const asset_load_params& params)
    {
        if(const auto parse_result = asset_loader<toml::parse_result>::load_asset(params); parse_result)
        {
            render::shader::glsl_builder builder(parse_result["name"].value_or(params.file_path.string()));
            builder.with_file_loader(sys::load_string_from_pack);

            // Load the shader module(s)
            if(const auto shader_table = parse_result["shaders"])
            {
                if(const auto vertex_shader = shader_table["vertex"].as_string())
                {
                    builder.load_module<render::shader::shader_type::vertex>(vertex_shader->get());
                }
                if(const auto fragment_shader = shader_table["fragment"].as_string())
                {
                    builder.load_module<render::shader::shader_type::fragment>(fragment_shader->get());
                }
                if(const auto geometry_shader = shader_table["geometry"].as_string())
                {
                    builder.load_module<render::shader::shader_type::geometry>(geometry_shader->get());
                }
                if(const auto tesselation_control_shader = shader_table["tesselation_control"].as_string())
                {
                    builder.load_module<render::shader::shader_type::tesselation_control>(
                        tesselation_control_shader->get());
                }
                if(const auto tesselation_evaluation_shader = shader_table["tesselation_evaluation"].as_string())
                {
                    builder.load_module<render::shader::shader_type::tesselation_evaluation>(
                        tesselation_evaluation_shader->get());
                }
                if(const auto mesh_shader = shader_table["mesh"].as_string())
                {
                    builder.load_module<render::shader::shader_type::mesh>(mesh_shader->get());
                }
                if(const auto compute_shader = shader_table["compute"].as_string())
                {
                    builder.load_module<render::shader::shader_type::compute>(compute_shader->get());
                }
            }
            // Add the injections
            if(const auto injections_table = parse_result["injections"].as_table())
            {
                std::unordered_map<std::string, std::string> injections;
                for(const auto& [key, val] : *injections_table)
                {
                    if(const auto str = val.as_string())
                    {
                        injections.emplace(std::string(key.str()), str->get());
                    }
                }

                builder.with_injections(injections);
            }

            return builder.build_sync();
        }

        return {};
    }

}
