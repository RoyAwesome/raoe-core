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

namespace raoe::render
{
    enum class texture_format
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

    enum class texture_filter
    {
        nearest,
        linear,
    };

    enum class texture_wrap
    {
        clamp_to_edge,
        clamp_to_border,
        repeat,
        mirrored_repeat,
    };

    struct texture_params
    {
        texture_wrap wrap_u = texture_wrap::clamp_to_edge;
        texture_wrap wrap_v = texture_wrap::clamp_to_edge;
        texture_wrap wrap_w = texture_wrap::clamp_to_edge;
        texture_filter filter_min = texture_filter::nearest;
        texture_filter filter_mag = texture_filter::nearest;
    };

    enum class texture_type
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
        ~texture() { free_gpu_data(); }

        [[nodiscard]] uint32 native_id() const { return m_native_id; }
        [[nodiscard]] render::texture_format texture_format() const { return m_format; }
        [[nodiscard]] uint32 array_size() const { return m_array_size; }
        [[nodiscard]] bool mipmaps() const { return m_mipmaps; }
        [[nodiscard]] glm::ivec3 dim() const { return m_dim; }
        [[nodiscard]] const render::texture_params& texture_params() const { return m_params; }
        [[nodiscard]] render::texture_type texture_type() const { return m_texture_type; }

        texture(const texture&) = delete;
        texture& operator=(const texture&) = delete;

        void upload_to_gpu();
        void free_gpu_data();

        [[nodiscard]] bool has_cpu_data() const { return !m_data.empty(); }
        [[nodiscard]] std::span<const std::byte> cpu_data() const { return m_data; }
        [[nodiscard]] bool has_gpu_data() const { return m_native_id != 0; }

        void free_cpu_data()
        {
            m_data.clear();
            m_data.shrink_to_fit();
        }

      protected:
        explicit texture(std::span<const std::byte> data, render::texture_type type, render::texture_format format,
                         glm::ivec3 dim, const render::texture_params& params, int32 array_size = 1,
                         bool mipmaps = false);

        explicit texture(const render::texture_type type = texture_type::none)
            : m_texture_type(type)
        {
        }

        uint32_t m_native_id = 0;

        texture(texture&& other) noexcept
            : m_native_id(std::exchange(other.m_native_id, {}))
            , m_format(std::exchange(other.m_format, texture_format::unknown))
            , m_array_size(std::exchange(other.m_array_size, 1))
            , m_mipmaps(std::exchange(other.m_mipmaps, false))
            , m_dim(std::exchange(other.m_dim, glm::ivec3(0, 0, 0)))
            , m_params(std::exchange(other.m_params, texture_params()))
            , m_texture_type(std::exchange(other.m_texture_type, texture_type::none))
        {
        }

        texture& operator=(texture&& other) noexcept
        {
            m_native_id = std::exchange(other.m_native_id, {});
            m_format = std::exchange(other.m_format, texture_format::unknown);
            m_array_size = std::exchange(other.m_array_size, 1);
            m_mipmaps = std::exchange(other.m_mipmaps, false);
            m_dim = std::exchange(other.m_dim, glm::ivec3(0, 0, 0));
            m_params = std::exchange(other.m_params, texture_params());
            m_texture_type = std::exchange(other.m_texture_type, texture_type::none);
            return *this;
        }

      private:
        std::vector<std::byte> m_data;
        render::texture_format m_format = texture_format::unknown;
        int32 m_array_size = 1;
        bool m_mipmaps = false;
        glm::ivec3 m_dim = glm::ivec3(0, 0, 0);
        render::texture_params m_params = {};
        render::texture_type m_texture_type = texture_type::none;
    };

    template<texture_type TTextureType>
    class typed_texture final : public texture
    {
      public:
        typed_texture()
            : texture(TTextureType)
        {
        }

        typed_texture(const std::span<const std::byte> data, const render::texture_format format, const glm::ivec3 dim,
                      const render::texture_params& params, const bool mipmaps = true)
            requires(TTextureType == texture_type::texture_3d)
            : texture(data, TTextureType, format, dim, params, 1, mipmaps)
        {
        }
        typed_texture(const std::span<const std::byte> data, const render::texture_format format, const glm::ivec2 dim,
                      const render::texture_params& params, const bool mipmaps = true)
            requires(TTextureType == texture_type::texture_2d || TTextureType == texture_type::cubemap)
            : texture(data, TTextureType, format, glm::vec3(dim, 1), params, 1, mipmaps)
        {
        }
        typed_texture(const std::span<const std::byte> data, const render::texture_format format, const int32 dim,
                      const render::texture_params& params, const bool mipmaps = true)
            requires(TTextureType == texture_type::texture_1d)
            : texture(data, TTextureType, format, glm::vec3(dim, 1, 1), params, 1, mipmaps)
        {
        }

        typed_texture(const std::span<const glm::u8vec4> data, const glm::ivec3 dim,
                      const render::texture_params& params, const bool mipmaps = true)
            requires(TTextureType == texture_type::texture_3d)
            : texture(std::as_bytes(data), TTextureType, texture_format::rgba8, dim, params, 1, mipmaps)
        {
        }

        typed_texture(const std::span<const glm::u8vec4> data, const glm::ivec2 dim,
                      const render::texture_params& params, const bool mipmaps = true)
            requires(TTextureType == texture_type::texture_2d || TTextureType == texture_type::cubemap)
            : texture(std::as_bytes(data), TTextureType, texture_format::rgba8, glm::vec3(dim, 1), params, 1, mipmaps)
        {
        }

        typed_texture(const std::span<const glm::u8vec4> data, const int32 dim, const render::texture_params& params,
                      const bool mipmaps = true)
            requires(TTextureType == texture_type::texture_1d)
            : texture(std::as_bytes(data), TTextureType, texture_format::rgba8, glm::vec3(dim, 1, 1), params, 1,
                      mipmaps)
        {
        }

        typed_texture(const std::span<const std::byte> data, const render::texture_format format, const glm::ivec2 dim,
                      const render::texture_params& params, const int32 array_size, const bool mipmaps = true)
            requires(TTextureType == texture_type::array_2d || TTextureType == texture_type::array_cube)
            : texture(data, TTextureType, format, glm::vec3(dim, 1), params, array_size, mipmaps)
        {
            check_if(array_size > 0, "Array size must be greater than 0.");
        }

        typed_texture(const std::span<const std::byte> data, const render::texture_format format, const int32 dim,
                      const render::texture_params& params, const int32 array_size, const bool mipmaps = true)
            requires(TTextureType == texture_type::array_1d)
            : texture(data, TTextureType, format, glm::vec3(dim, 1, 1), params, array_size, mipmaps)
        {
            check_if(array_size > 0, "Array size must be greater than 0.");
        }

        typed_texture(const std::span<const glm::u8vec4> data, const glm::ivec2 dim,
                      const render::texture_params& params, const int32 array_size, const bool mipmaps = true)
            requires(TTextureType == texture_type::array_2d || TTextureType == texture_type::array_cube)
            : texture(std::as_bytes(data), TTextureType, texture_format::rgba8, glm::vec3(dim, 1), params, array_size,
                      mipmaps)
        {
            check_if(array_size > 0, "Array size must be greater than 0.");
        }

        typed_texture(const std::span<const glm::u8vec4> data, const int32 dim, const render::texture_params& params,
                      const int32 array_size, const bool mipmaps = true)
            requires(TTextureType == texture_type::array_1d)
            : texture(std::as_bytes(data), TTextureType, texture_format::rgba8, glm::vec3(dim, 1, 1), params,
                      array_size, mipmaps)
        {
            check_if(array_size > 0, "Array size must be greater than 0.");
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

    using texture_1d = typed_texture<texture_type::texture_1d>;
    using texture_2d = typed_texture<texture_type::texture_2d>;
    using texture_3d = typed_texture<texture_type::texture_3d>;
    using texture_cubemap = typed_texture<texture_type::cubemap>;
    using texture_array_1d = typed_texture<texture_type::array_1d>;
    using texture_array_2d = typed_texture<texture_type::array_2d>;
    using texture_array_cube = typed_texture<texture_type::array_cube>;

    class render_texture final : public texture
    {
        // TODO: Implement render texture
      private:
        // Make this public when render textures are implemented
        explicit render_texture(const glm::ivec3 dim, const render::texture_params& params = {})
            : texture({}, texture_type::texture_2d, texture_format::rgba8, dim, params, 1, false)
        {
        }
    };

    template<>
    inline constexpr auto shader_uniform_type_v<texture> = renderer_type::any_texture;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_1d> = renderer_type::texture1d;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_2d> = renderer_type::texture2d;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_3d> = renderer_type::texture3d;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_cubemap> = renderer_type::texture_cube;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_array_1d> = renderer_type::texture1d_array;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_array_2d> = renderer_type::texture2d_array;
    template<>
    inline constexpr auto shader_uniform_type_v<texture_array_cube> = renderer_type::texture_cube_array;
    template<>
    inline constexpr auto shader_uniform_type_v<render_texture> = renderer_type::texture2d;

}
