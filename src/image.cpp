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

#include "image.h"

namespace SourceExplorer
{
    bool operator==(const lak::color4_t &lhs, const lak::color4_t& rhs)
    {
        return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;// && lhs.a == rhs.a;
    }

    lak::color4_t ColorFrom8bit(uint8_t RGB)
    {
        return {
            (uint8_t)(((RGB >> 4) & 0x7) * (0xFF / 0x3)),    // 1110 0000
            (uint8_t)(((RGB >> 2) & 0x7) * (0xFF / 0x3)),    // 0001 1100
            (uint8_t)(((RGB >> 0) & 0x2) * (0xFF / 0x2)),    // 0000 0011
            255
        };
    }

    lak::color4_t ColorFrom15bit(uint16_t RGB)
    {
        return {
            (uint8_t)((RGB & 0x7C00) >> 7), // * (0xFF / 0x1F), // 0111 1100 0000 0000
            (uint8_t)((RGB & 0x03E0) >> 2), // * (0xFF / 0x1F), // 0000 0011 1110 0000
            (uint8_t)((RGB & 0x001F) << 3), // * (0xFF / 0x1F)  // 0000 0000 0001 1111
            255
        };
    }

    lak::color4_t ColorFrom16bit(uint16_t RGB)
    {
        return {
            (uint8_t)((RGB & 0xF800) >> 8), // * (0xFF / 0x1F), // 1111 1000 0000 0000
            (uint8_t)((RGB & 0x07E0) >> 3), // * (0xFF / 0x3F), // 0000 0111 1110 0000
            (uint8_t)((RGB & 0x001F) << 3), // * (0xFF / 0x1F)  // 0000 0000 0001 1111
            255
        };
    }

    lak::color4_t ColorFrom8bit(lak::memory &strm, const lak::color4_t palette[256])
    {
        if (palette)
            return palette[strm.read_u8()];
        return ColorFrom8bit(strm.read_u8());
    }

    lak::color4_t ColorFrom15bit(lak::memory &strm)
    {
        uint16_t val = strm.read_u8();
        val |= (uint16_t)strm.read_u8() << 8;
        return ColorFrom15bit(val);
    }

    lak::color4_t ColorFrom16bit(lak::memory &strm)
    {
        uint16_t val = strm.read_u8();
        val |= (uint16_t)strm.read_u8() << 8;
        return ColorFrom16bit(val);
    }

    lak::color4_t ColorFrom24bitBGR(lak::memory &strm)
    {
        lak::color4_t rtn;
        rtn.b = strm.read_u8();
        rtn.g = strm.read_u8();
        rtn.r = strm.read_u8();
        rtn.a = 255;
        return rtn;
    }

    lak::color4_t ColorFrom32bitBGRA(lak::memory &strm)
    {
        lak::color4_t rtn;
        rtn.b = strm.read_u8();
        rtn.g = strm.read_u8();
        rtn.r = strm.read_u8();
        rtn.a = strm.read_u8();
        return rtn;
    }

    lak::color4_t ColorFrom24bitRGB(lak::memory &strm)
    {
        lak::color4_t rtn;
        rtn.r = strm.read_u8();
        rtn.g = strm.read_u8();
        rtn.b = strm.read_u8();
        rtn.a = 255;
        return rtn;
    }

    lak::color4_t ColorFrom32bitRGBA(lak::memory &strm)
    {
        lak::color4_t rtn;
        rtn.r = strm.read_u8();
        rtn.g = strm.read_u8();
        rtn.b = strm.read_u8();
        rtn.a = strm.read_u8();
        return rtn;
    }

    lak::color4_t ColorFromMode(lak::memory &strm, const graphics_mode_t mode, const lak::color4_t palette[256])
    {
        switch(mode)
        {
            case graphics_mode_t::GRAPHICS2:
            case graphics_mode_t::GRAPHICS3:
                return ColorFrom8bit(strm, palette);
            case graphics_mode_t::GRAPHICS6:
                return ColorFrom15bit(strm);
            case graphics_mode_t::GRAPHICS7:
                return ColorFrom16bit(strm);
                // return ColorFrom15bit(strm);
            case graphics_mode_t::GRAPHICS4:
            default:
                // return ColorFrom32bitBGRA(strm);
                return ColorFrom24bitBGR(strm);
        }
    }

    uint8_t ColorModeSize(const graphics_mode_t mode)
    {
        switch(mode)
        {
            case graphics_mode_t::GRAPHICS2:
            case graphics_mode_t::GRAPHICS3:
                return 1;
            case graphics_mode_t::GRAPHICS6:
                return 2;
            case graphics_mode_t::GRAPHICS7:
                return 2;
            case graphics_mode_t::GRAPHICS4:
            default:
                // return 4;
                return 3;
        }
    }

    uint16_t BitmapPaddingSize(uint16_t width, uint8_t colSize, uint8_t bytes)
    {
        uint16_t num = bytes - ((width * colSize) % bytes);
        return (uint16_t)std::ceil((double)(num == bytes ? 0 : num) / (double)colSize);
    }

    image_t &image_t::initTexture()
    {
        DEBUG("Generating Texture");
        texture = CreateTexture(bitmap);
        return *this;
    }

    size_t ReadRLE(lak::memory &strm, lak::image4_t &bitmap, graphics_mode_t mode, const lak::color4_t palette[256] = nullptr)
    {
        const size_t pointSize = ColorModeSize(mode);
        const uint16_t pad = BitmapPaddingSize(bitmap.size().x, pointSize);
        size_t pos = 0;
        size_t i = 0;

        size_t start = strm.position;

        while(true)
        {
            uint8_t command = strm.read_u8();

            if (command == 0)
                break;

            if (command > 128)
            {
                command -= 128;
                for (uint8_t n = 0; n < command; ++n)
                {
                    if ((pos++) % (bitmap.size().x + pad) < bitmap.size().x)
                        bitmap[i++] = ColorFromMode(strm, mode);
                    else
                        strm.position += pointSize;
                }
            }
            else
            {
                lak::color4_t col = ColorFromMode(strm, mode);
                for (uint8_t n = 0; n < command; ++n)
                {
                    if ((pos++) % (bitmap.size().x + pad) < bitmap.size().x)
                        bitmap[i++] = col;
                }
            }
        }

        return strm.position - start;
    }

    size_t ReadRGB(lak::memory &strm, lak::image4_t &bitmap, graphics_mode_t mode, const lak::color4_t palette[256] = nullptr)
    {
        const size_t pointSize = ColorModeSize(mode);
        const uint16_t pad = BitmapPaddingSize(bitmap.size().x, pointSize);
        size_t i = 0;

        size_t start = strm.position;

        for (size_t y = 0; y < bitmap.size().y; ++y)
        {
            for (size_t x = 0; x < bitmap.size().x; ++x)
            {
                bitmap[i++] = ColorFromMode(strm, mode, palette);
            }
            strm.position += pad * pointSize;
        }

        return strm.position - start;
    }

    void ReadAlpha(lak::memory &strm, lak::image4_t &bitmap)
    {
        const uint16_t pad = BitmapPaddingSize(bitmap.size().x, 1, 4);
        size_t i = 0;

        for (size_t y = 0; y < bitmap.size().y; ++y)
        {
            for (size_t x = 0; x < bitmap.size().x; ++x)
            {
                bitmap[i++].a = strm.read_u8();
            }
            strm.position += pad;
        }
    }

    void ReadTransparent(lak::color4_t &transparent, lak::image4_t &bitmap)
    {
        for (size_t i = 0; i < bitmap.contig_size(); ++i)
        {
            if (bitmap[i] == transparent)
                bitmap[i].a = transparent.a;
        }
    }

    void _CreateImage(lak::memory &strm, image_t &img, const bool colorTrans, const lak::color4_t palette[256] = nullptr)
    {
        img.bitmap.resize(img.size);

        uint8_t colSize = ColorModeSize(img.graphicsMode);
        DEBUG("Color Size: 0x" << (uint32_t)colSize);
        uint16_t pad = BitmapPaddingSize(img.size.x, colSize);
        DEBUG("Bitmap Padding: 0x" << (uint32_t)pad);

        lak::memory newStrm;
        if ((img.flags & image_flag_t::LZX) != image_flag_t::NONE)
        {
            uint32_t decompressed = strm.read_u32(); (void)decompressed;
            newStrm = Inflate(strm.read(strm.read_u32())); // is this right?
        }
        else
        {
            newStrm = strm;
        }

        size_t alphaSize = 0;
        if ((img.flags & (image_flag_t::RLE | image_flag_t::RLEW | image_flag_t::RLET)) != image_flag_t::NONE)
        {
            size_t bytesRead = ReadRLE(newStrm, img.bitmap, img.graphicsMode, palette);
            DEBUG("Bytes Read: 0x" << bytesRead);
            alphaSize = img.dataSize - bytesRead;
        }
        else
        {
            size_t bytesRead = ReadRGB(newStrm, img.bitmap, img.graphicsMode, palette);
            DEBUG("Bytes Read: 0x" << bytesRead);
            alphaSize = img.dataSize - bytesRead;
        }
        DEBUG("Alpha Size: 0x" << alphaSize);

        if ((img.flags & image_flag_t::ALPHA) != image_flag_t::NONE)
        {
            ReadAlpha(newStrm, img.bitmap);
        }
        else if (colorTrans)
        {
            ReadTransparent(img.transparent, img.bitmap);
        }
    }

    image_t CreateImage(lak::memory &strm, const bool colorTrans, const bool old, const lak::color4_t palette[256])
    {
        DEBUG("\nCreating Image");
        image_t img;
        img.old = old;
        DEBUG("Old: " << (old ? "true" : "false"));
        // img.handle = strm.read_u16();
        DEBUG("Handle: 0x" << (uint32_t)img.handle);
        if (old)
            img.checksum = strm.read_u16();
        else
            img.checksum = strm.read_u32();
        DEBUG("Checksum: 0x" << (uint32_t)img.checksum);
        img.reference = strm.read_u32();
        DEBUG("Reference: 0x" << (uint32_t)img.reference);
        img.dataSize = strm.read_u32();
        DEBUG("Data Size: 0x" << (uint32_t)img.dataSize);
        img.size.x = strm.read_u16();
        img.size.y = strm.read_u16();
        DEBUG("Size: 0x" << (uint32_t)img.size.x << " * 0x" << (uint32_t)img.size.y);
        img.graphicsMode = (graphics_mode_t)strm.read_u8();
        DEBUG("Graphics Mode: 0x" << (uint32_t)img.graphicsMode);
        img.flags = (image_flag_t)strm.read_u8();
        DEBUG("Flags: 0x" << (uint32_t)img.flags);
        #if 0
        if (img.graphicsMode <= graphics_mode_t::GRAPHICS3)
        {
            img.paletteEntries = strm.read_u8();
            for (size_t i = 0; i < img.palette.size(); ++i) // where is this size coming from???
                palette[i] = ColorFrom32bitRGBA(strm); // not sure if RGBA or BGRA
            img.count = strm.read_u32();
        }
        #endif
        if (!old) DEBUG("Unknown: 0x" << (int)strm.read_u16()); // padding?
        img.hotspot.x = strm.read_u16();
        img.hotspot.y = strm.read_u16();
        DEBUG("Hotspot: 0x" << (uint32_t)img.hotspot.x << ", 0x" << (uint32_t)img.hotspot.y);
        img.action.x = strm.read_u16();
        img.action.y = strm.read_u16();
        DEBUG("Action: 0x" << (uint32_t)img.action.x << ", 0x" << (uint32_t)img.action.y);
        if (!old) img.transparent = ColorFrom32bitRGBA(strm);

        _CreateImage(strm, img, colorTrans, palette);
        return img;
    }

    // image_t CreateImage(lak::memory &strm, const bool colorTrans, const bool old, const lak::vec2u16_t sizeOverride)
    // {
    //     DEBUG("\nCreating Image");
    //     image_t img;
    //     img.old = old;
    //     img.size = sizeOverride;

    //     _CreateImage(strm, img, colorTrans);
    //     return img;
    // }

    lak::glTexture_t CreateTexture(const lak::image4_t &bitmap)
    {
        GLuint tex;
        glGenTextures(1, &tex);

        glBindTexture(GL_TEXTURE_2D, tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)bitmap.size().x, (GLsizei)bitmap.size().y,
            0, GL_RGBA, GL_UNSIGNED_BYTE, &(bitmap[0].r));

        return lak::glTexture_t(std::move(tex), bitmap.size());
    }

    lak::glTexture_t CreateTexture(const image_t &image)
    {
        return CreateTexture(image.bitmap);
    }

    void ViewImage(const lak::glTexture_t &texture, const float scale)
    {
        ImGui::Image((ImTextureID)(uintptr_t)texture.get(),
            ImVec2(scale * (float)texture.size().x, scale * (float)texture.size().y));
    }
}
