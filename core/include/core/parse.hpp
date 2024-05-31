/*
Copyright 2022 Roy Awesome's Open Engine (RAOE)

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

#include "from_string.hpp"
#include "types.hpp"
#include <cctype>
#include <functional>
#include <iterator>
#include <string>
#include <tuple>

namespace raoe::core::parse
{
    inline namespace _
    {
        // Escaped checks if there is a \ before the cursor character (used when escaping sequences)
        inline bool is_escaped(std::string_view sv, int32 cursor)
        {
            const bool out_of_range = (cursor - 1) < 0;
            return !out_of_range && sv[cursor - 1] == '\\';
        }

        // Checks for if the cursor is on a whitespace char
        inline bool is_whitespace(std::string_view sv, int32 cursor)
        {
            return !!(std::isblank(sv.at(cursor)));
        }

        inline bool is_quote(std::string_view sv, int32 cursor)
        {
            return sv[cursor] == '\"' && !is_escaped(sv, cursor);
        }
        // Parse ", but if we see a \" skip that.
        inline bool is_control(std::string_view sv, int32 cursor)
        {
            return is_whitespace(sv, cursor) || is_quote(sv, cursor);
        }

        inline void parse_split(std::string_view from, std::output_iterator<std::string_view> auto out_itr)
        {
            using ScanFunc_T = std::function<bool(std::string_view, int32)>;

            int32 cursor = 0;
            while (cursor < from.length())
            {
                // Walk the cursor up to the first non-control character
                while (cursor < from.length() && is_control(from, cursor))
                {
                    cursor++;
                }

                // if we are at the end of the string, we're done
                if (cursor >= from.length())
                {
                    return;
                }

                int32 start = cursor;
                // Determine if the character right before the cursor is what we are looking for
                ScanFunc_T search_query = is_whitespace;
                if (((cursor - 1) >= 0) && is_quote(from, cursor - 1))
                {
                    search_query = is_quote;
                }

                // Walk the cursor forward until we match what we are looking for (or we hit the end of the string)
                while (cursor < from.length() && !search_query(from, cursor))
                {
                    cursor++;
                }

                // substring from start to cursor, add it to the out iterator
                *out_itr++ = from.substr(start, cursor - start);
            }
        }

        inline std::vector<std::string_view> parse_split(std::string_view from)
        {
            std::vector<std::string_view> elems;
            parse_split(from, std::back_inserter(elems));
            return elems;
        }

        template <typename Tuple, std::size_t... I>
        std::array<bool, sizeof...(I)> parse_string_as_tuple(std::string_view command_line, Tuple& value,
                                                             std::index_sequence<I...>)
        {
            std::vector<std::string_view> elems = parse_split(command_line);
            // Pad out the elements in the vector if there are fewer elements than we expect.
            // this means we will pass "" for strings not provided by the user
            // this is fine, as the from_string() call will return false if it fails to parse an empty string
            for (int32 i = elems.size(); i < sizeof...(I); i++)
            {
                elems.push_back(std::string_view());
            }

            return {from_string(elems[I], std::get<I>(value))...};
        }
    }

    template <typename... Args>
    std::tuple<Args...> parse_tuple(std::string_view str)
    {
        std::tuple<Args...> tuple;
        auto results = parse_string_as_tuple(str, tuple, std::make_index_sequence<sizeof...(Args)>{});

        return tuple;
    }

    template <raoe::character Char>
    inline bool is_hex(const Char ch)
    {
        return (ch >= static_cast<Char>('0') && ch <= static_cast<Char>('9')) ||
               (ch >= static_cast<Char>('a') && ch <= static_cast<Char>('f')) ||
               (ch >= static_cast<Char>('A') && ch <= static_cast<Char>('F'));
    }
}