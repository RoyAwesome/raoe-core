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

#include "render/render.hpp"

#include "render/buffer.hpp"
#include "render/types.hpp"

namespace raoe::render
{
    namespace shader
    {
        class shader;
    }

    template<typename Range, typename TVertexData>
    concept range_of = std::ranges::range<Range> && std::same_as<std::ranges::range_value_t<Range>, TVertexData>;

    class mesh_element
    {
      public:
        mesh_element() = default;
        template<type_described TVertexData, index_buffer_type TIndexType>
        explicit mesh_element(const std::span<TVertexData> vertex_elements, const std::span<TIndexType> index_elements)
        {
            set_data(vertex_elements, index_elements);
        }
        template<std::ranges::range TVertexRange, std::ranges::range TIndexRange>
            requires type_described<std::ranges::range_value_t<std::remove_cvref_t<TVertexRange>>> &&
                     index_buffer_type<std::ranges::range_value_t<std::remove_cvref_t<TIndexRange>>>
        explicit mesh_element(TVertexRange&& vertex_elements, TIndexRange&& index_elements)
        {
            set_data(std::span(std::ranges::data(vertex_elements), std::ranges::size(vertex_elements)),
                     std::span(std::ranges::data(index_elements), std::ranges::size(index_elements)));
        }

        // Typed set_data with an index.  Takes a span of vertex data and a span of index data and uses that as a mesh
        // element.
        template<type_described TVertexData, index_buffer_type TIndexType>
        mesh_element& set_data(const std::span<TVertexData> vertex_elements, const std::span<TIndexType> index_elements)
        {
            return set_data(std::as_bytes(vertex_elements), renderer_type_of<std::remove_cv_t<TVertexData>>::elements(),
                            vertex_elements.size(), sizeof(TVertexData), std::as_bytes(index_elements),
                            renderer_type_of<std::remove_cv_t<TIndexType>>::elements(), index_elements.size(),
                            sizeof(TIndexType));
        }

        template<std::ranges::range TVertexRange, std::ranges::range TIndexRange>
            requires type_described<std::ranges::range_value_t<std::remove_cvref_t<TVertexRange>>> &&
                     index_buffer_type<std::ranges::range_value_t<std::remove_cvref_t<TIndexRange>>>
        mesh_element& set_data(TVertexRange&& vertex_elements, TIndexRange&& index_elements)
        {
            return set_data(std::span(std::ranges::data(vertex_elements), std::ranges::size(vertex_elements)),
                            std::span(std::ranges::data(index_elements), std::ranges::size(index_elements)));
        }

        template<std::ranges::range TVertexRange>
            requires type_described<std::ranges::range_value_t<TVertexRange>>
        mesh_element& set_data(TVertexRange&& vertex_elements)
        {
            auto vertex_elements_data =
                std::span(std::ranges::data(vertex_elements), std::ranges::size(vertex_elements));
            return set_data(vertex_elements_data);
        }

        // Typed set_data without an index.  Takes a span of vertex data and uses that as a mesh
        // element.
        template<type_described TVertexData>
        mesh_element& set_data(const std::span<TVertexData> vertex_elements)
        {
            return set_data(std::as_bytes(vertex_elements),
                            renderer_type_of<std::remove_cvref_t<TVertexData>>::elements(), vertex_elements.size(),
                            sizeof(TVertexData), {}, {}, 0, 0);
        }

        // Type Erased set_data.
        mesh_element& set_data(std::span<const std::byte> vertex_data,
                               std::span<const type_description> vertex_elements, std::size_t vertex_count,
                               std::size_t vertex_size, std::span<const std::byte> index_data,
                               std::span<const type_description> index_elements, std::size_t index_count,
                               std::size_t index_size);

        mesh_element& generate_buffers();

        [[nodiscard]] bool is_indexed() const { return !m_index_element_type.empty(); }

        [[nodiscard]] std::span<const type_description> vertex_element_type() const { return m_vertex_element_type; }
        [[nodiscard]] std::span<const type_description> index_element_type() const { return m_index_element_type; }

        [[nodiscard]] std::span<const std::byte> vertex_data() const { return m_vertex_data; }
        [[nodiscard]] std::span<const std::byte> index_data() const { return m_index_data; }

        [[nodiscard]] std::size_t vertex_count() const { return m_vertex_count; }
        [[nodiscard]] std::size_t index_count() const { return m_index_count; }

        [[nodiscard]] std::size_t vertex_size() const { return m_vertex_size; }
        [[nodiscard]] std::size_t index_size() const { return m_index_size; }

        [[nodiscard]] const vertex_buffer* get_vertex_buffer() const
        {
            return m_vertex_buffer ? &m_vertex_buffer.value() : nullptr;
        }
        [[nodiscard]] const index_buffer* get_index_buffer() const
        {
            return m_index_buffer ? &m_index_buffer.value() : nullptr;
        }

        [[nodiscard]] bool is_valid() const { return !m_vertex_element_type.empty(); }
        [[nodiscard]] bool is_dirty() const { return m_dirty; }

        void release()
        {
            if(m_vertex_buffer)
            {
                m_vertex_buffer->release();
                m_vertex_buffer.reset();

                m_vertex_element_type = {};
                m_vertex_data = {};
                m_vertex_count = 0;
                m_vertex_size = 0;
            }
            if(m_index_buffer)
            {
                m_index_buffer->release();
                m_index_buffer.reset();

                m_index_element_type = {};
                m_index_data = {};
                m_index_count = 0;
                m_index_size = 0;
            }
            m_dirty = true;
        }

      private:
        std::span<const type_description> m_vertex_element_type;
        std::span<const type_description> m_index_element_type;

        std::vector<std::byte> m_vertex_data;
        std::vector<std::byte> m_index_data;

        std::size_t m_vertex_count = 0;
        std::size_t m_vertex_size = 0;
        std::size_t m_index_count = 0;
        std::size_t m_index_size = 0;

        std::optional<vertex_buffer> m_vertex_buffer;
        std::optional<index_buffer> m_index_buffer;

        bool m_dirty = false;
    };

    struct mesh
    {
        using mesh_part = std::tuple<generic_handle<mesh_element>, generic_handle<shader::material>>;

        mesh() = default;
        mesh(mesh&&) = default;
        mesh& operator=(mesh&&) = default;

        mesh(const mesh&) = default;
        mesh& operator=(const mesh&) = default;

        explicit mesh(std::span<mesh_part> in_elements) noexcept
            : m_elements(in_elements.begin(), in_elements.end())
        {
        }

        explicit mesh(const generic_handle<mesh_element>& in_element,
                      const generic_handle<shader::material>& in_material = {})
            : m_elements({std::make_tuple(in_element, in_material)})
        {
        }

        std::string m_debug_name;
        std::vector<mesh_part> m_elements;
    };

}