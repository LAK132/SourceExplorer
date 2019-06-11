// // Copyright (c) Mathias Kaerlev 2012, Lucas Kleiss 2019

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

// #include "image.h"

// namespace SourceExplorer
// {
//     bool operator==(const lak::color4_t &lhs, const lak::color4_t& rhs)
//     {
//         return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;// && lhs.a == rhs.a;
//     }

//     image_t &image_t::initTexture()
//     {
//         DEBUG("Generating Texture");
//         texture = CreateTexture(bitmap);
//         return *this;
//     }

//     void _CreateImage(lak::memory &strm, image_t &img, const bool colorTrans, const lak::color4_t palette[256] = nullptr)
//     {
//         img.bitmap.resize(img.size);

//         uint8_t colSize = ColorModeSize(img.graphicsMode);
//         DEBUG("Color Size: 0x" << (uint32_t)colSize);
//         uint16_t pad = BitmapPaddingSize(img.size.x, colSize);
//         DEBUG("Bitmap Padding: 0x" << (uint32_t)pad);

//         lak::memory newStrm;
//         if ((img.flags & image_flag_t::LZX) != image_flag_t::NONE)
//         {
//             uint32_t decompressed = strm.read_u32(); (void)decompressed;
//             newStrm = Inflate(strm.read(strm.read_u32())); // is this right?
//         }
//         else
//         {
//             newStrm = strm;
//         }

//         size_t alphaSize = 0;
//         if ((img.flags & (image_flag_t::RLE | image_flag_t::RLEW | image_flag_t::RLET)) != image_flag_t::NONE)
//         {
//             size_t bytesRead = ReadRLE(newStrm, img.bitmap, img.graphicsMode, palette);
//             DEBUG("Bytes Read: 0x" << bytesRead);
//             alphaSize = img.dataSize - bytesRead;
//         }
//         else
//         {
//             size_t bytesRead = ReadRGB(newStrm, img.bitmap, img.graphicsMode, palette);
//             DEBUG("Bytes Read: 0x" << bytesRead);
//             alphaSize = img.dataSize - bytesRead;
//         }
//         DEBUG("Alpha Size: 0x" << alphaSize);

//         if ((img.flags & image_flag_t::ALPHA) != image_flag_t::NONE)
//         {
//             ReadAlpha(newStrm, img.bitmap);
//         }
//         else if (colorTrans)
//         {
//             ReadTransparent(img.transparent, img.bitmap);
//         }
//     }

//     image_t CreateImage(lak::memory &strm, const bool colorTrans, const bool old, const lak::color4_t palette[256])
//     {
//         DEBUG("Creating Image");
//         image_t img;
//         img.old = old;
//         DEBUG("Old: " << (old ? "true" : "false"));
//         // img.handle = strm.read_u16();
//         DEBUG("Handle: 0x" << (uint32_t)img.handle);
//         if (old)
//             img.checksum = strm.read_u16();
//         else
//             img.checksum = strm.read_u32();
//         DEBUG("Checksum: 0x" << (uint32_t)img.checksum);
//         img.reference = strm.read_u32();
//         DEBUG("Reference: 0x" << (uint32_t)img.reference);
//         img.dataSize = strm.read_u32();
//         DEBUG("Data Size: 0x" << (uint32_t)img.dataSize);
//         img.size.x = strm.read_u16();
//         img.size.y = strm.read_u16();
//         DEBUG("Size: 0x" << (uint32_t)img.size.x << " * 0x" << (uint32_t)img.size.y);
//         img.graphicsMode = (graphics_mode_t)strm.read_u8();
//         if (ColorModeSize(img.graphicsMode) == 1 && palette == nullptr)
//         {
//             ERROR("Reading 8bit Image Without Palette");
//         }
//         DEBUG("Graphics Mode: 0x" << (uint32_t)img.graphicsMode);
//         img.flags = (image_flag_t)strm.read_u8();
//         DEBUG("Flags: 0x" << (uint32_t)img.flags);
//         #if 0
//         if (img.graphicsMode <= graphics_mode_t::GRAPHICS3)
//         {
//             img.paletteEntries = strm.read_u8();
//             for (size_t i = 0; i < img.palette.size(); ++i) // where is this size coming from???
//                 palette[i] = ColorFrom32bitRGBA(strm); // not sure if RGBA or BGRA
//             img.count = strm.read_u32();
//         }
//         #endif
//         if (!old) { uint16_t unk = strm.read_u16(); DEBUG("Unknown: 0x" << (int)unk); } // padding?
//         img.hotspot.x = strm.read_u16();
//         img.hotspot.y = strm.read_u16();
//         DEBUG("Hotspot: 0x" << (uint32_t)img.hotspot.x << ", 0x" << (uint32_t)img.hotspot.y);
//         img.action.x = strm.read_u16();
//         img.action.y = strm.read_u16();
//         DEBUG("Action: 0x" << (uint32_t)img.action.x << ", 0x" << (uint32_t)img.action.y);
//         if (!old) img.transparent = ColorFrom32bitRGBA(strm);

//         _CreateImage(strm, img, colorTrans, palette);
//         return img;
//     }

//     // image_t CreateImage(lak::memory &strm, const bool colorTrans, const bool old, const lak::vec2u16_t sizeOverride)
//     // {
//     //     DEBUG("\nCreating Image");
//     //     image_t img;
//     //     img.old = old;
//     //     img.size = sizeOverride;

//     //     _CreateImage(strm, img, colorTrans);
//     //     return img;
//     // }

//     lak::glTexture_t CreateTexture(const image_t &image)
//     {
//         return CreateTexture(image.bitmap);
//     }
// }
