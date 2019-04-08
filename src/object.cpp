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
        using namespace object;
        switch (type)
        {
            case object_type_t::PLAYER:          return "Player";
            case object_type_t::KEYBOARD:        return "Keyboard";
            case object_type_t::CREATE:          return "Create";
            case object_type_t::TIMER:           return "Timer";
            case object_type_t::GAME:            return "Game";
            case object_type_t::SPEAKER:         return "Speaker";
            case object_type_t::SYSTEM:          return "System";
            case object_type_t::QUICK_BACKDROP:  return "Quick Backdrop";
            case object_type_t::BACKDROP:        return "Backdrop";
            case object_type_t::ACTIVE:          return "Active";
            case object_type_t::TEXT:            return "Text";
            case object_type_t::QUESTION:        return "Question";
            case object_type_t::SCORE:           return "Score";
            case object_type_t::LIVES:           return "Lives";
            case object_type_t::COUNTER:         return "Counter";
            case object_type_t::RTF:             return "RTF";
            case object_type_t::SUB_APPLICATION: return "Sub Application";
            default: return "Invalid";
        }
    }

    namespace object
    {
        void shape_t::read(lak::memstrm_t &strm)
        {
            borderSize = strm.readInt<uint16_t>();
            borderColor = strm.readRGBA();
            shape = strm.readInt<shape_type_t>();
            fill = strm.readInt<fill_type_t>();

            if (shape == shape_type_t::LINE_SHAPE)
            {
                line = strm.readInt<line_flags_t>();
            }
            else if (fill == fill_type_t::SOLID_FILL)
            {
                color1 = strm.readRGBA();
            }
            else if (fill == fill_type_t::GRADIENT_FILL)
            {
                color1 = strm.readRGBA();
                color2 = strm.readRGBA();
                gradient = strm.readInt<gradient_flags_t>();
            }
            else if (fill == fill_type_t::MOTIF_FILL)
            {
                image = strm.readInt<uint16_t>();
            }
        }

        void quick_backdrop_t::read(lak::memstrm_t &strm)
        {
            size = strm.readInt<uint32_t>();
            obstacle = strm.readInt<uint16_t>();
            collision = strm.readInt<uint16_t>();
            width = strm.readInt<uint32_t>();
            height = strm.readInt<uint32_t>();
            shape.read(strm);
        }

        void backdrop_t::read(lak::memstrm_t &strm)
        {
            size = strm.readInt<uint32_t>();
            obstacle = strm.readInt<uint16_t>();
            collision = strm.readInt<uint16_t>();
            width = strm.readInt<uint32_t>();
            height = strm.readInt<uint32_t>();
            image = strm.readInt<uint16_t>();
        }

        void common_t::read(lak::memstrm_t &strm)
        {

        }
    }
}
