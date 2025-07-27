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

// Color list adapted from MonoGame (https://github.com/MonoGame/MonoGame) licensed under the MIT license

#pragma once

#include "core/core.hpp"
#include "glm/ext.hpp"

namespace raoe::render::colors
{
    constexpr glm::u8vec4 from_hex_abgr(const uint32 hex)
    {
        // packed in AA BB GG RR format
        // glm is in R G B A format
        const uint8 a = (hex >> 24) & 0xff; // alpha
        const uint8 b = (hex >> 16) & 0xff; // blue
        const uint8 g = (hex >> 8) & 0xff;  // green
        const uint8 r = hex & 0xff;         // red
        return glm::u8vec4(r, g, b, a);
    }

    constexpr glm::u8vec4 from_hex_rgba(const uint32 hex)
    {
        // packed in RR GG BB AA format
        // glm is in R G B A format
        const uint8 r = (hex >> 24) & 0xff; // red
        const uint8 g = (hex >> 16) & 0xff; // green
        const uint8 b = (hex >> 8) & 0xff;  // blue
        const uint8 a = hex & 0xff;         // alpha
        return glm::u8vec4(r, g, b, a);
    }

    inline constexpr glm::u8vec4 transparent_black = from_hex_abgr(0x00000000);
    inline constexpr glm::u8vec4 alice_blue = from_hex_abgr(0xfffff8f0);
    inline constexpr glm::u8vec4 antique_white = from_hex_abgr(0xffd7ebfa);
    inline constexpr glm::u8vec4 aqua = from_hex_abgr(0xffffff00);
    inline constexpr glm::u8vec4 aquamarine = from_hex_abgr(0xffd4ff7f);
    inline constexpr glm::u8vec4 azure = from_hex_abgr(0xfffffff0);
    inline constexpr glm::u8vec4 beige = from_hex_abgr(0xffdcf5f5);
    inline constexpr glm::u8vec4 bisque = from_hex_abgr(0xffc4e4ff);
    inline constexpr glm::u8vec4 black = from_hex_abgr(0xff000000);
    inline constexpr glm::u8vec4 blanched_almond = from_hex_abgr(0xffcdebff);
    inline constexpr glm::u8vec4 blue = from_hex_abgr(0xffff0000);
    inline constexpr glm::u8vec4 blue_violet = from_hex_abgr(0xffe22b8a);
    inline constexpr glm::u8vec4 brown = from_hex_abgr(0xff2a2aa5);
    inline constexpr glm::u8vec4 burly_wood = from_hex_abgr(0xff87b8de);
    inline constexpr glm::u8vec4 cadet_blue = from_hex_abgr(0xffa09e5f);
    inline constexpr glm::u8vec4 chartreuse = from_hex_abgr(0xff00ff7f);
    inline constexpr glm::u8vec4 chocolate = from_hex_abgr(0xff1e69d2);
    inline constexpr glm::u8vec4 coral = from_hex_abgr(0xff507fff);
    inline constexpr glm::u8vec4 cornflower_blue = from_hex_abgr(0xffed9564);
    inline constexpr glm::u8vec4 cornsilk = from_hex_abgr(0xffdcf8ff);
    inline constexpr glm::u8vec4 crimson = from_hex_abgr(0xff3c14dc);
    inline constexpr glm::u8vec4 cyan = from_hex_abgr(0xffffff00);
    inline constexpr glm::u8vec4 dark_blue = from_hex_abgr(0xff8b0000);
    inline constexpr glm::u8vec4 dark_cyan = from_hex_abgr(0xff8b8b00);
    inline constexpr glm::u8vec4 dark_goldenrod = from_hex_abgr(0xff0b86b8);
    inline constexpr glm::u8vec4 dark_gray = from_hex_abgr(0xffa9a9a9);
    inline constexpr glm::u8vec4 dark_green = from_hex_abgr(0xff006400);
    inline constexpr glm::u8vec4 dark_khaki = from_hex_abgr(0xff6bb7bd);
    inline constexpr glm::u8vec4 dark_magenta = from_hex_abgr(0xff8b008b);
    inline constexpr glm::u8vec4 dark_olive_green = from_hex_abgr(0xff2f6b55);
    inline constexpr glm::u8vec4 dark_orange = from_hex_abgr(0xff008cff);
    inline constexpr glm::u8vec4 dark_orchid = from_hex_abgr(0xffcc3299);
    inline constexpr glm::u8vec4 dark_red = from_hex_abgr(0xff00008b);
    inline constexpr glm::u8vec4 dark_salmon = from_hex_abgr(0xff7a96e9);
    inline constexpr glm::u8vec4 dark_sea_green = from_hex_abgr(0xff8bbc8f);
    inline constexpr glm::u8vec4 dark_slate_blue = from_hex_abgr(0xff8b3d48);
    inline constexpr glm::u8vec4 dark_slate_gray = from_hex_abgr(0xff4f4f2f);
    inline constexpr glm::u8vec4 dark_turquoise = from_hex_abgr(0xffd1ce00);
    inline constexpr glm::u8vec4 dark_violet = from_hex_abgr(0xffd30094);
    inline constexpr glm::u8vec4 deep_pink = from_hex_abgr(0xff9314ff);
    inline constexpr glm::u8vec4 deep_sky_blue = from_hex_abgr(0xffffbf00);
    inline constexpr glm::u8vec4 dim_gray = from_hex_abgr(0xff696969);
    inline constexpr glm::u8vec4 dodger_blue = from_hex_abgr(0xffff901e);
    inline constexpr glm::u8vec4 firebrick = from_hex_abgr(0xff2222b2);
    inline constexpr glm::u8vec4 floral_white = from_hex_abgr(0xfff0faff);
    inline constexpr glm::u8vec4 forest_green = from_hex_abgr(0xff228b22);
    inline constexpr glm::u8vec4 fuchsia = from_hex_abgr(0xffff00ff);
    inline constexpr glm::u8vec4 gainsboro = from_hex_abgr(0xffdcdcdc);
    inline constexpr glm::u8vec4 ghost_white = from_hex_abgr(0xfffff8f8);
    inline constexpr glm::u8vec4 gold = from_hex_abgr(0xff00d7ff);
    inline constexpr glm::u8vec4 goldenrod = from_hex_abgr(0xff20a5da);
    inline constexpr glm::u8vec4 gray = from_hex_abgr(0xff808080);
    inline constexpr glm::u8vec4 green = from_hex_abgr(0xff008000);
    inline constexpr glm::u8vec4 green_yellow = from_hex_abgr(0xff2fffad);
    inline constexpr glm::u8vec4 honeydew = from_hex_abgr(0xfff0fff0);
    inline constexpr glm::u8vec4 hotPink = from_hex_abgr(0xffb469ff);
    inline constexpr glm::u8vec4 indianRed = from_hex_abgr(0xff5c5ccd);
    inline constexpr glm::u8vec4 indigo = from_hex_abgr(0xff82004b);
    inline constexpr glm::u8vec4 ivory = from_hex_abgr(0xfff0ffff);
    inline constexpr glm::u8vec4 khaki = from_hex_abgr(0xff8ce6f0);
    inline constexpr glm::u8vec4 lavender = from_hex_abgr(0xfffae6e6);
    inline constexpr glm::u8vec4 lavender_blush = from_hex_abgr(0xfff5f0ff);
    inline constexpr glm::u8vec4 lawn_green = from_hex_abgr(0xff00fc7c);
    inline constexpr glm::u8vec4 lemon_chiffon = from_hex_abgr(0xffcdfaff);
    inline constexpr glm::u8vec4 light_blue = from_hex_abgr(0xffe6d8ad);
    inline constexpr glm::u8vec4 light_coral = from_hex_abgr(0xff8080f0);
    inline constexpr glm::u8vec4 light_cyan = from_hex_abgr(0xffffffe0);
    inline constexpr glm::u8vec4 light_goldenrod_yellow = from_hex_abgr(0xffd2fafa);
    inline constexpr glm::u8vec4 light_gray = from_hex_abgr(0xffd3d3d3);
    inline constexpr glm::u8vec4 light_green = from_hex_abgr(0xff90ee90);
    inline constexpr glm::u8vec4 light_pink = from_hex_abgr(0xffc1b6ff);
    inline constexpr glm::u8vec4 light_salmon = from_hex_abgr(0xff7aa0ff);
    inline constexpr glm::u8vec4 light_sea_green = from_hex_abgr(0xffaab220);
    inline constexpr glm::u8vec4 light_sky_blue = from_hex_abgr(0xffface87);
    inline constexpr glm::u8vec4 light_slate_gray = from_hex_abgr(0xff998877);
    inline constexpr glm::u8vec4 light_steel_blue = from_hex_abgr(0xffdec4b0);
    inline constexpr glm::u8vec4 light_yellow = from_hex_abgr(0xffe0ffff);
    inline constexpr glm::u8vec4 lime = from_hex_abgr(0xff00ff00);
    inline constexpr glm::u8vec4 limeGreen = from_hex_abgr(0xff32cd32);
    inline constexpr glm::u8vec4 linen = from_hex_abgr(0xffe6f0fa);
    inline constexpr glm::u8vec4 magenta = from_hex_abgr(0xffff00ff);
    inline constexpr glm::u8vec4 maroon = from_hex_abgr(0xff000080);
    inline constexpr glm::u8vec4 medium_aquamarine = from_hex_abgr(0xffaacd66);
    inline constexpr glm::u8vec4 medium_blue = from_hex_abgr(0xffcd0000);
    inline constexpr glm::u8vec4 medium_orchid = from_hex_abgr(0xffd355ba);
    inline constexpr glm::u8vec4 medium_purple = from_hex_abgr(0xffdb7093);
    inline constexpr glm::u8vec4 medium_sea_green = from_hex_abgr(0xff71b33c);
    inline constexpr glm::u8vec4 medium_slate_blue = from_hex_abgr(0xffee687b);
    inline constexpr glm::u8vec4 medium_spring_green = from_hex_abgr(0xff9afa00);
    inline constexpr glm::u8vec4 medium_turquoise = from_hex_abgr(0xffccd148);
    inline constexpr glm::u8vec4 medium_violet_red = from_hex_abgr(0xff8515c7);
    inline constexpr glm::u8vec4 midnight_blue = from_hex_abgr(0xff701919);
    inline constexpr glm::u8vec4 mint_cream = from_hex_abgr(0xfffafff5);
    inline constexpr glm::u8vec4 misty_rose = from_hex_abgr(0xffe1e4ff);
    inline constexpr glm::u8vec4 moccasin = from_hex_abgr(0xffb5e4ff);
    inline constexpr glm::u8vec4 navajo_white = from_hex_abgr(0xffaddeff);
    inline constexpr glm::u8vec4 navy = from_hex_abgr(0xff800000);
    inline constexpr glm::u8vec4 neon_pink = from_hex_abgr(0xffff00ff);
    inline constexpr glm::u8vec4 old_lace = from_hex_abgr(0xffe6f5fd);
    inline constexpr glm::u8vec4 olive = from_hex_abgr(0xff008080);
    inline constexpr glm::u8vec4 olive_drab = from_hex_abgr(0xff238e6b);
    inline constexpr glm::u8vec4 orange = from_hex_abgr(0xff00a5ff);
    inline constexpr glm::u8vec4 orange_red = from_hex_abgr(0xff0045ff);
    inline constexpr glm::u8vec4 orchid = from_hex_abgr(0xffd670da);
    inline constexpr glm::u8vec4 pale_goldenrod = from_hex_abgr(0xffaae8ee);
    inline constexpr glm::u8vec4 pale_green = from_hex_abgr(0xff98fb98);
    inline constexpr glm::u8vec4 pale_turquoise = from_hex_abgr(0xffeeeeaf);
    inline constexpr glm::u8vec4 pale_violet_red = from_hex_abgr(0xff9370db);
    inline constexpr glm::u8vec4 papaya_whip = from_hex_abgr(0xffd5efff);
    inline constexpr glm::u8vec4 peach_puff = from_hex_abgr(0xffb9daff);
    inline constexpr glm::u8vec4 peru = from_hex_abgr(0xff3f85cd);
    inline constexpr glm::u8vec4 pink = from_hex_abgr(0xffcbc0ff);
    inline constexpr glm::u8vec4 plum = from_hex_abgr(0xffdda0dd);
    inline constexpr glm::u8vec4 powder_blue = from_hex_abgr(0xffe6e0b0);
    inline constexpr glm::u8vec4 purple = from_hex_abgr(0xff800080);
    inline constexpr glm::u8vec4 red = from_hex_abgr(0xff0000ff);
    inline constexpr glm::u8vec4 rosy_brown = from_hex_abgr(0xff8f8fbc);
    inline constexpr glm::u8vec4 royal_blue = from_hex_abgr(0xffe16941);
    inline constexpr glm::u8vec4 saddle_brown = from_hex_abgr(0xff13458b);
    inline constexpr glm::u8vec4 salmon = from_hex_abgr(0xff7280fa);
    inline constexpr glm::u8vec4 sandy_brown = from_hex_abgr(0xff60a4f4);
    inline constexpr glm::u8vec4 sea_green = from_hex_abgr(0xff578b2e);
    inline constexpr glm::u8vec4 sea_shell = from_hex_abgr(0xffeef5ff);
    inline constexpr glm::u8vec4 sienna = from_hex_abgr(0xff2d52a0);
    inline constexpr glm::u8vec4 silver = from_hex_abgr(0xffc0c0c0);
    inline constexpr glm::u8vec4 sky_blue = from_hex_abgr(0xffebce87);
    inline constexpr glm::u8vec4 slate_blue = from_hex_abgr(0xffcd5a6a);
    inline constexpr glm::u8vec4 slate_gray = from_hex_abgr(0xff908070);
    inline constexpr glm::u8vec4 snow = from_hex_abgr(0xfffafaff);
    inline constexpr glm::u8vec4 spring_green = from_hex_abgr(0xff7fff00);
    inline constexpr glm::u8vec4 steel_blue = from_hex_abgr(0xffb48246);
    inline constexpr glm::u8vec4 tan = from_hex_abgr(0xff8cb4d2);
    inline constexpr glm::u8vec4 teal = from_hex_abgr(0xff808000);
    inline constexpr glm::u8vec4 thistle = from_hex_abgr(0xffd8bfd8);
    inline constexpr glm::u8vec4 tomato = from_hex_abgr(0xff4763ff);
    inline constexpr glm::u8vec4 turquoise = from_hex_abgr(0xffd0e040);
    inline constexpr glm::u8vec4 violet = from_hex_abgr(0xffee82ee);
    inline constexpr glm::u8vec4 wheat = from_hex_abgr(0xffb3def5);
    inline constexpr glm::u8vec4 white = from_hex_abgr(std::numeric_limits<uint32>::max());
    inline constexpr glm::u8vec4 white_smoke = from_hex_abgr(0xfff5f5f5);
    inline constexpr glm::u8vec4 yellow = from_hex_abgr(0xff00ffff);
    inline constexpr glm::u8vec4 yellow_green = from_hex_abgr(0xff32cd9a);

}