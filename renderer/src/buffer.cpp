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

#include "render/buffer.hpp"
#include "render_private.hpp"

#include "glad/glad.h"

namespace raoe::render
{

    buffer::~buffer()
    {
        release();
    }
    void buffer::release()
    {
        if(m_native_buffer != 0)
        {
            glDeleteBuffers(1, &m_native_buffer);
            m_native_buffer = 0;
        }
    }

    void buffer::set_data(const std::span<const std::byte> data, const std::span<const type_description> elements,
                          const std::size_t element_count, const std::size_t element_stride)
    {
        m_elements = elements;
        m_element_count = element_count;
        m_element_stride = element_stride;

        m_bytes = data.size();

        if(m_native_buffer == 0)
        {
            glCreateBuffers(1, &m_native_buffer);
        }

        glNamedBufferData(m_native_buffer, static_cast<GLsizeiptr>(data.size_bytes()), data.data(),
                          is_dynamic() ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }
}