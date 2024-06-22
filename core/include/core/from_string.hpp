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

#include "string.hpp"
#include "typename.hpp"
#include "types.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <concepts>
#include <string_view>

namespace raoe
{
    inline int parse_base(std::string_view fmt)
    {
        if(raoe::string::contains(fmt, 'x'))
        {
            return 16;
        }
        else if(raoe::string::contains(fmt, 'b'))
        {
            return 2;
        }
        else if(raoe::string::contains(fmt, 'o'))
        {
            return 8;
        }
        else if(raoe::string::contains(fmt, 'd'))
        {
            return 10;
        }

        // base format specifier
        // nNN
        if(auto pos = fmt.find_first_of('n'); pos != std::string_view::npos && pos + 3 < fmt.size())
        {
            int base;
            auto result = std::from_chars(fmt.data() + pos + 1, fmt.data() + pos + 3, base);
            if(result.ec != std::errc::invalid_argument && result.ec != std::errc::result_out_of_range)
            {
                return base;
            }
        }

        return 10;
    }

    inline bool parse_endian(std::string_view fmt)
    {
        if(raoe::string::contains(fmt, 'B') && std::endian::native != std::endian::big)
        {
            return true;
        }
        else if(raoe::string::contains(fmt, 'L') && std::endian::native != std::endian::little)
        {
            return true;
        }

        return false;
    }
}

namespace raoe
{
    template <typename T>
    concept from_stringable = requires(std::string_view a, T& b, std::string_view fmt) {
        {
            from_string(a, b, fmt)
        } -> std::same_as<bool>;
    };

    // FORMAT SPECIFIERS
    // x - hexadecimal
    // B - big endian
    // L - little endian
    inline bool from_string(std::string_view arg, std::integral auto& value, std::string_view fmt = {})
    {
        int base = raoe::parse_base(fmt);
        bool swap_endian = parse_endian(fmt);

        auto result = std::from_chars(arg.data(), arg.data() + arg.size(), value, base);
        if(swap_endian)
        {
            value = raoe::byteswap(value);
        }
        const bool success = result.ec != std::errc::invalid_argument;
        return success;
    }

    inline bool from_string(std::string_view arg, std::floating_point auto& value, std::string_view fmt = {})
    {
        auto result = std::from_chars(arg.data(), arg.data() + arg.size(), value);
        const bool success = result.ec != std::errc::invalid_argument;
        return success;
    }

    // Specialization for strings.  This is because they do not need to be parsed
    inline bool from_string(std::string_view arg, std::string& value, std::string_view fmt = {})
    {
        value = std::string(arg);
        return true;
    }

    inline bool from_string(std::string_view arg, std::string_view& value, std::string_view fmt = {})
    {
        value = arg;
        return true;
    }
}