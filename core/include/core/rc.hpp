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
#include "core/core.hpp"

namespace raoe
{
    enum class rc_strength
    {
        strong,
        weak,
    };

    namespace _internal
    {
        struct ref
        {
            int32 strong_count = 0;
            int32 weak_count = 0;
        };
    }

    template<rc_strength strength>
    struct rc
    {
        template<rc_strength other_strength>
        friend struct rc;

        struct rc_init_refcount_tag
        {
        };

        rc()
            : m_ref(nullptr)
        {
        }

        explicit rc(rc_init_refcount_tag)
            requires(strength == rc_strength::strong)
            : m_ref(new _internal::ref {.strong_count = 1, .weak_count = 0})
        {
        }

        explicit rc(const rc<rc_strength::strong>& other)
            requires(strength == rc_strength::strong)
            : m_ref(other.m_ref)
        {
            if(m_ref)
            {
                ++m_ref->strong_count;
            }
        }

        template<rc_strength other_strength>
        explicit rc(const rc<other_strength>& other)
            requires(strength == rc_strength::weak)
            : m_ref(other.m_ref)
        {
            if(m_ref)
            {
                ++m_ref->weak_count;
            }
        }

        rc(const rc&& other) noexcept
            : m_ref(std::exchange(other.m_ref, nullptr))
        {
        }

        rc& operator=(const rc& other)
        {
            // If we assign to self, don't increment refcounts
            if(m_ref == other.m_ref)
            {
                return *this;
            }
            m_ref = other.m_ref;
            if(m_ref)
            {
                if constexpr(strength == rc_strength::strong)
                {
                    ++m_ref->strong_count;
                }
                if constexpr(strength == rc_strength::weak)
                {
                    ++m_ref->weak_count;
                }
            }
            return *this;
        }

        rc& operator=(rc&& other) noexcept
        {
            m_ref = std::exchange(other.m_ref, nullptr);
            return *this;
        }

        ~rc()
        {
            if(m_ref)
            {
                if constexpr(strength == rc_strength::strong)
                {
                    --m_ref->strong_count;
                }
                if constexpr(strength == rc_strength::weak)
                {
                    --m_ref->weak_count;
                }
                if(m_ref->strong_count <= 0 && m_ref->weak_count <= 0)
                {
                    delete m_ref;
                }
            }
        }

      protected:
        [[nodiscard]] bool valid_ref() const { return m_ref != nullptr; }
        [[nodiscard]] bool has_strong_ref() const { return m_ref ? m_ref->strong_count > 0 : false; }

        // Upgrading weak to strong.
        // This is protected, because care needs to be had when doing this.
        explicit rc(const rc<rc_strength::weak>& other)
            requires(strength == rc_strength::strong)
            : m_ref(other.m_ref)
        {
            if(m_ref)
            {
                ++m_ref->strong_count;
            }
        }

      private:
        _internal::ref* m_ref = nullptr;
    };
}