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

#include "render/mesh.hpp"
#include "render_private.hpp"

#include "glad/glad.h"

namespace raoe::render
{
    mesh_element& mesh_element::set_data(const std::span<const std::byte> vertex_data,
                                         const std::span<const type_description> vertex_elements,
                                         const std::size_t vertex_count, const std::size_t vertex_size,
                                         const std::span<const std::byte> index_data,
                                         const std::span<const type_description> index_elements,
                                         const std::size_t index_count, const std::size_t index_size)
    {
        check_if(!vertex_elements.empty(), "Vertex Element Type is empty");

        m_vertex_element_type = vertex_elements;
        m_index_element_type = index_elements;

        m_vertex_count = vertex_count;
        m_index_count = index_count;

        m_vertex_size = vertex_size;
        m_index_size = index_size;

        m_vertex_data = std::vector<std::byte>(vertex_data.begin(), vertex_data.end());
        m_index_data = std::vector<std::byte>(index_data.begin(), index_data.end());

        m_dirty = true;
        return *this;
    }

    mesh_element& mesh_element::generate_buffers()
    {
        if(!m_dirty)
        {
            return *this;
        }

        check_if(is_valid(), "Mesh Element is not valid");

        if(!m_vertex_buffer)
        {
            m_vertex_buffer.emplace();
        }
        m_vertex_buffer->set_data(m_vertex_data, m_vertex_element_type, m_vertex_count, m_vertex_size);

        if(is_indexed())
        {
            if(!m_index_buffer)
            {
                m_index_buffer.emplace();
            }
            m_index_buffer->set_data(m_index_data, m_index_element_type, m_index_count, m_index_size);
        }

        m_dirty = false;
        return *this;
    }
}
