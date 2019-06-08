// Copyright (c) Mathias Kaerlev 2012, LAK132 2019

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

#include "object.h"

namespace SourceExplorer
{
    std::string GetObjectTypeString(object::object_type_t type)
    {
        switch (type)
        {
            case object::object_type_t::PLAYER:          return "Player";
            case object::object_type_t::KEYBOARD:        return "Keyboard";
            case object::object_type_t::CREATE:          return "Create";
            case object::object_type_t::TIMER:           return "Timer";
            case object::object_type_t::GAME:            return "Game";
            case object::object_type_t::SPEAKER:         return "Speaker";
            case object::object_type_t::SYSTEM:          return "System";
            case object::object_type_t::QUICK_BACKDROP:  return "Quick Backdrop";
            case object::object_type_t::BACKDROP:        return "Backdrop";
            case object::object_type_t::ACTIVE:          return "Active";
            case object::object_type_t::TEXT:            return "Text";
            case object::object_type_t::QUESTION:        return "Question";
            case object::object_type_t::SCORE:           return "Score";
            case object::object_type_t::LIVES:           return "Lives";
            case object::object_type_t::COUNTER:         return "Counter";
            case object::object_type_t::RTF:             return "RTF";
            case object::object_type_t::SUB_APPLICATION: return "Sub Application";
            default: return "Invalid";
        }
    }

    namespace object
    {
        void shape_t::read(lak::memory &strm)
        {
            borderSize = strm.read_u16();
            // borderColor = strm.readRGBA();
            borderColor.r = strm.read_u8();
            borderColor.g = strm.read_u8();
            borderColor.b = strm.read_u8();
            borderColor.a = strm.read_u8();
            shape = (shape_type_t)strm.read_u16();
            fill = (fill_type_t)strm.read_u16();

            if (shape == shape_type_t::LINE)
            {
                line = (line_flags_t)strm.read_u16();
            }
            else if (fill == fill_type_t::SOLID)
            {
                // color1 = strm.readRGBA();
                color1.r = strm.read_u8();
                color1.g = strm.read_u8();
                color1.b = strm.read_u8();
                color1.a = strm.read_u8();
            }
            else if (fill == fill_type_t::GRADIENT)
            {
                // color1 = strm.readRGBA();
                color1.r = strm.read_u8();
                color1.g = strm.read_u8();
                color1.b = strm.read_u8();
                color1.a = strm.read_u8();
                // color2 = strm.readRGBA();
                color2.r = strm.read_u8();
                color2.g = strm.read_u8();
                color2.b = strm.read_u8();
                color2.a = strm.read_u8();
                gradient = (gradient_flags_t)strm.read_u16();
            }
            else if (fill == fill_type_t::MOTIF)
            {
                image = strm.read_u16();
            }
        }

        void quick_backdrop_t::read(lak::memory &strm)
        {
            size = strm.read_u32();
            obstacle = strm.read_u16();
            collision = strm.read_u16();
            width = strm.read_u32();
            height = strm.read_u32();
            shape.read(strm);
        }

        void backdrop_t::read(lak::memory &strm)
        {
            size = strm.read_u32();
            obstacle = strm.read_u16();
            collision = strm.read_u16();
            width = strm.read_u32();
            height = strm.read_u32();
            image = strm.read_u16();
        }

        void common_t::read(lak::memory &strm, bool newobj)
        {
            newObj = newobj;

            if (newObj)
            {
                counter = strm.read_u16();
                version = strm.read_u16();
                strm.position += 2;
                movements = strm.read_u16();
                extension = strm.read_u16();
                animations = strm.read_u16();
            }
            else
            {
                movements = strm.read_u16();
                animations = strm.read_u16();
                version = strm.read_u16();
                counter = strm.read_u16();
                system = strm.read_u16();
                strm.position += 2;
            }

            flags = strm.read_u32();

            size_t end = strm.position + 8 * 2;

            // qualifiers.clear();
            // qualifiers.reserve(8);
            // for (size_t i = 0; i < 8; ++i)
            // {
            //     int16_t qualifier = strm.read_s16();
            //     if (qualifier == -1)
            //         break;
            //     qualifiers.push_back(qualifier);
            // }

            strm.position = end;

            if (newObj)
                system = strm.read_u16();
            else
                extension = strm.read_u16();

            values = strm.read_u16();
            strings = strm.read_u16();
            newFlags = strm.read_u32();
            preferences = strm.read_u32();
            identifier = strm.read_u32();
            // backColor = strm.readRGBA();
            backColor.r = strm.read_u8();
            backColor.g = strm.read_u8();
            backColor.b = strm.read_u8();
            backColor.a = strm.read_u8();
            fadeIn = strm.read_u32();
            fadeOut = strm.read_u32();
        }
    }
}
