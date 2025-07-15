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

#include "render/texture.hpp"
#include "render_private.hpp"

#include "glad/glad.h"

namespace raoe::render::texture
{

    inline bool is_array_texture(const type texture_type)
    {
        return texture_type == type::array_1d || texture_type == type::array_2d || texture_type == type::array_cube;
    }

    inline bool has_2d(const type texture_type)
    {
        return texture_type == type::texture_2d || texture_type == type::array_2d || texture_type == type::texture_3d;
    }

    inline std::size_t format_size(const format format)
    {
        switch(format)
        {
            case format::stencil8:
            case format::r8: return 1;
            case format::rg8:
            case format::r16:
            case format::r16f: return 2;
            case format::rgb8: return 3;
            case format::rgba8:
            case format::r32f:
            case format::rg16:
            case format::rg16f: return 4;
            case format::rgb16:
            case format::rgb16f: return 6;
            case format::rgba16:
            case format::rgba16f:
            case format::rg32f: return 8;
            case format::rgb32f: return 12;
            case format::rgba32f: return 16;
            default: panic("Invalid format.");
        }
    }

    inline GLenum gl_sized_format(const format format)
    {
        switch(format)
        {
            case format::r8: return GL_R8;
            case format::r16: return GL_R16;
            case format::r16f: return GL_R16F;
            case format::r32f: return GL_R32F;
            case format::rg8: return GL_RG8;
            case format::rg16: return GL_RG16;
            case format::rg16f: return GL_RG16F;
            case format::rg32f: return GL_RG32F;
            case format::rgb8: return GL_RGB8;
            case format::rgb16: return GL_RGB16;
            case format::rgb16f: return GL_RGB16F;
            case format::rgb32f: return GL_RGB32F;
            case format::rgba8: return GL_RGBA8;
            case format::rgba16: return GL_RGBA16;
            case format::rgba16f: return GL_RGBA16F;
            case format::rgba32f: return GL_RGBA32F;
            case format::stencil8: return GL_STENCIL_INDEX8;
            default: panic("Invalid format.");
        }
    }

    inline GLenum gl_base_format(const format format)
    {
        switch(format)
        {
            case format::r8:
            case format::r16:
            case format::r16f:
            case format::r32f: return GL_RED;
            case format::rg8:
            case format::rg16:
            case format::rg16f:
            case format::rg32f: return GL_RG;
            case format::rgb8:
            case format::rgb16:
            case format::rgb16f:
            case format::rgb32f: return GL_RGB;
            case format::rgba8:
            case format::rgba16:
            case format::rgba16f:
            case format::rgba32f: return GL_RGBA;
            case format::stencil8: return GL_STENCIL_INDEX;
            default: panic("Invalid format.");
        }
    }

    inline GLenum gl_texture_type(const type texture_type)
    {
        switch(texture_type)
        {
            case type::array_1d: return GL_TEXTURE_1D_ARRAY;
            case type::array_2d: return GL_TEXTURE_2D_ARRAY;
            case type::array_cube: return GL_TEXTURE_CUBE_MAP_ARRAY;
            case type::cubemap: return GL_TEXTURE_CUBE_MAP;
            case type::texture_1d: return GL_TEXTURE_1D;
            case type::texture_2d: return GL_TEXTURE_2D;
            case type::texture_3d: return GL_TEXTURE_3D;
            default: panic("Invalid texture type.");
        }
    }

    inline GLint gl_wrap(const wrap wrap)
    {
        switch(wrap)
        {
            case wrap::clamp_to_edge: return GL_CLAMP_TO_EDGE;
            case wrap::clamp_to_border: return GL_CLAMP_TO_BORDER;
            case wrap::repeat: return GL_REPEAT;
            case wrap::mirrored_repeat: return GL_MIRRORED_REPEAT;
        }
        panic("Invalid wrap mode.");
    }

    inline GLint gl_filter(const filter filter)
    {
        switch(filter)
        {
            case filter::nearest: return GL_NEAREST;
            case filter::linear: return GL_LINEAR;
        }
        panic("Invalid filter mode.");
    }

    texture::texture(const std::span<const std::byte> data, const type texture_type, const format format,
                     const glm::ivec3 dim, const params& params, int32 array_size, const bool mipmaps)
        : m_data(data.begin(), data.end())
        , m_array_size(is_array_texture(texture_type) ? array_size : 1)
        , m_mipmaps(mipmaps)
        , m_dim(dim)
        , m_params(params)
        , m_texture_type(texture_type)
    {
        // Validate the texture data
        check_if(array_size < GL_MAX_ARRAY_TEXTURE_LAYERS, "Array size exceeds maximum array texture layers.");
        check_if(dim.x > 0 && dim.y > 0 && dim.z > 0, "Texture dimensions must be greater than 0.");
        check_if(data.size_bytes() > 0, "Texture data is empty.");
        check_if(dim.x * dim.y * dim.z * array_size * format_size(format) == data.size_bytes(),
                 "Texture data size does not match dimensions.");
        check_if(dim.x <= GL_MAX_TEXTURE_SIZE && dim.y <= GL_MAX_TEXTURE_SIZE && dim.z <= GL_MAX_TEXTURE_SIZE,
                 "Texture dimensions exceed maximum texture size.");
    }

    texture::~texture()
    {
        free_gpu_data();
    }
    void texture::upload_to_gpu()
    {
        check_if(has_cpu_data(), "Cannot upload texture to GPU without CPU data.");
        // Generate the texture
        if(m_native_id == 0)
        {
            glCreateTextures(gl_texture_type(m_texture_type), 1, &m_native_id);
        }

        glTextureParameteri(m_native_id, GL_TEXTURE_WRAP_S, gl_wrap(m_params.wrap_u));
        if(has_2d(m_texture_type))
        {
            glTextureParameteri(m_native_id, GL_TEXTURE_WRAP_T, gl_wrap(m_params.wrap_v));
        }
        if(m_texture_type == type::texture_3d)
        {
            glTextureParameteri(m_native_id, GL_TEXTURE_WRAP_R, gl_wrap(m_params.wrap_w));
        }
        glTextureParameteri(m_native_id, GL_TEXTURE_MIN_FILTER, gl_filter(m_params.filter_min));
        glTextureParameteri(m_native_id, GL_TEXTURE_MAG_FILTER, gl_filter(m_params.filter_mag));

        // Set the texture size
        if(m_texture_type == type::texture_1d || m_texture_type == type::array_1d)
        {
            if(m_array_size > 1)
            {
                glTextureStorage2D(m_native_id, 1, gl_sized_format(m_format), m_dim.x, m_array_size);
                glTextureSubImage2D(m_native_id, 0, 0, 0, m_dim.x, m_array_size, gl_base_format(m_format),
                                    GL_UNSIGNED_BYTE, m_data.data());
            }
            else
            {
                glTextureStorage1D(m_native_id, 1, gl_sized_format(m_format), m_dim.x);
                glTextureSubImage1D(m_native_id, 0, 0, m_dim.x, gl_base_format(m_format), GL_UNSIGNED_BYTE,
                                    m_data.data());
            }
        }
        else if(m_texture_type == type::texture_2d || m_texture_type == type::array_2d ||
                m_texture_type == type::cubemap)
        {
            if(m_array_size > 1)
            {
                glTextureStorage3D(m_native_id, 1, gl_sized_format(m_format), m_dim.x, m_dim.y, m_array_size);
                glTextureSubImage3D(m_native_id, 0, 0, 0, 0, m_dim.x, m_dim.y, m_array_size, gl_base_format(m_format),
                                    GL_UNSIGNED_BYTE, m_data.data());
            }
            else
            {
                glTextureStorage2D(m_native_id, 1, gl_sized_format(m_format), m_dim.x, m_dim.y);
                glTextureSubImage2D(m_native_id, 0, 0, 0, m_dim.x, m_dim.y, gl_base_format(m_format), GL_UNSIGNED_BYTE,
                                    m_data.data());
            }
        }
        else if(m_texture_type == type::texture_3d)
        {
            glTextureStorage3D(m_native_id, 1, gl_sized_format(m_format), m_dim.x, m_dim.y, m_dim.z);
            glTextureSubImage3D(m_native_id, 0, 0, 0, 0, m_dim.x, m_dim.y, m_dim.z, gl_base_format(m_format),
                                GL_UNSIGNED_BYTE, m_data.data());
        }
        else
        {
            panic("Invalid texture type."); // TODO: Cubemaps
        }

        if(mipmaps())
        {
            glGenerateTextureMipmap(m_native_id);
        }
    }
    void texture::free_gpu_data()
    {
        if(m_native_id != 0)
        {
            glDeleteTextures(1, &m_native_id);
            m_native_id = 0;
        }
    }

}