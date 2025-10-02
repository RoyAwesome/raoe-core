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

#include <string>

namespace raoe
{
    // A simple URI parser based on RFC 3986
    // Does not do any percent-decoding or other normalization
    // Only supports the following format:
    // scheme:[//[userinfo@]host[:port]]path[?query][#fragment]
    // Examples:
    // http://user:pass@host:8080/path?query#fragment
    // ftp://host/path
    // texture:/path/to/texture.png
    // file:///path/to/file.txt
    struct uri
    {
        explicit uri(const std::string_view in_uri)
            : m_full_string(in_uri)
        {
            parse();
        }

        explicit uri(const char* in_uri)
            : m_full_string(in_uri)
        {
            parse();
        }

        [[nodiscard]] const std::string& str() const { return m_full_string; }
        [[nodiscard]] std::string_view scheme() const { return m_scheme; }
        [[nodiscard]] std::string_view userinfo() const { return m_userinfo; }
        [[nodiscard]] std::string_view host() const { return m_host; }
        [[nodiscard]] uint16 port() const { return m_port; }
        [[nodiscard]] std::string_view path() const { return m_path; }
        [[nodiscard]] std::string_view query() const { return m_query; }
        [[nodiscard]] std::string_view fragment() const { return m_fragment; }

      private:
        void parse()
        {
            // Based on RFC 3986
            // scheme:[//[userinfo@]host[:port]]path[?query][#fragment]
            int32 pos = 0;
            auto peek = [this, &pos](const std::size_t length = 1) -> char {
                if(pos + length < m_full_string.size())
                {
                    return m_full_string[pos];
                }
                return '\0';
            };
            auto get = [this, &pos]() -> char {
                if(pos < m_full_string.size())
                {
                    return m_full_string[pos++];
                }
                return '\0';
            };
            // Reads until the stop_char or the end of the string, returns the position of the first character after the
            // stop_char
            auto peek_until = [this, &pos](const char stop_char) -> int32 {
                int32 offset = pos;
                while(offset < m_full_string.size() && m_full_string[offset] != stop_char)
                {
                    offset++;
                }
                return offset;
            };
            auto has_more = [this, &pos]() -> bool { return pos < m_full_string.size(); };
            int32 scheme_end = pos;
            int32 authority_end = pos;
            int32 path_first_colon = -1;
            while(has_more())
            {
                const char c = get();
                if(c == ':')
                {
                    // if the next two characters are a "//", then we for sure have a scheme.  Otherwise, we might have
                    // a scheme, so lets store some info and check later
                    if(peek(2) == '/' && peek(1) == '/')
                    {
                        m_scheme = std::string_view(m_full_string.begin(), m_full_string.begin() + pos - 1);

                        scheme_end = pos;
                    }
                    if(path_first_colon == -1 && m_scheme.empty())
                    {
                        path_first_colon = pos - 1;
                    }
                }
                if(c == '/')
                {
                    if(peek() == '/')
                    {
                        get(); // Eat the second /
                        // we have an userinfo. read until we find a @ or a /
                        // If we read to the end without finding a @, we have no authority
                        if(const int32 userinfo_end = peek_until('@'); userinfo_end < m_full_string.size() - 1)
                        {
                            m_userinfo =
                                std::string_view(m_full_string.begin() + pos, m_full_string.begin() + userinfo_end);

                            pos = userinfo_end;
                            get(); // Eat the @
                        }
                        // if we have a [, we might have a ipv6
                        if(peek() == '[')
                        {
                            if(const int32 host_end = peek_until(']'); host_end > 0)
                            {
                                m_host =
                                    std::string_view(m_full_string.begin() + pos, m_full_string.begin() + host_end + 1);
                                pos = host_end + 1;
                            }
                        }
                        else
                        {
                            // we have a host. read until we find a : or a /
                            const int32 path_start = peek_until('/');
                            const int32 port_start = peek_until(':');
                            const int32 host_end = std::min(path_start, port_start);

                            m_host = std::string_view(m_full_string.begin() + pos, m_full_string.begin() + host_end);

                            pos = host_end;
                        }
                        // if the next character is a :, we have a port
                        if(peek() == ':')
                        {
                            get(); // Eat the :
                            const int32 port_end = peek_until('/');
                            // read the port
                            std::from_chars(m_full_string.data() + pos, m_full_string.data() + port_end, m_port);
                            pos = port_end;
                        }
                        authority_end = pos;
                    }
                }

                // if we encounter a ? we have a query, and also everything before that is the path
                if(c == '?')
                {
                    // set the path
                    const int32 path_start = std::max(scheme_end, authority_end);
                    m_path = std::string_view(m_full_string.begin() + path_start, m_full_string.begin() + pos - 1);

                    // read until we find a # or the end of the string
                    const int32 query_end = peek_until('#');
                    m_query = std::string_view(m_full_string.begin() + pos, m_full_string.begin() + query_end);
                    pos = query_end;
                    // The rest of the string is the fragment
                    if(query_end <= m_full_string.size() - 1)
                    {
                        m_fragment = std::string_view(m_full_string.begin() + query_end + 1, m_full_string.end());
                        pos = static_cast<int32>(m_full_string.size());
                    }
                }
                if(c == '#')
                {
                    // set the path
                    const int32 path_start = std::max(scheme_end, authority_end);
                    m_path = std::string_view(m_full_string.begin() + path_start, m_full_string.begin() + pos - 1);

                    // The rest of the string is the fragment
                    if(pos <= m_full_string.size() - 1)
                    {
                        m_fragment = std::string_view(m_full_string.begin() + pos, m_full_string.end());
                        pos = static_cast<int32>(m_full_string.size());
                    }
                }
            }
            // If we never set the path, it is everything after the authority or scheme
            if(m_path.empty() && (m_query.empty() || m_fragment.empty()))
            {
                const int32 path_start = std::max(scheme_end, authority_end);
                m_path = std::string_view(m_full_string.begin() + path_start, m_full_string.begin() + pos);
            }

            // if we have no scheme yet, and we have a first colon in the path, the substring of the path before the
            // colon is the scheme
            if(m_scheme.empty() && path_first_colon > 0)
            {
                m_scheme = std::string_view(m_full_string.begin(), m_full_string.begin() + path_first_colon);
                m_path = std::string_view(m_full_string.begin() + path_first_colon + 1, m_full_string.end());
            }
        }
        std::string m_full_string;
        std::string_view m_scheme;
        std::string_view m_userinfo;
        std::string_view m_host;
        uint16 m_port = 0;
        std::string_view m_path;
        std::string_view m_query;
        std::string_view m_fragment;
    };
}

template<>
struct std::hash<raoe::uri>
{
    [[nodiscard]] std::size_t operator()(raoe::uri const& id) const noexcept
    {
        return std::hash<std::string>()(id.str());
    }
};

template<>
struct std::formatter<raoe::uri>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const raoe::uri& id, std::format_context& ctx) const { return format_to(ctx.out(), "{}", id.str()); }
};