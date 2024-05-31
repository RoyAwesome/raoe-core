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

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <format>
#include <random>
#include <span>
#include <type_traits>

#include "parse.hpp"
#include "string.hpp"
#include "types.hpp"

namespace raoe
{
    struct uuid
    {
        friend uuid make_random_uuid_v4();
        friend std::hash<uuid>;
        friend std::formatter<uuid>;

        constexpr uuid() { m_bytes.fill(0); }
        constexpr uuid(std::span<uint8, 16> in_bytes)
        {
            if(std::is_constant_evaluated())
            {
                for(int i = 0; i < 16; i++)
                {
                    m_bytes[i] = in_bytes[i];
                }
            }
            else
            {
                std::memmove(m_bytes.data(), in_bytes.data(), 16);
            }
        }

        constexpr uuid(uint32 a, uint16 b, uint16 c, uint8 d, uint8 e, uint8 f, uint8 g, uint8 h, uint8 i, uint8 j,
                       uint8 k)
            : uuid(a, b, c, {d, e, f, g, h, i, j, k})
        {
        }

        constexpr uuid(uint32 a, uint16 b, uint16 c, uint16 d, std::span<uint8, 6> bytes)
        {
            if(std::is_constant_evaluated())
            {
                for(int i = 0; i < 4; i++)
                {
                    m_bytes[i] = ((uint8*)&a)[i];
                }
                for(int i = 0; i < 2; i++)
                {
                    m_bytes[i + 4] = ((uint8*)&b)[i];
                }
                for(int i = 0; i < 2; i++)
                {
                    m_bytes[i + 6] = ((uint8*)&c)[i];
                }
                for(int i = 0; i < 2; i++)
                {
                    m_bytes[i + 8] = ((uint8*)&d)[i];
                }
                for(uint8 i = 0; i < 6; i++)
                {
                    m_bytes[i + 10] = bytes[i];
                }
            }
            else
            {
                std::memmove(m_bytes.data(), &a, 4);
                std::memcpy(m_bytes.data() + 4, &b, 2);
                std::memcpy(m_bytes.data() + 6, &c, 2);
                std::memcpy(m_bytes.data() + 8, &d, 2);
                for(uint8 i = 0; i < 6; i++)
                {
                    m_bytes[i + 10] = bytes[i];
                }
            }
        }

        constexpr uuid(uint32 a, uint16 b, uint16 c, std::array<uint8, 8> bytes)
        {
            if(std::is_constant_evaluated())
            {
                for(int i = 0; i < 4; i++)
                {
                    m_bytes[i] = ((uint8*)&a)[i];
                }
                for(int i = 0; i < 2; i++)
                {
                    m_bytes[i + 4] = ((uint8*)&b)[i];
                }
                for(int i = 0; i < 2; i++)
                {
                    m_bytes[i + 6] = ((uint8*)&c)[i];
                }
                for(uint8 i = 0; i < 8; i++)
                {
                    m_bytes[i + 8] = bytes[i];
                }
            }
            else
            {
                std::memmove(m_bytes.data(), &a, 4);
                std::memcpy(m_bytes.data() + 4, &b, 2);
                std::memcpy(m_bytes.data() + 6, &c, 2);
                for(uint8 i = 0; i < 8; i++)
                {
                    m_bytes[i + 8] = bytes[i];
                }
            }
        }

        constexpr uuid(const uuid& other)
            : m_bytes(other.m_bytes)
        {
        }

        constexpr uuid& operator=(const uuid& other)
        {
            m_bytes = other.m_bytes;
            return *this;
        }

        constexpr uuid(uuid&& other)
            : m_bytes(std::exchange(other.m_bytes, std::array<uint8, 16> {}))
        {
        }
        constexpr uuid& operator=(uuid&& other)
        {
            m_bytes = std::exchange(other.m_bytes, std::array<uint8, 16> {});
            return *this;
        }

        constexpr std::strong_ordering operator<=>(const uuid& other) const { return m_bytes <=> other.m_bytes; }

        bool operator==(const uuid& other) const noexcept = default;
        bool operator!=(const uuid& other) const noexcept = default;
        bool operator>(const uuid& other) const noexcept = default;
        bool operator>=(const uuid& other) const noexcept = default;
        bool operator<=(const uuid& other) const noexcept = default;
        bool operator<(const uuid& other) const noexcept = default;

      private:
        std::array<uint8, 16> m_bytes;
    };

    inline uuid make_random_uuid_v4()
    {
        std::random_device r;
        std::mt19937_64 gen(r());
        std::uniform_int_distribution<> dis(0, 255);

        uuid id;
        for(int i = 0; i < 16; i++)
        {
            id.m_bytes[i] = dis(gen);
        }

        // variant must be 10xxxxxxx
        id.m_bytes[8] &= 0xBF;
        id.m_bytes[8] |= 0x80;

        // version must be 0100xxxx
        id.m_bytes[6] &= 0x4F;
        id.m_bytes[6] |= 0x40;

        return id;
    }

    /// create a uuid that is unique without caring about the mode or how it's generated
    /// this should be the uuid best suited for the platform (ie: on windows it will be windows format TODO: this)
    /// or it will be a random v4 uuid if there is no well suited platform uuid (TODO: this is actually what it always
    /// returns)
    /// TODO: implement the best suited platform
    inline uuid make_uuid()
    {
        return make_random_uuid_v4();
    }

    inline bool from_string(std::string_view arg, uuid& id)
    {
        // If we are smaller than 36 characters, we can't be a valid uuid
        if(arg.size() < 36)
        {
            return false;
        }
        // If we are a microsoft style guid, remove the braces
        if(arg.starts_with('{'))
        {
            arg = arg.substr(1, arg.size() - 2);
        }

        // Split the substring into the parts based on the -
        std::vector<std::string_view> parts;
        raoe::string::split(arg, '-', std::back_inserter(parts));

        // If we don't have 5 parts, we can't be a valid uuid
        if(parts.size() != 5)
        {
            return false;
        }

        // Parse the parts into the uuid
        uint32 a;
        uint16 b;
        uint16 c;
        uint16 d;
        raoe::from_string(parts[0], a, "xB");
        raoe::from_string(parts[1], b, "xB");
        raoe::from_string(parts[2], c, "xB");
        raoe::from_string(parts[3], d, "xB");

        // and then the rest of them
        std::array<uint8, 6> bytes;
        // for each 2- character pair in the last part
        for(uint8 j = 0; j < 6; j++)
        {
            raoe::from_string(parts[4].substr(j * 2, 2), bytes[j], "xB");
        }

        id = uuid(a, b, c, d, bytes);
        return true;
    }
}

namespace std
{
    template <>
    struct hash<raoe::uuid>
    {
        [[nodiscard]] std::size_t operator()(raoe::uuid const& id) const
        {
            uint64 l = static_cast<uint64>(id.m_bytes[0]) << 56 | static_cast<uint64>(id.m_bytes[1]) << 48 |
                       static_cast<uint64>(id.m_bytes[2]) << 40 | static_cast<uint64>(id.m_bytes[3]) << 32 |
                       static_cast<uint64>(id.m_bytes[4]) << 24 | static_cast<uint64>(id.m_bytes[5]) << 16 |
                       static_cast<uint64>(id.m_bytes[6]) << 8 | static_cast<uint64>(id.m_bytes[7]);

            uint64 h = static_cast<uint64>(id.m_bytes[8]) << 56 | static_cast<uint64>(id.m_bytes[9]) << 48 |
                       static_cast<uint64>(id.m_bytes[10]) << 40 | static_cast<uint64>(id.m_bytes[11]) << 32 |
                       static_cast<uint64>(id.m_bytes[12]) << 24 | static_cast<uint64>(id.m_bytes[13]) << 16 |
                       static_cast<uint64>(id.m_bytes[14]) << 8 | static_cast<uint64>(id.m_bytes[15]);

            if constexpr(sizeof(std::size_t) > 4)
            {
                return std::size_t(l ^ h);
            }
            else
            {
                uint64 hash = l ^ h;
                return size_t(uint32(hash >> 32) ^ uint32(hash));
            }
        }
    };

    template <>
    struct std::formatter<raoe::uuid>
    {
        constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

        auto format(const raoe::uuid& id, std::format_context& ctx)
        {
            uint32 first = static_cast<uint32>(id.m_bytes[0]) << 24 | static_cast<uint32>(id.m_bytes[1]) << 16 |
                           static_cast<uint32>(id.m_bytes[2]) << 8 | static_cast<uint32>(id.m_bytes[3]);

            uint16 second = static_cast<uint16>(id.m_bytes[4]) << 8 | static_cast<uint16>(id.m_bytes[5]);

            uint16 third = static_cast<uint16>(id.m_bytes[6]) << 8 | static_cast<uint16>(id.m_bytes[7]);

            uint16 fourth = static_cast<uint16>(id.m_bytes[8]) << 8 | static_cast<uint16>(id.m_bytes[9]);

            return std::format_to(ctx.out(), "{:08x}-{:04x}-{:04x}-{:04x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}", first,
                                  second, third, fourth, id.m_bytes[10], id.m_bytes[11], id.m_bytes[12], id.m_bytes[13],
                                  id.m_bytes[14], id.m_bytes[15]);
        }
    };
}