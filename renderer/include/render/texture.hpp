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

#include "render/types.hpp"

namespace raoe::render::texture
{
    enum class format
    {
        unknown,
        r8,
        rg8,
        rgb8,
        rgba8,
        r16,
        rg16,
        rgb16,
        rgba16,
        r16f,
        rg16f,
        rgb16f,
        rgba16f,
        r32f,
        rg32f,
        rgb32f,
        rgba32f,
        stencil8,
        count,

        standard = rgba8,
    };

    enum class filter
    {
        nearest,
        linear,
    };

    enum class wrap
    {
        clamp_to_edge,
        clamp_to_border,
        repeat,
        mirrored_repeat,
    };

    struct params
    {
        wrap wrap_u = wrap::clamp_to_edge;
        wrap wrap_v = wrap::clamp_to_edge;
        wrap wrap_w = wrap::clamp_to_edge;
        filter filter_min = filter::nearest;
        filter filter_mag = filter::nearest;
    };

    enum class type
    {
        none,
        texture_1d,
        texture_2d,
        texture_3d,
        cubemap,
        array_1d,
        array_2d,
        array_cube,
    };

    class texture
    {
      public:
        ~texture();

        [[nodiscard]] uint32 native_id() const { return m_native_id; }
        [[nodiscard]] format texture_format() const { return m_format; }
        [[nodiscard]] uint32 array_size() const { return m_array_size; }
        [[nodiscard]] bool mipmaps() const { return m_mipmaps; }
        [[nodiscard]] glm::ivec3 dim() const { return m_dim; }
        [[nodiscard]] const params& texture_params() const { return m_params; }
        [[nodiscard]] type texture_type() const { return m_texture_type; }

        texture(const texture&) = delete;
        texture& operator=(const texture&) = delete;

        void upload_to_gpu();
        void free_gpu_data();

        [[nodiscard]] bool has_cpu_data() const { return !m_data.empty(); }
        [[nodiscard]] std::span<const std::byte> cpu_data() const { return m_data; }

        void free_cpu_data()
        {
            m_data.clear();
            m_data.shrink_to_fit();
        }

      protected:
        explicit texture(std::span<const std::byte> data, type texture_type, format format, glm::ivec3 dim,
                         const params& params, int32 array_size = 1, bool mipmaps = false);

        explicit texture(const type texture_type = type::none)
            : m_texture_type(texture_type)
        {
        }

        uint32_t m_native_id = 0;

        texture(texture&& other) noexcept
            : m_native_id(std::exchange(other.m_native_id, {}))
            , m_format(std::exchange(other.m_format, format::unknown))
            , m_array_size(std::exchange(other.m_array_size, 1))
            , m_mipmaps(std::exchange(other.m_mipmaps, false))
            , m_dim(std::exchange(other.m_dim, glm::ivec3(0, 0, 0)))
            , m_params(std::exchange(other.m_params, params()))
            , m_texture_type(std::exchange(other.m_texture_type, type::none))
        {
        }

        texture& operator=(texture&& other) noexcept
        {
            m_native_id = std::exchange(other.m_native_id, {});
            m_format = std::exchange(other.m_format, format::unknown);
            m_array_size = std::exchange(other.m_array_size, 1);
            m_mipmaps = std::exchange(other.m_mipmaps, false);
            m_dim = std::exchange(other.m_dim, glm::ivec3(0, 0, 0));
            m_params = std::exchange(other.m_params, params());
            m_texture_type = std::exchange(other.m_texture_type, type::none);
            return *this;
        }

      private:
        std::vector<std::byte> m_data;
        format m_format = format::unknown;
        int32 m_array_size = 1;
        bool m_mipmaps = false;
        glm::ivec3 m_dim = glm::ivec3(0, 0, 0);
        params m_params = {};
        type m_texture_type = type::none;
    };

    template<type TTextureType>
    class typed_texture final : public texture
    {
      public:
        typed_texture()
            : texture(TTextureType)
        {
        }

        typed_texture(const std::span<const std::byte> data, const format format, const glm::ivec3 dim,
                      const params& params, const int32 array_size = 1, const bool mipmaps = true)
            : texture(data, TTextureType, format, dim, params, array_size, mipmaps)
        {
        }
        typed_texture(const std::span<const glm::u8vec4> data, const format format, const glm::ivec3 dim,
                      const params& params, const int32 array_size = 1, const bool mipmaps = true)
            : texture(std::as_bytes(data), TTextureType, format, dim, params, array_size, mipmaps)
        {
        }

        typed_texture(typed_texture&& other) noexcept
            : texture(std::move(other))
        {
        }

        typed_texture& operator=(typed_texture&& other) noexcept
        {
            texture::operator=(std::move(other));
            return *this;
        }
    };

    using texture_1d = typed_texture<type::texture_1d>;
    using texture_2d = typed_texture<type::texture_2d>;
    using texture_3d = typed_texture<type::texture_3d>;
    using texture_cubemap = typed_texture<type::cubemap>;
    using texture_array_1d = typed_texture<type::array_1d>;
    using texture_array_2d = typed_texture<type::array_2d>;
    using texture_array_cube = typed_texture<type::array_cube>;

    class render_texture final : public texture
    {
        // TODO: Implement render texture
    };

}

namespace raoe::render
{
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture> = renderer_type::any_texture;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_1d> = renderer_type::texture1d;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_2d> = renderer_type::texture2d;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_3d> = renderer_type::texture3d;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_cubemap> = renderer_type::texture_cube;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_array_1d> = renderer_type::texture1d_array;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_array_2d> = renderer_type::texture2d_array;
    template<>
    inline constexpr auto shader_uniform_type_v<texture::texture_array_cube> = renderer_type::texture_cube_array;
}