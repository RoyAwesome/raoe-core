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

#include <functional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "core/stream.hpp"

#include "texture.hpp"
#include "types.hpp"

namespace raoe::render::shader
{
    enum class build_flags
    {
        none = 0,
        vertex = 1 << 0,
        fragment = 1 << 1,
        geometry = 1 << 2,
        tesselation_control = 1 << 3,
        tesselation_evaluation = 1 << 4,

        mesh = 1 << 5,
        compute = 1 << 6,

        any_draw = vertex | fragment | geometry | tesselation_control | tesselation_evaluation | mesh,
        classic_pipeline = vertex | fragment | geometry | tesselation_control | tesselation_evaluation,
    };
}

RAOE_CORE_FLAGS_ENUM(raoe::render::shader::build_flags);

namespace raoe::render::shader
{
    namespace glsl
    {
        using file_load_callback_t = std::function<std::string(const std::string&)>;
        std::string preprocess(std::string source, const file_load_callback_t& load_file_callback);
    }

    enum class shader_type
    {
        vertex,
        fragment,

        geometry,
        tesselation_control,
        tesselation_evaluation,

        mesh,

        compute,

        count,
    };

    inline build_flags from_type(const shader_type shader_type)
    {
        switch(shader_type)
        {
            case shader_type::vertex: return build_flags::vertex;
            case shader_type::fragment: return build_flags::fragment;
            case shader_type::geometry: return build_flags::geometry;
            case shader_type::tesselation_control: return build_flags::tesselation_control;
            case shader_type::tesselation_evaluation: return build_flags::tesselation_evaluation;
            case shader_type::mesh: return build_flags::mesh;
            case shader_type::compute: return build_flags::compute;
            default: panic("Invalid shader type", underlying(shader_type));
        }
    }

    enum class shader_lang
    {
        none = 0,
        glsl,
        spirv,

        count,
    };

    template<shader_lang TLang>
    struct basic_builder;

    template<shader_lang TLang, shader_type TType>
    struct source
    {
        static_assert(TLang != shader_lang::none && TLang != shader_lang::count,
                      "Must have a valid shader lang (not none or count)");
        static_assert(TType != shader_type::count, "Must have a valid shader type (cannot be count)");

        friend struct basic_builder<TLang>;

        source() = default;
        source(const source&) = default;
        source& operator=(const source&) = default;

        source(source&&) = default;
        source& operator=(source&&) = default;

        [[nodiscard]] static shader_lang lang() { return TLang; }
        [[nodiscard]] static shader_type type() { return TType; }
        [[nodiscard]] std::span<const std::byte> source_bytes() const { return m_shader_source; }
        [[nodiscard]] bool valid() const { return !m_shader_source.empty(); }

        void preprocess(const glsl::file_load_callback_t& load_file_callback)
            requires(TLang == shader_lang::glsl)
        {
            const std::string src_str(reinterpret_cast<const char*>(m_shader_source.data()), m_shader_source.size());
            const std::string preprocessed = glsl::preprocess(src_str, load_file_callback);
            m_shader_source.clear();
            stream::read_stream_into(std::back_inserter(m_shader_source), preprocessed);
        }

      private:
        std::vector<std::byte> m_shader_source;
    };

    struct uniform
    {
        friend class shader;

        template<typename T>
            requires(is_valid_renderer_type(shader_uniform_type_v<T>) && !is_texture_type(shader_uniform_type_v<T>))
        uniform& operator=(const T& value)
        {
            check_if(m_type == renderer_type::none, "Uniform is not initialized.  Cannot assign value to it");
            check_if(m_type != shader_uniform_type_v<T>, "Uniform type mismatch.  Expecting {}, got {}", m_type,
                     shader_uniform_type_v<T>);

            set_uniform(raoe::as_bytes(value));

            return *this;
        }

        uniform& operator=(const texture& value)
        {
            check_if(m_type == renderer_type::none, "Uniform is not initialized.  Cannot assign value to it");
            set_uniform(value);

            return *this;
        }

        [[nodiscard]] renderer_type type() const { return m_type; }
        [[nodiscard]] uint8 texture_unit() const { return m_texture_unit; }
        [[nodiscard]] int32 native_id() const { return m_native_id; }

      private:
        explicit uniform(const int32 id, const renderer_type type, const uint8 texture_unit = 0)
            : m_type(type)
            , m_texture_unit(texture_unit)
            , m_native_id(id)
        {
        }
        void set_uniform(std::span<const std::byte> data, int32 element_count = 1) const;
        void set_uniform(const texture& texture) const;

        renderer_type m_type = renderer_type::none;
        uint8 m_texture_unit = 0;
        int32 m_native_id;
    };

    struct input
    {
        friend class shader;

        [[nodiscard]] std::string_view name() const { return m_name; }
        [[nodiscard]] int32 location() const { return m_location; }
        [[nodiscard]] renderer_type type() const { return m_type; }

      private:
        explicit input(const std::string_view name, const int32 location, const renderer_type type)
            : m_name(name)
            , m_location(location)
            , m_type(type)
        {
        }

        std::string m_name;
        int32 m_location = -1;
        renderer_type m_type = renderer_type::none;
    };

    class shader
    {
      public:
        friend class material;
        shader() = default;

        shader(const shader&) = default;
        shader& operator=(const shader&) = default;

        shader(shader&&) = default;
        shader& operator=(shader&&) = default;

        template<shader_lang TLang>
        friend struct basic_builder;

        [[nodiscard]] uniform& operator[](std::string_view name);
        [[nodiscard]] uniform& operator[](uint32 location);

        [[nodiscard]] uint32 native_id() const { return m_native_id; }
        [[nodiscard]] bool ready() const { return m_native_id != 0; }

        [[nodiscard]] std::ranges::range auto uniforms() { return m_uniforms | std::views::values; }
        [[nodiscard]] std::ranges::random_access_range auto inputs() const { return m_inputs; }

        // Binds the shader for use.
        void use() const;

      private:
        explicit shader(const uint32 id, std::string debug_name)
            : m_native_id(id)
            , m_debug_name(std::move(debug_name))
        {
            introspect();
        }

        static std::shared_ptr<shader> make_shared(uint32 id, std::string debug_name);

        void introspect();
        uint32 m_native_id = 0;
        std::unordered_map<uint32, uniform> m_uniforms = {};
        std::unordered_map<std::string, uint32> m_uniform_names = {};
        std::vector<input> m_inputs = {};
        std::string m_debug_name = {};
    };

    namespace _internal
    {
        uint32 compile_source_glsl(shader_type shader_type, std::span<const std::byte> source);
        uint32 create_program(std::span<uint32, underlying(shader_type::count)> modules);
    }

    template<shader_lang TLang>
    struct basic_builder
    {
        static_assert(TLang != shader_lang::none && TLang != shader_lang::count,
                      "Must have a valid shader lang (not none or count)");
        explicit basic_builder(const std::string_view name)
            : m_debug_name(name)
        {
        }

        [[nodiscard]] basic_builder& with_file_loader(glsl::file_load_callback_t loader)
        {
            m_load_file_callback = std::move(loader);
            return *this;
        }

        [[nodiscard]] basic_builder& add_module(source<TLang, shader_type::vertex> vertex_shader,
                                                source<TLang, shader_type::fragment> fragment_shader)
        {
            // check if we can build a fragment/vertex shader
            check_can_attach_module(build_flags::vertex);
            check_can_attach_module(build_flags::fragment);
            std::get<underlying(shader_type::vertex)>(m_sources) = std::move(vertex_shader);
            std::get<underlying(shader_type::fragment)>(m_sources) = std::move(fragment_shader);
            m_build_flags |= build_flags::vertex;
            m_build_flags |= build_flags::fragment;

            return *this;
        }

        [[nodiscard]] basic_builder& add_module(source<TLang, shader_type::geometry> geometry_shader)
        {
            // check if we can build a geometry shader
            check_can_attach_module(build_flags::geometry);
            std::get<underlying(shader_type::geometry)>(m_sources) = std::move(geometry_shader);
            m_build_flags |= build_flags::geometry;

            return *this;
        }

        [[nodiscard]] basic_builder& add_module(
            source<TLang, shader_type::tesselation_control> tesselation_control_shader,
            source<TLang, shader_type::tesselation_evaluation> tesselation_evaluation_shader)
        {
            // check if we can build a tesselation shader
            check_can_attach_module(build_flags::tesselation_control);
            check_can_attach_module(build_flags::tesselation_evaluation);
            std::get<underlying(shader_type::tesselation_control)>(m_sources) = std::move(tesselation_control_shader);
            std::get<underlying(shader_type::tesselation_evaluation)>(m_sources) =
                std::move(tesselation_evaluation_shader);
            m_build_flags |= build_flags::tesselation_control;
            m_build_flags |= build_flags::tesselation_evaluation;

            return *this;
        }

        [[nodiscard]] basic_builder& add_module(source<TLang, shader_type::mesh> mesh_shader,
                                                source<TLang, shader_type::fragment> fragment_shader)
        {
            // check if we can build a mesh shader
            check_can_attach_module(build_flags::mesh);
            check_can_attach_module(build_flags::fragment);
            std::get<underlying(shader_type::mesh)>(m_sources) = std::move(mesh_shader);
            std::get<underlying(shader_type::fragment)>(m_sources) = std::move(fragment_shader);
            m_build_flags |= build_flags::mesh;
            m_build_flags |= build_flags::fragment;

            return *this;
        }

        template<shader_type TType>
        [[nodiscard]] basic_builder& add_module(std::istream& stream)
        {
            // check if we can build this shader type
            check_can_attach_module(from_type(TType));
            stream::read_stream_into(std::back_inserter(std::get<underlying(TType)>(m_sources).m_shader_source),
                                     stream);
            if constexpr(TLang == shader_lang::glsl)
            {
                std::get<underlying(TType)>(m_sources).preprocess(m_load_file_callback);
            }

            m_build_flags |= from_type(TType);

            return *this;
        }
        template<shader_type TType>
        [[nodiscard]] basic_builder& add_module(std::string_view source_text)
        {
            // check if we can build this shader type
            check_can_attach_module(from_type(TType));

            stream::read_stream_into(std::back_inserter(std::get<underlying(TType)>(m_sources).m_shader_source),
                                     source_text);
            if constexpr(TLang == shader_lang::glsl)
            {
                std::get<underlying(TType)>(m_sources).preprocess(m_load_file_callback);
            }

            m_build_flags |= from_type(TType);

            return *this;
        }

        [[nodiscard]] basic_builder& add_module(source<TLang, shader_type::compute> compute_shader)
        {
            // check if we can build a compute shader
            check_can_attach_module(build_flags::compute);
            std::get<underlying(shader_type::compute)>(m_sources) = std::move(compute_shader);
            m_build_flags |= build_flags::compute;

            return *this;
        }

        [[nodiscard]] std::shared_ptr<shader> build_sync() const
        {
            check_can_build();

            std::array<uint32, underlying(shader_type::count)> shader_ids {};

            if constexpr(TLang == shader_lang::glsl)
            {
                std::apply(
                    [&shader_ids](auto&&... args) {
                        ((shader_ids[underlying(args.type())] =
                              _internal::compile_source_glsl(args.type(), args.source_bytes())),
                         ...);
                    },
                    m_sources);
            }
            else if constexpr(TLang == shader_lang::spirv)
            {
                static_assert(false, "spirv not implemented yet");
            }

            return shader::make_shared(_internal::create_program(shader_ids), m_debug_name);
        }

      private:
        void check_can_build() const
        {
            if(m_build_flags == build_flags::compute)
            {
                return;
            }

            // If we have a fragment shader, we must have a vertex shader or a mesh shader
            if(has_any_flags(m_build_flags, build_flags::fragment) &&
               !has_any_flags(m_build_flags, build_flags::vertex | build_flags::mesh))
            {
                panic("Cannot build a fragment shader without a vertex or mesh shader");
            }
            // if we have a vertex or mesh shader, we must have a fragment shader
            if(has_any_flags(m_build_flags, build_flags::vertex | build_flags::mesh) &&
               !has_any_flags(m_build_flags, build_flags::fragment))
            {
                panic("Cannot build a vertex or mesh shader without a fragment shader");
            }

            // If we have any tesselation shader, we must both types of tesselation shaders and a vertex shader
            if(has_any_flags(m_build_flags, build_flags::tesselation_control | build_flags::tesselation_evaluation) &&
               !(has_any_flags(m_build_flags, build_flags::tesselation_control) &&
                 has_any_flags(m_build_flags, build_flags::tesselation_evaluation) &&
                 has_any_flags(m_build_flags, build_flags::vertex)))
            {
                panic("Cannot build a tesselation shader without both tesselation shaders and a vertex shader");
            }

            // If we have a geometry shader, we must have a vertex shader
            if(has_any_flags(m_build_flags, build_flags::geometry) &&
               !has_any_flags(m_build_flags, build_flags::vertex))
            {
                panic("Cannot build a geometry shader without a vertex shader");
            }
        }

        void check_can_attach_module(const build_flags attempted_operation) const
        {
            // If we are nothing, we accept any attempt
            if(m_build_flags == build_flags::none)
            {
                return;
            }

            if(has_any_flags(m_build_flags, attempted_operation))
            {
                panic("Cannot attach that shader, it's already attached");
            }

            // Make sure we don't have compute attaching to any drawing shader
            if(has_any_flags(m_build_flags, build_flags::any_draw) &&
               has_any_flags(attempted_operation, build_flags::compute))
            {
                panic("Cannot attach a compute shader to a shader that already has a drawing shader");
            }
            // and make sure we dont attach a drawing shader to a compute shader
            if(has_any_flags(m_build_flags, build_flags::compute) &&
               has_any_flags(attempted_operation, build_flags::any_draw))
            {
                panic("Cannot attach a compute shader to a drawing shader");
            }

            // Ensure that we don't have a mesh shader attached to a classic pipeline
            if(has_any_flags(m_build_flags, build_flags::classic_pipeline) &&
               has_any_flags(attempted_operation, build_flags::mesh))
            {
                panic("Cannot attach a mesh shader to a classic pipeline");
            }
            // and ensure that we don't have a classic pipeline shader attached to a mesh shader
            if(has_any_flags(m_build_flags, build_flags::mesh) &&
               has_any_flags(attempted_operation, build_flags::classic_pipeline))
            {
                panic("Cannot attach a classic pipeline shader to a mesh shader");
            }
        }

        using shader_source_container =
            std::tuple<source<TLang, shader_type::vertex>, source<TLang, shader_type::fragment>,
                       source<TLang, shader_type::geometry>, source<TLang, shader_type::tesselation_control>,
                       source<TLang, shader_type::tesselation_evaluation>, source<TLang, shader_type::mesh>,
                       source<TLang, shader_type::compute>>;
        shader_source_container m_sources;

        build_flags m_build_flags = build_flags::none;
        std::string m_debug_name;
        glsl::file_load_callback_t m_load_file_callback;
    };

    using glsl_builder = basic_builder<shader_lang::glsl>;
    using spirv_builder = basic_builder<shader_lang::spirv>;

    class material
    {
      public:
        struct setter
        {
          private:
            material& m_this;
            uint32 m_location = 0;

          public:
            explicit setter(material& m_this, const uint32 location = 0)
                : m_this(m_this)
                , m_location(location)
            {
                check_if(location > 0, "Cannot set uniform for material without a valid location");
            }
            template<typename T>
                requires(is_valid_renderer_type(shader_uniform_type_v<T>) && !is_texture_type(shader_uniform_type_v<T>))
            setter& operator=(T&& value)
            {
                m_this.set_uniform(value);
                return *this;
            }
        };
        friend struct setter;

        explicit material(std::shared_ptr<shader> shader)
            : m_shader(std::move(shader))
        {
            check_if(m_shader != nullptr, "Material shader cannot be null");
        }

        setter operator[](const std::string_view name) { return setter {*this, get_location_for_name(name)}; }
        setter operator[](const uint32 location) { return setter {*this, location}; }

      private:
        uint32 get_location_for_name(const std::string_view name) const
        {
            const auto it = m_shader->m_uniform_names.find(std::string(name));
            if(it == m_shader->m_uniform_names.end())
            {
                return 0;
            }
            return it->second;
        }

        template<typename T>
            requires(is_valid_renderer_type(shader_uniform_type_v<T>) && !is_texture_type(shader_uniform_type_v<T>))
        void set_uniform(const int32 location, T&& value)
        {
            shader_uniform_variant variant = std::forward<T>(value);
            m_uniforms.insert_or_assign(location, variant);
        }
        std::shared_ptr<shader> m_shader;
        using shader_uniform_variant =
            std::variant<std::monostate, std::shared_ptr<texture>, glm::vec2, glm::vec3, glm::vec4, glm::u8vec4, uint8,
                         int8, uint16, int16, uint32, int32, float, double>;
        std::unordered_map<uint32, shader_uniform_variant> m_uniforms;
    };

}

RAOE_CORE_DECLARE_FORMATTER(
    raoe::render::shader::shader_type, switch(value) {
        case raoe::render::shader::shader_type::vertex: return format_to(ctx.out(), "shader::vertex");
        case raoe::render::shader::shader_type::fragment: return format_to(ctx.out(), "shader::fragment");
        case raoe::render::shader::shader_type::geometry: return format_to(ctx.out(), "shader::geometry");
        case raoe::render::shader::shader_type::tesselation_control:
            return format_to(ctx.out(), "shader::tesselation_control");
        case raoe::render::shader::shader_type::tesselation_evaluation:
            return format_to(ctx.out(), "shader::tesselation_evaluation");
        case raoe::render::shader::shader_type::mesh: return format_to(ctx.out(), "shader::mesh");
        case raoe::render::shader::shader_type::compute: return format_to(ctx.out(), "shader::compute");
        default: return format_to(ctx.out(), "shader::unknown");
    })
RAOE_CORE_DECLARE_FORMATTER(
    raoe::render::shader::shader_lang, switch(value) {
        case raoe::render::shader::shader_lang::glsl: return format_to(ctx.out(), "lang::glsl");
        case raoe::render::shader::shader_lang::spirv: return format_to(ctx.out(), "lang::spirv");
        default: return format_to(ctx.out(), "lang::unknown");
    })
