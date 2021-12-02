/*
Copyright (C) 2020 Phillip Lamp
This file is part of the CTAGDRC project <https://github.com/p-hlp/CTAGDRC>.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <cstdint>

namespace Constants
{
    namespace Colors
    {
        //Backgrounds
        constexpr uint32_t bg_App{0xff5C2A2A}; // bg color (will use an image instead).
        constexpr uint32_t bg_DarkGrey{0xff333333};
        constexpr uint32_t bg_MidGrey{0xff616161};
        constexpr uint32_t bg_LightGrey{0xff9e9e9e};

        //Gradients
        constexpr uint32_t gd_LightGreyStart{0xffeeeee};
        constexpr uint32_t gd_LightGreyEnd{0xffc4c4c4};
        constexpr uint32_t gd_DarktGreyStart{0xff646464};
        constexpr uint32_t gd_DarkGreyEnd{0xff464646};

        //Misc
        constexpr uint32_t statusOutline{0xff8f0000};
        constexpr uint32_t knobShadow{0xff0000};
    }

    namespace Margins
    {
        constexpr float small = 2.0f;
        constexpr float medium = 4.0f;
        constexpr float big = 8.0f;
    }

    namespace Parameter
    {
        constexpr float inputStart = -30.0f;
        constexpr float inputEnd = 30.0f;
        constexpr float inputInterval = 0.1f;

        constexpr float mixStart = 0.001f;
        constexpr float mixEnd = 1.0f;
        constexpr float mixInterval = 0.001f;

        constexpr float airStart = 0.001f;
        constexpr float airEnd = 12.0f;
        constexpr float airInterval = 0.5f;

    }
}
