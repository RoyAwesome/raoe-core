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

#include <algorithm>
#include <cctype>
#include <iterator>
#include <locale>
#include <numeric>
#include <sstream>
#include <vector>

namespace raoe::string
{
    // Left Trim a string, inline.  Removes all whitespace characters from the left side of the string, modifying it in
    // place
    inline void ltrim(std::string& s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    }

    // Right Trim a string, inline.  Removes all whitespace characters from the right side of the string, modifying it
    // in place
    inline void rtrim(std::string& s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    // Trim a string, inline.  Removes all whitespace characters from the right and left side of the string, modifying
    // it in place
    inline void trim(std::string& s)
    {
        ltrim(s);
        rtrim(s);
    }

    // Right Trim a string, copy.  Removes all whitespace characters from the right side of the string, returning a copy
    // of the original string
    inline std::string rtrim_c(std::string s)
    {
        rtrim(s);
        return s;
    }

    // Left Trim a string, copy.  Removes all whitespace characters from the left side of the string, returning a copy
    // of the original string
    inline std::string ltrim_c(std::string s)
    {
        ltrim(s);
        return s;
    }

    // Trim a string, copy.  Removes all whitespace characters from the left and right side of the string, returning a
    // copy of the original string
    inline std::string trim_c(std::string s)
    {
        rtrim(s);
        ltrim(s);
        return s;
    }

    inline void split(const std::string& s, char delim, std::output_iterator<std::string> auto out_itr)
    {
        std::istringstream iss(s);
        std::string item;
        while(std::getline(iss, item, delim))
        {
            if(!item.empty())
            {
                *out_itr++ = item;
            }
        }
    }

    inline std::vector<std::string> split(const std::string& s, char delim)
    {
        std::vector<std::string> elems;
        split(s, delim, std::back_inserter(elems));
        return elems;
    }

    inline std::string_view trim_l(std::string_view s)
    {
        return s.substr(s.find_first_not_of(' '));
    }

    inline std::string_view trim_r(std::string_view s)
    {
        return s.substr(0, s.find_last_not_of(' ') + 1);
    }

    inline std::string_view trim(std::string_view s)
    {
        return trim_r(trim_l(s));
    }

    inline void split(std::string_view sv, std::string_view delimiter,
                      std::output_iterator<std::string_view> auto out_itr)
    {
        size_t start = 0;
        size_t cursor = start;
        while(cursor != sv.length())
        {
            while(cursor != sv.length() && sv.substr(cursor, delimiter.length()).compare(delimiter) != 0)
            {
                cursor++;
            }

            *out_itr++ = sv.substr(start, cursor - start);

            if(cursor == sv.length())
            {
                return;
            }

            cursor++;
            start = cursor;
        }
    }

    inline void split(std::string_view sv, char delimiter, std::output_iterator<std::string_view> auto out_itr)
    {
        size_t start = 0;
        size_t cursor = start;
        while(cursor != sv.length())
        {
            while(cursor != sv.length() && sv[cursor] != delimiter)
            {
                cursor++;
            }

            *out_itr++ = sv.substr(start, cursor - start);

            if(cursor == sv.length())
            {
                return;
            }

            cursor++;
            start = cursor;
        }
    }

    inline std::string join(const auto& range, const std::string_view delimiter)
    {
        if(range.empty())
        {
            return std::string {};
        }
        std::stringstream sstream;
        for(const auto& item : range)
        {
            sstream << item;
            if(&item != &range.back())
            {
                sstream << delimiter;
            }
        }

        return sstream.str();
    }

    inline std::string_view token(std::string_view sv, std::string_view token)
    {
        return sv.substr(0, sv.find_first_of(token));
    }

    inline bool contains(std::string_view sv, std::string_view token)
    {
        return sv.find(token) != std::string_view::npos;
    }

    inline bool contains(std::string_view sv, char token)
    {
        return std::ranges::any_of(sv, [token](char c) { return c == token; });
    }

    inline std::size_t replace_all(std::string& str, std::string_view what, std::string_view with)
    {
        std::size_t count {};
        std::size_t pos {};
        while((pos = str.find(what, pos)) != std::string::npos)
        {
            str.replace(pos, what.length(), with);
            pos += with.length();
            ++count;
        }
        return count;
    }

}