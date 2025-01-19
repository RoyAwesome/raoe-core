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

#include <format>

#define RAOE_CORE_DECLARE_FORMATTER_IMPL(type, ns, ...)                                                                \
    template <>                                                                                                        \
    struct ns::formatter<type>                                                                                         \
    {                                                                                                                  \
        template <typename ParseContext>                                                                               \
        constexpr auto parse(ParseContext& ctx) const                                                                  \
        {                                                                                                              \
            return ctx.begin();                                                                                        \
        }                                                                                                              \
                                                                                                                       \
        template <typename FormatContext>                                                                              \
        auto format(const type& value, FormatContext& ctx) const                                                       \
        {                                                                                                              \
            __VA_ARGS__                                                                                                \
        }                                                                                                              \
    };

#define RAOE_CORE_DECLARE_FORMATTER(type, ...) RAOE_CORE_DECLARE_FORMATTER_IMPL(type, std, __VA_ARGS__)