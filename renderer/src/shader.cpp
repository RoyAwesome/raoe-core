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
#include <unordered_set>
#include <utility>

#include <map>
#include <set>

#include "glad/glad.h"

namespace raoe::render::shader
{
    uint32 _internal::compile_source_glsl(shader_type shader_type, const std::span<const std::byte> source,
                                          std::string_view debug_name)
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
        spdlog::info("Compiling {} Shader '{}' - stage: {}", shader_lang::glsl, debug_name, shader_type);
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
            log.resize(log_size - 1);

            ensure_always("Failed to compile shader '{}':\n{}\nSource:\n{}", debug_name, log, source_str);

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

    uniform& shader::uniform_accessor::operator[](std::string_view name) const
    {
        if(!m_shader.ready())
        {
            panic("Shader {} - trying to get a uniform when the shader is not compiled", m_shader.m_debug_name);
        }

        const auto l_it = m_shader.m_uniform_names.find(std::string(name));
        if(l_it == m_shader.m_uniform_names.end())
        {
            panic("Shader {} - Uniform not found: {}", m_shader.m_debug_name, name);
        }
        if(auto* u = direct_access(l_it->second); u != nullptr)
        {
            return *u;
        }
        panic("Shader {} - Uniform not found: {}", m_shader.m_debug_name, name);
    }
    uniform& shader::uniform_accessor::operator[](uint32 location) const
    {
        if(!m_shader.ready())
        {
            panic("Shader {} - trying to get a uniform when the shader is not compiled", m_shader.m_debug_name);
        }

        if(auto* u = direct_access(location); u != nullptr)
        {
            return *u;
        }
        panic("Shader {} - Uniform not found: {}", m_shader.m_debug_name, location);
    }
    uniform* shader::uniform_accessor::direct_access(const uint32 location) const
    {
        const auto it = m_shader.m_uniforms.find(location);
        if(it == m_shader.m_uniforms.end())
        {
            return nullptr;
        }
        return &it->second;
    }
    uniform_block& shader::uniform_block_accessor::operator[](std::string_view name) const
    {
        if(!m_shader.ready())
        {
            panic("Shader {} - trying to get a uniform when the shader is not compiled", m_shader.m_debug_name);
        }

        const auto l_it = m_shader.m_uniform_names.find(std::string(name));
        if(l_it == m_shader.m_uniform_names.end())
        {
            panic("Shader {} - Uniform not found: {}", m_shader.m_debug_name, name);
        }
        if(auto* u = direct_access(l_it->second); u != nullptr)
        {
            return *u;
        }
        panic("Shader {} - Uniform not found: {}", m_shader.m_debug_name, name);
    }
    uniform_block& shader::uniform_block_accessor::operator[](uint32 binding) const
    {
        if(!m_shader.ready())
        {
            panic("Shader {} - trying to get a uniform when the shader is not compiled", m_shader.m_debug_name);
        }

        if(auto* u = direct_access(binding); u != nullptr)
        {
            return *u;
        }
        panic("Shader {} - Uniform not found: {}", m_shader.m_debug_name, binding);
    }
    uniform_block* shader::uniform_block_accessor::direct_access(const uint32 binding) const
    {
        const auto it = m_shader.m_uniform_blocks.find(binding);
        if(it == m_shader.m_uniform_blocks.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    void shader::use() const
    {
        check_if(native_id() > 0, "Shader {} - trying to use a shader that is not compiled", m_debug_name);
        int32 current_program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
        if(current_program != native_id())
        {
            glUseProgram(native_id());
        }
    }
    std::string shader::debug_string() const
    {
        std::stringstream ss;
        ss << std::format("Shader '{}' (ID: {})\n", m_debug_name, m_native_id);
        ss << "Uniforms:\n";
        for(const auto& [location, uniform] : m_uniforms)
        {

            ss << std::format("  Name: {}, Location: {}, Type: {}, Binding: {}\n", uniform.name(), location,
                              uniform.type(), uniform.native_id());
        }
        ss << "Uniform Blocks:\n";
        for(const auto& [binding, block] : m_uniform_blocks)
        {
            ss << std::format("  Binding: {} name: {}", binding, block.name());
            for(const auto& desc : block.m_block_type_description)
            {
                ss << std::format("\n\t{}", desc);
            }
            ss << "\n";
        }
        ss << "Inputs:\n";
        for(const auto& input : m_inputs)
        {
            ss << std::format("  Name: {}, Location: {}, Type: {}\n", input.name(), input.location(), input.type());
        }

        return ss.str();
    }
    std::shared_ptr<shader> shader::make_shared(const uint32 id, std::string debug_name)
    {
        // ReSharper disable once CppDFAMemoryLeak
        // Use a custom deleter to delete the shader when the shared_ptr is destroyed
        return {new shader(id, std::move(debug_name)), [](const shader* ptr) {
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
            auto [name_length, gl_type, array_size, location] = properties;
            std::string name;
            if(name_length > 0)
            {
                name = std::string(name_length, '\0');
                glGetProgramResourceName(m_native_id, GL_UNIFORM, i, static_cast<GLsizei>(name.size()), nullptr,
                                         name.data());
                name.pop_back(); // Remove the \0
            }

            // Acquire the binding point for textures.
            uint8 binding = 0;
            const renderer_type utype = gl_type_to_renderer_type(gl_type);
            if(utype == renderer_type::texture1d || utype == renderer_type::texture2d ||
               utype == renderer_type::texture3d || utype == renderer_type::texture_cube ||
               utype == renderer_type::texture1d_array || utype == renderer_type::texture2d_array ||
               utype == renderer_type::texture_cube_array)
            {
                glGetUniformiv(m_native_id, location, reinterpret_cast<GLint*>(&binding));
            }

            m_uniforms.emplace(properties[3], uniform(name, location, utype, binding));
            if(!name.empty())
            {
                m_uniform_names.emplace(name, location);
            }
        }

        // get the uniform blocks
        GLint uniform_block_count = 0;
        glGetProgramInterfaceiv(m_native_id, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &uniform_block_count);
        constexpr std::array<GLenum, 3> uniform_block_properties = {GL_NAME_LENGTH, GL_NUM_ACTIVE_VARIABLES,
                                                                    GL_BUFFER_BINDING};
        constexpr std::array<GLenum, 1> uniform_block_index = {GL_ACTIVE_VARIABLES};
        constexpr std::array<GLenum, 5> uniform_properties_ub = {GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION,
                                                                 GL_OFFSET};
        for(int32 block_index = 0; block_index < uniform_block_count; block_index++)
        {
            std::array<GLint, 3> block_properties {};
            glGetProgramResourceiv(m_native_id, GL_UNIFORM_BLOCK, block_index, uniform_block_properties.size(),
                                   uniform_block_properties.data(), block_properties.size(), nullptr,
                                   block_properties.data()); // query the data about the block
            auto [name_length, num_active_uniforms, binding] = block_properties;

            // get the name of the block
            std::string block_name(name_length, '\0');
            if(name_length > 0)
            {
                glGetProgramResourceName(m_native_id, GL_UNIFORM_BLOCK, block_index,
                                         static_cast<GLsizei>(block_name.size()), nullptr, block_name.data());
                block_name.pop_back(); // Remove the \0
            }
            std::vector<type_description> block_type_description;
            if(num_active_uniforms > 0)
            {

                // Get the type description of each uniform.

                std::vector<GLint> block_uniforms(num_active_uniforms);
                glGetProgramResourceiv(m_native_id, GL_UNIFORM_BLOCK, block_index, uniform_block_index.size(),
                                       uniform_block_index.data(), static_cast<GLint>(block_uniforms.size()), nullptr,
                                       block_uniforms.data()); // get the indices of the active uniforms in the block

                for(int32 uniform_index = 0; uniform_index < num_active_uniforms; uniform_index++)
                {
                    std::array<GLint, 5> block_uniform_properties {};
                    glGetProgramResourceiv(m_native_id, GL_UNIFORM, block_uniforms[uniform_index],
                                           uniform_properties_ub.size(), uniform_properties_ub.data(),
                                           block_uniform_properties.size(), nullptr,
                                           block_uniform_properties.data()); // get the properties of the uniform
                    auto [name_length, gl_type, array_size, location, offset] = block_uniform_properties;
                    std::string name(name_length, '\0');
                    if(name_length > 0)
                    {
                        glGetProgramResourceName(m_native_id, GL_UNIFORM, block_uniforms[uniform_index],
                                                 static_cast<GLsizei>(name.size()), nullptr, name.data());
                        name.pop_back(); // Remove the \0
                    }
                    const renderer_type utype = gl_type_to_renderer_type(gl_type);
                    block_type_description.emplace_back(type_description {
                        .type = utype,
                        .offset = static_cast<std::size_t>(offset),
                        .hint = type_hint::none,
                        .array_size = static_cast<std::size_t>(array_size),
                    });
                }
            }

            m_uniform_blocks.emplace(binding, uniform_block(block_name, block_index, binding, block_type_description));
            m_uniform_block_names.emplace(block_name, binding);
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
    void material::use()
    {

        check_if(m_shader != nullptr, "Material shader cannot be null");
        m_shader->use();
        for(auto& [location, name, data] : m_uniforms)
        {
            if(!location)
            {
                spdlog::warn("Attempting to set uniform {} in shader {}, but it has no location", name,
                             m_shader->debug_name());
                continue;
            }
            if(!m_shader->is_valid_uniform_location(*location))
            {
                spdlog::warn("Attempting to set uniform {} in shader {}, but the location {} is not valid", name,
                             m_shader->debug_name(), *location);
                continue;
            }
            std::visit(
                [&, this]<typename T0>(T0&& value) {
                    using T = std::decay_t<T0>;
                    if constexpr(std::is_same_v<T, std::monostate>)
                    {
                        spdlog::warn("Attempting to set uniform {} in shader {}, but it is not set", name,
                                     m_shader->debug_name());
                    }
                    else if constexpr(is_valid_renderer_type(shader_uniform_type_v<T>) &&
                                      !is_texture_type(shader_uniform_type_v<T>))
                    {
                        m_shader->uniforms()[*location] = value;
                    }
                    else if constexpr(std::is_same_v<T, generic_handle<texture>>)
                    {
                        if(value != nullptr)
                        {
                            if(!value.get()->has_gpu_data())
                            {
                                value.get()->upload_to_gpu();
                            }

                            m_shader->uniforms()[*location] = *value.get();
                        }
                        else
                        {
                            spdlog::warn("Attempting to set uniform {} in shader {}, but the texture handle is null",
                                         name, m_shader->debug_name());
                        }
                    }
                },
                data);
        }
    }

    void uniform::set_uniform(const std::span<const std::byte> data, const int32 element_count) const
    {
        check_if(element_count > 0, "Uniform count ({}) must be greater than 0", element_count);

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

    void uniform::set_uniform(const texture& texture) const
    {
        glBindTextureUnit(m_texture_unit, texture.native_id());
    }

    uniform_block& uniform_block::operator=(const uniform_buffer& buffer)
    {
        // check that the buffer type matches the block type
        const auto& block_type = block_type_description();
        const auto& buffer_type = buffer.elements();

        check_if(elements_equal(block_type, buffer_type),
                 "Uniform block '{}' type description does not match the buffer type description (block size: {}, "
                 "buffer size: {}; \n DBG \n\tblock_type: {} \n\t buffer_type: {})",
                 name(), block_type.size(), buffer_type.size(), block_type, buffer_type);

        check_if(buffer.is_valid(), "Uniform buffer is not valid");

        // bind the buffer to the binding point
        glBindBufferBase(GL_UNIFORM_BUFFER, m_binding, buffer.native_buffer());
        return *this;
    }
}

std::string preprocess_r(std::string source, const raoe::render::shader::glsl::file_load_callback_t& load_file_callback,
                         std::unordered_set<std::string>& included_files,
                         const std::unordered_map<std::string, std::string>& injections, uint32 original_file_index = 0)
{

    std::size_t pos = 0;
    int32 original_file_line = 1;
    auto peek = [&source, &pos]() -> char {
        if(pos + 1 < source.size())
        {
            return source[pos];
        }
        return '\0';
    };
    auto get = [&source, &pos]() -> char {
        if(pos < source.size())
        {
            return source[pos++];
        }
        return '\0';
    };
    auto matching_end_char = [](const char start_char) -> char {
        switch(start_char)
        {
            case '\"': return '\"';
            case '\'': return '\'';
            case '<': return '>';
            default: return '\0';
        }
    };
    auto has_more = [&source, &pos]() -> bool { return pos < source.size(); };
    // find all elements that start with #
    while(has_more())
    {
        const char c = get();
        if(c == '\n')
        {
            original_file_line++;
        }
        if(c == '#')
        {
            if(const char next_char = peek(); next_char == 'i') // Might be 'include'
            {
                const std::size_t start_pos = pos - 1; // get the start pos of the #
                // read the word 'include'
                std::string include_word;
                include_word.reserve(10);
                // read chars until whitespace
                while(has_more() && !isspace(peek()))
                {
                    include_word += get();
                }
                if(include_word == "include")
                {
                    // seek forward until we find the first " or ' or <
                    char start_char = get();
                    while(has_more() && start_char != '\"' && start_char != '<' && start_char != '\'')
                    {
                        start_char = get();
                    }
                    // read chars until we find the end char
                    std::string include_path;
                    include_path.reserve(255);
                    // read until we find the matching end char
                    while(has_more() && peek() != matching_end_char(start_char))
                    {
                        include_path += get();
                    }
                    include_path.shrink_to_fit();
                    // eat the end char
                    get();
                    const std::size_t end_pos = pos; // get the end pos.
                    // trim whitespace from the include path
                    raoe::string::trim(include_path);
                    std::string file_contents = load_file_callback(include_path);

                    // check to see if we have a #pragma once in the file
                    if(file_contents.contains("#pragma once"))
                    {
                        if(included_files.contains(include_path))
                        {
                            // remove the include line, replace it with nothing
                            source.replace(start_pos, end_pos - start_pos, "");
                            pos = start_pos; // move the pos to the start pos + 1
                            continue;
                        }
                    }
                    // Always add the included files even if they dont have a #pragma once, as we use the count
                    // later for file counting
                    included_files.insert(include_path);

                    // remove the #pragma once directive if they exists
                    // find any line that contains #pragma once and remove the entire line
                    std::size_t pragma_pos = file_contents.find("#pragma once");
                    while(pragma_pos != std::string::npos)
                    {
                        // find the start of the line
                        std::size_t line_start = file_contents.rfind('\n', pragma_pos);
                        if(line_start == std::string::npos)
                        {
                            line_start = 0;
                        }
                        else
                        {
                            line_start++; // move to the next char after the \n
                        }
                        // find the end of the line
                        std::size_t line_end = file_contents.find('\n', pragma_pos);
                        if(line_end == std::string::npos)
                        {
                            line_end = file_contents.size();
                        }
                        else
                        {
                            line_end++; // move to the next char after the \n
                        }
                        file_contents.erase(line_start, line_end - line_start);
                        pragma_pos = file_contents.find("#pragma once", line_start);
                    }

                    // add the #line directive
                    {
                        // Find the #version directive and insert after the line after it
                        uint32 line = 0;
                        uint64 version_pos = file_contents.find("#version");
                        if(version_pos != std::string::npos)
                        {
                            // count the number of lines before the #version directive
                            line = std::count(file_contents.begin(),
                                              file_contents.begin() + static_cast<int64>(version_pos), '\n') +
                                   1;
                            version_pos = file_contents.find('\n', version_pos) + 1;
                            if(version_pos == std::string::npos)
                            {
                                file_contents += '\n';
                                version_pos += file_contents.find('\n', version_pos) + 1;
                            }
                        }
                        else
                        {
                            version_pos = 0;
                        }
                        std::string line_directive = std::format("#line {} {}\n", line + 1, included_files.size());
                        // insert the line directive after the #version directive
                        file_contents.insert(version_pos, line_directive);
                        if(!file_contents.ends_with('\n'))
                        {
                            file_contents.push_back('\n');
                        }
                    }
                    // recursively preprocess the included file
                    file_contents = preprocess_r(std::move(file_contents), load_file_callback, included_files,
                                                 injections, included_files.size());

                    std::string line_directive =
                        std::format("#line {} {}", original_file_line + 1, original_file_index);
                    // and replace the include statement with the file contents
                    source.replace(start_pos, end_pos - start_pos, file_contents + line_directive);
                    pos = start_pos + file_contents.size() + line_directive.size();
                }
                if(include_word == "inject")
                {
                    char start_char = get();
                    while(has_more() && start_char != '\"' && start_char != '<' && start_char != '\'')
                    {
                        start_char = get();
                    }
                    // read chars until we find the end char
                    std::string inject_path;
                    inject_path.reserve(255);
                    // read until we find the matching end char
                    while(has_more() && peek() != matching_end_char(start_char))
                    {
                        inject_path += get();
                    }
                    inject_path.shrink_to_fit();
                    // eat the end char
                    get();
                    const std::size_t end_pos = pos; // get the end pos.
                    // trim whitespace from the include path
                    raoe::string::trim(inject_path);

                    // check if we have an injection for this path
                    if(const auto it = injections.find(inject_path); it == injections.end())
                    {
                        // No injection, just remove the line
                        source.replace(start_pos, end_pos - start_pos, "");
                        pos = start_pos; // move the pos back to the start pos, as we removed the entire line
                    }
                    else
                    {
                        // remove the #inject line, replace it with the injection
                        source.replace(start_pos, end_pos - start_pos, it->second);
                        pos = start_pos + it->second.size();
                    }
                }
            }
        }
    }

    return source;
}

std::string raoe::render::shader::glsl::preprocess(std::string source, const file_load_callback_t& load_file_callback,
                                                   const std::unordered_map<std::string, std::string>& injections)
{
    std::unordered_set<std::string> included_files;
    return preprocess_r(std::move(source), load_file_callback, included_files, injections);
}
void raoe::render::shader::glsl::injections_for_shader_type(std::unordered_map<std::string, std::string>& injections,
                                                            const shader_type type)
{
    std::string common_injection;
    for(int i = 0; i < std::to_underlying(shader_type::count); i++)
    {
        constexpr std::array<const char*, std::to_underlying(shader_type::count)> shader_type_defines = {
            "_RAOE_STAGE_VERTEX",
            "_RAOE_STAGE_FRAGMENT",
            "_RAOE_STAGE_GEOMETRY",
            "_RAOE_STAGE_TESSELLATION_CONTROL",
            "_RAOE_STAGE_TESSELLATION_EVALUATION",
            "_RAOE_STAGE_MESH",
            "_RAOE_STAGE_COMPUTE",
        };
        common_injection +=
            std::format("#define {} {}", shader_type_defines[i], i == std::to_underlying(type) ? "1" : "0");

        // Add a newline if this is not the last element
        if(i + 1 < std::to_underlying(shader_type::count))
        {
            common_injection += '\n';
        }
    }
    injections.emplace("_RAOE_COMMON_DEFINES", std::move(common_injection));
}
