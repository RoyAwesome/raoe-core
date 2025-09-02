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

    enum class buffer_type
    {
        vertex,
        index,
        uniform,
    };

    struct buffer
    {
        explicit buffer()
            : buffer(false)
        {
        }

        explicit buffer(const bool in_dynamic)
            : m_native_buffer(0)
            , m_dynamic(in_dynamic)
            , m_bytes(0)
            , m_element_count(0)
            , m_element_stride(0)
        {
        }

        ~buffer();

        buffer(const buffer&) = delete;
        buffer& operator=(const buffer&) = delete;

        [[nodiscard]] uint32 native_buffer() const { return m_native_buffer; }
        [[nodiscard]] bool is_valid() const { return m_native_buffer != 0; }
        [[nodiscard]] bool is_dynamic() const { return m_dynamic; }
        [[nodiscard]] std::size_t bytes() const { return m_bytes; }
        [[nodiscard]] std::size_t element_count() const { return m_element_count; }
        [[nodiscard]] std::size_t element_stride() const { return m_element_stride; }
        [[nodiscard]] std::span<const type_description> elements() const { return m_elements; }

        void release();

      protected:
        buffer(buffer&& other) noexcept
            : m_elements(std::exchange(other.m_elements, {}))
            , m_native_buffer(std::exchange(other.m_native_buffer, {}))
            , m_dynamic(std::exchange(other.m_dynamic, {}))
            , m_bytes(std::exchange(other.m_bytes, {}))
            , m_element_count(std::exchange(other.m_element_count, {}))
            , m_element_stride(std::exchange(other.m_element_stride, {}))
        {
        }

        buffer& operator=(buffer&& other) noexcept
        {
            m_elements = std::exchange(other.m_elements, {});
            m_native_buffer = std::exchange(other.m_native_buffer, {});
            m_dynamic = std::exchange(other.m_dynamic, {});
            m_bytes = std::exchange(other.m_bytes, {});
            m_element_count = std::exchange(other.m_element_count, {});
            m_element_stride = std::exchange(other.m_element_stride, {});
            return *this;
        }

        // Type Erased set_data function.  This requires a span of bytes and a span of type descriptions
        // as well as the element count and stride (size of the element type)
        void set_data(std::span<const std::byte> data, std::span<const type_description> elements,
                      std::size_t element_count, std::size_t element_stride);

        std::span<const type_description> m_elements;
        uint32 m_native_buffer;
        bool m_dynamic;
        std::size_t m_bytes;
        std::size_t m_element_count;
        std::size_t m_element_stride;
    };

    template<buffer_type TBufferType>
    struct typed_buffer : buffer
    {
        explicit typed_buffer() = default;

        explicit typed_buffer(bool in_dynamic)
            : typed_buffer(in_dynamic)
        {
        }

        typed_buffer(typed_buffer&& other) noexcept
            : buffer(std::forward<buffer>(other))
        {
        }

        typed_buffer& operator=(typed_buffer&& other) noexcept
        {
            buffer::operator=(std::forward<buffer>(other));
            return *this;
        }

        // typed set_data function.  sets the data for the underlying opengl buffer
        // this will acquire the type description for the type T and use that to set the data
        template<type_described T>
        void set_data(const std::span<T> data)
        {
            if constexpr(TBufferType == buffer_type::index)
            {
                static_assert(std::is_same_v<T, uint32> || std::is_same_v<T, uint16> || std::is_same_v<T, uint8>,
                              "Index buffer must be uint32, uint16, or uint8");
            }

            buffer::set_data(std::as_bytes(data), renderer_type_of<T>::elements(), data.size(), sizeof(T));
        }

        template<type_described T>
        void set_data(const T& data)
        {
            set_data(std::span<const T>(&data, 1));
        }

        void set_data(const std::span<const std::byte> data, const std::span<const type_description> elements,
                      const std::size_t element_count, std::size_t element_stride)
        {
            if constexpr(TBufferType == buffer_type::index)
            {
                if(!(element_stride == 8 || element_stride == 16 || element_stride != 32))
                {
                    panic("Type Erased set_data index buffer called with an invalid element size {} (must be 8, "
                          "16, or 32)",
                          element_stride);
                }
            }
            buffer::set_data(data, elements, element_count, element_stride);
        }
    };

    using vertex_buffer = typed_buffer<buffer_type::vertex>;
    using index_buffer = typed_buffer<buffer_type::index>;
    using uniform_buffer = typed_buffer<buffer_type::uniform>;

    // Type erased buffer type that can't be moved into a typed buffer object
    struct any_buffer : buffer
    {
        explicit any_buffer() = default;

        explicit any_buffer(const bool in_dynamic)
            : buffer(in_dynamic)
        {
        }

        any_buffer(any_buffer&& other) noexcept
            : buffer(std::forward<buffer>(other))
        {
        }

        any_buffer& operator=(any_buffer&& other) noexcept
        {
            buffer::operator=(std::forward<buffer>(other));
            return *this;
        }

        // Type Erased set_data function.  This requires a span of bytes and a span of type descriptions
        // as well as the element count and stride (size of the element type)
        void set_data(const std::span<const std::byte> data, const std::span<const type_description> elements,
                      const std::size_t element_count, const std::size_t element_stride)
        {
            buffer::set_data(data, elements, element_count, element_stride);
        }
    };

}