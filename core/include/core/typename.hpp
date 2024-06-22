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

// Taken from: https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/

#pragma once

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace raoe::core
{
    template <std::size_t... Idxs>
    constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
    {
        return std::array {str[Idxs]..., '\n'};
    }

    constexpr std::string_view trim_class(std::string_view str)
    {
        constexpr auto class_prefix = std::string_view {"class"};
        auto class_start = str.find(class_prefix);
        if(class_start != std::string_view::npos)
        {
            return str.substr(class_start + class_prefix.length(), str.length());
        }
        return str;
    }
    constexpr std::string_view trim_whitespace(std::string_view str)
    {
        auto firstNotWhitespace = str.find_first_not_of(" \t\n");
        auto lastNotWhitespace = str.find_last_not_of(" \t\n");
        return str.substr(firstNotWhitespace, lastNotWhitespace + 1);
    }

    template <typename T>
    constexpr auto type_name_array()
    {
#if defined(__clang__)
        constexpr auto prefix = std::string_view {"[T = "};
        constexpr auto suffix = std::string_view {"]"};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
        constexpr auto prefix = std::string_view {"with T = "};
        constexpr auto suffix = std::string_view {"]"};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
        constexpr auto prefix = std::string_view {"type_name_array<"};
        constexpr auto suffix = std::string_view {">(void)"};
        constexpr auto function = std::string_view {__FUNCSIG__};
#else
#error Unsupported compiler
#endif

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.rfind(suffix);

        static_assert(start < end);

        constexpr auto name = trim_whitespace(trim_class(function.substr(start, (end - start))));

        return substring_as_array(name, std::make_index_sequence<name.size()> {});
    }

    template <typename T>
    struct type_name_holder
    {
        static inline constexpr auto value = type_name_array<T>();
    };

    template <typename T>
    constexpr std::string_view name_of()
    {
        constexpr auto& value = type_name_holder<T>::value;
        return trim_whitespace(std::string_view {value.data(), value.size()});
    }

    template <typename R, typename... Args>
    constexpr int count_args(R (*func)(Args...))
    {
        return sizeof...(Args);
    }

}
