// // Copyright (c) Mathias Kaerlev 2012, LAK132 2019

// // This file is part of Anaconda.

// // Anaconda is free software: you can redistribute it and/or modify
// // it under the terms of the GNU General Public License as published by
// // the Free Software Foundation, either version 3 of the License, or
// // (at your option) any later version.

// // Anaconda is distributed in the hope that it will be useful,
// // but WITHOUT ANY WARRANTY; without even the implied warranty of
// // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// // GNU General Public License for more details.

// // You should have received a copy of the GNU General Public License
// // along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

// #include <stdint.h>
// #include <iostream>
// #include <string>
// #include <vector>
// #include <istream>
// #include <fstream>
// #include <cmath>
// extern "C" {
// #include <GL/gl3w.h>
// }

// #include <lak/image.h>

// #include "lak.h"

// #include "defines.h"

// #ifndef IMAGE_H
// #define IMAGE_H

// namespace SourceExplorer
// {
//     uint16_t BitmapPaddingSize(
//         uint16_t width,
//         uint8_t colSize,
//         uint8_t bytes = 2
//     );

//     struct image_t
//     {
//         using handle_t = uint16_t;

//         bool old = false;
//         uint32_t dataSize = 0;
//         handle_t handle = 0;
//         uint16_t checksum; // uint8_t if old
//         uint32_t reference = 0;
//         lak::vec2u16_t size = {};
//         graphics_mode_t graphicsMode = graphics_mode_t::GRAPHICS4;
//         image_flag_t flags = image_flag_t::RLE;

//         uint32_t count = 0;
//         lak::vec2u16_t hotspot = {};
//         lak::vec2u16_t action = {};

//         uint8_t paletteEntries = 0;
//         std::vector<lak::color4_t> palette = {};
//         lak::color4_t transparent = {};
//         lak::image4_t bitmap = {};

//         lak::glTexture_t texture;
//         image_t &initTexture();
//     };

//     image_t CreateImage(
//         lak::memory &strm,
//         const bool colorTrans,
//         const bool old,
//         const lak::color4_t palette[256] = nullptr
//     );

//     // // "fucked" version
//     // image_t CreateImage(
//     //     lak::memory &strm,
//     //     const bool colorTrans,
//     //     const bool old,
//     //     const lak::vec2u16_t sizeOverride
//     // );

//     lak::glTexture_t CreateTexture(
//         const image_t &image
//     );
// }

// #endif
