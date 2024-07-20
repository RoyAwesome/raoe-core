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

#include "core/core.hpp"
#include <istream>
#include <string_view>

namespace raoe::stream
{
    bool read_stream_into(std::output_iterator<std::byte> auto into_container, std::istream& from_stream)
    {
        std::for_each(std::istreambuf_iterator<char>(from_stream), std::istreambuf_iterator<char>(),
                      [&into_container](const char c) { (*into_container++) = std::byte(c); });
        return true;
    }

    template <typename TChar, typename TTraits = std::char_traits<TChar>>
    bool read_stream_into(std::output_iterator<std::byte> auto into_container,
                          std::basic_string_view<TChar, TTraits> from_string)
    {
        std::for_each(from_string.begin(), from_string.end(),
                      [&into_container](TChar c)
                      {
                          auto bytes = raoe::as_bytes(c);
                          for(auto byte : bytes)
                          {
                              (*into_container++) = byte;
                          }
                      });
        // Ensure we always zero terminate this string
        (*into_container++) = std::byte(0);
        return true;
    }
    template <typename TChar>
    bool read_stream_into(std::output_iterator<std::byte> auto into_container, std::basic_string<TChar> from_string)
    {
        return read_stream_into(into_container, std::basic_string_view<TChar>(from_string));
    }
}