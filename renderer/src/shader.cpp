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

#include "render/shader.hpp"

#include "render_private.hpp"
#include <iterator>
#include <unordered_map>

#include "glad/glad.h"

namespace raoe::render::shader
{
    uint32 _internal::compile_source_glsl(shader_type shader_type, const std::span<const std::byte> source)
    {
        if(source.empty())
        {
            return GL_NONE;
        }

        GLenum gl_shader_type = GL_NONE;
        switch(shader_type)
        {
            case shader_type::vertex: gl_shader_type = GL_VERTEX_SHADER; break;
            case shader_type::fragment: gl_shader_type = GL_FRAGMENT_SHADER; break;
            case shader_type::geometry: gl_shader_type = GL_GEOMETRY_SHADER; break;
            case shader_type::tesselation_control: gl_shader_type = GL_TESS_CONTROL_SHADER; break;
            case shader_type::tesselation_evaluation: gl_shader_type = GL_TESS_EVALUATION_SHADER; break;
            case shader_type::mesh: panic("Mesh Shaders Not Supported (yet)"); break;
            case shader_type::compute: gl_shader_type = GL_COMPUTE_SHADER; break;
            default: panic("Invalid shader type", underlying(shader_type));
        }
        spdlog::info("Compiling {} Shader - stage: {}", shader_lang::glsl, shader_type);
        const GLuint shader = glCreateShader(gl_shader_type);
        if(shader == 0)
        {
            panic("Failed to create shader object");
        }
        const auto* source_str = reinterpret_cast<const GLchar*>(source.data());

        glShaderSource(shader, 1, &source_str, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if(success == GL_FALSE)
        {
            GLint log_size = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);

            std::string log;
            log.resize(log_size);
            glGetShaderInfoLog(shader, log_size, &log_size, log.data());

            ensure_always("Failed to compile shader: {}", log);

            glDeleteShader(shader);
            return GL_NONE;
        }

        return shader;
    }

    uint32 _internal::create_program(const std::span<uint32, underlying(shader_type::count)> modules)
    {
        const GLuint program = glCreateProgram();
        for(const auto module : modules)
        {
            if(module != 0)
            {
                glAttachShader(program, module);
            }
        }

        glLinkProgram(program);

        GLint success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if(success == GL_FALSE)
        {
            GLint log_size = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);

            std::string log;
            log.resize(log_size);
            glGetProgramInfoLog(program, log_size, &log_size, log.data());

            ensure_always("Failed to link shader: {}", log);

            glDeleteProgram(program);
            return GL_NONE;
        }

        return program;
    }

    constexpr renderer_type gl_type_to_renderer_type(const GLenum gl_type)
    {
        switch(gl_type)
        {
            case GL_INT: return renderer_type::i32;
            case GL_UNSIGNED_INT: return renderer_type::u32;
            case GL_FLOAT: return renderer_type::f32;
            case GL_DOUBLE: return renderer_type::f64;
            case GL_FLOAT_VEC2: return renderer_type::vec2;
            case GL_FLOAT_VEC3: return renderer_type::vec3;
            case GL_FLOAT_VEC4: return renderer_type::vec4;
            case GL_FLOAT_MAT2: return renderer_type::mat2;
            case GL_FLOAT_MAT3: return renderer_type::mat3;
            case GL_FLOAT_MAT4: return renderer_type::mat4;
            case GL_SAMPLER_1D: return renderer_type::texture1d;
            case GL_SAMPLER_2D: return renderer_type::texture2d;
            case GL_SAMPLER_3D: return renderer_type::texture3d;
            case GL_SAMPLER_CUBE: return renderer_type::texture_cube;
            case GL_SAMPLER_1D_ARRAY: return renderer_type::texture1d_array;
            case GL_SAMPLER_2D_ARRAY: return renderer_type::texture2d_array;
            case GL_SAMPLER_CUBE_MAP_ARRAY: return renderer_type::texture_cube_array;
            default: return renderer_type::none;
        }
    }

    uniform& shader::operator[](std::string_view name)
    {
        if(!ready())
        {
            panic("Shader {} - trying to get a uniform when the shader is not compiled", m_debug_name);
        }

        const auto l_it = m_uniform_names.find(std::string(name));
        if(l_it == m_uniform_names.end())
        {
            panic("Shader {} - Uniform not found: {}", m_debug_name, name);
        }
        return this->operator[](l_it->second);
    }

    uniform& shader::operator[](uint32 location)
    {
        if(!ready())
        {
            panic("Shader {} - trying to get a uniform when the shader is not compiled", m_debug_name);
        }

        const auto it = m_uniforms.find(location);
        if(it == m_uniforms.end())
        {
            panic("Shader {} - Uniform not found: {}", m_debug_name, location);
        }
        return it->second;
    }

    std::shared_ptr<shader> shader::make_shared(const uint32 id, std::string debug_name)
    {

        return {new shader(id, std::move(debug_name)), [](shader* ptr) {
                    if(ptr->m_native_id != 0)
                    {
                        glDeleteProgram(ptr->m_native_id);
                    }
                    delete ptr;
                }};
    }
    void shader::introspect()
    {
        GLint uniform_count = 0;
        glGetProgramiv(m_native_id, GL_ACTIVE_UNIFORMS, &uniform_count);
        constexpr std::array<GLenum, 4> uniform_properties = {GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION};

        for(int32 i = 0; i < uniform_count; i++)
        {
            std::array<GLint, 4> properties {};
            glGetProgramResourceiv(m_native_id, GL_UNIFORM, i, uniform_properties.size(), uniform_properties.data(),
                                   properties.size(), nullptr, properties.data());

            std::string name;
            if(properties[0] > 0)
            {
                name = std::string(properties[0], '\0');
                glGetProgramResourceName(m_native_id, GL_UNIFORM, i, static_cast<GLsizei>(name.size()), nullptr,
                                         name.data());
                name.pop_back(); // Remove the \0
                m_uniform_names.emplace(name, properties[3]);
            }

            // Acquire the binding point for textures.
            uint8 binding = 0;
            const renderer_type utype = gl_type_to_renderer_type(properties[1]);
            if(utype == renderer_type::texture1d || utype == renderer_type::texture2d ||
               utype == renderer_type::texture3d || utype == renderer_type::texture_cube ||
               utype == renderer_type::texture1d_array || utype == renderer_type::texture2d_array ||
               utype == renderer_type::texture_cube_array)
            {
                glGetUniformiv(m_native_id, properties[3], reinterpret_cast<GLint*>(&binding));
            }

            m_uniforms.emplace(properties[3], uniform(properties[3], utype, binding));
        }

        // get the vertex attributes
        GLint input_count = 0;
        glGetProgramInterfaceiv(m_native_id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &input_count);
        constexpr std::array<GLenum, 3> attribute_properties = {GL_NAME_LENGTH, GL_TYPE, GL_LOCATION};
        for(int32 i = 0; i < input_count; i++)
        {
            std::array<GLint, 3> properties {};
            glGetProgramResourceiv(m_native_id, GL_PROGRAM_INPUT, i, attribute_properties.size(),
                                   attribute_properties.data(), properties.size(), nullptr, properties.data());

            if(properties[0] > 0)
            {
                std::string name(properties[0], '\0');
                glGetProgramResourceName(m_native_id, GL_PROGRAM_INPUT, i, static_cast<GLsizei>(name.size()), nullptr,
                                         name.data());
                name.pop_back(); // Remove the \0

                const renderer_type attribute_type = gl_type_to_renderer_type(properties[1]);

                m_inputs.emplace_back(input {name, properties[2], attribute_type});
            }
        }

        // sort the inputs by location
        std::ranges::sort(m_inputs, [](const input& a, const input& b) { return a.location() < b.location(); });
    }

    void uniform::set_uniform(const std::span<const std::byte> data, const int32 element_count) const
    {
        if(element_count < 1)
        {
            panic("Uniform count must be greater than 0");
        }

        switch(m_type)
        {
            case renderer_type::i32:
                glUniform1iv(m_native_id, element_count, reinterpret_cast<const GLint*>(data.data()));

                break;
            case renderer_type::u32:
                glUniform1uiv(m_native_id, element_count, reinterpret_cast<const GLuint*>(data.data()));

                break;
            case renderer_type::f32:
                glUniform1fv(m_native_id, element_count, reinterpret_cast<const GLfloat*>(data.data()));
                break;
            case renderer_type::f64:
                glUniform1dv(m_native_id, element_count, reinterpret_cast<const GLdouble*>(data.data()));
                break;
            case renderer_type::vec2:
                glUniform2fv(m_native_id, element_count,
                             glm::value_ptr(*reinterpret_cast<const glm::vec2*>(data.data())));
                break;
            case renderer_type::vec3:
                glUniform3fv(m_native_id, element_count,
                             glm::value_ptr(*reinterpret_cast<const glm::vec3*>(data.data())));
                break;
            case renderer_type::vec4:
                glUniform4fv(m_native_id, element_count,
                             glm::value_ptr(*reinterpret_cast<const glm::vec4*>(data.data())));
                break;
            case renderer_type::mat2:
                glUniformMatrix2fv(m_native_id, element_count, GL_FALSE,
                                   glm::value_ptr(*reinterpret_cast<const glm::mat2*>(data.data())));
                break;
            case renderer_type::mat3:
                glUniformMatrix3fv(m_native_id, element_count, GL_FALSE,
                                   glm::value_ptr(*reinterpret_cast<const glm::mat3*>(data.data())));
                break;
            case renderer_type::mat4:
                glUniformMatrix4fv(m_native_id, element_count, GL_FALSE,
                                   glm::value_ptr(*reinterpret_cast<const glm::mat4*>(data.data())));
                break;
            default: panic("Invalid renderer type", underlying(m_type));
        }
    }

    void uniform::set_uniform(const texture::texture& texture) const
    {
        glBindTextureUnit(m_texture_unit, texture.native_id());
    }

}