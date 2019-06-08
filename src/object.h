// Copyright (c) Mathias Kaerlev 2012, Lucas Kleiss 2019

// This file is part of Anaconda.

// Anaconda is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Anaconda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

#include "lak.h"
#include "defines.h"

#ifndef OBJECT_H
#define OBJECT_H

namespace SourceExplorer
{
    namespace object
    {
        enum class object_type_t : int16_t
        {
            PLAYER          = -7,
            KEYBOARD        = -6,
            CREATE          = -5,
            TIMER           = -4,
            GAME            = -3,
            SPEAKER         = -2,
            SYSTEM          = -1,
            QUICK_BACKDROP  = 0,
            BACKDROP        = 1,
            ACTIVE          = 2,
            TEXT            = 3,
            QUESTION        = 4,
            SCORE           = 5,
            LIVES           = 6,
            COUNTER         = 7,
            RTF             = 8,
            SUB_APPLICATION = 9
        };

        enum class shape_type_t : uint16_t
        {
            LINE        = 1,
            RECTANGLE   = 2,
            ELLIPSE     = 3
        };

        enum class fill_type_t : uint16_t
        {
            NONE        = 0,
            SOLID       = 1,
            GRADIENT    = 2,
            MOTIF       = 3
        };

        enum class line_flags_t : uint16_t
        {
            NONE        = 0,
            INVERSE_X   = 1 << 0,
            INVERSE_Y   = 1 << 1
        };

        enum class gradient_flags_t : uint16_t
        {
            HORIZONTAL  = 0,
            VERTICAL    = 1,
        };

        struct shape_t
        {
            fill_type_t fill;
            shape_type_t shape;
            line_flags_t line;
            gradient_flags_t gradient;
            uint16_t borderSize;
            lak::color4_t borderColor;
            lak::color4_t color1, color2;
            uint16_t image;

            void read(lak::memory &strm);
        };

        struct quick_backdrop_t
        {
            uint32_t size;
            uint16_t obstacle;
            uint16_t collision;
            uint32_t width;
            uint32_t height;
            shape_t shape;

            void read(lak::memory &strm);
        };

        struct backdrop_t
        {
            uint32_t size;
            uint16_t obstacle;
            uint16_t collision;
            uint32_t width;
            uint32_t height;
            uint16_t image;

            void read(lak::memory &strm);
        };

        struct common_t
        {
            uint32_t size;
            uint16_t movements;
            uint16_t animations;
            uint16_t counter;
            uint16_t system;
            uint32_t fadeIn;
            uint32_t fadeOut;
            uint16_t values;
            uint16_t strings;
            uint16_t extension;
            // std::vector<int16_t> qualifiers;

            uint16_t version;
            uint32_t flags;
            uint32_t newFlags;
            uint32_t preferences;
            uint32_t identifier;
            lak::color4_t backColor;

            bool newObj;

            void read(lak::memory &strm, bool newobj);
        };
    }

    std::string GetObjectTypeString(
        object::object_type_t type
    );
}

#endif // OBJECT_H