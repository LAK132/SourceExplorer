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

    lak::color4_t ColorFrom8bit(lak::memstrm_t &strm)
    {
        return ColorFrom8bit(strm.readInt<uint8_t>());
    }

    lak::color4_t ColorFrom15bit(lak::memstrm_t &strm)
    {
        uint16_t val = strm.readInt<uint8_t>();
        val |= (uint16_t)strm.readInt<uint8_t>() << 8;
        return ColorFrom15bit(val);
    }

    lak::color4_t ColorFrom16bit(lak::memstrm_t &strm)
    {
        uint16_t val = strm.readInt<uint8_t>();
        val |= (uint16_t)strm.readInt<uint8_t>() << 8;
        return ColorFrom16bit(val);
    }

    lak::color4_t ColorFrom24bitBGR(lak::memstrm_t &strm)
    {
        lak::color4_t rtn;
        rtn.b = strm.readInt<uint8_t>();
        rtn.g = strm.readInt<uint8_t>();
        rtn.r = strm.readInt<uint8_t>();
        rtn.a = 255;
        return rtn;
    }

    lak::color4_t ColorFrom32bitBGRA(lak::memstrm_t &strm)
    {
        lak::color4_t rtn;
        rtn.b = strm.readInt<uint8_t>();
        rtn.g = strm.readInt<uint8_t>();
        rtn.r = strm.readInt<uint8_t>();
        rtn.a = strm.readInt<uint8_t>();
        return rtn;
    }

    lak::color4_t ColorFrom24bitRGB(lak::memstrm_t &strm)
    {
        lak::color4_t rtn;
        rtn.r = strm.readInt<uint8_t>();
        rtn.g = strm.readInt<uint8_t>();
        rtn.b = strm.readInt<uint8_t>();
        rtn.a = 255;
        return rtn;
    }

    lak::color4_t ColorFrom32bitRGBA(lak::memstrm_t &strm)
    {
        lak::color4_t rtn;
        rtn.r = strm.readInt<uint8_t>();
        rtn.g = strm.readInt<uint8_t>();
        rtn.b = strm.readInt<uint8_t>();
        rtn.a = strm.readInt<uint8_t>();
        return rtn;
    }

    lak::color4_t ColorFromMode(lak::memstrm_t &strm, const graphics_mode_t mode)
    {
        switch(mode)
        {
            case GRAPHICS2:
            case GRAPHICS3:
                return ColorFrom8bit(strm);
            case GRAPHICS6:
                return ColorFrom15bit(strm);
            case GRAPHICS7:
                return ColorFrom16bit(strm);
                // return ColorFrom15bit(strm);
            case GRAPHICS4:
            default:
                // return ColorFrom32bitBGRA(strm);
                return ColorFrom24bitBGR(strm);
        }
    }

    uint8_t ColorModeSize(const graphics_mode_t mode)
    {
        switch(mode)
        {
            case GRAPHICS2:
            case GRAPHICS3:
                return 1;
            case GRAPHICS6:
                return 2;
            case GRAPHICS7:
                return 2;
            case GRAPHICS4:
            default:
                // return 4;
                return 3;
        }
    }

    lak::color4_t &bitmap_t::operator[](const size_t index)
    {
        return pixels[index];
    }

    lak::color4_t &bitmap_t::operator[](const lak::vec2s_t index)
    {
        return pixels[(index.y * size.x) + index.x];
    }

    lak::color4_t &bitmap_t::operator()(const size_t x, const size_t y)
    {
        return operator[]({x, y});
    }

    void bitmap_t::resize(const lak::vec2s_t toSize)
    {
        size = toSize;
        pixels.resize(size.x * size.y);
    }

    uint16_t BitmapPaddingSize(uint16_t width, uint8_t colSize, uint8_t bytes)
    {
        uint16_t num = bytes - ((width * colSize) % bytes);
        return (uint16_t)std::ceil((double)(num == bytes ? 0 : num) / (double)colSize);
    }

    image_t &image_t::initTexture()
    {
        DEBUG("Generating Texture");
        texture = lak::glTexture_t({bitmap.size.x, bitmap.size.y});

        glBindTexture(GL_TEXTURE_2D, texture.get());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)bitmap.size.x, (GLsizei)bitmap.size.y,
            0, GL_RGBA, GL_UNSIGNED_BYTE, &(bitmap.pixels[0].r));

        return *this;
    }

    size_t ReadRLE(lak::memstrm_t &strm, bitmap_t &bitmap, graphics_mode_t mode)
    {
        const size_t pointSize = ColorModeSize(mode);
        const uint16_t pad = BitmapPaddingSize(bitmap.size.x, pointSize);
        size_t pos = 0;
        size_t i = 0;

        size_t start = strm.position;

        while(true)
        {
            uint8_t command = strm.readInt<uint8_t>();

            if (command == 0)
                break;

            if (command > 128)
            {
                command -= 128;
                for (uint8_t n = 0; n < command; ++n)
                {
                    if ((pos++) % (bitmap.size.x + pad) < bitmap.size.x)
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
                    if ((pos++) % (bitmap.size.x + pad) < bitmap.size.x)
                        bitmap[i++] = col;
                }
            }
        }

        return strm.position - start;
    }

    size_t ReadRGB(lak::memstrm_t &strm, bitmap_t &bitmap, graphics_mode_t mode)
    {
        const size_t pointSize = ColorModeSize(mode);
        const uint16_t pad = BitmapPaddingSize(bitmap.size.x, pointSize);
        size_t i = 0;

        size_t start = strm.position;

        for (size_t y = 0; y < bitmap.size.y; ++y)
        {
            for (size_t x = 0; x < bitmap.size.x; ++x)
            {
                bitmap[i++] = ColorFromMode(strm, mode);
            }
            strm.position += pad * pointSize;
        }

        return strm.position - start;
    }

    void ReadAlpha(lak::memstrm_t &strm, bitmap_t &bitmap)
    {
        const uint16_t pad = BitmapPaddingSize(bitmap.size.x, 1, 4);
        size_t i = 0;

        for (size_t y = 0; y < bitmap.size.y; ++y)
        {
            for (size_t x = 0; x < bitmap.size.x; ++x)
            {
                bitmap[i++].a = strm.readInt<uint8_t>();
            }
            strm.position += pad;
        }
    }

    void ReadTransparent(lak::color4_t &transparent, bitmap_t &bitmap)
    {
        for (size_t i = 0; i < bitmap.pixels.size(); ++i)
        {
            if (bitmap[i] == transparent)
                bitmap[i].a = transparent.a;
        }
    }

    void _CreateImage(lak::memstrm_t &strm, image_t &img, const bool colorTrans)
    {
        img.bitmap.resize(img.size);

        uint8_t colSize = ColorModeSize(img.graphicsMode);
        DEBUG("Color Size: 0x" << (uint32_t)colSize);
        uint16_t pad = BitmapPaddingSize(img.size.x, colSize);
        DEBUG("Bitmap Padding: 0x" << (uint32_t)pad);

        lak::memstrm_t newStrm;
        if (img.flags & image_flag_t::LZX)
        {
            uint32_t decompressed = strm.readInt<uint32_t>(); (void)decompressed;
            newStrm = Inflate(strm.readBytes(strm.readInt<uint32_t>())); // is this right?
        }
        else
        {
            newStrm = strm;
        }

        size_t alphaSize = 0;
        if (img.flags & (image_flag_t::RLE | image_flag_t::RLEW | image_flag_t::RLET))
        {
            size_t bytesRead = ReadRLE(newStrm, img.bitmap, img.graphicsMode);
            DEBUG("Bytes Read: 0x" << bytesRead);
            alphaSize = img.dataSize - bytesRead;
        }
        else
        {
            size_t bytesRead = ReadRGB(newStrm, img.bitmap, img.graphicsMode);
            DEBUG("Bytes Read: 0x" << bytesRead);
            alphaSize = img.dataSize - bytesRead;
        }
        DEBUG("Alpha Size: 0x" << alphaSize);

        if (img.flags & image_flag_t::ALPHA)
        {
            ReadAlpha(newStrm, img.bitmap);
        }
        else if (colorTrans)
        {
            ReadTransparent(img.transparent, img.bitmap);
        }
    }

    image_t CreateImage(lak::memstrm_t &strm, const bool colorTrans, const bool old)
    {
        DEBUG("\nCreating Image");
        image_t img;
        img.old = old;
        DEBUG("Old: " << (old ? "true" : "false"));
        // img.handle = strm.readInt<uint16_t>();
        DEBUG("Handle: 0x" << (uint32_t)img.handle);
        if (old)
            img.checksum = strm.readInt<uint16_t>();
        else
            img.checksum = strm.readInt<uint32_t>();
        DEBUG("Checksum: 0x" << (uint32_t)img.checksum);
        img.reference = strm.readInt<uint32_t>();
        DEBUG("Reference: 0x" << (uint32_t)img.reference);
        img.dataSize = strm.readInt<uint32_t>();
        DEBUG("Data Size: 0x" << (uint32_t)img.dataSize);
        img.size.x = strm.readInt<uint16_t>();
        img.size.y = strm.readInt<uint16_t>();
        DEBUG("Size: 0x" << (uint32_t)img.size.x << " * 0x" << (uint32_t)img.size.y);
        img.graphicsMode = strm.readInt<graphics_mode_t>();
        DEBUG("Graphics Mode: 0x" << (uint32_t)img.graphicsMode);
        img.flags = strm.readInt<image_flag_t>();
        DEBUG("Flags: 0x" << (uint32_t)img.flags);
        #if 0
        if (img.graphicsMode <= graphics_mode_t::GRAPHICS3)
        {
            img.paletteEntries = strm.readInt<uint8_t>();
            for (size_t i = 0; i < img.palette.size(); ++i) // where is this size coming from???
                palette[i] = ColorFrom32bitRGBA(strm); // not sure if RGBA or BGRA
            img.count = strm.readInt<uint32_t>();
        }
        #endif
        if (!old) strm.readInt<uint16_t>(); // padding?
        img.hotspot.x = strm.readInt<uint16_t>();
        img.hotspot.y = strm.readInt<uint16_t>();
        DEBUG("Hotspot: 0x" << (uint32_t)img.hotspot.x << " * 0x" << (uint32_t)img.hotspot.y);
        img.action.x = strm.readInt<uint16_t>();
        img.action.y = strm.readInt<uint16_t>();
        DEBUG("Action: 0x" << (uint32_t)img.action.x << " * 0x" << (uint32_t)img.action.y);
        if (!old) img.transparent = ColorFrom32bitRGBA(strm);

        _CreateImage(strm, img, colorTrans);
        return img;
    }

    image_t CreateImage(lak::memstrm_t &strm, const bool colorTrans, const bool old, const lak::vec2u16_t sizeOverride)
    {
        DEBUG("\nCreating Image");
        image_t img;
        img.old = old;
        img.size = sizeOverride;

        _CreateImage(strm, img, colorTrans);
        return img;
    }

    GLuint CreateTexture(const image_t &image)
    {
        DEBUG("Creating Texture");
        GLuint tex;
        glGenTextures(1, &tex);

        glBindTexture(GL_TEXTURE_2D, tex);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)image.bitmap.size.x, (GLsizei)image.bitmap.size.y,
            0, GL_RGBA, GL_UNSIGNED_BYTE, &(image.bitmap.pixels[0].r));

        return tex;
    }

    void ViewImage(const image_t &img)
    {
        ImGui::Image((ImTextureID)(uintptr_t)img.texture.get(),
            ImVec2((float)img.texture.size().x, (float)img.texture.size().y));
    }
}
