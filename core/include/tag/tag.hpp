/*
Copyright 2022-2024 Roy Awesome's Open Engine (RAOE)

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

#include <concepts>
#include <format>
#include <string>
#include <string_view>

#include "ctre.hpp"

namespace raoe
{
    class tag
    {
        friend struct std::hash<raoe::tag>;

      public:
        constexpr tag()
            : m_tag()
        {
        }

        constexpr tag(const char* in_tag)
            : tag(std::string_view(in_tag))
        {
        }

        constexpr explicit tag(std::string_view in_tag)
            : m_tag(in_tag)
            , m_colon_pos(m_tag.find_first_of(':'))
            , m_hash_pos(m_tag.find_first_of('#'))
        {
            const bool has_colon_no_prefix = !m_tag.starts_with(':') && prefix().length() == 0;
            const bool has_no_colon = m_tag.find_first_of(':') == std::string::npos;
            if(has_no_colon || has_colon_no_prefix)
            {
                m_tag = std::format("{}:{}", default_prefix(), m_tag);
                m_colon_pos = m_tag.find_first_of(':');
                m_hash_pos = m_tag.find_first_of('#');
            }

            // Validate the prefix
            {
                auto prefix_matcher = ctre::match<"[a-zA-Z0-9_\\-\\.#]*$">;
                if(!prefix_matcher(m_tag.begin(), m_tag.begin() + m_colon_pos))
                {
                    m_tag = "";
                    m_colon_pos = std::string::npos;
                    m_hash_pos = std::string::npos;
                    return;
                }
            }

            // Validate the identifier
            {
                auto ident_matcher = ctre::match<"[a-zA-Z0-9_\\-\\.\\/]*$">;
                if(!ident_matcher(m_tag.begin() + m_colon_pos + 1, m_tag.end()))
                {
                    m_tag = "";
                    m_colon_pos = std::string::npos;
                    m_hash_pos = std::string::npos;
                    return;
                }
            }
            // Check if we're just a colon.  That means both the prefix and the ident failed to parse, so just zero us
            // out
            if(m_tag == ":")
            {
                m_tag = "";
                m_colon_pos = std::string::npos;
                m_hash_pos = std::string::npos;
                return;
            }
        }
        constexpr explicit tag(std::string_view prefix, std::string_view path)
            : tag(std::format("{}:{}", prefix, path))
        {
        }

        tag(const tag&) = default;
        tag& operator=(const tag&) = default;

        constexpr tag(tag&& other) noexcept
            : m_tag(std::move(std::exchange(other.m_tag, "")))
            , m_colon_pos(std::exchange(other.m_colon_pos, std::string::npos))
            , m_hash_pos(std::exchange(other.m_hash_pos, std::string::npos))
        {
        }
        constexpr tag& operator=(tag&& other) noexcept
        {
            m_tag = std::move(std::exchange(other.m_tag, ""));
            m_colon_pos = std::exchange(other.m_colon_pos, std::string::npos);
            m_hash_pos = std::exchange(other.m_hash_pos, std::string::npos);
            return *this;
        }

        [[nodiscard]] constexpr std::string_view prefix() const noexcept
        {
            auto raw = raw_prefix();
            if(m_hash_pos == std::string::npos)
            {
                return raw;
            }

            return raw.substr(0, m_hash_pos);
        }

        [[nodiscard]] constexpr std::string_view type() const noexcept
        {
            using namespace std::literals::string_view_literals;
            auto raw = raw_prefix();
            if(m_hash_pos == std::string::npos)
            {
                return ""sv;
            }
            return raw.substr(m_hash_pos + 1);
        }

        [[nodiscard]] constexpr std::string_view identifier() const noexcept
        {
            using namespace std::literals::string_view_literals;
            if(m_colon_pos == std::string::npos || m_colon_pos == m_tag.length())
            {
                return ""sv;
            }
            return std::string_view(m_tag).substr(m_colon_pos + 1);
        }

        constexpr auto operator<=>(const tag& rhs) const noexcept = default;

        constexpr operator std::string_view() const noexcept { return m_tag; }
        constexpr operator const std::string&() const noexcept { return m_tag; }
        constexpr operator const char*() const noexcept { return m_tag.c_str(); }
        constexpr operator bool() const noexcept { return prefix().length() != 0 && identifier().length() != 0; }

        constexpr bool matches(const tag& other)
        {
            // if either type is empty, we don't care about the type
            // otherwise check if both types are equal.
            if(type() != "" && other.type() != "" && type() != other.type())
            {
                return false;
            }
            return prefix() == other.prefix() && identifier() == other.identifier();
        }

        [[nodiscard]] constexpr static std::string_view default_prefix() noexcept
        {
            using namespace std::literals::string_view_literals;
            return "raoe"sv;
        }

        [[nodiscard]] constexpr const char* c_str() const noexcept { return m_tag.c_str(); }

      private:
        constexpr std::string_view raw_prefix() const noexcept
        {
            using namespace std::literals::string_view_literals;
            if(m_colon_pos == std::string::npos || m_colon_pos == m_tag.length() - 1)
            {
                return ""sv;
            }
            return std::string_view(m_tag).substr(0, m_colon_pos);
        }

        std::string m_tag;
        std::string_view::size_type m_colon_pos;
        std::string_view::size_type m_hash_pos;
    };

    namespace assets
    {
        using tag = raoe::tag;
    }
}

template <>
struct std::hash<raoe::tag>
{
    std::size_t operator()(const raoe::tag& tag) const noexcept { return std::hash<std::string> {}(tag.m_tag); }
};

template <>
struct std::formatter<raoe::tag>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) const
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const raoe::tag& tag, FormatContext& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(tag));
    }
};

#if RAOE_CORE_USE_SPDLOG

#include "spdlog/spdlog.h"

template <>
struct fmt::formatter<raoe::tag>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) const
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const raoe::tag& tag, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "{}", std::string_view(tag));
    }
};

#endif
