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

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <istream>
#include <fstream>
#include <cmath>
extern "C" {
#include <GL/gl3w.h>
}

#include "lak.h"

#include "defines.h"

#ifndef IMAGE_H
#define IMAGE_H

namespace SourceExplorer
{
    lak::color4_t ColorFrom8bit(uint8_t RGB);
    lak::color4_t ColorFrom15bit(uint16_t RGB);
    lak::color4_t ColorFrom16bit(uint16_t RGB);

    lak::color4_t ColorFrom8bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom15bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom16bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom24bit(lak::memstrm_t &strm);
    lak::color4_t ColorFrom32bit(lak::memstrm_t &strm);

    lak::color4_t ColorFromMode(lak::memstrm_t &strm, const graphics_mode_t mode);

    struct bitmap_t
    {
        lak::vec2s_t size;
        std::vector<lak::color4_t> pixels;

        lak::color4_t &operator[](const size_t index);
        lak::color4_t &operator[](const lak::vec2s_t index);
        lak::color4_t &operator()(const size_t x, const size_t y);
        void resize(const lak::vec2s_t toSize);
    };

    uint16_t BitmapPaddingSize(
        uint16_t width,
        uint8_t colSize,
        uint8_t bytes = 2
    );

    struct image_t
    {
        using handle_t = uint16_t;

        bool old = false;
        uint32_t dataSize = 0;
        handle_t handle = 0;
        uint16_t checksum; // uint8_t if old
        uint32_t reference = 0;
        lak::vec2u16_t size = {};
        graphics_mode_t graphicsMode = GRAPHICS4;
        image_flag_t flags = RLE;

        uint32_t count = 0;
        lak::vec2u16_t hotspot = {};
        lak::vec2u16_t action = {};

        uint8_t paletteEntries = 0;
        std::vector<lak::color4_t> palette = {};
        lak::color4_t transparent = {};
        bitmap_t bitmap = {};

        lak::glTexture_t texture;
        image_t &initTexture();
    };

    image_t CreateImage(
        lak::memstrm_t &strm,
        const bool colorTrans,
        const bool old
    );

    // "fucked" version
    image_t CreateImage(
        lak::memstrm_t &strm,
        const bool colorTrans,
        const bool old,
        const lak::vec2u16_t sizeOverride
    );

    lak::glTexture_t CreateTexture(
        const bitmap_t &bitmap
    );

    lak::glTexture_t CreateTexture(
        const image_t &image
    );

    void ViewImage(
        const lak::glTexture_t &texture
    );
}

#endif
