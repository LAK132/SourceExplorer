#include "explorer.h"

namespace SourceExplorer
{
    bool debugConsole = true;
    m128i_t _xmmword;
    std::vector<uint8_t> _magic_key;
    uint8_t _magic_char;

    std::vector<uint8_t> &operator += (std::vector<uint8_t> &lhs, const std::vector<uint8_t> &rhs)
    {
        lhs.insert(lhs.cend(), rhs.cbegin(), rhs.cend());
        return lhs;
    }

    error_t LoadGame(source_explorer_t &srcexp)
    {
        DEBUG("\nLoading Game");
        DEBUG("Path: '" << srcexp.file.u8string() << "'");

        srcexp.state = game_t{};

        srcexp.state.file.memory = lak::LoadFile(srcexp.file);

        DEBUG("File Size: 0x" << srcexp.state.file.memory.size());

        error_t err = ParsePEHeader(srcexp.state.file, srcexp.state);
        if (err != error_t::OK)
        {
            ERROR("Error Parsing PE Header " << (uint32_t)err);
            return err;
        }

        DEBUG("Successfully Parsed PE Header");

        err = srcexp.state.game.read(srcexp.state, srcexp.state.file);
        if (err != error_t::OK)
        {
            ERROR("Error Reading Entry " << (uint32_t)err);

            // we can try to recover from this
            // return err;
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

        _xmmword.m128i_u32[0] = 0;
        _xmmword.m128i_u32[1] = 1;
        _xmmword.m128i_u32[2] = 2;
        _xmmword.m128i_u32[3] = 3;

        _magic_char = '6';

        _magic_key.clear();
        _magic_key = KeyString(srcexp.state.title);
        _magic_key.reserve(256);
        if (_magic_key.size() < 256)
            _magic_key += KeyString(srcexp.state.copyright);
        if (_magic_key.size() < 256)
            _magic_key += KeyString(srcexp.state.project);

        _magic_key.resize(256);

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

        return err;
    }

    error_t ParsePEHeader(lak::memstrm_t &strm, game_t &gameState)
    {
        DEBUG("\nParsing PE header");

        uint16_t exeSig = strm.readInt<uint16_t>();
        DEBUG("EXE Signature: 0x" << exeSig);
        if (exeSig != WIN_EXE_SIG)
            return INVALID_EXE_SIGNATURE;

        strm.position = WIN_EXE_PNT;
        strm.position = strm.readInt<uint16_t>();
        DEBUG("EXE Pointer: 0x" << strm.position);

        int32_t peSig = strm.readInt<int32_t>();
        DEBUG("PE Signature: 0x" << peSig);
        DEBUG("Pos: 0x" << strm.position);
        if (peSig != WIN_PE_SIG)
            return INVALID_PE_SIGNATURE;

        strm.position += 2;

        uint16_t numHeaderSections = strm.readInt<uint16_t>();
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
            std::string name = strm.readString<char>();
            DEBUG("Name: " << name);
            if (name == ".extra")
            {
                strm.position = start + 0x14;
                pos = strm.readInt<int32_t>();
                break;
            }
            else if (i >= numHeaderSections - 1)
            {
                strm.position = start + 0x10;
                uint32_t size = strm.readInt<uint32_t>();
                uint32_t addr = strm.readInt<uint32_t>();
                DEBUG("Size: 0x" << size);
                DEBUG("Addr: 0x" << addr);
                pos = size + addr;
                break;
            }
            strm.position = start + 0x28;
            DEBUG("Pos: 0x" << strm.position);
        }

        strm.position = pos;
        uint16_t firstShort = strm.readInt<uint16_t>();
        strm.position = pos;
        uint32_t pameMagic = strm.readInt<uint32_t>();
        strm.position = pos;
        uint64_t packMagic = strm.readInt<uint64_t>();
        strm.position = pos;

        if (firstShort == HEADER || pameMagic == HEADER_GAME)
        {
            DEBUG("Old Game");
            gameState.oldGame = true;
            gameState.state = {}; // gameState.state.clear();
            gameState.state.push(OLD);
        }
        else if (packMagic == HEADERPACK)
        {
            DEBUG("New Game");
            gameState.oldGame = false;
            gameState.state = {}; // gameState.state.clear();
            gameState.state.push(NEW);
            pos = ParsePackData(strm, gameState);
        }
        else
            return INVALID_GAME_HEADER;

        uint32_t header = strm.readInt<uint32_t>();
        DEBUG("Header: 0x" << header);

        gameState.unicode = false;
        if (header == HEADER_UNIC)
        {
            gameState.unicode = true;
            gameState.oldGame = false;
        }
        else if (header != HEADER_GAME)
            return INVALID_GAME_HEADER;

        do {
            gameState.runtimeVersion = strm.readInt<product_code_t>();
            DEBUG("Runtime Version: 0x" << gameState.runtimeVersion);
            if (gameState.runtimeVersion == CNCV1VER)
            {
                DEBUG("CNCV1VER");
                // cnc = true;
                // readCNC(strm);
                break;
            }
            gameState.runtimeSubVersion = strm.readInt<uint16_t>();
            DEBUG("Runtime Sub-Version: 0x" << gameState.runtimeSubVersion);
            gameState.productVersion = strm.readInt<uint32_t>();
            DEBUG("Product Version: 0x" << gameState.productVersion);
            gameState.productBuild = strm.readInt<uint32_t>();
            DEBUG("Product Build: 0x" << gameState.productBuild);
        } while(0);

        return error_t::OK;
    }

    uint64_t ParsePackData(lak::memstrm_t &strm, game_t &gameState)
    {
        DEBUG("\nParsing pack data");

        uint64_t start = strm.position;
        uint64_t header = strm.readInt<uint64_t>();
        DEBUG("Header: 0x" << header);
        uint32_t headerSize = strm.readInt<uint32_t>(); (void)headerSize;
        DEBUG("Header Size: 0x" << headerSize);
        uint32_t dataSize = strm.readInt<uint32_t>();
        DEBUG("Data Size: 0x" << dataSize);

        strm.position = start + dataSize - 0x20;

        header = strm.readInt<uint32_t>();
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

        uint32_t formatVersion = strm.readInt<uint32_t>(); (void) formatVersion;
        DEBUG("Format Version: 0x" << formatVersion);

        strm.position += 0x8;

        int32_t count = strm.readInt<int32_t>();
        assert(count >= 0);
        DEBUG("Pack Count: 0x" << count);

        uint64_t off = strm.position;
        DEBUG("Offset: 0x" << off);

        for (int32_t i = 0; i < count; ++i)
        {
            if ((strm.memory.size() - strm.position) < 2)
                break;

            uint32_t val = strm.readInt<uint16_t>();
            if ((strm.memory.size() - strm.position) < val)
                break;

            strm.position += val;
            if ((strm.memory.size() - strm.position) < 4)
                break;

            val = strm.readInt<uint32_t>();
            if ((strm.memory.size() - strm.position) < val)
                break;

            strm.position += val;
        }

        header = strm.readInt<uint32_t>();
        DEBUG("Header: 0x" << header);

        bool hasBingo = (header != HEADER_GAME) && (header != HEADER_UNIC);
        DEBUG("Has Bingo: " << (hasBingo ? "true" : "false"));

        strm.position = off;

        gameState.packFiles.resize(count);

        for (int32_t i = 0; i < count; ++i)
        {
            uint32_t read = strm.readInt<uint16_t>();
            // size_t strstart = strm.position;

            DEBUG("Pack 0x" << i+1 << " of 0x" << count << ", filename length: 0x" << read);

            if (unicode)
                gameState.packFiles[i].filename = strm.readString<char16_t>(read);
            else
                gameState.packFiles[i].filename = lak::strconv<char16_t>(strm.readString<char>(read));

            // strm.position = strstart + (unicode ? read * 2 : read);

            // DEBUG("String Start: 0x" << strstart);
            WDEBUG(L"Packfile '" << lak::strconv<wchar_t>(gameState.packFiles[i].filename) << L"'");
            DEBUG("Packfile '" << lak::strconv<char>(gameState.packFiles[i].filename) << "'");

            if (hasBingo)
                gameState.packFiles[i].bingo = strm.readInt<uint32_t>();
            else
                gameState.packFiles[i].bingo = 0;

            DEBUG("Bingo: 0x" << gameState.packFiles[i].bingo);

            if (unicode)
                read = strm.readInt<uint32_t>();
            else
                read = strm.readInt<uint16_t>();

            DEBUG("Pack File Data Size: 0x" << read);
            gameState.packFiles[i].data = strm.readBytes(read);
        }

        header = strm.readInt<uint32_t>(); // PAMU sometimes
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
            lak::memstrm_t strm(decode);
            if (unicode)
                return strm.readString<char16_t>();
            else
                return lak::strconv<char16_t>(strm.readString<char>());
        }
        else
        {
            auto decode = entry.decodeHeader();
            lak::memstrm_t strm(decode);
            if (unicode)
                return strm.readString<char16_t>();
            else
                return lak::strconv<char16_t>(strm.readString<char>());
        }
    }

    error_t ReadFixedData(lak::memstrm_t &strm, data_point_t &data, const size_t size)
    {
        data.position = strm.position;
        data.expectedSize = 0;
        data.data = strm.readBytes(size);
        return error_t::OK;
    }

    error_t ReadDynamicData(lak::memstrm_t &strm, data_point_t &data)
    {
        return ReadFixedData(strm, data, strm.readInt<uint32_t>());
    }

    error_t ReadCompressedData(lak::memstrm_t &strm, data_point_t &data)
    {
        data.position = strm.position;
        data.expectedSize = strm.readInt<uint32_t>();
        data.data = strm.readBytes(strm.readInt<uint32_t>());
        return error_t::OK;
    }

    error_t ReadReverseCompressedData(lak::memstrm_t &strm, data_point_t &data)
    {
        data.position = strm.position;
        data.data = strm.readBytes(strm.readInt<uint32_t>());
        data.expectedSize = strm.readInt<uint32_t>();
        return error_t::OK;
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

    std::vector<uint8_t> Decode(const std::vector<uint8_t> &encoded, chunk_t id, encoding_t mode)
    {
        switch (mode)
        {
            case MODE3:
            case MODE2:
                return Decrypt(encoded, id, mode);
            case MODE1:
                return Decompress(encoded);
            default:
                if (encoded.size() > 0 && encoded[0] == 0x78)
                    return Decompress(encoded);
                else
                    return encoded;
        }
    }

    std::vector<uint8_t> Decompress(const std::vector<uint8_t> &compressed)
    {
        std::deque<uint8_t> buffer;
        lak::tinflate_error_t err = lak::tinflate(compressed, buffer, nullptr);
        if (err == lak::tinflate_error_t::OK)
            return std::vector<uint8_t>(buffer.begin(), buffer.end());
        else
            return compressed;
    }

    std::vector<uint8_t> Decrypt(const std::vector<uint8_t> &encrypted, chunk_t ID, encoding_t mode)
    {
        if (mode == MODE3)
        {
            if (encrypted.size() <= 4) return encrypted;
            // TODO: check endian
            // size_t dataLen = *reinterpret_cast<const uint32_t*>(&encrypted[0]);
            std::vector<uint8_t> mem(encrypted.begin() + 4, encrypted.end());

            if ((ID & 0x1) != 0)
                mem[0] ^= (ID & 0xFF) ^ (ID >> 0x8);

            if (DecodeChunk(mem, _magic_key, _xmmword))
            {
                if (mem.size() <= 4) return mem;
                // dataLen = *reinterpret_cast<uint32_t*>(&mem[0]);
                return Decompress(std::vector<uint8_t>(mem.begin()+4, mem.end()));
            }
            DEBUG("MODE 3 Decryption Failed");
            return mem;
        }
        else
        {
            if (encrypted.size() < 1) return encrypted;

            std::vector<uint8_t> mem = encrypted;

            if (ID & 0x1)
                mem[0] ^= (ID & 0xFF) ^ (ID >> 0x8);

            if (!DecodeChunk(mem, _magic_key, _xmmword))
                DEBUG("MODE 2 Decryption Failed");
            return mem;
        }
    }

    lak::memstrm_t resource_entry_t::streamHeader() const
    {
        return header.data;
    }

    lak::memstrm_t resource_entry_t::stream() const
    {
        return data.data;
    }

    lak::memstrm_t resource_entry_t::decodeHeader() const
    {
        return header.decode(ID, mode);
    }

    lak::memstrm_t resource_entry_t::decode() const
    {
        return data.decode(ID, mode);
    }

    lak::memstrm_t data_point_t::decode(const chunk_t ID, const encoding_t mode) const
    {
        return lak::memstrm_t(Decode(data.memory, ID, mode));
    }

    void entry_t::read(game_t &game, lak::memstrm_t &strm)
    {
        position = strm.position;
        ID = strm.readInt<chunk_t>();
        mode = strm.readInt<encoding_t>();
    }

    void entry_t::view(source_explorer_t &srcexp) const
    {
        ImGui::Text("Position: 0x%zX", position);
        ImGui::Text("ID: 0x%zX", (size_t)ID);
        ImGui::Text("Mode: MODE%zu", (size_t)mode);

        ImGui::Text("Header Position: 0x%zX", header.position);
        ImGui::Text("Header Size: 0x%zX", header.data.size());

        ImGui::Text("Data Position: 0x%zX", data.position);
        ImGui::Text("Data Size: 0x%zX", data.data.size());

        if (ImGui::Button("View Memory"))
            srcexp.view = this;
    }

    lak::memstrm_t entry_t::decode() const
    {
        return data.decode(ID, mode);
    }

    lak::memstrm_t entry_t::decodeHeader() const
    {
        return header.decode(ID, mode);
    }

    // All MAIN_COMP seems to also be PRE_READ4, this might not be coincidental?

    error_t title_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: result = error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
                if (game.unicode)
                    value = entry.decode().readString<char16_t>();
                else
                    value = lak::strconv<char16_t>(entry.decode().readString<char>());
            } break;
            case MODE2: result = error_t::NO_MODE2;
            case MODE3: result = error_t::NO_MODE3;
            default: result = error_t::INVALID_MODE;
        }

        return result;
    }

    error_t title_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Title", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);
            ImGui::Text("Value: '%s'", lak::strconv<char>(value).c_str());

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t author_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: result = error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
                if (game.unicode)
                    value = entry.decode().readString<char16_t>();
                else
                    value = lak::strconv<char16_t>(entry.decode().readString<char>());
            } break;
            case MODE2: result = error_t::NO_MODE2;
            case MODE3: result = error_t::NO_MODE3;
            default: result = error_t::INVALID_MODE;
        }

        return result;
    }

    error_t author_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Author", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);
            ImGui::Text("Value: '%s'", lak::strconv<char>(value).c_str());

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t copyright_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: result = error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
                if (game.unicode)
                    value = entry.decode().readString<char16_t>();
                else
                    value = lak::strconv<char16_t>(entry.decode().readString<char>());
            } break;
            case MODE2: result = error_t::NO_MODE2;
            case MODE3: result = error_t::NO_MODE3;
            default: result = error_t::INVALID_MODE;
        }

        return result;
    }

    error_t copyright_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Copyright", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);
            ImGui::Text("Value: '%s'", lak::strconv<char>(value).c_str());

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t output_path_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: result = error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
                if (game.unicode)
                    value = entry.decode().readString<char16_t>();
                else
                    value = lak::strconv<char16_t>(entry.decode().readString<char>());
            } break;
            case MODE2: result = error_t::NO_MODE2;
            case MODE3: result = error_t::NO_MODE3;
            default: result = error_t::INVALID_MODE;
        }

        return result;
    }

    error_t output_path_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Output Path", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);
            ImGui::Text("Value: '%s'", lak::strconv<char>(value).c_str());

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t project_path_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: result = error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
                if (game.unicode)
                    value = entry.decode().readString<char16_t>();
                else
                    value = lak::strconv<char16_t>(entry.decode().readString<char>());
            } break;
            case MODE2: result = error_t::NO_MODE2;
            case MODE3: result = error_t::NO_MODE3;
            default: result = error_t::INVALID_MODE;
        }

        return result;
    }

    error_t project_path_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Project Path", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);
            ImGui::Text("Value: '%s'", lak::strconv<char>(value).c_str());

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t vitalise_preview_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t vitalise_preview_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Vitalise Preview", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t menu_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: return error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t menu_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Menu", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t extension_path_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t extension_path_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Extension Path", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t extensions_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t extensions_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Extensions", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t global_events_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t global_events_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Global Events", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t extension_data_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t extension_data_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Extension Data", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t additional_extensions_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t additional_extensions_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Additional Extensions", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t application_doc_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t application_doc_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Application Doc", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t other_extenion_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t other_extenion_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Other Extension", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t global_values_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t global_values_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Global Values", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t global_strings_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t global_strings_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Global Strings", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t extension_list_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t extension_list_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Extension List", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t icon_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: return error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t icon_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Icon", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t demo_version_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t demo_version_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Demo Version", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t security_number_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: return error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t security_number_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Security Number", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t binary_files_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t binary_files_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Binary Files", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t menu_images_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t menu_images_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Menu Images", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t about_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t about_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX About", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t global_value_names_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t global_value_names_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Global Value Names", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t global_string_names_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t global_string_names_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Global String Names", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t movement_extensions_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t movement_extensions_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Movement Extensions", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t object_bank2_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x8);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t object_bank2_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Object Bank 2", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t exe_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t exe_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX EXE Only", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t protection_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: result = error_t::NO_MODE2;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: result = error_t::NO_MODE3;
            default: result = error_t::INVALID_MODE;
        }

        return result;
    }

    error_t protection_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Protection", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t shaders_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: return error_t::NO_MODE0;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t shaders_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Shaders", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t extended_header_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t extended_header_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Extended Header", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t spacer_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t spacer_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Spacer", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t chunk_224F_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: {
                ReadDynamicData(strm, entry.data);
            } break;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t chunk_224F_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Chunk 224F", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t title2_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadDynamicData(strm, entry.header);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t title2_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Title 2", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    namespace object
    {
        error_t name_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: return error_t::NO_MODE2;
                case MODE3: return error_t::NO_MODE3;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t name_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Name", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t properties_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t properties_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Properties", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t effect_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t effect_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Effect", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: result = error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: result = error_t::INVALID_MODE;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case OBJNAME:
                        name = std::make_unique<name_t>();
                        result = name->read(game, strm);
                        break;

                    case OBJPROP:
                        properties = std::make_unique<properties_t>();
                        result = properties->read(game, strm);
                        break;

                    case OBJEFCT:
                        effect = std::make_unique<effect_t>();
                        result = effect->read(game, strm);

                    case LAST:
                        end = std::make_unique<last_t>();
                        result = end->read(game, strm);
                        goto finished;

                    default: result = error_t::INVALID_CHUNK; break;
                }
            }

            finished:
            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Object", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x8);
                } break;
                case MODE1: result = error_t::NO_MODE1;
                case MODE2: result = error_t::NO_MODE2;
                case MODE3: result = error_t::NO_MODE3;
                default: result = error_t::INVALID_MODE;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case OBJHEAD:
                        items.emplace_back();
                        result = items.back().read(game, strm);
                        break;

                    default: goto finished;
                }
            }

            finished:
            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Object Bank", (size_t)entry.ID))
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
        error_t handles_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t handles_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Frame Handles", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t name_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: return error_t::NO_MODE2;
                case MODE3: return error_t::NO_MODE3;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t name_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Name", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t header_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t header_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Header", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t password_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t password_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Password", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t palette_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t palette_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Palette", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t object_instance_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t object_instance_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Object Instance", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t fade_in_frame_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t fade_in_frame_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Fade In Frame", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t fade_out_frame_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t fade_out_frame_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Fade Out Frame", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t fade_in_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t fade_in_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Fade In", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t fade_out_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t fade_out_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Fade Out", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t events_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t events_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Events", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t play_head_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t play_head_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Play Head", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t additional_item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t additional_item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Additional Item", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t additional_item_instance_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t additional_item_instance_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Additional Item Instance", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t layers_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t layers_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Layers", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t virtual_size_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t virtual_size_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Virtual Size", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t demo_file_path_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t demo_file_path_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Demo File Path", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t random_seed_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t random_seed_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Random Seed", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t layer_effect_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t layer_effect_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Layer Effect", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t blueray_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t blueray_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Blueray", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t movement_time_base_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE1: return error_t::NO_MODE1;
                case MODE2: return error_t::NO_MODE2;
                case MODE3: return error_t::NO_MODE3;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t movement_time_base_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Movement Time Base", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t mosaic_image_table_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t mosaic_image_table_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Mosaic Image Table", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t effects_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: return error_t::NO_MODE0;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: return error_t::NO_MODE2;
                case MODE3: return error_t::NO_MODE3;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t effects_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Effects", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t iphone_options_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t iphone_options_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX iPhone Options", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: result = error_t::INVALID_MODE;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case FRAMENAME:
                        name = std::make_unique<name_t>();
                        result = name->read(game, strm);
                        break;

                    case FRAMEHEADER:
                        header = std::make_unique<header_t>();
                        result = header->read(game, strm);
                        break;

                    case FRAMEPASSWORD:
                        password = std::make_unique<password_t>();
                        result = password->read(game, strm);
                        break;

                    case FRAMEPALETTE:
                        palette = std::make_unique<palette_t>();
                        result = palette->read(game, strm);
                        break;

                    case OBJINST:
                        objectInstance = std::make_unique<object_instance_t>();
                        result = objectInstance->read(game, strm);
                        break;

                    case FRAMEFADEIF:
                        fadeInFrame = std::make_unique<fade_in_frame_t>();
                        result = fadeInFrame->read(game, strm);
                        break;

                    case FRAMEFADEOF:
                        fadeOutFrame = std::make_unique<fade_out_frame_t>();
                        result = fadeOutFrame->read(game, strm);
                        break;

                    case FRAMEFADEI:
                        fadeIn = std::make_unique<fade_in_t>();
                        result = fadeIn->read(game, strm);
                        break;

                    case FRAMEFADEO:
                        fadeOut = std::make_unique<fade_out_t>();
                        result = fadeOut->read(game, strm);
                        break;

                    case FRAMEEVENTS:
                        events = std::make_unique<events_t>();
                        result = events->read(game, strm);
                        break;

                    case FRAMEPLYHEAD:
                        playHead = std::make_unique<play_head_t>();
                        result = playHead->read(game, strm);
                        break;

                    case FRAMEADDITEM:
                        additionalItem = std::make_unique<additional_item_t>();
                        result = additionalItem->read(game, strm);
                        break;

                    case FRAMEADDITEMINST:
                        additionalItemInstance = std::make_unique<additional_item_instance_t>();
                        result = additionalItemInstance->read(game, strm);
                        break;

                    case FRAMELAYERS:
                        layers = std::make_unique<layers_t>();
                        result = layers->read(game, strm);
                        break;

                    case FRAMEVIRTSIZE:
                        virtualSize = std::make_unique<virtual_size_t>();
                        result = virtualSize->read(game, strm);
                        break;

                    case DEMOFILEPATH:
                        demoFilePath = std::make_unique<demo_file_path_t>();
                        result = demoFilePath->read(game, strm);
                        break;

                    case RANDOMSEED:
                        randomSeed = std::make_unique<random_seed_t>();
                        result = randomSeed->read(game, strm);
                        break;

                    case FRAMELAYEREFFECT:
                        layerEffect = std::make_unique<layer_effect_t>();
                        result = layerEffect->read(game, strm);
                        break;

                    case FRAMEBLURAY:
                        blueray = std::make_unique<blueray_t>();
                        result = blueray->read(game, strm);
                        break;

                    case MOVETIMEBASE:
                        movementTimeBase = std::make_unique<movement_time_base_t>();
                        result = movementTimeBase->read(game, strm);
                        break;

                    case MOSAICIMGTABLE:
                        mosaicImageTable = std::make_unique<mosaic_image_table_t>();
                        result = mosaicImageTable->read(game, strm);
                        break;

                    case FRAMEEFFECTS:
                        effects = std::make_unique<effects_t>();
                        result = effects->read(game, strm);
                        break;

                    case FRAMEIPHONEOPTS:
                        iphoneOptions = std::make_unique<iphone_options_t>();
                        result = iphoneOptions->read(game, strm);
                        break;

                    case LAST:
                        end = std::make_unique<last_t>();
                        result = end->read(game, strm);
                        goto finished;

                    default: result = error_t::INVALID_CHUNK; break;
                }
            }

            finished:
            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Frame", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadDynamicData(strm, entry.header);
                } break;
                case MODE1: result = error_t::NO_MODE1;
                case MODE2: result = error_t::NO_MODE2;
                case MODE3: result = error_t::NO_MODE3;
                default: result = error_t::INVALID_MODE;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case FRAME:
                        items.emplace_back();
                        result = items.back().read(game, strm);
                        break;

                    default: goto finished;
                }
            }

            finished:
            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Frame Bank", (size_t)entry.ID))
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
        error_t item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE1: result = error_t::NO_MODE1; break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: result = error_t::NO_MODE3; break;
                default: result = error_t::INVALID_MODE;
            }

            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Image", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                if (ImGui::Button("View Image"))
                {
                    auto strm = entry.decode();
                    srcexp.image = std::move(CreateImage(strm, srcexp.state.oldGame).initTexture().texture);
                }

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t end_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Image Bank End", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x8);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x8);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: {
                    ReadFixedData(strm, entry.header, 0x8);
                    ReadCompressedData(strm, entry.data);
                } break;
                default: result = error_t::INVALID_MODE; break;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case ENDIMAGE:
                        end = std::make_unique<end_t>();
                        result = end->read(game, strm);
                        goto finished;

                    default:
                        items.emplace_back();
                        result = items.back().read(game, strm);
                        break;
                }
            }

            finished:
            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Image Bank", (size_t)entry.ID))
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
        error_t item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE1: result = error_t::NO_MODE1; break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: result = error_t::NO_MODE3; break;
                default: result = error_t::INVALID_MODE;
            }

            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Font", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t end_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Font Bank End", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x8);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x8);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: result = error_t::INVALID_MODE; break;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case ENDFONT:
                        end = std::make_unique<end_t>();
                        result = end->read(game, strm);
                        goto finished;

                    default:
                        items.emplace_back();
                        result = items.back().read(game, strm);
                        break;
                }
            }

            finished:
            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Font Bank", (size_t)entry.ID))
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
        error_t item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x14);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x14);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: result = error_t::NO_MODE3; break;
                default: result = error_t::INVALID_MODE;
            }

            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Sound", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t end_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Sound Bank End", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x8);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x8);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: {
                    ReadFixedData(strm, entry.header, 0x8);
                    ReadCompressedData(strm, entry.data);
                } break;
                default: result = error_t::INVALID_MODE; break;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case ENDSOUND:
                        end = std::make_unique<end_t>();
                        result = end->read(game, strm);
                        goto finished;

                    default:
                        items.emplace_back();
                        result = items.back().read(game, strm);
                        break;
                }
            }

            finished:
            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Sound Bank", (size_t)entry.ID))
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
        error_t item_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE1: result = error_t::NO_MODE1; break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: result = error_t::NO_MODE3; break;
                default: result = error_t::INVALID_MODE;
            }

            return result;
        }

        error_t item_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Music", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t end_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x4);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x4);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: {
                    ReadDynamicData(strm, entry.data);
                } break;
                case MODE3: {
                    ReadDynamicData(strm, entry.data);
                } break;
                default: return error_t::INVALID_MODE;
            }

            return error_t::OK;
        }

        error_t end_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Music Bank End", (size_t)entry.ID))
            {
                ImGui::Separator();

                entry.view(srcexp);

                ImGui::Separator();
                ImGui::TreePop();
            }

            return result;
        }

        error_t bank_t::read(game_t &game, lak::memstrm_t &strm)
        {
            entry.read(game, strm);
            error_t result = error_t::OK;

            switch (entry.mode)
            {
                case MODE0: {
                    ReadFixedData(strm, entry.header, 0x8);
                } break;
                case MODE1: {
                    ReadFixedData(strm, entry.header, 0x8);
                    ReadCompressedData(strm, entry.data);
                } break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: {
                    ReadFixedData(strm, entry.header, 0x8);
                    ReadCompressedData(strm, entry.data);
                } break;
                default: result = error_t::INVALID_MODE; break;
            }

            while (result == error_t::OK)
            {
                switch (strm.peekInt<chunk_t>())
                {
                    case ENDMUSIC:
                        end = std::make_unique<end_t>();
                        result = end->read(game, strm);
                        goto finished;

                    default:
                        items.emplace_back();
                        result = items.back().read(game, strm);
                        break;
                }
            }

            finished:
            return result;
        }

        error_t bank_t::view(source_explorer_t &srcexp) const
        {
            error_t result = error_t::OK;

            if (lak::TreeNode("0x%zX Music Bank", (size_t)entry.ID))
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

    error_t last_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: return error_t::NO_MODE1;
            case MODE2: return error_t::NO_MODE2;
            case MODE3: return error_t::NO_MODE3;
            default: return error_t::INVALID_MODE;
        }

        return error_t::OK;
    }

    error_t last_t::view(source_explorer_t &srcexp) const
    {
        error_t result = error_t::OK;

        if (lak::TreeNode("0x%zX Last", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            ImGui::Separator();
            ImGui::TreePop();
        }

        return result;
    }

    error_t header_t::read(game_t &game, lak::memstrm_t &strm)
    {
        entry.read(game, strm);
        error_t result = error_t::OK;

        switch (entry.mode)
        {
            case MODE0: {
                ReadFixedData(strm, entry.header, 0x4);
            } break;
            case MODE1: {
                ReadFixedData(strm, entry.header, 0x4);
                ReadCompressedData(strm, entry.data);
            } break;
            case MODE2: {
                ReadDynamicData(strm, entry.data);
            } break;
            case MODE3: result = error_t::NO_MODE3; break;
            default: result = error_t::INVALID_MODE; break;
        }

        while (result == error_t::OK)
        {
            chunk_t childID = strm.peekInt<chunk_t>();
            switch (childID)
            {
                case TITLE:
                    DEBUG("Reading Title");
                    title = std::make_unique<title_t>();
                    result = title->read(game, strm);
                    break;

                case AUTHOR:
                    DEBUG("Reading Author");
                    author = std::make_unique<author_t>();
                    result = author->read(game, strm);
                    break;

                case COPYRIGHT:
                    DEBUG("Reading Copyright");
                    copyright = std::make_unique<copyright_t>();
                    result = copyright->read(game, strm);
                    break;

                case PROJPATH:
                    DEBUG("Reading Project Path");
                    projectPath = std::make_unique<project_path_t>();
                    result = projectPath->read(game, strm);
                    break;

                case OUTPATH:
                    DEBUG("Reading Project Output Path");
                    outputPath = std::make_unique<output_path_t>();
                    result = outputPath->read(game, strm);
                    break;

                case VITAPREV:
                    DEBUG("Reading Project Vitalise Preview");
                    vitalisePreview = std::make_unique<vitalise_preview_t>();
                    result = vitalisePreview->read(game, strm);
                    break;

                case MENU:
                    DEBUG("Reading Project Menu");
                    menu = std::make_unique<menu_t>();
                    result = menu->read(game, strm);
                    break;

                case EXTPATH:
                    DEBUG("Reading Extension Path");
                    extensionPath = std::make_unique<extension_path_t>();
                    result = extensionPath->read(game, strm);
                    break;

                case EXTENS:
                    DEBUG("Reading Extensions");
                    extensions = std::make_unique<extensions_t>();
                    result = extensions->read(game, strm);
                    break;

                case EXTDATA:
                    DEBUG("Reading Extension Data");
                    extensionData = std::make_unique<extension_data_t>();
                    result = extensionData->read(game, strm);
                    break;

                case ADDEXTNS:
                    DEBUG("Reading Additional Extensions");
                    additionalExtensions = std::make_unique<additional_extensions_t>();
                    result = additionalExtensions->read(game, strm);
                    break;

                case APPDOC:
                    DEBUG("Reading Application Doc");
                    appDoc = std::make_unique<application_doc_t>();
                    result = appDoc->read(game, strm);
                    break;

                case OTHEREXT:
                    DEBUG("Reading Other Extension");
                    otherExtension = std::make_unique<other_extenion_t>();
                    result = otherExtension->read(game, strm);
                    break;

                case EXTNLIST:
                    DEBUG("Reading Extension List");
                    extensionList = std::make_unique<extension_list_t>();
                    result = extensionList->read(game, strm);
                    break;

                case ICON:
                    DEBUG("Reading Icon");
                    icon = std::make_unique<icon_t>();
                    result = icon->read(game, strm);
                    break;

                case DEMOVER:
                    DEBUG("Reading Demo Version");
                    demoVersion = std::make_unique<demo_version_t>();
                    result = demoVersion->read(game, strm);
                    break;

                case SECNUM:
                    DEBUG("Reading Security Number");
                    security = std::make_unique<security_number_t>();
                    result = security->read(game, strm);
                    break;

                case BINFILES:
                    DEBUG("Reading Binary Files");
                    binaryFiles = std::make_unique<binary_files_t>();
                    result = binaryFiles->read(game, strm);
                    break;

                case MENUIMAGES:
                    DEBUG("Reading Menu Images");
                    menuImages = std::make_unique<menu_images_t>();
                    result = menuImages->read(game, strm);
                    break;

                case ABOUT:
                    DEBUG("Reading About");
                    about = std::make_unique<about_t>();
                    result = about->read(game, strm);
                    break;

                case MOVEMNTEXTNS:
                    DEBUG("Reading Movement Extensions");
                    movementExtensions = std::make_unique<movement_extensions_t>();
                    result = movementExtensions->read(game, strm);
                    break;

                case OBJECTBANK2:
                    DEBUG("Reading Object Bank 2");
                    objectBank2 = std::make_unique<object_bank2_t>();
                    result = objectBank2->read(game, strm);
                    break;

                case EXEONLY:
                    DEBUG("Reading EXE Only");
                    exe = std::make_unique<exe_t>();
                    result = exe->read(game, strm);
                    break;

                case PROTECTION:
                    DEBUG("Reading Protection");
                    protection = std::make_unique<protection_t>();
                    result = protection->read(game, strm);
                    break;

                case SHADERS:
                    DEBUG("Reading Shaders");
                    shaders = std::make_unique<shaders_t>();
                    result = shaders->read(game, strm);
                    break;

                case EXTDHEADER:
                    DEBUG("Reading Extended Header");
                    extendedHeader = std::make_unique<extended_header_t>();
                    result = extendedHeader->read(game, strm);
                    break;

                case SPACER:
                    DEBUG("Reading Spacer");
                    spacer = std::make_unique<spacer_t>();
                    result = spacer->read(game, strm);
                    break;

                case CHUNK224F:
                    DEBUG("Reading Chunk 224F");
                    chunk224F = std::make_unique<chunk_224F_t>();
                    result = chunk224F->read(game, strm);
                    break;

                case TITLE2:
                    DEBUG("Reading Title 2");
                    title2 = std::make_unique<title2_t>();
                    result = title2->read(game, strm);
                    break;

                case GLOBALEVENTS:
                    DEBUG("Reading Global Events");
                    globalEvents = std::make_unique<global_events_t>();
                    result = globalEvents->read(game, strm);
                    break;

                case GLOBALSTRS:
                    DEBUG("Reading Global Strings");
                    globalStrings = std::make_unique<global_strings_t>();
                    result = globalStrings->read(game, strm);
                    break;

                case GLOBALSTRNAMES:
                    DEBUG("Reading Global String Names");
                    globalStringNames = std::make_unique<global_string_names_t>();
                    result = globalStringNames->read(game, strm);
                    break;

                case GLOBALVALS:
                    DEBUG("Reading Global Values");
                    globalValues = std::make_unique<global_values_t>();
                    result = globalValues->read(game, strm);
                    break;

                case GLOBALVALNAMES:
                    DEBUG("Reading Global Value Names");
                    globalValueNames = std::make_unique<global_value_names_t>();
                    result = globalValueNames->read(game, strm);
                    break;

                case FRAMEHANDLES:
                    DEBUG("Reading Frame Handles");
                    frameHandles = std::make_unique<frame::handles_t>();
                    result = frameHandles->read(game, strm);
                    break;

                case FRAMEBANK:
                    DEBUG("Reading Fame Bank");
                    frameBank = std::make_unique<frame::bank_t>();
                    result = frameBank->read(game, strm);
                    break;

                case OBJECTBANK:
                    DEBUG("Reading Object Bank");
                    objectBank = std::make_unique<object::bank_t>();
                    result = objectBank->read(game, strm);
                    break;

                case IMAGEBANK:
                    DEBUG("Reading Image Bank");
                    imageBank = std::make_unique<image::bank_t>();
                    result = imageBank->read(game, strm);
                    break;

                case SOUNDBANK:
                    DEBUG("Reading Sound Bank");
                    soundBank = std::make_unique<sound::bank_t>();
                    result = soundBank->read(game, strm);
                    break;

                case MUSICBANK:
                    DEBUG("Reading Music Bank");
                    musicBank = std::make_unique<music::bank_t>();
                    result = musicBank->read(game, strm);
                    break;

                case FONTBANK:
                    DEBUG("Reading Font Bank");
                    fontBank = std::make_unique<font::bank_t>();
                    result = fontBank->read(game, strm);
                    break;

                case LAST:
                    DEBUG("Reading Last");
                    last = std::make_unique<last_t>();
                    result = last->read(game, strm);
                    goto finished;

                default:
                    DEBUG("Invalid Chunk 0x" << (size_t)childID);
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

        if (lak::TreeNode("0x%zX Game Header", (size_t)entry.ID))
        {
            ImGui::Separator();

            entry.view(srcexp);

            if (title) title->view(srcexp);
            if (author) author->view(srcexp);
            if (copyright) copyright->view(srcexp);
            if (outputPath) outputPath->view(srcexp);
            if (projectPath) projectPath->view(srcexp);

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
            if (about) about->view(srcexp);
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

#if 0
    error_t ReadEntry(lak::memstrm_t &strm, std::stack<chunk_t> &state, resource_entry_t &entry)
    {
        static std::string depth;
        depth += '|';

        error_t result;
        bool more = true;
        get_data_flag_t dataMode[(uint16_t)encoding_t::MODECOUNT];

        DEBUG(depth << "\n"
            << depth << "Reading Entry\n"
            << depth << "Position: 0x" << strm.position << "\n"
            << depth << "State: 0x" << state.top());

        entry.chunks.clear();
        entry.info.position = strm.position;
        entry.ID = strm.readInt<chunk_t>();
        entry.mode = strm.readInt<encoding_t>();

        DEBUG(depth << "Entry ID: 0x" << entry.ID);
        DEBUG(depth << "Entry Mode: 0x" << entry.mode);

        if (state.top() != IMAGEBANK && state.top() != MUSICBANK &&
            state.top() != SOUNDBANK && state.top() != FONTBANK &&
            entry.ID != ENTRY && (entry.ID < VITAPREV || entry.ID > LAST))
        {
            ERROR(depth << "Invalid State: 0x" << state.top() << " Or ID: 0x" << entry.ID);
            result = error_t::INVALID_STATE;
            goto result_return;
        }

        if (entry.mode > MODE3)
        {
            ERROR(depth << "Invalid Mode: 0x" << entry.mode);
            result = error_t::INVALID_MODE;
            goto result_return;
        }

        switch (state.top())
        {
            case IMAGEBANK:
            case MUSICBANK:
            case FONTBANK: {
                if (entry.ID == ENDIMAGE || entry.ID == ENDMUSIC || entry.ID == ENDFONT)
                {
                    state.push(SKIP);
                }
                else
                {
                    state.push(NOCHILD);

                    dataMode[0] = MAIN_COMP;
                    dataMode[1] = NONE;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                }
            } break;
            case SOUNDBANK: {
                if (entry.ID == ENDSOUND)
                {
                    state.push(SKIP);
                }
                else
                {
                    state.push(NOCHILD);
                    dataMode[0] = PRE_READ14 | MAIN_COMP;
                    dataMode[1] = PRE_READ14 | MAIN_COMP;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                }
            } break;
            case FRAMEBANK: {
                if (entry.ID != FRAME)
                {
                    state.push(SKIP);
                }
                else
                {
                    dataMode[0] = PRE_READ4;
                    dataMode[1] = NONE;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                }
            } break;
            case FRAME: {
                if (entry.ID != LAST)
                {
                    if (entry.ID == FRAME || (entry.ID >> 8) != FRAME_STATE)
                    {
                        ERROR(depth << "Frame Error");
                        state.push(SKIP);
                        break;
                    }
                }
            } // do not break
            case DEFAULT:
            case VITA:
            case NEW:
            case OLD:
            default: switch (entry.ID)
            {
                case HEADER: {
                    // dataMode[0] = PRE_READNEXT;
                    // dataMode[1] = PRE_READNEXT | MAIN_COMP;
                    dataMode[0] = PRE_READ4;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = MAIN_NORM;
                    dataMode[3] = NONE; // MAIN_NORM;
                } break;
                case MENU:
                case FRAMEEFFECTS: {
                    dataMode[0] = NONE;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case ICON: {
                    dataMode[0] = NONE;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case EXTNLIST: {
                    dataMode[0] = PRE_READINT;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case SECNUM:
                case FRAMEHEADER:
                case FRAMEHANDLES:
                case FRAMEVIRTSIZE:
                case FRAMEPALETTE:
                case FRAMELAYERS:
                case FRAMELAYEREFFECT:
                case OBJINST:
                case FRAMEEVENTS:
                case FRAMEFADEI:
                case FRAMEFADEO:
                case OBJHEAD: {
                    dataMode[0] = NONE;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = PRE_READINT;
                    dataMode[3] = MAIN_NORM;
                } break;
                case SPACER:
                case ENTRY:
                case EXTDATA:
                case MOVETIMEBASE:
                case TITLE2:
                case FRAMEBANK:
                case EXEONLY: {
                    dataMode[0] = PRE_READINT; //PRE_READ15;
                    dataMode[1] = NONE;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case FRAMENAME:
                case OBJNAME: {
                    dataMode[0] = PRE_READINT;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = NONE; // MAIN_COMP;
                    dataMode[3] = MAIN_COMP;
                } break;
                case PROTECTION: {
                    dataMode[0] = PRE_READINT; // PRE_READ8;
                    dataMode[1] = NONE;
                    dataMode[2] = MAIN_NORM;
                    dataMode[3] = NONE;
                } break;
                case SHADERS: {
                    dataMode[0] = NONE;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case MUSICBANK:
                case SOUNDBANK:
                case IMAGEBANK: {
                    dataMode[0] = PRE_READ8;
                    dataMode[1] = PRE_READ8 | MAIN_COMP;
                    dataMode[2] = NONE; // PRE_READ8 | MAIN_COMP;
                    dataMode[3] = PRE_READ8 | MAIN_COMP;
                } break;
                case FONTBANK: {
                    dataMode[0] = PRE_READ8;// | MAIN_COMP;
                    dataMode[1] = PRE_READ8;// | MAIN_COMP;
                    dataMode[2] = MAIN_NORM;
                    dataMode[3] = MAIN_NORM;
                } break;
                case ENDMUSIC:
                case ENDSOUND:
                case ENDIMAGE:
                case ENDFONT: {
                    dataMode[0] = PRE_READ4;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = PRE_READINT | MAIN_NORM;
                    dataMode[3] = PRE_READINT | MAIN_NORM;
                } break;
                //case UNKNOWN8:
                case OBJECTBANK2:
                case OBJECTBANK: {
                    dataMode[0] = PRE_READ8;
                    dataMode[1] = NONE;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case LAST: {
                    dataMode[0] = PRE_READ4;
                    dataMode[1] = NONE;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case PROJPATH: {
                    dataMode[0] = PRE_READINT;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = MAIN_NORM;
                    dataMode[3] = MAIN_NORM;
                } break;
                case BINFILES: {
                    dataMode[0] = PRE_READINT;
                    dataMode[1] = NONE;
                    dataMode[2] = NONE;
                    dataMode[3] = NONE;
                } break;
                case TITLE:
                //case TITLE2:
                case AUTHOR:
                case OUTPATH:
                case COPYRIGHT:
                default: {
                    dataMode[0] = PRE_READ4;
                    dataMode[1] = PRE_READ4 | MAIN_COMP;
                    dataMode[2] = MAIN_NORM;
                    dataMode[3] = MAIN_NORM;
                } break;
            } break;
        }

        if (state.top() == SKIP)
        {
            strm.position = entry.info.position;
            result = error_t::OK;
            goto pop_state_return;
        }

        if (dataMode[entry.mode] == NONE)
        {
            ERROR(depth << "No Read Mode For MODE" << entry.mode);
            switch (entry.mode)
            {
                case MODE0: result = error_t::NO_MODE0; break;
                case MODE1: result = error_t::NO_MODE1; break;
                case MODE2: result = error_t::NO_MODE2; break;
                case MODE3: result = error_t::NO_MODE3; break;
                default: result = error_t::INVALID_MODE; break;
            }
            if (state.top() == NOCHILD)
                goto pop_state_return;
            goto result_return;
        }

        entry.info.data = strm.readBytesToCursor(entry.info.position);

        if (dataMode[entry.mode] & PRE_READINT)
        {
            ReadDynamicData(strm, entry.header);
        }
        else if (dataMode[entry.mode] & PRE_READ4)
        {
            ReadFixedData(strm, entry.header, 0x4);
        }
        else if (dataMode[entry.mode] & PRE_READ8)
        {
            ReadFixedData(strm, entry.header, 0x8);
        }
        else if (dataMode[entry.mode] & PRE_READ14)
        {
            ReadFixedData(strm, entry.header, 0x14);
        }
        else if (dataMode[entry.mode] & PRE_READ15)
        {
            ReadFixedData(strm, entry.header, 0x15);
        }

        if (dataMode[entry.mode] & MAIN_NORM)
        {
            ReadDynamicData(strm, entry.data);
        }
        else if (dataMode[entry.mode] & MAIN_COMP)
        {
            ReadCompressedData(strm, entry.data);
        }
        else if (dataMode[entry.mode] & MAIN_REV_COMP)
        {
            ReadReverseCompressedData(strm, entry.data);
        }

        // else ReadEntryData(strm, entry, dataMode[entry.mode]);

        if (state.top() == NOCHILD)
            goto pop_state_return;

        state.push(entry.ID);

        DEBUG(depth << "State: 0x" << state.top());

        while (more && strm.position < strm.memory.size())
        {
            entry.chunks.emplace_back();
            resource_entry_t &child = entry.chunks.back();
            child.parent = entry.ID;

            switch (entry.ID)
            {
                case HEADER:
                case OBJHEAD: {
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    if (child.ID == LAST)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case FRAME: {
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    // if ((child.ID >> 8) != FRAME_STATE)
                    if (child.ID == LAST || child.ID == FRAME || ((child.ID >> 8) != FRAME_STATE))
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case OBJECTBANK2:
                case OBJECTBANK: {
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    if (child.ID != OBJHEAD)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case FRAMEBANK: {
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    if (child.ID != FRAME)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case IMAGEBANK: {
                    //state.push(IMAGE_STATE);
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    //state.pop();
                    if (child.ID == ENDIMAGE)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case FONTBANK: {
                    //state.push(FONT_STATE);
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    //state.pop();
                    if (child.ID == ENDFONT)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case SOUNDBANK: {
                    //state.push(SOUND_STATE);
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    //state.pop();
                    if (child.ID == ENDSOUND)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case MUSICBANK: {
                    //state.push(MUSIC_STATE);
                    error_t err = ReadEntry(strm, state, child);
                    if (err != OK) { depth.pop_back(); return err; }
                    //state.pop();
                    if (child.ID == ENDMUSIC)
                    {
                        entry.chunks.pop_back();
                        more = false;
                    }
                } break;
                case NEW:
                case OLD:
                case VITA:
                case DEFAULT:
                case IMAGE_STATE:
                case SOUND_STATE:
                case MUSIC_STATE:
                case FONT_STATE:
                default: {
                    entry.chunks.pop_back();
                    more = false;
                } break;
            }
        }

        pop_state_return:
        state.pop();

        result_return:
        depth.pop_back();
        return result;
    }
#endif
}

#include "encryption.cpp"
#include "image.cpp"
