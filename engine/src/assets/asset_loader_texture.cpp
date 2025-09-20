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
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image/stb_image.h"

int read(void* stream, char* out_data, const int size)
{
    const auto file = static_cast<std::istream*>(stream);
    file->read(out_data, size);
    return static_cast<int>(file->gcount());
}

void skip(void* stream, const int size)
{
    const auto file = static_cast<std::istream*>(stream);
    file->seekg(size, std::ios_base::cur);
}

int eof(void* stream)
{
    const auto* file = static_cast<std::istream*>(stream);
    return file->eof() ? 1 : 0;
}

raoe::render::texture_wrap string_to_texture_wrap(std::string_view str)
{
    if(str == "clamp_to_edge")
    {
        return raoe::render::texture_wrap::clamp_to_edge;
    }
    if(str == "clamp_to_border")
    {
        return raoe::render::texture_wrap::clamp_to_border;
    }
    if(str == "repeat")
    {
        return raoe::render::texture_wrap::repeat;
    }
    if(str == "mirrored_repeat")
    {
        return raoe::render::texture_wrap::mirrored_repeat;
    }

    raoe::ensure("Unknown texture wrap mode: {}. Defaulting to clamp_to_edge.", str);
    return raoe::render::texture_wrap::clamp_to_edge;
}

raoe::render::texture_filter string_to_texture_filter(std::string_view str)
{
    if(str == "nearest" || str == "point")
    {
        return raoe::render::texture_filter::nearest;
    }
    if(str == "linear")
    {
        return raoe::render::texture_filter::linear;
    }

    raoe::ensure("Unknown texture filter mode: {}. Defaulting to nearest.", str);
    return raoe::render::texture_filter::nearest;
}

raoe::render::texture_2d raoe::engine::asset_loader<raoe::render::typed_texture<
    raoe::render::texture_type::texture_2d>>::load_asset(const asset_load_params& file_params)
{
    const stbi_io_callbacks callbacks = {
        .read = &read,
        .skip = &skip,
        .eof = &eof,
    };
    int width = 0, height = 0, channels = 0;
    unsigned char* data =
        stbi_load_from_callbacks(&callbacks, &file_params.file_stream, &width, &height, &channels, STBI_default);

    check_if(data != nullptr, "Failed to load texture from file: {}", file_params.file_path);
    const std::span texture_data(reinterpret_cast<std::byte*>(data), static_cast<size_t>(width * height * channels));

    auto fmt = render::texture_format::unknown;
    switch(channels)
    {
        case 1: fmt = render::texture_format::r8; break;
        case 2: fmt = render::texture_format::rg8; break;
        case 3: fmt = render::texture_format::rgb8; break;
        case 4: fmt = render::texture_format::rgba8; break;
        default: break;
    }

    // pull data from the meta file
    render::texture_params texture_params;
    bool use_mipmaps = true;
    if(file_params.metadata != nullptr)
    {
        if(auto texture_table = (*file_params.metadata)["texture"])
        {
            if(const std::optional<std::string_view> wrap_u = texture_table["wrap_u"].value<std::string_view>())
            {
                texture_params.wrap_u = string_to_texture_wrap(*wrap_u);
            }
            if(const std::optional<std::string_view> wrap_v = texture_table["wrap_v"].value<std::string_view>())
            {
                texture_params.wrap_v = string_to_texture_wrap(*wrap_v);
            }
            if(const std::optional<std::string_view> wrap_w = texture_table["wrap_w"].value<std::string_view>())
            {
                texture_params.wrap_w = string_to_texture_wrap(*wrap_w);
            }
            if(const std::optional<std::string_view> filter_min = texture_table["filter_min"].value<std::string_view>())
            {
                texture_params.filter_min = string_to_texture_filter(*filter_min);
            }
            if(const std::optional<std::string_view> filter_mag = texture_table["filter_mag"].value<std::string_view>())
            {
                texture_params.filter_mag = string_to_texture_filter(*filter_mag);
            }
            if(const std::optional<bool> mipmaps = texture_table["mipmaps"].value<bool>())
            {
                use_mipmaps = *mipmaps;
            }
        }
    }

    auto texture = render::texture_2d(texture_data, fmt, glm::ivec2(width, height), texture_params, use_mipmaps);
    stbi_image_free(data);
    return texture;
}