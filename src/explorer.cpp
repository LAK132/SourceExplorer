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

#include "explorer.h"

namespace SourceExplorer
{
    bool debugConsole = true;
    bool developerConsole = false;
    bool errorOnlyConsole = false;
    bool forceCompat = false;
    m128i_t _xmmword;
    std::vector<uint8_t> _magic_key;
    game_mode_t _mode = game_mode_t::_OLD;
    uint8_t _magic_char;


    std::vector<uint8_t> &operator += (std::vector<uint8_t> &lhs, const std::vector<uint8_t> &rhs)
    {
        lhs.insert(lhs.cend(), rhs.cbegin(), rhs.cend());
        return lhs;
    }

    error_t LoadGame(source_explorer_t &srcexp)
    {
        DEBUG("Loading Game");

        srcexp.state = game_t{};
        srcexp.state.compat = forceCompat;

        srcexp.state.file = lak::LoadFile(srcexp.exe.path);

        DEBUG("File Size: 0x" << srcexp.state.file.size());

        error_t err = ParsePEHeader(srcexp.state.file, srcexp.state);
        if (err != error_t::OK)
        {
            ERROR("Error 0x" << (uint32_t)err << " While Parsing PE Header" <<
                ", At: 0x" << srcexp.state.file.position);
            return err;
        }

        DEBUG("Successfully Parsed PE Header");

        _magic_key.clear();

        _xmmword.m128i_u32[0] = 0;
        _xmmword.m128i_u32[1] = 1;
        _xmmword.m128i_u32[2] = 2;
        _xmmword.m128i_u32[3] = 3;

        if (srcexp.state.productBuild < 284 ||
            srcexp.state.oldGame ||
            srcexp.state.compat)
            _mode = game_mode_t::_OLD;
        else if (srcexp.state.productBuild > 284)
            _mode = game_mode_t::_288;
        else
            _mode = game_mode_t::_284;

        if (_mode == game_mode_t::_OLD)
            _magic_char = 99; // '6';
        else
            _magic_char = 54; // 'c';

        err = srcexp.state.game.read(srcexp.state, srcexp.state.file);
        if (err != error_t::OK)
        {
            ERROR("Error Reading Entry: Error Code 0x" << (uint32_t)err <<
                ", At: 0x" << srcexp.state.file.position);
        }
        else
        {
            DEBUG("Successfully Read Game Entry");
        }

        DEBUG("Unicode: " << (srcexp.state.unicode ? "true" : "false"));

        if (srcexp.state.game.projectPath)
            srcexp.state.project = srcexp.state.game.projectPath->value;

        if (srcexp.state.game.title)
            srcexp.state.title = srcexp.state.game.title->value;

        if (srcexp.state.game.copyright)
            srcexp.state.copyright = srcexp.state.game.copyright->value;

        DEBUG("Project Path: " << lak::strconv<char>(srcexp.state.project));
        DEBUG("Title: " << lak::strconv<char>(srcexp.state.title));
        DEBUG("Copyright: " << lak::strconv<char>(srcexp.state.copyright));

        if (srcexp.state.game.imageBank)
        {
            const auto &images = srcexp.state.game.imageBank->items;
            for (size_t i = 0; i < images.size(); ++i)
            {
                srcexp.state.imageHandles[images[i].entry.handle] = i;
            }
        }

        if (srcexp.state.game.objectBank)
        {
            const auto &objects = srcexp.state.game.objectBank->items;
            for (size_t i = 0; i < objects.size(); ++i)
            {
                srcexp.state.objectHandles[objects[i].handle] = i;
            }
        }

        return err;
    }

    void GetEncryptionKey(game_t &gameState)
    {
        _magic_key.clear();
        _magic_key.reserve(256);

        if (_mode == game_mode_t::_284)
        {
            if (gameState.game.projectPath)
                _magic_key += KeyString(gameState.game.projectPath->value);
            if (_magic_key.size() < 0x80 && gameState.game.title)
                _magic_key += KeyString(gameState.game.title->value);
            if (_magic_key.size() < 0x80 && gameState.game.copyright)
                _magic_key += KeyString(gameState.game.copyright->value);
        }
        else
        {
            if (gameState.game.title)
                _magic_key += KeyString(gameState.game.title->value);
            if (_magic_key.size() < 0x80 && gameState.game.copyright)
                _magic_key += KeyString(gameState.game.copyright->value);
            if (_magic_key.size() < 0x80 && gameState.game.projectPath)
                _magic_key += KeyString(gameState.game.projectPath->value);
        }
        _magic_key.resize(0x100);
        std::memset(&_magic_key[0x80], 0, 0x80);

        uint8_t *keyPtr = &(_magic_key[0]);
        memset(keyPtr + 128, 0, 0x80U);
        size_t len = strlen((char *)keyPtr);
        uint8_t accum = _magic_char;
        uint8_t hash = _magic_char;
        for (size_t i = 0; i <= len; ++i)
        {
            hash = (hash << 7) + (hash >> 1);
            *keyPtr ^= hash;
            accum += *keyPtr * ((hash & 1) + 2);
            ++keyPtr;
        }
        *keyPtr = accum;
    }

    error_t ParsePEHeader(lak::memory &strm, game_t &gameState)
    {
        DEBUG("Parsing PE header");

        uint16_t exeSig = strm.read_u16();
        DEBUG("EXE Signature: 0x" << exeSig);
        if (exeSig != WIN_EXE_SIG)
        {
            ERROR("Invalid EXE Signature, Expected: 0x" << WIN_EXE_SIG <<
                ", At: 0x" << (strm.position - 2));
            return error_t::INVALID_EXE_SIGNATURE;
        }

        strm.position = WIN_EXE_PNT;
        strm.position = strm.read_u16();
        DEBUG("EXE Pointer: 0x" << strm.position <<
            ", At: 0x" << (size_t)WIN_EXE_PNT);

        int32_t peSig = strm.read_s32();
        DEBUG("PE Signature: 0x" << peSig);
        if (peSig != WIN_PE_SIG)
        {
            ERROR("Invalid PE Signature, Expected: 0x" << WIN_PE_SIG <<
                ", At: 0x" << (strm.position - 4));
            return error_t::INVALID_PE_SIGNATURE;
        }

        strm.position += 2;

        uint16_t numHeaderSections = strm.read_u16();
        DEBUG("Number Of Header Sections: " << numHeaderSections);

        strm.position += 16;

        const uint16_t optionalHeader = 0x60;
        const uint16_t dataDir = 0x80;
        strm.position += optionalHeader + dataDir;
        DEBUG("Pos: 0x" << strm.position);

        uint64_t pos = 0;
        for (uint16_t i = 0; i < numHeaderSections; ++i)
        {
            uint64_t start = strm.position;
            std::string name = strm.read_string();
            DEBUG("Name: " << name);
            if (name == ".extra")
            {
                strm.position = start + 0x14;
                pos = strm.read_s32();
                break;
            }
            else if (i >= numHeaderSections - 1)
            {
                strm.position = start + 0x10;
                uint32_t size = strm.read_u32();
                uint32_t addr = strm.read_u32();
                DEBUG("Size: 0x" << size);
                DEBUG("Addr: 0x" << addr);
                pos = size + addr;
                break;
            }
            strm.position = start + 0x28;
            DEBUG("Pos: 0x" << strm.position);
        }

        while (true)
        {
            strm.position = pos;
            uint16_t firstShort = strm.read_u16();
            DEBUG("First Short: 0x" << firstShort);
            strm.position = pos;
            uint32_t pameMagic = strm.read_u32();
            DEBUG("PAME Magic: 0x" << pameMagic);
            strm.position = pos;
            uint64_t packMagic = strm.read_u64();
            DEBUG("Pack Magic: 0x" << packMagic);
            strm.position = pos;
            DEBUG("Pos: 0x" << pos);

            if (firstShort == (uint16_t)chunk_t::HEADER || pameMagic == HEADER_GAME)
            {
                DEBUG("Old Game");
                gameState.oldGame = true;
                gameState.state = {};
                gameState.state.push(chunk_t::OLD);
                break;
            }
            else if (packMagic == HEADER_PACK)
            {
                DEBUG("New Game");
                gameState.oldGame = false;
                gameState.state = {};
                gameState.state.push(chunk_t::NEW);
                pos = ParsePackData(strm, gameState);
                break;
            }
            else if (firstShort == 0x222C)
            {
                strm.position += 4;
                strm.position += strm.read_u32();
                pos = strm.position;
            }
            else if (firstShort == 0x7F7F)
            {
                pos += 8;
            }
            else
            {
                ERROR("Invalid Game Header");
                return error_t::INVALID_GAME_HEADER;
            }

            if (pos > strm.size())
            {
                ERROR("Invalid Game Header: pos (0x" << pos << ") > strm.size (0x" << strm.size() << ")");
                return error_t::INVALID_GAME_HEADER;
            }
        }

        uint32_t header = strm.read_u32();
        DEBUG("Header: 0x" << header);

        gameState.unicode = false;
        if (header == HEADER_UNIC)
        {
            gameState.unicode = true;
            gameState.oldGame = false;
        }
        else if (header != HEADER_GAME)
        {
            ERROR("Invalid Game Header: 0x" << header <<
                ", Expected: 0x" << HEADER_GAME <<
                ", At: 0x" << (strm.position - 4));
            return error_t::INVALID_GAME_HEADER;
        }

        do {
            gameState.runtimeVersion = (product_code_t)strm.read_u16();
            DEBUG("Runtime Version: 0x" << (int)gameState.runtimeVersion);
            if (gameState.runtimeVersion == product_code_t::CNCV1VER)
            {
                DEBUG("CNCV1VER");
                // cnc = true;
                // readCNC(strm);
                break;
            }
            gameState.runtimeSubVersion = strm.read_u16();
            DEBUG("Runtime Sub-Version: 0x" << gameState.runtimeSubVersion);
            gameState.productVersion = strm.read_u32();
            DEBUG("Product Version: 0x" << gameState.productVersion);
            gameState.productBuild = strm.read_u32();
            DEBUG("Product Build: 0x" << gameState.productBuild);
        } while(0);

        return error_t::OK;
    }

    uint64_t ParsePackData(lak::memory &strm, game_t &gameState)
    {
        DEBUG("Parsing pack data");

        uint64_t start = strm.position;
        uint64_t header = strm.read_u64();
        DEBUG("Header: 0x" << header);
        uint32_t headerSize = strm.read_u32(); (void)headerSize;
        DEBUG("Header Size: 0x" << headerSize);
        uint32_t dataSize = strm.read_u32();
        DEBUG("Data Size: 0x" << dataSize);

        strm.position = start + dataSize - 0x20;

        header = strm.read_u32();
        DEBUG("Head: 0x" << header);
        bool unicode = header == HEADER_UNIC;
        if (unicode)
        {
            DEBUG("Unicode Game");
        }
        else
        {
            DEBUG("ASCII Game");
        }

        strm.position = start + 0x10;

        uint32_t formatVersion = strm.read_u32(); (void) formatVersion;
        DEBUG("Format Version: 0x" << formatVersion);

        strm.position += 0x8;

        int32_t count = strm.read_s32();
        assert(count >= 0);
        DEBUG("Pack Count: 0x" << count);

        uint64_t off = strm.position;
        DEBUG("Offset: 0x" << off);

        for (int32_t i = 0; i < count; ++i)
        {
            if ((strm.size() - strm.position) < 2)
                break;

            uint32_t val = strm.read_u16();
            if ((strm.size() - strm.position) < val)
                break;

            strm.position += val;
            if ((strm.size() - strm.position) < 4)
                break;

            val = strm.read_u32();
            if ((strm.size() - strm.position) < val)
                break;

            strm.position += val;
        }

        header = strm.read_u32();
        DEBUG("Header: 0x" << header);

        bool hasBingo = (header != HEADER_GAME) && (header != HEADER_UNIC);
        DEBUG("Has Bingo: " << (hasBingo ? "true" : "false"));

        strm.position = off;

        gameState.packFiles.resize(count);

        for (int32_t i = 0; i < count; ++i)
        {
            uint32_t read = strm.read_u16();
            // size_t strstart = strm.position;

            DEBUG("Pack 0x" << i+1 << " of 0x" << count << ", filename length: 0x" << read << ", pos: 0x" << strm.position);

            if (unicode)
                gameState.packFiles[i].filename = strm.read_u16string(read);
            else
                gameState.packFiles[i].filename = lak::strconv<char16_t>(strm.read_string(read));

            // strm.position = strstart + (unicode ? read * 2 : read);

            // DEBUG("String Start: 0x" << strstart);
            WDEBUG(L"Packfile '" << lak::strconv<wchar_t>(gameState.packFiles[i].filename) << L"'");
            DEBUG("Packfile '" << lak::strconv<char>(gameState.packFiles[i].filename) << "'");

            if (hasBingo)
                gameState.packFiles[i].bingo = strm.read_u32();
            else
                gameState.packFiles[i].bingo = 0;

            DEBUG("Bingo: 0x" << gameState.packFiles[i].bingo);

            // if (unicode)
            //     read = strm.read_u32();
            // else
            //     read = strm.read_u16();
            read = strm.read_u32();

            DEBUG("Pack File Data Size: 0x" << read << ", Pos: 0x" << strm.position);
            gameState.packFiles[i].data = strm.read(read);
        }

        header = strm.read_u32(); // PAMU sometimes
        DEBUG("Header: 0x" << header);

        if (header == HEADER_GAME || header == HEADER_UNIC)
        {
            uint32_t pos = (uint32_t)strm.position;
            strm.position -= 0x4;
            return pos;
        }
        return strm.position;
    }

    std::u16string ReadString(const resource_entry_t &entry, const bool unicode)
    {
        if (entry.data.data.size() > 0)
        {
            auto decode = entry.decode();
            lak::memory strm(decode);
            if (unicode)
                return strm.read_u16string();
            else
                return lak::strconv<char16_t>(strm.read_string());
        }
        else
        {
            auto decode = entry.decodeHeader();
            lak::memory strm(decode);
            if (unicode)
                return strm.read_u16string();
            else
                return lak::strconv<char16_t>(strm.read_string());
        }
    }

    error_t ReadFixedData(lak::memory &strm, data_point_t &data, const size_t size)
    {
        data.position = strm.position;
        data.expectedSize = 0;
        data.data = strm.read(size);
        return error_t::OK;
    }

    error_t ReadDynamicData(lak::memory &strm, data_point_t &data)
    {
        return ReadFixedData(strm, data, strm.read_u32());
    }

    error_t ReadCompressedData(lak::memory &strm, data_point_t &data)
    {
        data.position = strm.position;
        data.expectedSize = strm.read_u32();
        data.data = strm.read(strm.read_u32());
        return error_t::OK;
    }

    error_t ReadRevCompressedData(lak::memory &strm, data_point_t &data)
    {
        data.position = strm.position;
        uint32_t compressed = strm.read_u32();
        data.expectedSize = strm.read_u32();
        if (compressed >= 4)
        {
            data.data = strm.read(compressed - 4);
            return error_t::OK;
        }
        data.data = lak::memory{};
        return error_t::OK;
    }

    error_t ReadStreamCompressedData(lak::memory &strm, data_point_t &data)
    {
        data.position = strm.position;
        data.expectedSize = strm.read_u32();
        size_t start = strm.position;
        StreamDecompress(strm, data.expectedSize);
        data.data = strm.read_range(start);
        return error_t::OK;
    }

    lak::color4_t ColorFrom8bit(uint8_t RGB)
    {
        return { RGB, RGB, RGB, 255 };
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

    uint16_t BitmapPaddingSize(uint16_t width, uint8_t colSize, uint8_t bytes = 2)
    {
        uint16_t num = bytes - ((width * colSize) % bytes);
        return (uint16_t)std::ceil((double)(num == bytes ? 0 : num) / (double)colSize);
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
                        bitmap[i++] = ColorFromMode(strm, mode, palette);
                    else
                        strm.position += pointSize;
                }
            }
            else
            {
                lak::color4_t col = ColorFromMode(strm, mode, palette);
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

    void ReadTransparent(const lak::color4_t &transparent, lak::image4_t &bitmap)
    {
        for (size_t i = 0; i < bitmap.contig_size(); ++i)
        {
            if (lak::color3_t(bitmap[i]) == lak::color3_t(transparent))
                bitmap[i].a = transparent.a;
        }
    }

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

    void ViewImage(source_explorer_t &srcexp, const float scale)
    {
        // :TODO: Select palette
        ImGui::Image((ImTextureID)(uintptr_t)srcexp.image.get(),
            ImVec2(scale * (float)srcexp.image.size().x,
                   scale * (float)srcexp.image.size().y));
    }

    const char *GetTypeString(const resource_entry_t &entry)
    {
        switch (entry.parent)
        {
            case chunk_t::IMAGEBANK: return "Image";
            case chunk_t::SOUNDBANK: return "Sound";
            case chunk_t::MUSICBANK: return "Music";
            case chunk_t::FONTBANK: return "Font";
            default: break;
        }
        switch(entry.ID)
        {
            case chunk_t::ENTRY:            return "Entry (ERROR)";

            case chunk_t::VITAPREV:         return "Vitalise Preview";

            case chunk_t::HEADER:           return "Header";
            case chunk_t::TITLE:            return "Title";
            case chunk_t::AUTHOR:           return "Author";
            case chunk_t::MENU:             return "Menu";
            case chunk_t::EXTPATH:          return "Extra Path";
            case chunk_t::EXTENS:           return "Extensions (deprecated)";
            case chunk_t::OBJECTBANK:       return "Object Bank";

            case chunk_t::GLOBALEVENTS:     return "Global Events";
            case chunk_t::FRAMEHANDLES:     return "Frame Handles";
            case chunk_t::EXTDATA:          return "Extra Data";
            case chunk_t::ADDEXTNS:         return "Additional Extensions (deprecated)";
            case chunk_t::PROJPATH:         return "Project Path";
            case chunk_t::OUTPATH:          return "Output Path";
            case chunk_t::APPDOC:           return "App Doc";
            case chunk_t::OTHEREXT:         return "Other Extension(s)";
            case chunk_t::GLOBALVALS:       return "Global Values";
            case chunk_t::GLOBALSTRS:       return "Global Strings";
            case chunk_t::EXTNLIST:         return "Extensions List";
            case chunk_t::ICON:             return "Icon";
            case chunk_t::DEMOVER:          return "DEMOVER";
            case chunk_t::SECNUM:           return "Security Number";
            case chunk_t::BINFILES:         return "Binary Files";
            case chunk_t::MENUIMAGES:       return "Menu Images";
            case chunk_t::ABOUT:            return "About";
            case chunk_t::COPYRIGHT:        return "Copyright";
            case chunk_t::GLOBALVALNAMES:   return "Global Value Names";
            case chunk_t::GLOBALSTRNAMES:   return "Global String Names";
            case chunk_t::MOVEMNTEXTNS:     return "Movement Extensions";
            // case chunk_t::UNKNOWN8:         return "UNKNOWN8";
            case chunk_t::OBJECTBANK2:      return "Object Bank 2";
            case chunk_t::EXEONLY:          return "EXE Only";
            case chunk_t::PROTECTION:       return "Protection";
            case chunk_t::SHADERS:          return "Shaders";
            case chunk_t::EXTDHEADER:       return "Extended Header";
            case chunk_t::SPACER:           return "Spacer";
            case chunk_t::FRAMEBANK:        return "Frame Bank";
            case chunk_t::CHUNK224F:        return "CHUNK 224F";
            case chunk_t::TITLE2:           return "Title2";

            case chunk_t::FRAME:            return "Frame";
            case chunk_t::FRAMEHEADER:      return "Frame Header";
            case chunk_t::FRAMENAME:        return "Frame Name";
            case chunk_t::FRAMEPASSWORD:    return "Frame Password";
            case chunk_t::FRAMEPALETTE:     return "Frame Palette";
            case chunk_t::OBJINST:          return "Frame Object Instances";
            case chunk_t::FRAMEFADEIF:      return "Frame Fade In Frame";
            case chunk_t::FRAMEFADEOF:      return "Frame Fade Out Frame";
            case chunk_t::FRAMEFADEI:       return "Frame Fade In";
            case chunk_t::FRAMEFADEO:       return "Frame Fade Out";
            case chunk_t::FRAMEEVENTS:      return "Frame Events";
            case chunk_t::FRAMEPLYHEAD:     return "Frame Play Header";
            case chunk_t::FRAMEADDITEM:     return "Frame Additional Item";
            case chunk_t::FRAMEADDITEMINST: return "Frame Additional Item Instance";
            case chunk_t::FRAMELAYERS:      return "Frame Layers";
            case chunk_t::FRAMEVIRTSIZE:    return "Frame Virtical Size";
            case chunk_t::DEMOFILEPATH:     return "Demo File Path";
            case chunk_t::RANDOMSEED:       return "Random Seed";
            case chunk_t::FRAMELAYEREFFECT: return "Frame Layer Effect";
            case chunk_t::FRAMEBLURAY:      return "Frame BluRay Options";
            case chunk_t::MOVETIMEBASE:     return "Frame Movement Timer Base";
            case chunk_t::MOSAICIMGTABLE:   return "Mosaic Image Table";
            case chunk_t::FRAMEEFFECTS:     return "Frame Effects";
            case chunk_t::FRAMEIPHONEOPTS:  return "Frame iPhone Options";

            case chunk_t::PAERROR:          return "PA (ERROR)";

            case chunk_t::OBJHEAD:          return "Object Header";
            case chunk_t::OBJNAME:          return "Object Name";
            case chunk_t::OBJPROP:          return "Object Properties";
            case chunk_t::OBJUNKN:          return "Object (Unknown)";
            case chunk_t::OBJEFCT:          return "Object Effect";

            case chunk_t::ENDIMAGE:         return "Image Handles";
            case chunk_t::ENDFONT:          return "Font Handles";
            case chunk_t::ENDSOUND:         return "Sound Handles";
            case chunk_t::ENDMUSIC:         return "Music Handles";

            case chunk_t::IMAGEBANK:        return "Image Bank";
            case chunk_t::FONTBANK:         return "Font Bank";
            case chunk_t::SOUNDBANK:        return "Sound Bank";
            case chunk_t::MUSICBANK:        return "Music Bank";

            case chunk_t::LAST:             return "Last";

            case chunk_t::DEFAULT:
            case chunk_t::VITA:
            case chunk_t::UNICODE:
            case chunk_t::NEW:
            case chunk_t::OLD:
            case chunk_t::FRAME_STATE:
            case chunk_t::IMAGE_STATE:
            case chunk_t::FONT_STATE:
            case chunk_t::SOUND_STATE:
            case chunk_t::MUSIC_STATE:
            case chunk_t::NOCHILD:
            case chunk_t::SKIP:
            default: return "INVALID";
        }
    }

    std::string GetObjectTypeString(object_type_t type)
    {
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

    std::pair<bool, std::vector<uint8_t>> Decode(const std::vector<uint8_t> &encoded, chunk_t id, encoding_t mode)
    {
        switch (mode)
        {
            case encoding_t::MODE3:
            case encoding_t::MODE2:
                return Decrypt(encoded, id, mode);
            case encoding_t::MODE1:
                return {true, Inflate(encoded)};
            default:
                if (encoded.size() > 0 && encoded[0] == 0x78)
                    return {true, Inflate(encoded)};
                else
                    return {true, encoded};
        }
    }

    std::vector<uint8_t> Inflate(const std::vector<uint8_t> &deflated)
    {
        std::deque<uint8_t> buffer;
        lak::tinflate_error_t err = lak::tinflate(deflated, buffer, nullptr);
        if (err == lak::tinflate_error_t::OK)
            return std::vector<uint8_t>(buffer.begin(), buffer.end());
        else
            return deflated;
    }

    std::vector<uint8_t> Decompress(const std::vector<uint8_t> &compressed, unsigned int outSize)
    {
        std::vector<uint8_t> result; result.resize(outSize);
        tinf_uncompress(&(result[0]), &outSize, &(compressed[0]), compressed.size());
        return result;
    }

    std::vector<uint8_t> StreamDecompress(lak::memory &strm, unsigned int outSize)
    {
        std::vector<uint8_t> result; result.resize(outSize);
        strm.position += tinf_uncompress(&(result[0]), &outSize, strm.get(), strm.remaining());
        return result;
    }

    std::pair<bool, std::vector<uint8_t>> Decrypt(const std::vector<uint8_t> &encrypted, chunk_t ID, encoding_t mode)
    {
        if (mode == encoding_t::MODE3)
        {
            if (encrypted.size() <= 4) return {false, encrypted};
            // TODO: check endian
            // size_t dataLen = *reinterpret_cast<const uint32_t*>(&encrypted[0]);
            std::vector<uint8_t> mem(encrypted.begin() + 4, encrypted.end());

            if ((_mode != game_mode_t::_284) && ((uint16_t)ID & 0x1) != 0)
                mem[0] ^= ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

            if (DecodeChunk(mem, _magic_key, _xmmword))
            {
                if (mem.size() <= 4) return {false, mem};
                // dataLen = *reinterpret_cast<uint32_t*>(&mem[0]);
                return {true, Inflate(std::vector<uint8_t>(mem.begin()+4, mem.end()))};
            }
            DEBUG("MODE 3 Decryption Failed");
            return {false, mem};
        }
        else
        {
            if (encrypted.size() < 1) return {false, encrypted};

            std::vector<uint8_t> mem = encrypted;

            if ((_mode != game_mode_t::_284) && (uint16_t)ID & 0x1)
                mem[0] ^= ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

            if (!DecodeChunk(mem, _magic_key, _xmmword))
            {
                if (mode == encoding_t::MODE2)
                {
                    DEBUG("MODE 2 Decryption Failed");
                    return {false, mem};
                }
            }
            return {true, mem};
        }
    }

    object::item_t *GetObject(game_t &game, uint16_t handle)
    {
        if (game.game.objectBank &&
            game.objectHandles[handle] < game.game.objectBank->items.size())
        {
            return &game.game.objectBank->items[game.objectHandles[handle]];
        }
        return nullptr;
    }

    image::item_t *GetImage(game_t &game, uint32_t handle)
    {
        if (game.game.imageBank &&
            game.imageHandles[handle] < game.game.imageBank->items.size())
        {
            return &game.game.imageBank->items[game.imageHandles[handle]];
        }
        return nullptr;
    }

    lak::memory resource_entry_t::streamHeader() const
    {
        return header.data;
    }

    lak::memory resource_entry_t::stream() const
    {
        return data.data;
    }

    lak::memory resource_entry_t::decodeHeader() const
    {
        return header.decode(ID, mode);
    }

    lak::memory resource_entry_t::decode() const
    {
        return data.decode(ID, mode);
    }

    lak::memory data_point_t::decode(const chunk_t ID, const encoding_t mode) const
    {
        auto [success, mem] = Decode(data._data, ID, mode);
        if (success)
            return lak::memory(mem);
        return lak::memory();
    }

    error_t chunk_entry_t::read(game_t &game, lak::memory &strm)
    {
        if (!strm.remaining()) return error_t::OUT_OF_DATA;

        position = strm.position;
        old = game.oldGame;
        ID = (chunk_t)strm.read_u16();
        mode = (encoding_t)strm.read_u16();

        if ((mode == encoding_t::MODE2 || mode == encoding_t::MODE3) && _magic_key.size() < 256)
            GetEncryptionKey(game);

        if (mode == encoding_t::MODE1)
        {
            if (game.oldGame)
            {
                ReadRevCompressedData(strm, data);
            }
            else
            {
                ReadFixedData(strm, header, 0x4);
                ReadCompressedData(strm, data);
            }
        }
        else
        {
            ReadDynamicData(strm, data);
        }
        end = strm.position;

        return error_t::OK;
    }

    void chunk_entry_t::view(source_explorer_t &srcexp) const
    {
        if (lak::TreeNode("Entry Information##%zX", position))
        {
            ImGui::Separator();
            if (old)
                ImGui::Text("Old Entry");
            else
                ImGui::Text("New Entry");
            ImGui::Text("Position: 0x%zX", position);
            ImGui::Text("End Pos: 0x%zX", end);
            ImGui::Text("Size: 0x%zX", end - position);

            ImGui::Text("ID: 0x%zX", (size_t)ID);
            ImGui::Text("Mode: MODE%zu", (size_t)mode);

            ImGui::Text("Header Position: 0x%zX", header.position);
            ImGui::Text("Header Expected Size: 0x%zX", header.expectedSize);
            ImGui::Text("Header Size: 0x%zX", header.data.size());

            ImGui::Text("Data Position: 0x%zX", data.position);
            ImGui::Text("Data Expected Size: 0x%zX", data.expectedSize);
            ImGui::Text("Data Size: 0x%zX", data.data.size());

            ImGui::Separator();
            ImGui::TreePop();
        }

        if (ImGui::Button("View Memory"))
            srcexp.view = this;
    }

    error_t item_entry_t::read(game_t &game, lak::memory &strm, bool compressed, size_t headersize)
    {
        if (!strm.remaining()) return error_t::OUT_OF_DATA;

        position = strm.position;
        old = game.oldGame;
        mode = encoding_t::MODE0;
        handle = strm.read_u32();
        if (!game.oldGame && game.productBuild >= 284) --handle;

        if (game.oldGame)
        {
            ReadStreamCompressedData(strm, data);
            mode = encoding_t::MODE1; // hack because one of MMF1.5 or tinf_uncompress is a bitch
        }
        else
        {
            if (headersize > 0)
                ReadFixedData(strm, header, headersize);

            if (compressed)
                ReadCompressedData(strm, data);
            else
                ReadDynamicData(strm, data);
        }
        end = strm.position;

        return error_t::OK;
    }

    void item_entry_t::view(source_explorer_t &srcexp) const
    {
        if (lak::TreeNode("Entry Information##%zX", position))
        {
            if (old)
                ImGui::Text("Old Entry");
            else
                ImGui::Text("New Entry");
            ImGui::Text("Position: 0x%zX", position);
            ImGui::Text("End Pos: 0x%zX", end);
            ImGui::Text("Size: 0x%zX", end - position);

            ImGui::Text("Handle: 0x%zX", (size_t)handle);

            ImGui::Text("Header Position: 0x%zX", header.position);
            ImGui::Text("Header Expected Size: 0x%zX", header.expectedSize);
            ImGui::Text("Header Size: 0x%zX", header.data.size());

            ImGui::Text("Data Position: 0x%zX", data.position);
            ImGui::Text("Data Expected Size: 0x%zX", data.expectedSize);
            ImGui::Text("Data Size: 0x%zX", data.data.size());

            ImGui::Separator();
            ImGui::TreePop();
        }

        if (ImGui::Button("View Memory"))
            srcexp.view = this;
    }

    lak::memory basic_entry_t::decode() const
    {
        lak::memory result;
        if (old)
        {
            switch (mode)
            {
                case encoding_t::MODE0: result = data.data; break;
                case encoding_t::MODE1: {
                    result = data.data;
                    uint8_t magic = result.read_u8();
                    uint16_t len = result.read_u16();
                    if (magic == 0x0F && (size_t)len == data.expectedSize)
                    {
                        result = result.read(result.remaining());
                    }
                    else
                    {
                        result.position -= 3;
                        result = Decompress(result._data, data.expectedSize);
                    }
                } break;
                case encoding_t::MODE2: ERROR("No Old Decode For Mode 2"); break;
                case encoding_t::MODE3: ERROR("No Old Decode For Mode 3"); break;
                default: break;
            }
        }
        else
        {
            switch (mode)
            {
                case encoding_t::MODE3:
                case encoding_t::MODE2:
                    if (auto [success, mem] = Decrypt(data.data._data, ID, mode); success)
                    {
                        result = mem;
                    }
                    else
                    {
                        ERROR("Decryption Failed");
                        result.clear();
                    }
                    break;
                case encoding_t::MODE1:
                    result = Inflate(data.data._data);
                    break;
                default:
                    if (data.data.size() > 0 && data.data[0] == 0x78)
                        result = Inflate(data.data._data);
                    else
                        result = data.data._data;
                    break;
            }
        }
        return result;
    }

    lak::memory basic_entry_t::decodeHeader() const
    {
        lak::memory result;
        if (old)
        {

        }
        else
        {
            switch (mode)
            {
                case encoding_t::MODE3:
                case encoding_t::MODE2:
                    if (auto [success, mem] = Decrypt(data.data._data, ID, mode); success)
                        result = mem;
                    else
                        result.clear();
                    break;
                case encoding_t::MODE1:
                    result = Inflate(header.data._data);
                    break;
                default:
                    if (header.data.size() > 0 && header.data[0] == 0x78)
                        result = Inflate(header.data._data);
                    else
                        result = header.data._data;
                    break;
            }
        }
        return result;
    }

    lak::memory basic_entry_t::raw() const
    {
        return data.data;
    }

    lak::memory basic_entry_t::rawHeader() const
    {
        return header.data;
    }

    std::u16string ReadStringEntry(game_t &game, const chunk_entry_t &entry)
    {
        std::u16string result;

        if (game.oldGame)
        {
            switch (entry.mode)
            {
                case encoding_t::MODE0:
                case encoding_t::MODE1: {
                    result = lak::strconv_u16(entry.decode().read_string());
                } break;
                default: {
                    ERROR("Invalid String Mode: 0x" << (int)entry.mode <<
                        ", Chunk: 0x" << (int)entry.ID);
                } break;
            }
        }
        else
        {
            if (game.unicode)
                result = entry.decode().read_u16string();
            else
                result = lak::strconv<char16_t>(entry.decode().read_string());
        }

        return result;
    }

    error_t basic_chunk_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading Basic Chunk");
        error_t result = entry.read(game, strm);
        return result;
    }

    error_t basic_chunk_t::basic_view(source_explorer_t &srcexp, const char *name) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t basic_item_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading Basic Chunk");
        error_t result = entry.read(game, strm, true);
        return result;
    }

    error_t basic_item_t::basic_view(source_explorer_t &srcexp, const char *name) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t string_chunk_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading String Chunk");
        error_t result = entry.read(game, strm);
        if (result == error_t::OK)
            value = ReadStringEntry(game, entry);
        else
            ERROR("0x" << (size_t)result << ": Failed To Read String Chunk");
        return result;
    }

    error_t string_chunk_t::view(source_explorer_t &srcexp, const char *name, const bool preview) const
    {
        error_t result = error_t::OK;
        std::string str = string();
        bool open = false;

        if (preview)
            open = lak::TreeNode("0x%zX %s '%s'##%zX", (size_t)entry.ID, name, str.c_str(), entry.position);
        else
            open = lak::TreeNode("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position);

        if (open)
        {
            ImGui::Separator();

            entry.view(srcexp);
            ImGui::Text("String: '%s'", str.c_str());
            ImGui::Text("String Length: 0x%zX", value.size());

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    std::u16string string_chunk_t::u16string() const
    {
        return value;
    }

    std::u8string string_chunk_t::u8string() const
    {
        return lak::strconv<char8_t>(value);
    }

    std::string string_chunk_t::string() const
    {
        return lak::strconv<char>(value);
    }

    error_t vitalise_preview_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Vitalise Preview");
    }

    error_t menu_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Menu");
    }

    error_t extension_path_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Extension Path");
    }

    error_t extensions_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Extensions");
    }

    error_t global_events_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Global Events");
    }

    error_t extension_data_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Extension Data");
    }

    error_t additional_extensions_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Additional Extensions");
    }

    error_t application_doc_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Appliocation Doc");
    }

    error_t other_extenion_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Other Extension");
    }

    error_t global_values_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Global Values");
    }

    error_t global_strings_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Global Strings");
    }

    error_t extension_list_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Extension List");
    }

    error_t icon_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading Icon");
        error_t result = entry.read(game, strm);

        auto dstrm = entry.decode();
        dstrm.position = dstrm.peek_u32();

        std::vector<lak::color4_t> palette;
        palette.resize(16 * 16);

        for (auto &point : palette)
        {
            point.b = dstrm.read_u8();
            point.g = dstrm.read_u8();
            point.r = dstrm.read_u8();
            // point.a = dstrm.read_u8();
            point.a = 0xFF;
            ++dstrm.position;
        }

        bitmap.resize({16, 16});

        for (size_t y = 0; y < bitmap.size().y; ++y)
        {
            for (size_t x = 0; x < bitmap.size().x; ++x)
            {
                bitmap[{x, (bitmap.size().y - 1) - y}] = palette[dstrm.read_u8()];
            }
        }

        for (size_t i = 0; i < bitmap.contig_size() / 8; ++i)
        {
            uint8_t mask = dstrm.read_u8();
            for (size_t j = 0; j < 8; ++j)
            {
                if (0x1 & (mask >> (8 - j)))
                {
                    bitmap[i * j].a = 0x00;
                }
                else
                {
                    bitmap[i * j].a = 0xFF;
                }
            }
        }

        return result;
    }

    error_t icon_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Icon##%zX", (size_t)entry.ID, entry.position))
        {
            ImGui::Separator();

            entry.view(srcexp);

            if (ImGui::Button("View Image"))
            {
                srcexp.image = CreateTexture(bitmap);
            }

            ImGui::TreePop();
        }

        return result;
    }

    error_t demo_version_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Demo Version");
    }

    error_t security_number_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Security Number");
    }

    error_t binary_file_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading Binary File");

        if (game.unicode)
            name = strm.read_u16string_exact(strm.read_u16());
        else
            name = lak::strconv_u16(strm.read_string_exact(strm.read_u16()));

        data = strm.read(strm.read_u32());

        return error_t::OK;
    }

    error_t binary_file_t::view(source_explorer_t &srcexp) const
    {
        std::u8string str = lak::strconv_u8(name);
        if (lak::TreeNode("%s", str.c_str()))
        {
            ImGui::Separator();

            ImGui::Text("Name: %s", str.c_str());
            ImGui::Text("Data Size: 0x%zX", data.size());

            ImGui::Separator();
            ImGui::TreePop();
        }
        return error_t::OK;
    }

    error_t binary_files_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading Binary Files");
        error_t result = entry.read(game, strm);

        if (result == error_t::OK)
        {
            lak::memory bstrm = entry.decode();
            items.resize(bstrm.read_u32());
            for (auto &item : items)
                item.read(game, bstrm);
        }

        return result;
    }

    error_t binary_files_t::view(source_explorer_t &srcexp) const
    {
        if (lak::TreeNode("0x%zX Binary Files##%zX", (size_t)entry.ID, entry.position))
        {
            ImGui::Separator();

            entry.view(srcexp);

            int index = 0;
            for (const auto &item : items)
            {
                ImGui::PushID(index++);
                item.view(srcexp);
                ImGui::PopID();
            }

            ImGui::Separator();
            ImGui::TreePop();
        }
        return error_t::OK;
    }

    error_t menu_images_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Menu Images");
    }

    error_t global_value_names_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Global Value Names");
    }

    error_t global_string_names_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Global String Names");
    }

    error_t movement_extensions_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Movement Extensions");
    }

    error_t object_bank2_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Object Bank 2");
    }

    error_t exe_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "EXE Only");
    }

    error_t protection_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Protection");
    }

    error_t shaders_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Shaders");
    }

    error_t extended_header_t::read(game_t &game, lak::memory &strm)
    {
        error_t result = entry.read(game, strm);

        lak::memory estrm = entry.decode();
        flags = estrm.read_u32();
        buildType = estrm.read_u32();
        buildFlags = estrm.read_u32();
        screenRatioTolerance = estrm.read_u16();
        screenAngle = estrm.read_u16();

        game.compat |= buildType >= 0x10000000;

        return result;
    }

    error_t extended_header_t::view(source_explorer_t &srcexp) const
    {
        if (lak::TreeNode("0x%zX Extended Header##%zX", (size_t)entry.ID, entry.position))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Text("Flags: 0x%zX", (size_t)flags);
            ImGui::Text("Build Type: 0x%zX", (size_t)buildType);
            ImGui::Text("Build Flags: 0x%zX", (size_t)buildFlags);
            ImGui::Text("Screen Ratio Tolerance: 0x%zX", (size_t)screenRatioTolerance);
            ImGui::Text("Screen Angle: 0x%zX", (size_t)screenAngle);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return error_t::OK;
    }

    error_t spacer_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Spacer");
    }

    error_t chunk_224F_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Chunk 224F");
    }

    error_t title2_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Title 2");
    }

    namespace object
    {
        error_t effect_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Effect");
        }

        error_t shape_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Shape");

            borderSize = strm.read_u16();
            borderColor.r = strm.read_u8();
            borderColor.g = strm.read_u8();
            borderColor.b = strm.read_u8();
            borderColor.a = strm.read_u8();
            shape = (shape_type_t)strm.read_u16();
            fill = (fill_type_t)strm.read_u16();

            line = line_flags_t::NONE;
            gradient = gradient_flags_t::HORIZONTAL;
            handle = 0xFFFF;

            if (shape == shape_type_t::LINE)
            {
                line = (line_flags_t)strm.read_u16();
            }
            else if (fill == fill_type_t::SOLID)
            {
                color1.r = strm.read_u8();
                color1.g = strm.read_u8();
                color1.b = strm.read_u8();
                color1.a = strm.read_u8();
            }
            else if (fill == fill_type_t::GRADIENT)
            {
                color1.r = strm.read_u8();
                color1.g = strm.read_u8();
                color1.b = strm.read_u8();
                color1.a = strm.read_u8();
                color2.r = strm.read_u8();
                color2.g = strm.read_u8();
                color2.b = strm.read_u8();
                color2.a = strm.read_u8();
                gradient = (gradient_flags_t)strm.read_u16();
            }
            else if (fill == fill_type_t::MOTIF)
            {
                handle = strm.read_u16();
            }

            return error_t::OK;
        }

        error_t shape_t::view(source_explorer_t &srcexp) const
        {
            return error_t::OK;
        }

        error_t quick_backdrop_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Quick Backdrop");
            error_t result = entry.read(game, strm);

            if (result == error_t::OK)
            {
                lak::memory qstrm = entry.decode();
                size = qstrm.read_u32();
                obstacle = qstrm.read_u16();
                collision = qstrm.read_u16();
                dimension.x = qstrm.read_u32();
                dimension.y = qstrm.read_u32();
                shape.read(game, qstrm);
            }

            return result;
        }

        error_t quick_backdrop_t::view(source_explorer_t &srcexp) const
        {
            if (lak::TreeNode("0x%zX Properties (Quick Backdrop)##%zX", (size_t)entry.ID, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                if (shape.handle < 0xFFFF)
                {
                    auto *img = GetImage(srcexp.state, shape.handle);
                    if (img) img->view(srcexp);
                }

                ImGui::Separator();
                ImGui::TreePop();
            }

            return error_t::OK;
        }

        error_t backdrop_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Backdrop");
            error_t result = entry.read(game, strm);

            if (result == error_t::OK)
            {
                lak::memory bstrm = entry.decode();
                obstacle = bstrm.read_u16();
                collision = bstrm.read_u16();
                dimension.x = bstrm.read_u32();
                dimension.y = bstrm.read_u32();
                handle = bstrm.read_u16();
            }

            return result;
        }

        error_t backdrop_t::view(source_explorer_t &srcexp) const
        {
            if (lak::TreeNode("0x%zX Properties (Backdrop)##%zX", (size_t)entry.ID, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                if (handle < 0xFFFF)
                {
                    auto *img = GetImage(srcexp.state, handle);
                    if (img) img->view(srcexp);
                }

                ImGui::Separator();
                ImGui::TreePop();
            }

            return error_t::OK;
        }

        error_t animation_direction_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Animation Direction");

            minSpeed = strm.read_u8();
            maxSpeed = strm.read_u8();
            repeat = strm.read_u16();
            backTo = strm.read_u16();
            handles.resize(strm.read_u16());
            for (auto &handle : handles)
                handle = strm.read_u16();

            return error_t::OK;
        }

        error_t animation_direction_t::view(source_explorer_t &srcexp) const
        {
            ImGui::Text("Min Speed: %d", (int)minSpeed);
            ImGui::Text("Max Speed: %d", (int)maxSpeed);
            ImGui::Text("Repeat: 0x%zX", (size_t)repeat);
            ImGui::Text("Back To: 0x%zX", (size_t)backTo);
            ImGui::Text("Frames: 0x%zX", handles.size());

            int index = 0;
            for (const auto &handle : handles)
            {
                ImGui::PushID(index++);
                auto *img = GetImage(srcexp.state, handle);
                if (img) img->view(srcexp);
                ImGui::PopID();
            }

            return error_t::OK;
        }

        error_t animation_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Animation");

            const size_t begin = strm.position;

            size_t index = 0;
            for (auto &offset : offsets)
            {
                offset = strm.read_u16();
                if (offset != 0)
                {
                    const size_t pos = strm.position;
                    strm.position = begin + offset;
                    directions[index].read(game, strm);
                    strm.position = pos;
                }
                ++index;
            }

            return error_t::OK;
        }

        error_t animation_t::view(source_explorer_t &srcexp) const
        {
            size_t index = 0;
            for (const auto &direction : directions)
            {
                if (direction.handles.size() > 0 && lak::TreeNode("Animation Direction 0x%zX", index))
                {
                    ImGui::Separator();
                    direction.view(srcexp);
                    ImGui::Separator();
                    ImGui::TreePop();
                }
                ++index;
            }

            return error_t::OK;
        }

        error_t animation_header_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Animation Header");

            const size_t begin = strm.position;

            size = strm.read_u16();
            offsets.resize(strm.read_u16());
            animations.resize(offsets.size());

            size_t index = 0;
            for (auto &offset : offsets)
            {
                offset = strm.read_u16();
                if (offset != 0)
                {
                    const size_t pos = strm.position;
                    strm.position = begin + offset;
                    animations[index].read(game, strm);
                    strm.position = pos;
                }
                ++index;
            }

            return error_t::OK;
        }

        error_t animation_header_t::view(source_explorer_t &srcexp) const
        {
            if (ImGui::TreeNode("Animations"))
            {
                ImGui::Separator();
                ImGui::Text("Size: 0x%zX", size);
                ImGui::Text("Animations: %zu", animations.size());
                size_t index = 0;
                for (const auto &animation : animations)
                {
                    if (animation.offsets[index] > 0 && lak::TreeNode("Animation 0x%zX", index))
                    {
                        ImGui::Separator();
                        animation.view(srcexp);
                        ImGui::TreePop();
                    }
                    ++index;
                }
                ImGui::Separator();
                ImGui::TreePop();
            }

            return error_t::OK;
        }

        error_t common_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Common Object");
            error_t result = entry.read(game, strm);

            if (result == error_t::OK)
            {
                lak::memory cstrm = entry.decode();

                // if (game.productBuild >= 288 && !game.oldGame)
                //     mode = game_mode_t::_288;
                // if (game.productBuild >= 284 && !game.oldGame && !game.compat)
                //     mode = game_mode_t::_284;
                // else
                //     mode = game_mode_t::_OLD;
                mode = _mode;

                const size_t begin = cstrm.position;

                size = cstrm.read_u32();

                if (mode == game_mode_t::_288)
                {
                    animationsOffset = cstrm.read_u16();
                    movementsOffset = cstrm.read_u16(); // are these in the right order?
                    version = cstrm.read_u16();         // are these in the right order?
                    counterOffset = cstrm.read_u16();   // are these in the right order?
                    systemOffset = cstrm.read_u16();    // are these in the right order?
                }
                else if (mode == game_mode_t::_284)
                {
                    counterOffset = cstrm.read_u16();
                    version = cstrm.read_u16();
                    cstrm.position += 2;
                    movementsOffset = cstrm.read_u16();
                    extensionOffset = cstrm.read_u16();
                    animationsOffset = cstrm.read_u16();
                }
                else
                {
                    movementsOffset = cstrm.read_u16();
                    animationsOffset = cstrm.read_u16();
                    version = cstrm.read_u16();
                    counterOffset = cstrm.read_u16();
                    systemOffset = cstrm.read_u16();
                }

                flags = cstrm.read_u32();

                const size_t end = cstrm.position + 16;

                // qualifiers.clear();
                // qualifiers.reserve(8);
                // for (size_t i = 0; i < 8; ++i)
                // {
                //     int16_t qualifier = cstrm.read_s16();
                //     if (qualifier == -1)
                //         break;
                //     qualifiers.push_back(qualifier);
                // }

                cstrm.position = end;

                if (mode != game_mode_t::_284)
                    systemOffset = cstrm.read_u16();
                else
                    extensionOffset = cstrm.read_u16();

                valuesOffset = cstrm.read_u16();
                stringsOffset = cstrm.read_u16();
                newFlags = cstrm.read_u32();
                preferences = cstrm.read_u32();
                identifier = cstrm.read_u32();
                backColor.r = cstrm.read_u8();
                backColor.g = cstrm.read_u8();
                backColor.b = cstrm.read_u8();
                backColor.a = cstrm.read_u8();
                fadeInOffset = cstrm.read_u32();
                fadeOutOffset = cstrm.read_u32();

                if (animationsOffset > 0)
                {
                    cstrm.position = begin + animationsOffset;
                    animations = std::make_unique<animation_header_t>();
                    animations->read(game, cstrm);
                }
            }

            return result;
        }

        error_t common_t::view(source_explorer_t &srcexp) const
        {
            if (lak::TreeNode("0x%zX Properties (Common)##%zX", (size_t)entry.ID, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                if (mode == game_mode_t::_288)
                {
                    ImGui::Text("Movements Offset: 0x%zX", (size_t)movementsOffset);
                    ImGui::Text("Animations Offset: 0x%zX", (size_t)animationsOffset);
                    ImGui::Text("Version: 0x%zX", (size_t)version);
                    ImGui::Text("Counter Offset: 0x%zX", (size_t)counterOffset);
                    ImGui::Text("System Offset: 0x%zX", (size_t)systemOffset);
                    ImGui::Text("Flags: 0x%zX", (size_t)flags);
                    ImGui::Text("Extension Offset: 0x%zX", (size_t)extensionOffset);
                }
                else if (mode == game_mode_t::_284)
                {
                    ImGui::Text("Counter Offset: 0x%zX", (size_t)counterOffset);
                    ImGui::Text("Version: 0x%zX", (size_t)version);
                    ImGui::Text("Movements Offset: 0x%zX", (size_t)movementsOffset);
                    ImGui::Text("Extension Offset: 0x%zX", (size_t)extensionOffset);
                    ImGui::Text("Animations Offset: 0x%zX", (size_t)animationsOffset);
                    ImGui::Text("Flags: 0x%zX", (size_t)flags);
                    ImGui::Text("System Offset: 0x%zX", (size_t)systemOffset);
                }
                else
                {
                    ImGui::Text("Animations Offset: 0x%zX", (size_t)animationsOffset);
                    ImGui::Text("Movements Offset: 0x%zX", (size_t)movementsOffset);
                    ImGui::Text("Version: 0x%zX", (size_t)version);
                    ImGui::Text("Counter Offset: 0x%zX", (size_t)counterOffset);
                    ImGui::Text("System Offset: 0x%zX", (size_t)systemOffset);
                    ImGui::Text("Flags: 0x%zX", (size_t)flags);
                    ImGui::Text("Extension Offset: 0x%zX", (size_t)extensionOffset);
                }
                ImGui::Text("Values Offset: 0x%zX", (size_t)valuesOffset);
                ImGui::Text("Strings Offset: 0x%zX", (size_t)stringsOffset);
                ImGui::Text("New Flags: 0x%zX", (size_t)newFlags);
                ImGui::Text("Preferences: 0x%zX", (size_t)preferences);
                ImGui::Text("Identifier: 0x%zX", (size_t)identifier);
                ImGui::Text("Fade In Offset: 0x%zX", (size_t)fadeInOffset);
                ImGui::Text("Fade Out Offset: 0x%zX", (size_t)fadeOutOffset);

                lak::vec3f_t col = ((lak::vec3f_t)backColor) / 256.0f;
                ImGui::ColorEdit3("Background Color", &col.x);

                if (animations) animations->view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }
            return error_t::OK;
        }

        error_t item_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Object");
            error_t result = entry.read(game, strm);

            auto dstrm = entry.decode();
            handle = dstrm.read_u16();
            type = (object_type_t)dstrm.read_s16();
            dstrm.read_u16(); // flags
            dstrm.read_u16(); // "no longer used"
            inkEffect = dstrm.read_u32();
            inkEffectParam = dstrm.read_u32();

            while (result == error_t::OK)
            {
                switch ((chunk_t)strm.peek_u16())
                {
                    case chunk_t::OBJNAME:
                        name = std::make_unique<string_chunk_t>();
                        result = name->read(game, strm);
                        break;

                    case chunk_t::OBJPROP:
                        switch (type)
                        {
                            case object_type_t::QUICK_BACKDROP:
                                quickBackdrop = std::make_unique<quick_backdrop_t>();
                                quickBackdrop->read(game, strm);
                                break;
                            case object_type_t::BACKDROP:
                                backdrop = std::make_unique<backdrop_t>();
                                backdrop->read(game, strm);
                                break;
                            default:
                                common = std::make_unique<common_t>();
                                common->read(game, strm);
                                break;
                        }
                        break;

                    case chunk_t::OBJEFCT:
                        effect = std::make_unique<effect_t>();
                        result = effect->read(game, strm);
                        break;

                    case chunk_t::LAST:
                        end = std::make_unique<last_t>();
                        result = end->read(game, strm);
                    default: goto finished;
                }
            }
            finished:

            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX %s '%s'##%zX", (size_t)entry.ID,
                GetObjectTypeString(type).c_str(),
                (name ? lak::strconv<char>(name->value).c_str() : ""),
                entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Text("Handle: 0x%zX", (size_t)handle);
                ImGui::Text("Type: 0x%zX", (size_t)type);
                ImGui::Text("Ink Effect: 0x%zX", (size_t)inkEffect);
                ImGui::Text("Ink Effect Parameter: 0x%zX", (size_t)inkEffectParam);

                if (name) name->view(srcexp, "Name", true);

                if (quickBackdrop) quickBackdrop->view(srcexp);
                if (backdrop) backdrop->view(srcexp);
                if (common) common->view(srcexp);

                if (effect) effect->view(srcexp);
                if (end) end->view(srcexp);


                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        std::unordered_map<uint32_t, size_t> item_t::image_handles() const
        {
            std::unordered_map<uint32_t, size_t> result;

            if (quickBackdrop)
                ++result[quickBackdrop->shape.handle];
            if (backdrop)
                ++result[backdrop->handle];
            if (common && common->animations)
                for (const auto &animation : common->animations->animations)
                    for (int i = 0; i < 32; ++i)
                        if (animation.offsets[i] > 0)
                            for (auto handle : animation.directions[i].handles)
                                ++result[handle];

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Object Bank");
            error_t result = entry.read(game, strm);

            strm.position = entry.data.position;

            items.resize(strm.read_u32());

            for (auto &item : items)
            {
                if (result != error_t::OK) break;
                result = item.read(game, strm);
            }

            strm.position = entry.end;

            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Object Bank (%zu Items)##%zX",
                (size_t)entry.ID, items.size(), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const item_t &item : items)
                    item.view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }
    }

    namespace frame
    {
        error_t header_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Header");
        }

        error_t password_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Password");
        }

        error_t palette_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Frame Palette");
            error_t result = entry.read(game, strm);

            if (result == error_t::OK)
            {
                auto pstrm = entry.decode();
                unknown = pstrm.read_u32();
                for (auto &color : colors)
                {
                    color.r = pstrm.read_u8();
                    color.g = pstrm.read_u8();
                    color.b = pstrm.read_u8();
                    /* color.a = */pstrm.read_u8();
                    color.a = 255u;
                }
            }

            return result;
        }

        error_t palette_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Frame Palette##%zX", (size_t)entry.ID, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                uint8_t index = 0;
                for (auto color : colors)
                {
                    lak::vec3f_t col = ((lak::vec3f_t)color) / 256.0f;
                    char str[3];
                    sprintf(str, "%hhX", index++);
                    ImGui::ColorEdit3(str, &col.x);
                }

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }

        lak::image4_t palette_t::image() const
        {
            lak::image4_t result;

            result.resize({16, 16});
            for (int i = 0; i < 256; ++i)
                result[i] = colors[i];

            return result;
        }

        error_t object_instance_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Object Instance");

            handle = strm.read_u16();
            info = strm.read_u16();
            position.x = strm.read_s32();
            position.y = strm.read_s32();
            parentType = strm.read_u16();
            parentHandle = strm.read_u16();
            layer = strm.read_u16();
            unknown = strm.read_u16();

            return error_t::OK;
        }

        error_t object_instance_t::view(source_explorer_t &srcexp) const
        {
            auto *obj = GetObject(srcexp.state, handle);
            std::u8string str;
            if (obj && obj->name)
                str += lak::strconv_u8(obj->name->value);

            if (lak::TreeNode("0x%zX %s", (size_t)handle, str.c_str()))
            {
                ImGui::Separator();

                ImGui::Text("Handle: 0x%zX", (size_t)handle);
                ImGui::Text("Info: 0x%zX", (size_t)info);
                ImGui::Text("Position: (%li, %li)", (long)position.x, (long)position.y);
                ImGui::Text("Parent Type: 0x%zX", (size_t)parentType);
                ImGui::Text("Parent Handle: 0x%zX", (size_t)parentHandle);
                ImGui::Text("Layer: 0x%zX", (size_t)layer);
                ImGui::Text("Unknown: 0x%zX", (size_t)unknown);

                if (obj) obj->view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return error_t::OK;
        }

        error_t object_instances_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Object Instances");
            error_t result = entry.read(game, strm);

            if (result == error_t::OK)
            {
                auto hstrm = entry.decode();
                objects.clear();
                objects.resize(hstrm.read_u32());
                DEBUG("Objects: 0x" << objects.size());
                for (auto &handle : objects)
                    handle.read(game, hstrm);
            }

            return result;
        }

        error_t object_instances_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Object Instances##%zX", (size_t)entry.ID, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const auto &object : objects)
                    object.view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }

        error_t fade_in_frame_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Fade In Frame");
        }

        error_t fade_out_frame_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Fade Out Frame");
        }

        error_t fade_in_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Fade In");
        }

        error_t fade_out_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Fade Out");
        }

        error_t events_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Events");
        }

        error_t play_head_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Play Head");
        }

        error_t additional_item_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Additional Item");
        }

        error_t additional_item_instance_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Additional Item Instance");
        }

        error_t layers_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Layers");
        }

        error_t virtual_size_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Virtual Size");
        }

        error_t demo_file_path_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Demo File Path");
        }

        error_t random_seed_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Random Seed");
            error_t result = entry.read(game, strm);

            value = entry.decode().read_s16();

            return result;
        }

        error_t random_seed_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Random Seed##%zX", (size_t)entry.ID, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);
                ImGui::Text("Value: %i", (int)value);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t layer_effect_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Layer Effect");
        }

        error_t blueray_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Blueray");
        }

        error_t movement_time_base_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Movement Time Base");
        }

        error_t mosaic_image_table_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Mosaic Image Table");
        }

        error_t effects_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Effects");
        }

        error_t iphone_options_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "iPhone Options");
        }

        error_t chunk_334C_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Chunk 334C");
        }

        error_t item_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Frame");
            error_t result = entry.read(game, strm);

            strm.position = entry.position + 0x8;

            while (result == error_t::OK)
            {
                switch ((chunk_t)strm.peek_u16())
                {
                    case chunk_t::FRAMENAME:
                        name = std::make_unique<string_chunk_t>();
                        result = name->read(game, strm);
                        break;

                    case chunk_t::FRAMEHEADER:
                        header = std::make_unique<header_t>();
                        result = header->read(game, strm);
                        break;

                    case chunk_t::FRAMEPASSWORD:
                        password = std::make_unique<password_t>();
                        result = password->read(game, strm);
                        break;

                    case chunk_t::FRAMEPALETTE:
                        palette = std::make_unique<palette_t>();
                        result = palette->read(game, strm);
                        break;

                    case chunk_t::OBJINST:
                        objectInstances = std::make_unique<object_instances_t>();
                        result = objectInstances->read(game, strm);
                        break;

                    case chunk_t::FRAMEFADEIF:
                        fadeInFrame = std::make_unique<fade_in_frame_t>();
                        result = fadeInFrame->read(game, strm);
                        break;

                    case chunk_t::FRAMEFADEOF:
                        fadeOutFrame = std::make_unique<fade_out_frame_t>();
                        result = fadeOutFrame->read(game, strm);
                        break;

                    case chunk_t::FRAMEFADEI:
                        fadeIn = std::make_unique<fade_in_t>();
                        result = fadeIn->read(game, strm);
                        break;

                    case chunk_t::FRAMEFADEO:
                        fadeOut = std::make_unique<fade_out_t>();
                        result = fadeOut->read(game, strm);
                        break;

                    case chunk_t::FRAMEEVENTS:
                        events = std::make_unique<events_t>();
                        result = events->read(game, strm);
                        break;

                    case chunk_t::FRAMEPLYHEAD:
                        playHead = std::make_unique<play_head_t>();
                        result = playHead->read(game, strm);
                        break;

                    case chunk_t::FRAMEADDITEM:
                        additionalItem = std::make_unique<additional_item_t>();
                        result = additionalItem->read(game, strm);
                        break;

                    case chunk_t::FRAMEADDITEMINST:
                        additionalItemInstance = std::make_unique<additional_item_instance_t>();
                        result = additionalItemInstance->read(game, strm);
                        break;

                    case chunk_t::FRAMELAYERS:
                        layers = std::make_unique<layers_t>();
                        result = layers->read(game, strm);
                        break;

                    case chunk_t::FRAMEVIRTSIZE:
                        virtualSize = std::make_unique<virtual_size_t>();
                        result = virtualSize->read(game, strm);
                        break;

                    case chunk_t::DEMOFILEPATH:
                        demoFilePath = std::make_unique<demo_file_path_t>();
                        result = demoFilePath->read(game, strm);
                        break;

                    case chunk_t::RANDOMSEED:
                        randomSeed = std::make_unique<random_seed_t>();
                        result = randomSeed->read(game, strm);
                        break;

                    case chunk_t::FRAMELAYEREFFECT:
                        layerEffect = std::make_unique<layer_effect_t>();
                        result = layerEffect->read(game, strm);
                        break;

                    case chunk_t::FRAMEBLURAY:
                        blueray = std::make_unique<blueray_t>();
                        result = blueray->read(game, strm);
                        break;

                    case chunk_t::MOVETIMEBASE:
                        movementTimeBase = std::make_unique<movement_time_base_t>();
                        result = movementTimeBase->read(game, strm);
                        break;

                    case chunk_t::MOSAICIMGTABLE:
                        mosaicImageTable = std::make_unique<mosaic_image_table_t>();
                        result = mosaicImageTable->read(game, strm);
                        break;

                    case chunk_t::FRAMEEFFECTS:
                        effects = std::make_unique<effects_t>();
                        result = effects->read(game, strm);
                        break;

                    case chunk_t::FRAMEIPHONEOPTS:
                        iphoneOptions = std::make_unique<iphone_options_t>();
                        result = iphoneOptions->read(game, strm);
                        break;

                    case chunk_t::FRAMECHUNK334C:
                        chunk334C = std::make_unique<chunk_334C_t>();
                        result = chunk334C->read(game, strm);
                        break;

                    case chunk_t::LAST:
                        end = std::make_unique<last_t>();
                        result = end->read(game, strm);
                    default: goto finished;
                }
            }

            strm.position = entry.end;

            finished:
            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX '%s'##%zX", (size_t)entry.ID,
                (name ? lak::strconv<char>(name->value).c_str() : ""), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                if (name) name->view(srcexp, "Name", true);
                if (header) header->view(srcexp);
                if (password) password->view(srcexp);
                if (palette) palette->view(srcexp);
                if (objectInstances) objectInstances->view(srcexp);
                if (fadeInFrame) fadeInFrame->view(srcexp);
                if (fadeOutFrame) fadeOutFrame->view(srcexp);
                if (fadeIn) fadeIn->view(srcexp);
                if (fadeOut) fadeOut->view(srcexp);
                if (events) events->view(srcexp);
                if (playHead) playHead->view(srcexp);
                if (additionalItem) additionalItem->view(srcexp);
                if (layers) layers->view(srcexp);
                if (layerEffect) layerEffect->view(srcexp);
                if (virtualSize) virtualSize->view(srcexp);
                if (demoFilePath) demoFilePath->view(srcexp);
                if (randomSeed) randomSeed->view(srcexp);
                if (blueray) blueray->view(srcexp);
                if (movementTimeBase) movementTimeBase->view(srcexp);
                if (mosaicImageTable) mosaicImageTable->view(srcexp);
                if (effects) effects->view(srcexp);
                if (iphoneOptions) iphoneOptions->view(srcexp);
                if (chunk334C) chunk334C->view(srcexp);
                if (end) end->view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t handles_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Handles");
        }

        error_t bank_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Frame Bank");
            error_t result = entry.read(game, strm);

            items.clear();
            while (result == error_t::OK && (chunk_t)strm.peek_u16() == chunk_t::FRAME)
            {
                items.emplace_back();
                result = items.back().read(game, strm);
            }

            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Frame Bank (%zu Items)##%zX",
                (size_t)entry.ID, items.size(), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const item_t &item : items)
                    item.view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }
    }

    namespace image
    {
        error_t item_t::read(game_t &game, lak::memory &strm)
        {
            error_t result = entry.read(game, strm, true);

            if (result == error_t::OK)
            {
                lak::memory istrm = entry.decode();
                if (game.oldGame)
                    checksum = istrm.read_u16();
                else
                    checksum = istrm.read_u32();
                reference = istrm.read_u32();
                dataSize = istrm.read_u32();
                size.x = istrm.read_u16();
                size.y = istrm.read_u16();
                graphicsMode = (graphics_mode_t)istrm.read_u8();
                flags = (image_flag_t)istrm.read_u8();
                #if 0
                if (graphicsMode <= graphics_mode_t::GRAPHICS3)
                {
                    paletteEntries = istrm.read_u8();
                    for (size_t i = 0; i < palette.size(); ++i) // where is this size coming from???
                        palette[i] = ColorFrom32bitRGBA(strm); // not sure if RGBA or BGRA
                    count = strm.read_u32();
                }
                #endif
                if (!game.oldGame) unknown = istrm.read_u16();
                hotspot.x = istrm.read_u16();
                hotspot.y = istrm.read_u16();
                action.x = istrm.read_u16();
                action.y = istrm.read_u16();
                if (!game.oldGame) transparent = ColorFrom32bitRGBA(istrm);
                dataPosition = istrm.position;
            }

            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Image##%zX", (size_t)entry.handle, entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Text("Checksum: 0x%zX", (size_t)checksum);
                ImGui::Text("Reference: 0x%zX", (size_t)reference);
                ImGui::Text("Data Size: 0x%zX", (size_t)dataSize);
                ImGui::Text("Image Size: %zu * %zu", (size_t)size.x, (size_t)size.y);
                ImGui::Text("Graphics Mode: 0x%zX", (size_t)graphicsMode);
                ImGui::Text("Image Flags: 0x%zX", (size_t)flags);
                ImGui::Text("Unknown: 0x%zX", (size_t)unknown);
                ImGui::Text("Hotspot: (%zu, %zu)", (size_t)hotspot.x, (size_t)hotspot.y);
                ImGui::Text("Action: (%zu, %zu)", (size_t)action.x, (size_t)action.y);
                {
                    lak::vec4f_t col = ((lak::vec4f_t)transparent) / 255.0f;
                    ImGui::ColorEdit4("Transparent", &col.x);
                }
                ImGui::Text("Data Position: 0x%zX", dataPosition);

                if (ImGui::Button("View Image"))
                {
                    srcexp.image = std::move(CreateTexture(image(srcexp.dumpColorTrans)));
                }

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        lak::memory item_t::image_data() const
        {
            lak::memory strm = entry.decode().seek(dataPosition);
            if ((flags & image_flag_t::LZX) != image_flag_t::NONE)
            {
                [[maybe_unused]] const uint32_t decomplen = strm.read_u32();
                return lak::memory(Inflate(strm.read(strm.read_u32())));
            }
            else
            {
                return strm;
            }
        }

        lak::image4_t item_t::image(const bool colorTrans, const lak::color4_t palette[256]) const
        {
            lak::image4_t result;
            result.resize(size);

            lak::memory strm = image_data();

            [[maybe_unused]] size_t bytesRead;
            if ((flags & (image_flag_t::RLE | image_flag_t::RLEW | image_flag_t::RLET)) != image_flag_t::NONE)
            {
                bytesRead = ReadRLE(strm, result, graphicsMode, palette);
            }
            else
            {
                bytesRead = ReadRGB(strm, result, graphicsMode, palette);
            }
            DEBUG("Alpha Size: 0x" << (dataSize - bytesRead));

            if ((flags & image_flag_t::ALPHA) != image_flag_t::NONE)
            {
                ReadAlpha(strm, result);
            }
            else if (colorTrans)
            {
                ReadTransparent(transparent, result);
            }

            return result;
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Image Bank End");
        }

        error_t bank_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Image Bank");
            error_t result = entry.read(game, strm);

            strm.position = entry.data.position;

            items.resize(strm.read_u32());

            for (auto &item : items)
            {
                if (result != error_t::OK) break;
                result = item.read(game, strm);
            }

            strm.position = entry.end;

            if ((chunk_t)strm.peek_u16() == chunk_t::ENDIMAGE)
            {
                end = std::make_unique<end_t>();
                if (end) result = end->read(game, strm);
            }

            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Image Bank (%zu Items)##%zX",
                (size_t)entry.ID, items.size(), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const item_t &item : items)
                    item.view(srcexp);

                if (end) end->view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }
    }

    namespace font
    {
        error_t item_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Font");
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Font Bank End");
        }

        error_t bank_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Font Bank");
            error_t result = entry.read(game, strm);

            strm.position = entry.data.position;

            items.resize(strm.read_u32());

            for (auto &item : items)
            {
                if (result != error_t::OK) break;
                result = item.read(game, strm);
            }

            strm.position = entry.end;

            if ((chunk_t)strm.peek_u16() == chunk_t::ENDFONT)
            {
                end = std::make_unique<end_t>();
                if (end) result = end->read(game, strm);
            }

            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Font Bank (%zu Items)##%zX",
                (size_t)entry.ID, items.size(), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const item_t &item : items)
                    item.view(srcexp);

                if (end) end->view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }
    }

    namespace sound
    {
        error_t item_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Sound");
            error_t result = entry.read(game, strm, false, 0x18);
            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Sound");
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Sound Bank End");
        }

        error_t bank_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Sound Bank");
            error_t result = entry.read(game, strm);

            strm.position = entry.data.position;

            items.resize(strm.read_u32());

            for (auto &item : items)
            {
                if (result != error_t::OK) break;
                result = item.read(game, strm);
            }

            strm.position = entry.end;

            if ((chunk_t)strm.peek_u16() == chunk_t::ENDSOUND)
            {
                end = std::make_unique<end_t>();
                if (end) result = end->read(game, strm);
            }

            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Sound Bank (%zu Items)##%zX",
                (size_t)entry.ID, items.size(), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const item_t &item : items)
                    item.view(srcexp);

                if (end) end->view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }
    }

    namespace music
    {
        error_t item_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Music");
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            return basic_view(srcexp, "Music Bank End");
        }

        error_t bank_t::read(game_t &game, lak::memory &strm)
        {
            DEBUG("Reading Music Bank");
            error_t result = entry.read(game, strm);

            strm.position = entry.data.position;

            items.resize(strm.read_u32());

            for (auto &item : items)
            {
                if (result != error_t::OK) break;
                result = item.read(game, strm);
            }

            strm.position = entry.end;

            if ((chunk_t)strm.peek_u16() == chunk_t::ENDMUSIC)
            {
                end = std::make_unique<end_t>();
                if (end) result = end->read(game, strm);
            }

            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Music Bank (%zu Items)##%zX",
                (size_t)entry.ID, items.size(), entry.position))
            {
                ImGui::Separator();

                entry.view(srcexp);

                for (const item_t &item : items)
                    item.view(srcexp);

                if (end) end->view(srcexp);

                ImGui::Separator();

                ImGui::TreePop();
            }

            return result;
        }
    }

    error_t last_t::view(source_explorer_t &srcexp) const
    {
        return basic_view(srcexp, "Last");
    }

    error_t header_t::read(game_t &game, lak::memory &strm)
    {
        DEBUG("Reading Header");
        error_t result = entry.read(game, strm);

        while (result == error_t::OK)
        {
            chunk_t childID = (chunk_t)strm.peek_u16();
            switch (childID)
            {
                case chunk_t::TITLE:
                    DEBUG("Reading Title");
                    title = std::make_unique<string_chunk_t>();
                    result = title->read(game, strm);
                    break;

                case chunk_t::AUTHOR:
                    DEBUG("Reading Author");
                    author = std::make_unique<string_chunk_t>();
                    result = author->read(game, strm);
                    break;

                case chunk_t::COPYRIGHT:
                    DEBUG("Reading Copyright");
                    copyright = std::make_unique<string_chunk_t>();
                    result = copyright->read(game, strm);
                    break;

                case chunk_t::PROJPATH:
                    DEBUG("Reading Project Path");
                    projectPath = std::make_unique<string_chunk_t>();
                    result = projectPath->read(game, strm);
                    break;

                case chunk_t::OUTPATH:
                    DEBUG("Reading Project Output Path");
                    outputPath = std::make_unique<string_chunk_t>();
                    result = outputPath->read(game, strm);
                    break;

                case chunk_t::ABOUT:
                    DEBUG("Reading About");
                    about = std::make_unique<string_chunk_t>();
                    result = about->read(game, strm);
                    break;

                case chunk_t::VITAPREV:
                    DEBUG("Reading Project Vitalise Preview");
                    vitalisePreview = std::make_unique<vitalise_preview_t>();
                    result = vitalisePreview->read(game, strm);
                    break;

                case chunk_t::MENU:
                    DEBUG("Reading Project Menu");
                    menu = std::make_unique<menu_t>();
                    result = menu->read(game, strm);
                    break;

                case chunk_t::EXTPATH:
                    DEBUG("Reading Extension Path");
                    extensionPath = std::make_unique<extension_path_t>();
                    result = extensionPath->read(game, strm);
                    break;

                case chunk_t::EXTENS:
                    DEBUG("Reading Extensions");
                    extensions = std::make_unique<extensions_t>();
                    result = extensions->read(game, strm);
                    break;

                case chunk_t::EXTDATA:
                    DEBUG("Reading Extension Data");
                    extensionData = std::make_unique<extension_data_t>();
                    result = extensionData->read(game, strm);
                    break;

                case chunk_t::ADDEXTNS:
                    DEBUG("Reading Additional Extensions");
                    additionalExtensions = std::make_unique<additional_extensions_t>();
                    result = additionalExtensions->read(game, strm);
                    break;

                case chunk_t::APPDOC:
                    DEBUG("Reading Application Doc");
                    appDoc = std::make_unique<application_doc_t>();
                    result = appDoc->read(game, strm);
                    break;

                case chunk_t::OTHEREXT:
                    DEBUG("Reading Other Extension");
                    otherExtension = std::make_unique<other_extenion_t>();
                    result = otherExtension->read(game, strm);
                    break;

                case chunk_t::EXTNLIST:
                    DEBUG("Reading Extension List");
                    extensionList = std::make_unique<extension_list_t>();
                    result = extensionList->read(game, strm);
                    break;

                case chunk_t::ICON:
                    DEBUG("Reading Icon");
                    icon = std::make_unique<icon_t>();
                    result = icon->read(game, strm);
                    break;

                case chunk_t::DEMOVER:
                    DEBUG("Reading Demo Version");
                    demoVersion = std::make_unique<demo_version_t>();
                    result = demoVersion->read(game, strm);
                    break;

                case chunk_t::SECNUM:
                    DEBUG("Reading Security Number");
                    security = std::make_unique<security_number_t>();
                    result = security->read(game, strm);
                    break;

                case chunk_t::BINFILES:
                    DEBUG("Reading Binary Files");
                    binaryFiles = std::make_unique<binary_files_t>();
                    result = binaryFiles->read(game, strm);
                    break;

                case chunk_t::MENUIMAGES:
                    DEBUG("Reading Menu Images");
                    menuImages = std::make_unique<menu_images_t>();
                    result = menuImages->read(game, strm);
                    break;

                case chunk_t::MOVEMNTEXTNS:
                    DEBUG("Reading Movement Extensions");
                    movementExtensions = std::make_unique<movement_extensions_t>();
                    result = movementExtensions->read(game, strm);
                    break;

                case chunk_t::EXEONLY:
                    DEBUG("Reading EXE Only");
                    exe = std::make_unique<exe_t>();
                    result = exe->read(game, strm);
                    break;

                case chunk_t::PROTECTION:
                    DEBUG("Reading Protection");
                    protection = std::make_unique<protection_t>();
                    result = protection->read(game, strm);
                    break;

                case chunk_t::SHADERS:
                    DEBUG("Reading Shaders");
                    shaders = std::make_unique<shaders_t>();
                    result = shaders->read(game, strm);
                    break;

                case chunk_t::EXTDHEADER:
                    DEBUG("Reading Extended Header");
                    extendedHeader = std::make_unique<extended_header_t>();
                    result = extendedHeader->read(game, strm);
                    break;

                case chunk_t::SPACER:
                    DEBUG("Reading Spacer");
                    spacer = std::make_unique<spacer_t>();
                    result = spacer->read(game, strm);
                    break;

                case chunk_t::CHUNK224F:
                    DEBUG("Reading Chunk 224F");
                    chunk224F = std::make_unique<chunk_224F_t>();
                    result = chunk224F->read(game, strm);
                    break;

                case chunk_t::TITLE2:
                    DEBUG("Reading Title 2");
                    title2 = std::make_unique<title2_t>();
                    result = title2->read(game, strm);
                    break;

                case chunk_t::GLOBALEVENTS:
                    DEBUG("Reading Global Events");
                    globalEvents = std::make_unique<global_events_t>();
                    result = globalEvents->read(game, strm);
                    break;

                case chunk_t::GLOBALSTRS:
                    DEBUG("Reading Global Strings");
                    globalStrings = std::make_unique<global_strings_t>();
                    result = globalStrings->read(game, strm);
                    break;

                case chunk_t::GLOBALSTRNAMES:
                    DEBUG("Reading Global String Names");
                    globalStringNames = std::make_unique<global_string_names_t>();
                    result = globalStringNames->read(game, strm);
                    break;

                case chunk_t::GLOBALVALS:
                    DEBUG("Reading Global Values");
                    globalValues = std::make_unique<global_values_t>();
                    result = globalValues->read(game, strm);
                    break;

                case chunk_t::GLOBALVALNAMES:
                    DEBUG("Reading Global Value Names");
                    globalValueNames = std::make_unique<global_value_names_t>();
                    result = globalValueNames->read(game, strm);
                    break;

                case chunk_t::FRAMEHANDLES:
                    DEBUG("Reading Frame Handles");
                    frameHandles = std::make_unique<frame::handles_t>();
                    result = frameHandles->read(game, strm);
                    break;

                case chunk_t::FRAMEBANK:
                    DEBUG("Reading Fame Bank");
                    frameBank = std::make_unique<frame::bank_t>();
                    result = frameBank->read(game, strm);
                    break;

                case chunk_t::FRAME:
                    DEBUG("Reading Frame (Missing Frame Bank)");
                    if (frameBank) ERROR("Frame Bank Already Exists");
                    frameBank = std::make_unique<frame::bank_t>();
                    frameBank->items.clear();
                    while (result == error_t::OK && (chunk_t)strm.peek_u16() == chunk_t::FRAME) {
                            frameBank->items.emplace_back();
                            result = frameBank->items.back().read(game, strm);
                    }
                    break;

                // case chunk_t::OBJECTBANK2:
                //     DEBUG("Reading Object Bank 2");
                //     objectBank2 = std::make_unique<object_bank2_t>();
                //     result = objectBank2->read(game, strm);
                //     break;

                case chunk_t::OBJECTBANK:
                case chunk_t::OBJECTBANK2:
                    DEBUG("Reading Object Bank");
                    objectBank = std::make_unique<object::bank_t>();
                    result = objectBank->read(game, strm);
                    break;

                case chunk_t::IMAGEBANK:
                    DEBUG("Reading Image Bank");
                    imageBank = std::make_unique<image::bank_t>();
                    result = imageBank->read(game, strm);
                    break;

                case chunk_t::SOUNDBANK:
                    DEBUG("Reading Sound Bank");
                    soundBank = std::make_unique<sound::bank_t>();
                    result = soundBank->read(game, strm);
                    break;

                case chunk_t::MUSICBANK:
                    DEBUG("Reading Music Bank");
                    musicBank = std::make_unique<music::bank_t>();
                    result = musicBank->read(game, strm);
                    break;

                case chunk_t::FONTBANK:
                    DEBUG("Reading Font Bank");
                    fontBank = std::make_unique<font::bank_t>();
                    result = fontBank->read(game, strm);
                    break;

                case chunk_t::LAST:
                    DEBUG("Reading Last");
                    last = std::make_unique<last_t>();
                    result = last->read(game, strm);
                    goto finished;

                default:
                    DEBUG("Invalid Chunk: 0x" << (size_t)childID);
                    result = error_t::INVALID_CHUNK;
                    break;
            }
        }

        finished:
        return result;
    }

    error_t header_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Game Header##%zX", (size_t)entry.ID, entry.position))
        {
            ImGui::Separator();

            entry.view(srcexp);

            if (title) title->view(srcexp, "Title", true);
            if (author) author->view(srcexp, "Author", true);
            if (copyright) copyright->view(srcexp, "Copyright", true);
            if (outputPath) outputPath->view(srcexp, "Output Path");
            if (projectPath) projectPath->view(srcexp, "Project Path");
            if (about) about->view(srcexp, "About");

            if (vitalisePreview) vitalisePreview->view(srcexp);
            if (menu) menu->view(srcexp);
            if (extensionPath) extensionPath->view(srcexp);
            if (extensions) extensions->view(srcexp);
            if (extensionData) extensionData->view(srcexp);
            if (additionalExtensions) additionalExtensions->view(srcexp);
            if (appDoc) appDoc->view(srcexp);
            if (otherExtension) otherExtension->view(srcexp);
            if (extensionList) extensionList->view(srcexp);
            if (icon) icon->view(srcexp);
            if (demoVersion) demoVersion->view(srcexp);
            if (security) security->view(srcexp);
            if (binaryFiles) binaryFiles->view(srcexp);
            if (menuImages) menuImages->view(srcexp);
            if (movementExtensions) movementExtensions->view(srcexp);
            if (objectBank2) objectBank2->view(srcexp);
            if (exe) exe->view(srcexp);
            if (protection) protection->view(srcexp);
            if (shaders) shaders->view(srcexp);
            if (extendedHeader) extendedHeader->view(srcexp);
            if (spacer) spacer->view(srcexp);
            if (chunk224F) chunk224F->view(srcexp);
            if (title2) title2->view(srcexp);

            if (globalEvents) globalEvents->view(srcexp);
            if (globalStrings) globalStrings->view(srcexp);
            if (globalStringNames) globalStringNames->view(srcexp);
            if (globalValues) globalValues->view(srcexp);
            if (globalValueNames) globalValueNames->view(srcexp);

            if (frameHandles) frameHandles->view(srcexp);
            if (frameBank) frameBank->view(srcexp);
            if (objectBank) objectBank->view(srcexp);
            if (imageBank) imageBank->view(srcexp);
            if (soundBank) soundBank->view(srcexp);
            if (musicBank) musicBank->view(srcexp);
            if (fontBank) fontBank->view(srcexp);

            if (last) last->view(srcexp);

            ImGui::Separator();

            ImGui::TreePop();
        }

        return result;
    }
}

#include "encryption.cpp"
#include "image.cpp"
