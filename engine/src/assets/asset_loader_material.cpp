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

#include "render/colors.hpp"

namespace raoe::engine
{
    asset_load_result<render::shader::shader> asset_loader<render::shader::shader>::load_asset(
        const asset_load_params& params)
    {
        const auto parse_result = asset_loader<toml::table>::load_asset(params);
        if(!parse_result.has_value())
        {
            return std::unexpected(asset_load_error::append(parse_result.error(), params.file_path.string(),
                                                            "Unable to parse shader toml"));
        }

        auto& table = *parse_result;
        render::shader::glsl_builder builder(table["name"].value_or(params.file_path.string()));
        builder.with_file_loader(sys::load_string_from_pack);

        // Load the shader module(s)
        if(const auto shader_table = table["shaders"])
        {
            if(const auto vertex_shader = shader_table["vertex"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::vertex>())
                {
                    return std::unexpected(
                        asset_load_error(params.file_path.string(),
                                         builder.why_cant_attach_shader<render::shader::shader_type::vertex>(),
                                         vertex_shader->source().begin.line, vertex_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::vertex>(vertex_shader->get());
            }
            if(const auto fragment_shader = shader_table["fragment"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::fragment>())
                {
                    return std::unexpected(
                        asset_load_error(params.file_path.string(),
                                         builder.why_cant_attach_shader<render::shader::shader_type::fragment>(),
                                         fragment_shader->source().begin.line, fragment_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::fragment>(fragment_shader->get());
            }
            if(const auto geometry_shader = shader_table["geometry"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::geometry>())
                {
                    return std::unexpected(
                        asset_load_error(params.file_path.string(),
                                         builder.why_cant_attach_shader<render::shader::shader_type::geometry>(),
                                         geometry_shader->source().begin.line, geometry_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::geometry>(geometry_shader->get());
            }
            if(const auto tesselation_control_shader = shader_table["tesselation_control"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::tesselation_control>())
                {
                    return std::unexpected(asset_load_error(
                        params.file_path.string(),
                        builder.why_cant_attach_shader<render::shader::shader_type::tesselation_control>(),
                        tesselation_control_shader->source().begin.line,
                        tesselation_control_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::tesselation_control>(
                    tesselation_control_shader->get());
            }
            if(const auto tesselation_evaluation_shader = shader_table["tesselation_evaluation"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::tesselation_evaluation>())
                {
                    return std::unexpected(asset_load_error(
                        params.file_path.string(),
                        builder.why_cant_attach_shader<render::shader::shader_type::tesselation_evaluation>(),
                        tesselation_evaluation_shader->source().begin.line,
                        tesselation_evaluation_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::tesselation_evaluation>(
                    tesselation_evaluation_shader->get());
            }
            if(const auto mesh_shader = shader_table["mesh"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::mesh>())
                {
                    return std::unexpected(asset_load_error(
                        params.file_path.string(), builder.why_cant_attach_shader<render::shader::shader_type::mesh>(),
                        mesh_shader->source().begin.line, mesh_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::mesh>(mesh_shader->get());
            }
            if(const auto compute_shader = shader_table["compute"].as_string())
            {
                if(!builder.can_attach_module<render::shader::shader_type::compute>())
                {
                    return std::unexpected(
                        asset_load_error(params.file_path.string(),
                                         builder.why_cant_attach_shader<render::shader::shader_type::compute>(),
                                         compute_shader->source().begin.line, compute_shader->source().begin.column));
                }
                builder.load_module<render::shader::shader_type::compute>(compute_shader->get());
            }
        }
        // Add the injections
        if(const auto injections_table = table["injections"].as_table())
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

    asset_load_result<render::shader::material> asset_loader<render::shader::material>::load_asset(
        const asset_load_params& params)
    {
        const auto parse_result = asset_loader<toml::table>::load_asset(params);
        if(!parse_result.has_value())
        {
            return std::unexpected(asset_load_error::append(parse_result.error(), params.file_path.string(),
                                                            "Unable to parse shader toml"));
        }
        auto& table = *parse_result;
        // get and load the shader
        asset_handle<render::shader::shader> shader_asset;
        if(const auto shader_name = table["shader"].as_string())
        {
            uri shader_uri(shader_name->get());
            if(shader_uri.scheme() != "shader")
            {
                return std::unexpected(asset_load_error(
                    params.file_path.string(),
                    std::format("Material has a shader with an invalid scheme: {}", shader_uri.scheme()),
                    shader_name->source().begin.line, shader_name->source().begin.column));
            }

            // load the shader
            auto shader_load_result =
                raoe::engine::load_asset<render::shader::shader>(params.world, fs::path(shader_uri.path()));
            if(!shader_load_result.has_value())
            {

                return std::unexpected(
                    asset_load_error::append(shader_load_result.error(), params.file_path.string(),
                                             std::format("Failed to load shader for material: {}", shader_uri.path()),
                                             shader_name->source().begin.line, shader_name->source().begin.column));
            }
            shader_asset = std::move(shader_load_result.value());
        }

        if(!shader_asset.valid())
        {
            return std::unexpected(asset_load_error(
                params.file_path.string(), "Material is missing a shader (did you forget a top level 'shader' field?)",
                table.source().begin.line, table.source().begin.column));
        }

        render::draw_pass pass;
        if(const auto pass_name = table["pass"].as_string())
        {
            if(pass_name->get() == "pre_pass")
            {
                pass = render::draw_pass::pre_pass;
            }
            else if(pass_name->get() == "opaque_3d")
            {
                pass = render::draw_pass::opaque_3d;
            }
            else if(pass_name->get() == "transparent_3d")
            {
                pass = render::draw_pass::transparent_3d;
            }
            else if(pass_name->get() == "opaque_2d")
            {
                pass = render::draw_pass::opaque_2d;
            }
            else if(pass_name->get() == "transparent_2d")
            {
                pass = render::draw_pass::transparent_2d;
            }
            else
            {
                return std::unexpected(asset_load_error(
                    params.file_path.string(), std::format("Material has an invalid pass name: {}", pass_name->get()),
                    pass_name->source().begin.line, pass_name->source().begin.column));
            }
        }

        render::shader::material mat(shader_asset, pass);
        // set the uniforms
        if(const auto uniforms_table = table["uniforms"].as_table())
        {
            for(const auto& [key, val] : *uniforms_table)
            {
                if(const auto str = val.as_string())
                {
                    // try to load it as a texture
                    uri tex_uri(str->get());
                    // if we have a scheme, it must be texture
                    if(!tex_uri.scheme().empty())
                    {
                        if(tex_uri.scheme() != "texture")
                        {
                            return std::unexpected(
                                asset_load_error(params.file_path.string(),
                                                 std::format("Material has a uniform {} with an invalid scheme: {}",
                                                             key.str(), tex_uri.scheme()),
                                                 str->source().begin.line, str->source().begin.column));
                        }
                        auto tex_load_result =
                            raoe::engine::load_asset<render::texture_2d>(params.world, fs::path(tex_uri.path()));
                        if(!tex_load_result.has_value())
                        {
                            return std::unexpected(asset_load_error::append(
                                tex_load_result.error(), params.file_path.string(),
                                std::format("Failed to load texture for material uniform {}: {}", key.str(),
                                            tex_uri.path()),
                                str->source().begin.line, str->source().begin.column));
                        }

                        mat.set_uniform(std::string(key.str()), raoe::render::generic_handle(tex_load_result.value()));
                    }
                    // if we dont have a scheme, this might be a color in hex notation
                    else if(str->get().starts_with('#'))
                    {
                        // get the substring of the hex color without the # and up to 8 characters
                        std::string color = str->get().substr(1, 8);

                        if(color.size() < 6)
                        {
                            // If we have less than 6 digits, pad with zeros
                            color.insert(color.begin(), 6 - color.size(), '0');
                        }
                        if(color.size() < 8)
                        {
                            // if we have less than 8 digits, pad with FF for alpha
                            color.append(8 - color.size(), 'F');
                        }

                        int32 color_val = 0;
                        std::from_chars(str->get().data() + 1, str->get().data() + str->get().size(), color_val, 16);
                        mat.set_uniform(std::string(key.str()), render::colors::from_hex_rgba(color_val));
                    }
                    else
                    {
                        spdlog::warn("Material {} has a uniform {} with an invalid hex color: {}",
                                     params.file_path.string(), key.str(), str->get());
                    }
                }
                else if(const auto number = val.as_floating_point())
                {
                    mat.set_uniform(std::string(key.str()), static_cast<float>(number->get()));
                }
                else if(const auto integer = val.as_integer())
                {
                    mat.set_uniform(std::string(key.str()), static_cast<int32>(integer->get()));
                }
                else if(const auto boolean = val.as_boolean())
                {
                    mat.set_uniform(std::string(key.str()), static_cast<int8>(boolean->get()));
                }
                else if(const auto array = val.as_array())
                {
                    // If we have an array of numbers, we have a vector.  We need to count how big the vector is
                    // (could be 2, 3, or 4)
                    if(array->size() >= 2 && array->size() <= 4 &&
                       std::ranges::all_of(*array, [](const toml::node& n) { return n.is_number(); }))
                    {
                        std::array values = {0.0f, 0.0f, 0.0f, 0.0f};
                        for(size_t i = 0; i < array->size(); ++i)
                        {
                            if(const auto num_flt = (*array)[i].as_floating_point())
                            {
                                values[i] = static_cast<float>(num_flt->get());
                            }
                            else if(const auto num_int = (*array)[i].as_integer())
                            {
                                values[i] = static_cast<float>(num_int->get());
                            }
                        }
                        switch(array->size())
                        {
                            case 2: mat.set_uniform(std::string(key.str()), glm::vec2(values[0], values[1])); break;
                            case 3:
                                mat.set_uniform(std::string(key.str()), glm::vec3(values[0], values[1], values[2]));
                                break;
                            case 4:
                                mat.set_uniform(std::string(key.str()),
                                                glm::vec4(values[0], values[1], values[2], values[3]));
                                break;
                            default: break;
                        }
                    }
                }
                else
                {
                    spdlog::warn("Material {} has a uniform {} with an unsupported type", params.file_path.string(),
                                 key.str());
                }
            }
        }

        return mat;
    }
}
