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

#ifndef NOMINMAX
#  define NOMINMAX
#endif

#include "explorer.h"
#include "tostring.hpp"

namespace SourceExplorer
{
  bool forceCompat = false;
  encryption_table decryptor;
  m128i_t _xmmword;
  std::vector<uint8_t> _magic_key;
  game_mode_t _mode = game_mode_t::_OLD;
  uint8_t _magic_char;
  std::atomic<float> game_t::completed = 0.0f;

  using namespace std::string_literals;

  std::vector<uint8_t> &operator+=(std::vector<uint8_t> &lhs,
                                   const std::vector<uint8_t> &rhs)
  {
    lhs.insert(lhs.cend(), rhs.cbegin(), rhs.cend());
    return lhs;
  }

  error_t LoadGame(source_explorer_t &srcexp)
  {
    DEBUG("Loading Game");

    srcexp.state        = game_t{};
    srcexp.state.compat = forceCompat;

    srcexp.state.file = lak::read_file(srcexp.exe.path);

    DEBUG("File Size: 0x" << srcexp.state.file.size());

    error_t err = ParsePEHeader(srcexp.state.file, srcexp.state);
    if (err != error_t::ok)
    {
      ERROR("Error '" << error_name(err) << "' While Parsing PE Header, At: 0x"
                      << srcexp.state.file.position);
      return err;
    }

    DEBUG("Successfully Parsed PE Header");

    _magic_key.clear();

    _xmmword.m128i_u32[0] = 0;
    _xmmword.m128i_u32[1] = 1;
    _xmmword.m128i_u32[2] = 2;
    _xmmword.m128i_u32[3] = 3;

    if (srcexp.state.productBuild < 284 || srcexp.state.oldGame ||
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
    if (err != error_t::ok)
    {
      ERROR("Error '" << error_name(err) << "' While Parsing PE Header, At: 0x"
                      << srcexp.state.file.position);
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
    std::memset(_magic_key.data() + 0x80, 0, 0x80);

    uint8_t *keyPtr = _magic_key.data();
    size_t len      = strlen((char *)keyPtr);
    uint8_t accum   = _magic_char;
    uint8_t hash    = _magic_char;
    for (size_t i = 0; i <= len; ++i)
    {
      hash = (hash << 7) + (hash >> 1);
      *keyPtr ^= hash;
      accum += *keyPtr * ((hash & 1) + 2);
      ++keyPtr;
    }
    *keyPtr = accum;

    decryptor.valid = false;
  }

  bool DecodeChunk(lak::span<uint8_t> chunk)
  {
    if (!decryptor.valid &&
        !decryptor.init(lak::span(_magic_key).first<0x100>(), _magic_char))
      return false;

    return decryptor.decode(chunk);
  }

  error_t ParsePEHeader(lak::memory &strm, game_t &gameState)
  {
    DEBUG("Parsing PE header");

    uint16_t exeSig = strm.read_u16();
    DEBUG("EXE Signature: 0x" << exeSig);
    if (exeSig != WIN_EXE_SIG)
    {
      ERROR("Invalid EXE Signature, Expected: 0x" << WIN_EXE_SIG << ", At: 0x"
                                                  << (strm.position - 2));
      return error_t::invalid_exe_signature;
    }

    strm.position = WIN_EXE_PNT;
    strm.position = strm.read_u16();
    DEBUG("EXE Pointer: 0x" << strm.position << ", At: 0x"
                            << (size_t)WIN_EXE_PNT);

    int32_t peSig = strm.read_s32();
    DEBUG("PE Signature: 0x" << peSig);
    if (peSig != WIN_PE_SIG)
    {
      ERROR("Invalid PE Signature, Expected: 0x" << WIN_PE_SIG << ", At: 0x"
                                                 << (strm.position - 4));
      return error_t::invalid_pe_signature;
    }

    strm.position += 2;

    uint16_t numHeaderSections = strm.read_u16();
    DEBUG("Number Of Header Sections: " << numHeaderSections);

    strm.position += 16;

    const uint16_t optionalHeader = 0x60;
    const uint16_t dataDir        = 0x80;
    strm.position += optionalHeader + dataDir;
    DEBUG("Pos: 0x" << strm.position);

    uint64_t pos = 0;
    for (uint16_t i = 0; i < numHeaderSections; ++i)
    {
      uint64_t start   = strm.position;
      std::string name = strm.read_string();
      DEBUG("Name: " << name);
      if (name == ".extra")
      {
        strm.position = start + 0x14;
        pos           = strm.read_s32();
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
      strm.position       = pos;
      uint16_t firstShort = strm.read_u16();
      DEBUG("First Short: 0x" << firstShort);
      strm.position      = pos;
      uint32_t pameMagic = strm.read_u32();
      DEBUG("PAME Magic: 0x" << pameMagic);
      strm.position      = pos;
      uint64_t packMagic = strm.read_u64();
      DEBUG("Pack Magic: 0x" << packMagic);
      strm.position = pos;
      DEBUG("Pos: 0x" << pos);

      if (firstShort == (uint16_t)chunk_t::header || pameMagic == HEADER_GAME)
      {
        DEBUG("Old Game");
        gameState.oldGame = true;
        gameState.state   = {};
        gameState.state.push(chunk_t::old);
        break;
      }
      else if (packMagic == HEADER_PACK)
      {
        DEBUG("New Game");
        gameState.oldGame = false;
        gameState.state   = {};
        gameState.state.push(chunk_t::_new);
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
        return error_t::invalid_game_signature;
      }

      if (pos > strm.size())
      {
        ERROR("Invalid Game Header: pos (0x" << pos << ") > strm.size (0x"
                                             << strm.size() << ")");
        return error_t::invalid_game_signature;
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
      ERROR("Invalid Game Header: 0x" << header << ", Expected: 0x"
                                      << HEADER_GAME << ", At: 0x"
                                      << (strm.position - 4));
      return error_t::invalid_game_signature;
    }

    gameState.runtimeVersion = (product_code_t)strm.read_u16();
    DEBUG("Runtime Version: 0x" << (int)gameState.runtimeVersion);
    if (gameState.runtimeVersion == product_code_t::CNCV1VER)
    {
      DEBUG("CNCV1VER");
      // cnc = true;
      // readCNC(strm);
    }
    else
    {
      gameState.runtimeSubVersion = strm.read_u16();
      DEBUG("Runtime Sub-Version: 0x" << gameState.runtimeSubVersion);
      gameState.productVersion = strm.read_u32();
      DEBUG("Product Version: 0x" << gameState.productVersion);
      gameState.productBuild = strm.read_u32();
      DEBUG("Product Build: 0x" << gameState.productBuild);
    }

    return error_t::ok;
  }

  uint64_t ParsePackData(lak::memory &strm, game_t &gameState)
  {
    DEBUG("Parsing pack data");

    uint64_t start  = strm.position;
    uint64_t header = strm.read_u64();
    DEBUG("Header: 0x" << header);
    uint32_t headerSize = strm.read_u32();
    (void)headerSize;
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

    uint32_t formatVersion = strm.read_u32();
    (void)formatVersion;
    DEBUG("Format Version: 0x" << formatVersion);

    strm.position += 0x8;

    int32_t count = strm.read_s32();
    assert(count >= 0);
    DEBUG("Pack Count: 0x" << count);

    uint64_t off = strm.position;
    DEBUG("Offset: 0x" << off);

    for (int32_t i = 0; i < count; ++i)
    {
      if ((strm.size() - strm.position) < 2) break;
      uint32_t val = strm.read_u16();

      if ((strm.size() - strm.position) < val) break;
      strm.position += val;

      if ((strm.size() - strm.position) < 4) break;
      val = strm.read_u32();

      if ((strm.size() - strm.position) < val) break;
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

      DEBUG("Pack 0x" << i + 1 << " of 0x" << count << ", filename length: 0x"
                      << read << ", pos: 0x" << strm.position);

      if (unicode)
        gameState.packFiles[i].filename = strm.read_u16string_exact(read);
      else
        gameState.packFiles[i].filename =
          lak::strconv<char16_t>(strm.read_string_exact(read));

      // strm.position = strstart + (unicode ? read * 2 : read);

      // DEBUG("String Start: 0x" << strstart);
      WDEBUG(L"Packfile '" << lak::to_wstring(gameState.packFiles[i].filename)
                           << L"'");

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

  lak::color4_t ColorFrom8bit(uint8_t RGB) { return {RGB, RGB, RGB, 255}; }

  lak::color4_t ColorFrom15bit(uint16_t RGB)
  {
    return {(uint8_t)((RGB & 0x7C00) >> 7), // 0111 1100 0000 0000
            (uint8_t)((RGB & 0x03E0) >> 2), // 0000 0011 1110 0000
            (uint8_t)((RGB & 0x001F) << 3), // 0000 0000 0001 1111
            255};
  }

  lak::color4_t ColorFrom16bit(uint16_t RGB)
  {
    return {(uint8_t)((RGB & 0xF800) >> 8), // 1111 1000 0000 0000
            (uint8_t)((RGB & 0x07E0) >> 3), // 0000 0111 1110 0000
            (uint8_t)((RGB & 0x001F) << 3), // 0000 0000 0001 1111
            255};
  }

  lak::color4_t ColorFrom8bit(lak::memory &strm,
                              const lak::color4_t palette[256])
  {
    if (palette) return palette[strm.read_u8()];
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

  template<typename STRM>
  lak::color4_t ColorFrom32bitRGBA(STRM &strm)
  {
    lak::color4_t rtn;
    rtn.r = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.b = strm.read_u8();
    rtn.a = strm.read_u8();
    return rtn;
  }

  lak::color4_t ColorFromMode(lak::memory &strm,
                              const graphics_mode_t mode,
                              const lak::color4_t palette[256])
  {
    switch (mode)
    {
      case graphics_mode_t::graphics2:
      case graphics_mode_t::graphics3: return ColorFrom8bit(strm, palette);
      case graphics_mode_t::graphics6: return ColorFrom15bit(strm);
      case graphics_mode_t::graphics7:
        return ColorFrom16bit(strm);
        // return ColorFrom15bit(strm);
      case graphics_mode_t::graphics4:
      default:
        // return ColorFrom32bitBGRA(strm);
        return ColorFrom24bitBGR(strm);
    }
  }

  uint8_t ColorModeSize(const graphics_mode_t mode)
  {
    switch (mode)
    {
      case graphics_mode_t::graphics2:
      case graphics_mode_t::graphics3: return 1;
      case graphics_mode_t::graphics6: return 2;
      case graphics_mode_t::graphics7: return 2;
      case graphics_mode_t::graphics4:
      default:
        // return 4;
        return 3;
    }
  }

  uint16_t BitmapPaddingSize(uint16_t width,
                             uint8_t colSize,
                             uint8_t bytes = 2)
  {
    uint16_t num = bytes - ((width * colSize) % bytes);
    return (uint16_t)std::ceil((double)(num == bytes ? 0 : num) /
                               (double)colSize);
  }

  size_t ReadRLE(lak::memory &strm,
                 lak::image4_t &bitmap,
                 graphics_mode_t mode,
                 const lak::color4_t palette[256] = nullptr)
  {
    const size_t pointSize = ColorModeSize(mode);
    const uint16_t pad     = BitmapPaddingSize(bitmap.size().x, pointSize);
    size_t pos             = 0;
    size_t i               = 0;

    size_t start = strm.position;

    while (true)
    {
      uint8_t command = strm.read_u8();

      if (command == 0) break;

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

  size_t ReadRGB(lak::memory &strm,
                 lak::image4_t &bitmap,
                 graphics_mode_t mode,
                 const lak::color4_t palette[256] = nullptr)
  {
    const size_t pointSize = ColorModeSize(mode);
    const uint16_t pad     = BitmapPaddingSize(bitmap.size().x, pointSize);
    size_t i               = 0;

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
    size_t i           = 0;

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

  texture_t CreateTexture(const lak::image4_t &bitmap,
                          const lak::graphics_mode mode)
  {
    if (mode == lak::graphics_mode::OpenGL)
    {
      auto old_texture = lak::opengl::get_uint<1>(GL_TEXTURE_BINDING_2D);

      lak::opengl::texture result(GL_TEXTURE_2D);
      result.bind()
        .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
        .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
        .apply(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
        .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
        .build(0,
               GL_RGBA,
               (lak::vec2<GLsizei>)bitmap.size(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               bitmap.data());

      glBindTexture(GL_TEXTURE_2D, old_texture);

      return result;
    }
    else if (mode == lak::graphics_mode::Software)
    {
      texture_color32_t result;
      result.copy(
        bitmap.size().x, bitmap.size().y, (color32_t *)bitmap.data());
      return result;
    }
    else
    {
      FATAL("Unknown graphics mode: 0x" << (uintmax_t)mode);
      return std::monostate{};
    }
  }

  void ViewImage(source_explorer_t &srcexp, const float scale)
  {
    // :TODO: Select palette
    if (std::holds_alternative<lak::opengl::texture>(srcexp.image))
    {
      const auto &img = std::get<lak::opengl::texture>(srcexp.image);
      if (!img.get() || srcexp.graphicsMode != lak::graphics_mode::OpenGL)
      {
        ImGui::Text("No image selected.");
      }
      else
      {
        ImGui::Image(
          (ImTextureID)(uintptr_t)img.get(),
          ImVec2(scale * (float)img.size().x, scale * (float)img.size().y));
      }
    }
    else if (std::holds_alternative<texture_color32_t>(srcexp.image))
    {
      const auto &img = std::get<texture_color32_t>(srcexp.image);
      if (!img.pixels || srcexp.graphicsMode != lak::graphics_mode::Software)
      {
        ImGui::Text("No image selected.");
      }
      else
      {
        ImGui::Image((ImTextureID)(uintptr_t)&img,
                     ImVec2(scale * (float)img.w, scale * (float)img.h));
      }
    }
    else if (std::holds_alternative<std::monostate>(srcexp.image))
    {
      ImGui::Text("No image selected.");
    }
    else
    {
      ERROR("Invalid texture type");
    }
  }

  const char *GetTypeString(const basic_entry_t &entry)
  {
    switch (entry.ID)
    {
      case chunk_t::entry: return "Entry (ERROR)";

      case chunk_t::vitalise_preview: return "Vitalise Preview";

      case chunk_t::header: return "Header";
      case chunk_t::title: return "Title";
      case chunk_t::author: return "Author";
      case chunk_t::menu: return "Menu";
      case chunk_t::extra_path: return "Extra Path";
      case chunk_t::extensions: return "Extensions (deprecated)";
      case chunk_t::object_bank: return "Object Bank";

      case chunk_t::global_events: return "Global Events (deprecated)";
      case chunk_t::frame_handles: return "Frame Handles";
      case chunk_t::extra_data: return "Extra Data";
      case chunk_t::additional_extensions:
        return "Additional Extensions (deprecated)";
      case chunk_t::project_path: return "Project Path";
      case chunk_t::output_path: return "Output Path";
      case chunk_t::app_doc: return "App Doc";
      case chunk_t::other_extension: return "Other Extension(s)";
      case chunk_t::global_values: return "Global Values";
      case chunk_t::global_strings: return "Global Strings";
      case chunk_t::extensions_list: return "Extensions List";
      case chunk_t::icon: return "Icon";
      case chunk_t::demo_version: return "Demo Version";
      case chunk_t::security_number: return "Security Number";
      case chunk_t::binary_files: return "Binary Files";
      case chunk_t::menu_images: return "Menu Images";
      case chunk_t::about: return "About";
      case chunk_t::copyright: return "Copyright";
      case chunk_t::global_value_names: return "Global Value Names";
      case chunk_t::global_string_names: return "Global String Names";
      case chunk_t::movement_extensions: return "Movement Extensions";
      // case chunk_t::UNKNOWN8: return "UNKNOWN8";
      case chunk_t::object_bank2: return "Object Bank 2";
      case chunk_t::exe_only: return "EXE Only";
      case chunk_t::protection: return "Protection";
      case chunk_t::shaders: return "Shaders";
      case chunk_t::extended_header: return "Extended Header";
      case chunk_t::spacer: return "Spacer";
      case chunk_t::frame_bank: return "Frame Bank";
      case chunk_t::chunk224F: return "CHUNK 224F";
      case chunk_t::title2: return "Title2";

      case chunk_t::chunk2253: return "CHUNK 2253 (16 bytes?)";
      case chunk_t::object_names: return "Object Names";
      case chunk_t::chunk2255: return "CHUNK 2255 (Empty?)";
      case chunk_t::recompiled_object_properties:
        return "Recompiled Object Properties";
      case chunk_t::chunk2257: return "CHUNK 2257 (4 bytes?)";
      case chunk_t::font_meta: return "TrueType Fonts Meta";
      case chunk_t::font_chunk: return "TrueType Fonts Chunk";

      case chunk_t::frame: return "Frame";
      case chunk_t::frame_header: return "Frame Header";
      case chunk_t::frame_name: return "Frame Name";
      case chunk_t::frame_password: return "Frame Password";
      case chunk_t::frame_palette: return "Frame Palette";
      case chunk_t::frame_object_instances: return "Frame Object Instances";
      case chunk_t::frame_fade_in_frame: return "Frame Fade In Frame";
      case chunk_t::frame_fade_out_frame: return "Frame Fade Out Frame";
      case chunk_t::frame_fade_in: return "Frame Fade In";
      case chunk_t::frame_fade_out: return "Frame Fade Out";
      case chunk_t::frame_events: return "Frame Events";
      case chunk_t::frame_play_header: return "Frame Play Header";
      case chunk_t::frame_additional_items: return "Frame Additional Item";
      case chunk_t::frame_additional_items_instances:
        return "Frame Additional Item Instance";
      case chunk_t::frame_layers: return "Frame Layers";
      case chunk_t::frame_virtual_size: return "Frame Virtical Size";
      case chunk_t::demo_file_path: return "Frame Demo File Path";
      case chunk_t::random_seed: return "Frame Random Seed";
      case chunk_t::frame_layer_effect: return "Frame Layer Effect";
      case chunk_t::frame_bluray: return "Frame BluRay Options";
      case chunk_t::movement_timer_base: return "Frame Movement Timer Base";
      case chunk_t::mosaic_image_table: return "Frame Mosaic Image Table";
      case chunk_t::frame_effects: return "Frame Effects";
      case chunk_t::frame_iphone_options: return "Frame iPhone Options";
      case chunk_t::frame_chunk334C: return "Frame CHUNK 334C";

      case chunk_t::pa_error: return "PA (ERROR)";

      case chunk_t::object_header: return "Object Header";
      case chunk_t::object_name: return "Object Name";
      case chunk_t::object_properties: return "Object Properties";
      case chunk_t::object_chunk4447: return "Object CHUNK 4447";
      case chunk_t::object_effect: return "Object Effect";

      case chunk_t::image_handles: return "Image Handles";
      case chunk_t::font_handles: return "Font Handles";
      case chunk_t::sound_handles: return "Sound Handles";
      case chunk_t::music_handles: return "Music Handles";

      case chunk_t::image_bank: return "Image Bank";
      case chunk_t::font_bank: return "Font Bank";
      case chunk_t::sound_bank: return "Sound Bank";
      case chunk_t::music_bank: return "Music Bank";

      case chunk_t::last: return "Last";

      case chunk_t::_default:
      case chunk_t::vitalise:
      case chunk_t::unicode:
      case chunk_t::_new:
      case chunk_t::old:
      case chunk_t::frame_state:
      case chunk_t::image_state:
      case chunk_t::font_state:
      case chunk_t::sound_state:
      case chunk_t::music_state:
      case chunk_t::no_child:
      case chunk_t::skip:
      default: return "INVALID";
    }
  }

  const char *GetObjectTypeString(object_type_t type)
  {
    switch (type)
    {
      case object_type_t::quick_backdrop: return "Quick Backdrop";
      case object_type_t::backdrop: return "Backdrop";
      case object_type_t::active: return "Active";
      case object_type_t::text: return "Text";
      case object_type_t::question: return "Question";
      case object_type_t::score: return "Score";
      case object_type_t::lives: return "Lives";
      case object_type_t::counter: return "Counter";
      case object_type_t::RTF: return "RTF";
      case object_type_t::sub_application: return "Sub Application";
      case object_type_t::player: return "Player";
      case object_type_t::keyboard: return "Keyboard";
      case object_type_t::create: return "Create";
      case object_type_t::timer: return "Timer";
      case object_type_t::game: return "Game";
      case object_type_t::speaker: return "Speaker";
      case object_type_t::system: return "System";
      default: return "Unknown/Invalid";
    }
  }

  const char *GetObjectParentTypeString(object_parent_type_t type)
  {
    switch (type)
    {
      case object_parent_type_t::none: return "None";
      case object_parent_type_t::frame: return "Frame";
      case object_parent_type_t::frame_item: return "Frame Item";
      case object_parent_type_t::qualifier: return "Qualifier";
      default: return "Invalid";
    }
  }

  std::pair<bool, std::vector<uint8_t>> Decode(
    const std::vector<uint8_t> &encoded, chunk_t id, encoding_t mode)
  {
    switch (mode)
    {
      case encoding_t::mode3:
      case encoding_t::mode2: return Decrypt(encoded, id, mode);
      case encoding_t::mode1: return {true, Inflate(encoded)};
      default:
        if (encoded.size() > 0 && encoded[0] == 0x78)
          return {true, Inflate(encoded)};
        else
          return {true, encoded};
    }
  }

  std::optional<std::vector<uint8_t>> Inflate(
    const std::vector<uint8_t> &compressed,
    bool skip_header,
    bool anaconda,
    size_t max_size)
  {
    std::deque<uint8_t> buffer;
    tinf::decompression_state_t state;

    state.begin    = compressed.begin();
    state.end      = compressed.end();
    state.anaconda = anaconda;

    tinf::error_t error = tinf::error_t::OK;
    if (skip_header)
    {
      state.state = tinf::state_t::HEADER;
    }
    else
    {
      error = tinf::tinflate_header(state);
    }

    while (error == tinf::error_t::OK && !state.final &&
           buffer.size() < max_size)
    {
      error = tinf::tinflate_block(buffer, state);
    }

    if (error != tinf::error_t::OK)
    {
      ERROR("Failed To Inflate: " << tinf::error_name(error));
      DEBUG("Buffer Size: " << buffer.size());
      DEBUG("Max Size: " << max_size);
      DEBUG("Final? " << (state.final ? "True" : "False"));
      return std::nullopt;
    }

    return std::vector<uint8_t>(buffer.begin(), buffer.end());
  }

  bool Inflate(std::vector<uint8_t> &out,
               const std::vector<uint8_t> &compressed,
               bool skip_header,
               bool anaconda,
               size_t max_size)
  {
    auto result = Inflate(compressed, skip_header, anaconda, max_size);
    if (result)
      out = std::move(*result);
    else
      ERROR("... Failed To Inflate");
    return (bool)result;
  }

  bool Inflate(lak::memory &out,
               const std::vector<uint8_t> &compressed,
               bool skip_header,
               bool anaconda,
               size_t max_size)
  {
    auto result = Inflate(compressed, skip_header, anaconda, max_size);
    if (result)
      out = std::move(*result);
    else
      ERROR("... Failed To Inflate");
    return (bool)result;
  }

  std::vector<uint8_t> Inflate(const std::vector<uint8_t> &compressed)
  {
    if (auto result = Inflate(compressed, false, false); result)
      return *result;
    ERROR("Failed To Inflate");
    return compressed;
  }

  std::vector<uint8_t> Decompress(const std::vector<uint8_t> &compressed,
                                  unsigned int outSize)
  {
    if (auto result = Inflate(compressed, true, true); result) return *result;
    ERROR("Failed To Decompress");
    return compressed;
  }

  std::vector<uint8_t> StreamDecompress(lak::memory &strm,
                                        unsigned int outSize)
  {
    // TODO: Use newer tinf API
    std::deque<uint8_t> buffer;
    tinf::decompression_state_t state;
    state.state    = tinf::state_t::HEADER;
    state.anaconda = true;
    tinf::error_t err =
      tinf::tinflate(strm.cursor(), strm.end(), buffer, state);
    if (state.begin > strm.cursor())
      strm.position += state.begin - strm.cursor();
    if (err == tinf::error_t::OK)
      return std::vector<uint8_t>(buffer.begin(), buffer.end());
    else
    {
      ERROR("Failed To Decompress (" << tinf::error_name(err) << ")");
      return std::vector<uint8_t>(outSize, 0);
    }
  }

  std::pair<bool, std::vector<uint8_t>> Decrypt(
    const std::vector<uint8_t> &encrypted, chunk_t ID, encoding_t mode)
  {
    if (mode == encoding_t::mode3)
    {
      if (encrypted.size() <= 4) return {false, encrypted};
      // TODO: check endian
      // size_t dataLen = *reinterpret_cast<const uint32_t*>(&encrypted[0]);
      std::vector<uint8_t> mem(encrypted.begin() + 4, encrypted.end());

      if ((_mode != game_mode_t::_284) && ((uint16_t)ID & 0x1) != 0)
        mem[0] ^= ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

      if (DecodeChunk(mem))
      {
        if (mem.size() <= 4) return {false, mem};
        // dataLen = *reinterpret_cast<uint32_t*>(&mem[0]);

        // Try Inflate even if it doesn't need to be
        return {true,
                Inflate(std::vector<uint8_t>(mem.begin() + 4, mem.end()))};
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

      if (!DecodeChunk(mem))
      {
        if (mode == encoding_t::mode2)
        {
          DEBUG("MODE 2 Decryption Failed");
          return {false, mem};
        }
      }
      return {true, mem};
    }
  }

  frame::item_t *GetFrame(game_t &game, uint16_t handle)
  {
    if (game.game.frameBank && game.game.frameHandles)
    {
      if (handle >= game.game.frameHandles->handles.size()) return nullptr;
      handle = game.game.frameHandles->handles[handle];
      if (handle >= game.game.frameBank->items.size()) return nullptr;
      return &game.game.frameBank->items[handle];
    }
    return nullptr;
  }

  object::item_t *GetObject(game_t &game, uint16_t handle)
  {
    if (game.game.objectBank &&
        game.objectHandles.find(handle) != game.objectHandles.end() &&
        game.objectHandles[handle] < game.game.objectBank->items.size())
    {
      return &game.game.objectBank->items[game.objectHandles[handle]];
    }
    return nullptr;
  }

  image::item_t *GetImage(game_t &game, uint32_t handle)
  {
    if (game.game.imageBank &&
        game.imageHandles.find(handle) != game.imageHandles.end() &&
        game.imageHandles[handle] < game.game.imageBank->items.size())
    {
      return &game.game.imageBank->items[game.imageHandles[handle]];
    }
    return nullptr;
  }

  lak::memory data_point_t::decode(const chunk_t ID,
                                   const encoding_t mode) const
  {
    auto [success, mem] = Decode(data, ID, mode);
    if (success) return lak::memory(mem);
    return lak::memory();
  }

  error_t chunk_entry_t::read(game_t &game, lak::memory &strm)
  {
    if (!strm.remaining()) return error_t::out_of_data;

    position = strm.position;
    old      = game.oldGame;
    ID       = (chunk_t)strm.read_u16();
    mode     = (encoding_t)strm.read_u16();

    if ((mode == encoding_t::mode2 || mode == encoding_t::mode3) &&
        _magic_key.size() < 256)
      GetEncryptionKey(game);

    const auto chunkSize    = strm.read_u32();
    const auto chunkDataEnd = strm.position + chunkSize;

    if (mode == encoding_t::mode1)
    {
      data.expectedSize = strm.read_u32();

      if (game.oldGame)
      {
        data.position = strm.position;
        if (chunkSize > 4)
          data.data = strm.read(chunkSize - 4);
        else
          data.data.clear();
      }
      else
      {
        const auto dataSize = strm.read_u32();
        data.position       = strm.position;
        data.data           = strm.read(dataSize);
        if (strm.position > chunkDataEnd)
        {
          ERROR("Read Too Much Data");
        }
        else if (strm.position < chunkDataEnd)
        {
          ERROR("Read Too Little Data");
        }
        strm.position = chunkDataEnd;
      }
    }
    else
    {
      data.expectedSize = 0;
      data.position     = strm.position;
      data.data         = strm.read(chunkSize);
    }
    end = strm.position;

    return error_t::ok;
  }

  void chunk_entry_t::view(source_explorer_t &srcexp) const
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

      ImGui::Text("ID: 0x%zX", (size_t)ID);
      ImGui::Text("Mode: MODE%zu", (size_t)mode);

      ImGui::Text("Header Position: 0x%zX", header.position);
      ImGui::Text("Header Expected Size: 0x%zX", header.expectedSize);
      ImGui::Text("Header Size: 0x%zX", header.data.size());

      ImGui::Text("Data Position: 0x%zX", data.position);
      ImGui::Text("Data Expected Size: 0x%zX", data.expectedSize);
      ImGui::Text("Data Size: 0x%zX", data.data.size());

      ImGui::TreePop();
    }

    if (ImGui::Button("View Memory")) srcexp.view = this;
  }

  error_t item_entry_t::read(game_t &game,
                             lak::memory &strm,
                             bool compressed,
                             size_t headersize)
  {
    if (!strm.remaining()) return error_t::out_of_data;

    position = strm.position;
    old      = game.oldGame;
    mode     = encoding_t::mode0;
    handle   = strm.read_u32();

    const bool newItem = strm.peek_s32() == -1;
    if (newItem)
    {
      WARNING("New Item");
      game.recompiled |= true;
      headersize = 12;
      mode       = encoding_t::mode0;
      compressed = false;
    }

    if (!game.oldGame && headersize > 0)
    {
      header.position     = strm.position;
      header.expectedSize = 0;
      header.data         = strm.read(headersize);
    }

    data.expectedSize = game.oldGame || compressed ? strm.read_u32() : 0;

    size_t dataSize;
    if (game.oldGame)
    {
      const size_t start = strm.position;
      // Figure out exactly how long the compressed data is
      // :TODO: It should be possible to figure this out without
      // actually decompressing it... surely...
      if (const auto raw = StreamDecompress(strm, data.expectedSize);
          raw.size() != data.expectedSize)
      {
        WARNING("Actual decompressed size (0x"
                << raw.size() << ") was not equal to the expected size (0x"
                << data.expectedSize << ").");
      }
      dataSize      = strm.position - start;
      strm.position = start;
    }
    else
    {
      dataSize = strm.read_u32() + (newItem ? 20 : 0);
    }

    data.position = strm.position;
    data.data     = strm.read(dataSize);

    // hack because one of MMF1.5 or tinf_uncompress is a bitch
    if (game.oldGame) mode = encoding_t::mode1;

    end = strm.position;

    return error_t::ok;
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

      ImGui::TreePop();
    }

    if (ImGui::Button("View Memory")) srcexp.view = this;
  }

  lak::memory basic_entry_t::decode(size_t max_size) const
  {
    lak::memory result;
    if (old)
    {
      switch (mode)
      {
        case encoding_t::mode0: result = data.data; break;
        case encoding_t::mode1:
        {
          result        = data.data;
          uint8_t magic = result.read_u8();
          uint16_t len  = result.read_u16();
          if (magic == 0x0F && (size_t)len == data.expectedSize)
          {
            result = result.read(std::min(result.remaining(), max_size));
          }
          else
          {
            result.position -= 3;
            if (!Inflate(result,
                         result,
                         true,
                         true,
                         std::min(data.expectedSize, max_size)))
            {
              ERROR("... MODE1 Failed To Inflate");
              result.clear();
            }
          }
        }
        break;
        case encoding_t::mode2: ERROR("No Old Decode For MODE2"); break;
        case encoding_t::mode3: ERROR("No Old Decode For MODE3"); break;
        default: break;
      }
    }
    else
    {
      switch (mode)
      {
        case encoding_t::mode3:
        case encoding_t::mode2:
          if (auto [success, mem] = Decrypt(data.data, ID, mode); success)
          {
            result = mem;
          }
          else
          {
            ERROR("... Decryption Failed");
            result.clear();
          }
          break;
        case encoding_t::mode1:
          if (!Inflate(result, data.data, false, false, max_size))
          {
            ERROR("... MODE1 Failed To Inflate");
            result.clear();
          }
          break;
        default:
          if (data.data.size() > 0 && data.data[0] == 0x78)
          {
            if (!Inflate(result, data.data, false, false, max_size))
            {
              WARNING("... Guessed MODE1 Failed To Inflate");
              result = data.data;
            }
          }
          else
            result = data.data;
          break;
      }
    }
    return result;
  }

  lak::memory basic_entry_t::decodeHeader(size_t max_size) const
  {
    lak::memory result;
    if (old)
    {
    }
    else
    {
      switch (mode)
      {
        case encoding_t::mode3:
        case encoding_t::mode2:
          if (auto [success, mem] = Decrypt(data.data, ID, mode); success)
            result = mem;
          else
            result.clear();
          break;
        case encoding_t::mode1:
          if (!Inflate(result, header.data, false, false, max_size))
          {
            ERROR("MODE1 Inflation Failed");
            result.clear();
          }
          break;
        default:
          if (header.data.size() > 0 && header.data[0] == 0x78)
          {
            if (!Inflate(result, header.data, false, false, max_size))
            {
              DEBUG("Guessed MODE1 Inflation Failed");
              result = header.data;
            }
          }
          else
            result = header.data;
          break;
      }
    }
    return result;
  }

  const lak::memory &basic_entry_t::raw() const { return data.data; }

  const lak::memory &basic_entry_t::rawHeader() const { return header.data; }

  std::u16string ReadStringEntry(game_t &game, const chunk_entry_t &entry)
  {
    std::u16string result;

    if (game.oldGame)
    {
      switch (entry.mode)
      {
        case encoding_t::mode0:
        case encoding_t::mode1:
        {
          result = lak::to_u16string(entry.decode().read_string());
        }
        break;
        default:
        {
          ERROR("Invalid String Mode: 0x" << (int)entry.mode << ", Chunk: 0x"
                                          << (int)entry.ID);
        }
        break;
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

  error_t basic_chunk_t::basic_view(source_explorer_t &srcexp,
                                    const char *name) const
  {
    error_t result = error_t::ok;

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

  error_t basic_item_t::basic_view(source_explorer_t &srcexp,
                                   const char *name) const
  {
    error_t result = error_t::ok;

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
    if (result == error_t::ok)
      value = ReadStringEntry(game, entry);
    else
      ERROR("Failed To Read String Chunk (" << error_name(result) << ")");
    return result;
  }

  error_t string_chunk_t::view(source_explorer_t &srcexp,
                               const char *name,
                               const bool preview) const
  {
    error_t result  = error_t::ok;
    std::string str = string();
    bool open       = false;

    if (preview)
      open = lak::TreeNode("0x%zX %s '%s'##%zX",
                           (size_t)entry.ID,
                           name,
                           str.c_str(),
                           entry.position);
    else
      open =
        lak::TreeNode("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position);

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

  std::u16string string_chunk_t::u16string() const { return value; }

  std::u8string string_chunk_t::u8string() const
  {
    return lak::strconv<char8_t>(value);
  }

  std::string string_chunk_t::string() const
  {
    return lak::strconv<char>(value);
  }

  error_t strings_chunk_t::read(game_t &game, lak::memory &strm)
  {
    DEBUG("Reading Strings Chunk");
    error_t result = entry.read(game, strm);
    if (result == error_t::ok)
    {
      auto sstrm = entry.decode();

      while (sstrm.remaining())
      {
        auto str = sstrm.read_u16string(sstrm.remaining());
        DEBUG("    Read String (Len 0x" << str.length() << ")");
        if (!str.length()) break;
        values.emplace_back(str);
      }
    }
    else
    {
      ERROR("Failed To Read Strings Chunk (" << error_name(result) << ")");
    }
    return result;
  }

  error_t strings_chunk_t::basic_view(source_explorer_t &srcexp,
                                      const char *name) const
  {
    error_t result = error_t::ok;

    if (lak::TreeNode("0x%zX %s (%zu Items)##%zX",
                      (size_t)entry.ID,
                      name,
                      values.size(),
                      entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);
      for (const auto &s : values)
        ImGui::Text("%s", (const char *)lak::to_u8string(s).c_str());

      ImGui::Separator();
      ImGui::TreePop();
    }

    return result;
  }

  error_t strings_chunk_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Unknown Strings");
  }

  error_t compressed_chunk_t::view(source_explorer_t &srcexp) const
  {
    error_t result = error_t::ok;

    if (lak::TreeNode(
          "0x%zX Unknown Compressed##%zX", (size_t)entry.ID, entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);

      if (ImGui::Button("View Compressed"))
      {
        auto strm = entry.decode();
        strm.skip(8);
        std::deque<uint8_t> buffer;
        tinf::error_t err =
          tinf::tinflate(strm.read(strm.remaining()), buffer);
        if (err == tinf::error_t::OK)
          srcexp.buffer = std::vector(buffer.begin(), buffer.end());
        else
          ERROR("Failed To Decompress Chunk (" << tinf::error_name(err)
                                               << ")");
      }

      ImGui::Separator();
      ImGui::TreePop();
    }

    return result;
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

  error_t other_extension_t::view(source_explorer_t &srcexp) const
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

    auto dstrm     = entry.decode();
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
    error_t result = error_t::ok;

    if (lak::TreeNode("0x%zX Icon##%zX", (size_t)entry.ID, entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);

      ImGui::Text("Image Size: %zu * %zu",
                  (size_t)bitmap.size().x,
                  (size_t)bitmap.size().y);

      if (ImGui::Button("View Image"))
      {
        srcexp.image = CreateTexture(bitmap, srcexp.graphicsMode);
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
      name = lak::to_u16string(strm.read_string_exact(strm.read_u16()));

    data = strm.read(strm.read_u32());

    return error_t::ok;
  }

  error_t binary_file_t::view(source_explorer_t &srcexp) const
  {
    std::u8string str = lak::to_u8string(name);
    if (lak::TreeNode("%s", str.c_str()))
    {
      ImGui::Separator();

      ImGui::Text("Name: %s", str.c_str());
      ImGui::Text("Data Size: 0x%zX", data.size());

      ImGui::Separator();
      ImGui::TreePop();
    }
    return error_t::ok;
  }

  error_t binary_files_t::read(game_t &game, lak::memory &strm)
  {
    DEBUG("Reading Binary Files");
    error_t result = entry.read(game, strm);

    if (result == error_t::ok)
    {
      lak::memory bstrm = entry.decode();
      items.resize(bstrm.read_u32());
      for (auto &item : items) item.read(game, bstrm);
    }

    return result;
  }

  error_t binary_files_t::view(source_explorer_t &srcexp) const
  {
    if (lak::TreeNode(
          "0x%zX Binary Files##%zX", (size_t)entry.ID, entry.position))
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
    return error_t::ok;
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

    lak::memory estrm    = entry.decode();
    flags                = estrm.read_u32();
    buildType            = estrm.read_u32();
    buildFlags           = estrm.read_u32();
    screenRatioTolerance = estrm.read_u16();
    screenAngle          = estrm.read_u16();

    game.compat |= buildType >= 0x10000000;

    return result;
  }

  error_t extended_header_t::view(source_explorer_t &srcexp) const
  {
    if (lak::TreeNode(
          "0x%zX Extended Header##%zX", (size_t)entry.ID, entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);

      ImGui::Text("Flags: 0x%zX", (size_t)flags);
      ImGui::Text("Build Type: 0x%zX", (size_t)buildType);
      ImGui::Text("Build Flags: 0x%zX", (size_t)buildFlags);
      ImGui::Text("Screen Ratio Tolerance: 0x%zX",
                  (size_t)screenRatioTolerance);
      ImGui::Text("Screen Angle: 0x%zX", (size_t)screenAngle);

      ImGui::Separator();
      ImGui::TreePop();
    }

    return error_t::ok;
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

  error_t object_names_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Object Names");
  }

  error_t object_properties_t::read(game_t &game, lak::memory &strm)
  {
    error_t result = entry.read(game, strm);

    if (result != error_t::ok) return result;

    const auto end = strm.position;
    strm.position  = entry.data.position;

    while (strm.position < end && result == error_t::ok)
    {
      items.emplace_back();
      result = items.back().read(game, strm, false);
    }

    strm.position = end;

    return result;
  }

  error_t object_properties_t::view(source_explorer_t &srcexp) const
  {
    if (lak::TreeNode("0x%zX Object Properties (%zu Items)##%zX",
                      (size_t)entry.ID,
                      items.size(),
                      entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);

      for (const auto &item : items)
      {
        if (lak::TreeNode(
              "0x%zX Properties##%zX", (size_t)item.ID, item.position))
        {
          ImGui::Separator();

          item.view(srcexp);

          ImGui::Separator();
          ImGui::TreePop();
        }
      }

      ImGui::Separator();
      ImGui::TreePop();
    }

    return error_t::ok;
  }

  error_t truetype_fonts_meta_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "TrueType Fonts Meta");
  }

  error_t truetype_fonts_t::read(game_t &game, lak::memory &strm)
  {
    error_t result = entry.read(game, strm);

    if (result != error_t::ok) return result;

    const auto end = strm.position;
    strm.position  = entry.data.position;

    while (strm.position < end && result == error_t::ok)
    {
      items.emplace_back();
      result = items.back().read(game, strm, false);
    }

    strm.position = end;

    return result;
  }

  error_t truetype_fonts_t::view(source_explorer_t &srcexp) const
  {
    if (lak::TreeNode("0x%zX TrueType Fonts (%zu Items)##%zX",
                      (size_t)entry.ID,
                      items.size(),
                      entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);

      for (const auto &item : items)
      {
        if (lak::TreeNode("0x%zX Font##%zX", (size_t)item.ID, item.position))
        {
          ImGui::Separator();

          item.view(srcexp);

          ImGui::Separator();
          ImGui::TreePop();
        }
      }

      ImGui::Separator();
      ImGui::TreePop();
    }

    return error_t::ok;
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

      borderSize    = strm.read_u16();
      borderColor.r = strm.read_u8();
      borderColor.g = strm.read_u8();
      borderColor.b = strm.read_u8();
      borderColor.a = strm.read_u8();
      shape         = (shape_type_t)strm.read_u16();
      fill          = (fill_type_t)strm.read_u16();

      line     = line_flags_t::none;
      gradient = gradient_flags_t::horizontal;
      handle   = 0xFFFF;

      if (shape == shape_type_t::line)
      {
        line = (line_flags_t)strm.read_u16();
      }
      else if (fill == fill_type_t::solid)
      {
        color1.r = strm.read_u8();
        color1.g = strm.read_u8();
        color1.b = strm.read_u8();
        color1.a = strm.read_u8();
      }
      else if (fill == fill_type_t::gradient)
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
      else if (fill == fill_type_t::motif)
      {
        handle = strm.read_u16();
      }

      return error_t::ok;
    }

    error_t shape_t::view(source_explorer_t &srcexp) const
    {
      if (lak::TreeNode("Shape"))
      {
        ImGui::Separator();

        ImGui::Text("Border Size: 0x%zX", (size_t)borderSize);
        lak::vec4f_t col = ((lak::vec4f_t)borderColor) / 255.0f;
        ImGui::ColorEdit4("Border Color", &col.x);
        ImGui::Text("Shape: 0x%zX", (size_t)shape);
        ImGui::Text("Fill: 0x%zX", (size_t)fill);

        if (shape == shape_type_t::line)
        {
          ImGui::Text("Line: 0x%zX", (size_t)line);
        }
        else if (fill == fill_type_t::solid)
        {
          col = ((lak::vec4f_t)color1) / 255.0f;
          ImGui::ColorEdit4("Fill Color", &col.x);
        }
        else if (fill == fill_type_t::gradient)
        {
          col = ((lak::vec4f_t)color1) / 255.0f;
          ImGui::ColorEdit4("Gradient Color 1", &col.x);
          col = ((lak::vec4f_t)color2) / 255.0f;
          ImGui::ColorEdit4("Gradient Color 2", &col.x);
          ImGui::Text("Gradient Flags: 0x%zX", (size_t)gradient);
        }
        else if (fill == fill_type_t::motif)
        {
          ImGui::Text("Handle: 0x%zX", (size_t)handle);
        }

        ImGui::Separator();
        ImGui::TreePop();
      }
      return error_t::ok;
    }

    error_t quick_backdrop_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Quick Backdrop");
      error_t result = entry.read(game, strm);

      if (result == error_t::ok)
      {
        lak::memory qstrm = entry.decode();
        size              = qstrm.read_u32();
        obstacle          = qstrm.read_u16();
        collision         = qstrm.read_u16();
        dimension.x       = game.oldGame ? qstrm.read_u16() : qstrm.read_u32();
        dimension.y       = game.oldGame ? qstrm.read_u16() : qstrm.read_u32();
        shape.read(game, qstrm);
      }

      return result;
    }

    error_t quick_backdrop_t::view(source_explorer_t &srcexp) const
    {
      if (lak::TreeNode("0x%zX Properties (Quick Backdrop)##%zX",
                        (size_t)entry.ID,
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        ImGui::Text("Size: 0x%zX", (size_t)size);
        ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
        ImGui::Text("Collision: 0x%zX", (size_t)collision);
        ImGui::Text(
          "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);

        shape.view(srcexp);

        ImGui::Text("Handle: 0x%zX", (size_t)shape.handle);
        if (shape.handle < 0xFFFF)
        {
          auto *img = GetImage(srcexp.state, shape.handle);
          if (img) img->view(srcexp);
        }

        ImGui::Separator();
        ImGui::TreePop();
      }

      return error_t::ok;
    }

    error_t backdrop_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Backdrop");
      error_t result = entry.read(game, strm);

      if (result == error_t::ok)
      {
        lak::memory bstrm = entry.decode();
        size              = bstrm.read_u32();
        obstacle          = bstrm.read_u16();
        collision         = bstrm.read_u16();
        dimension.x       = game.oldGame ? bstrm.read_u16() : bstrm.read_u32();
        dimension.y       = game.oldGame ? bstrm.read_u16() : bstrm.read_u32();
        // if (!game.oldGame) bstrm.skip(2);
        handle = bstrm.read_u16();
      }

      return result;
    }

    error_t backdrop_t::view(source_explorer_t &srcexp) const
    {
      if (lak::TreeNode("0x%zX Properties (Backdrop)##%zX",
                        (size_t)entry.ID,
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        ImGui::Text("Size: 0x%zX", (size_t)size);
        ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
        ImGui::Text("Collision: 0x%zX", (size_t)collision);
        ImGui::Text(
          "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);
        ImGui::Text("Handle: 0x%zX", (size_t)handle);
        if (handle < 0xFFFF)
        {
          auto *img = GetImage(srcexp.state, handle);
          if (img) img->view(srcexp);
        }

        ImGui::Separator();
        ImGui::TreePop();
      }

      return error_t::ok;
    }

    error_t animation_direction_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Animation Direction");

      minSpeed = strm.read_u8();
      maxSpeed = strm.read_u8();
      repeat   = strm.read_u16();
      backTo   = strm.read_u16();
      handles.resize(strm.read_u16());
      for (auto &handle : handles) handle = strm.read_u16();

      return error_t::ok;
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

      return error_t::ok;
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
          strm.position    = begin + offset;
          directions[index].read(game, strm);
          strm.position = pos;
        }
        ++index;
      }

      return error_t::ok;
    }

    error_t animation_t::view(source_explorer_t &srcexp) const
    {
      size_t index = 0;
      for (const auto &direction : directions)
      {
        if (direction.handles.size() > 0 &&
            lak::TreeNode("Animation Direction 0x%zX", index))
        {
          ImGui::Separator();
          direction.view(srcexp);
          ImGui::Separator();
          ImGui::TreePop();
        }
        ++index;
      }

      return error_t::ok;
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
          strm.position    = begin + offset;
          animations[index].read(game, strm);
          strm.position = pos;
        }
        ++index;
      }

      return error_t::ok;
    }

    error_t animation_header_t::view(source_explorer_t &srcexp) const
    {
      if (ImGui::TreeNode("Animations"))
      {
        ImGui::Separator();
        ImGui::Text("Size: 0x%zX", (size_t)size);
        ImGui::Text("Animations: %zu", animations.size());
        size_t index = 0;
        for (const auto &animation : animations)
        {
          // TODO: figure out what this was meant to be checking
          if (/*animation.offsets[index] > 0 &&*/ lak::TreeNode(
            "Animation 0x%zX", index))
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

      return error_t::ok;
    }

    error_t common_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Common Object");
      error_t result = entry.read(game, strm);

      if (result == error_t::ok)
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

        if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
        {
          animationsOffset = cstrm.read_u16();
          movementsOffset  = cstrm.read_u16(); // are these in the right order?
          version          = cstrm.read_u16(); // are these in the right order?
          counterOffset    = cstrm.read_u16(); // are these in the right order?
          systemOffset     = cstrm.read_u16(); // are these in the right order?
        }
        // else if (mode == game_mode_t::_284)
        // {
        //     counterOffset = cstrm.read_u16();
        //     version = cstrm.read_u16();
        //     cstrm.position += 2;
        //     movementsOffset = cstrm.read_u16();
        //     extensionOffset = cstrm.read_u16();
        //     animationsOffset = cstrm.read_u16();
        // }
        else
        {
          movementsOffset  = cstrm.read_u16();
          animationsOffset = cstrm.read_u16();
          version          = cstrm.read_u16();
          counterOffset    = cstrm.read_u16();
          systemOffset     = cstrm.read_u16();
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

        if (mode == game_mode_t::_284)
          systemOffset = cstrm.read_u16();
        else
          extensionOffset = cstrm.read_u16();

        valuesOffset  = cstrm.read_u16();
        stringsOffset = cstrm.read_u16();
        newFlags      = cstrm.read_u32();
        preferences   = cstrm.read_u32();
        identifier    = cstrm.read_u32();
        backColor.r   = cstrm.read_u8();
        backColor.g   = cstrm.read_u8();
        backColor.b   = cstrm.read_u8();
        backColor.a   = cstrm.read_u8();
        fadeInOffset  = cstrm.read_u32();
        fadeOutOffset = cstrm.read_u32();

        if (animationsOffset > 0)
        {
          cstrm.position = begin + animationsOffset;
          animations     = std::make_unique<animation_header_t>();
          animations->read(game, cstrm);
        }
      }

      return result;
    }

    error_t common_t::view(source_explorer_t &srcexp) const
    {
      if (lak::TreeNode("0x%zX Properties (Common)##%zX",
                        (size_t)entry.ID,
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
        {
          ImGui::Text("Animations Offset: 0x%zX", (size_t)animationsOffset);
          ImGui::Text("Movements Offset: 0x%zX", (size_t)movementsOffset);
          ImGui::Text("Version: 0x%zX", (size_t)version);
          ImGui::Text("Counter Offset: 0x%zX", (size_t)counterOffset);
          ImGui::Text("System Offset: 0x%zX", (size_t)systemOffset);
          ImGui::Text("Flags: 0x%zX", (size_t)flags);
          ImGui::Text("Extension Offset: 0x%zX", (size_t)extensionOffset);
        }
        // else if (mode == game_mode_t::_284)
        // {
        //     ImGui::Text("Counter Offset: 0x%zX", (size_t)counterOffset);
        //     ImGui::Text("Version: 0x%zX", (size_t)version);
        //     ImGui::Text("Movements Offset: 0x%zX", (size_t)movementsOffset);
        //     ImGui::Text("Extension Offset: 0x%zX", (size_t)extensionOffset);
        //     ImGui::Text("Animations Offset: 0x%zX",
        //     (size_t)animationsOffset); ImGui::Text("Flags: 0x%zX",
        //     (size_t)flags); ImGui::Text("System Offset: 0x%zX",
        //     (size_t)systemOffset);
        // }
        else
        {
          ImGui::Text("Movements Offset: 0x%zX", (size_t)movementsOffset);
          ImGui::Text("Animations Offset: 0x%zX", (size_t)animationsOffset);
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
      return error_t::ok;
    }

    error_t item_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Object");
      error_t result = entry.read(game, strm);

      auto dstrm = entry.decode();
      handle     = dstrm.read_u16();
      type       = (object_type_t)dstrm.read_s16();
      dstrm.read_u16(); // flags
      dstrm.read_u16(); // "no longer used"
      inkEffect      = dstrm.read_u32();
      inkEffectParam = dstrm.read_u32();

      while (result == error_t::ok)
      {
        switch ((chunk_t)strm.peek_u16())
        {
          case chunk_t::object_name:
            name   = std::make_unique<string_chunk_t>();
            result = name->read(game, strm);
            break;

          case chunk_t::object_properties:
            switch (type)
            {
              case object_type_t::quick_backdrop:
                quickBackdrop = std::make_unique<quick_backdrop_t>();
                quickBackdrop->read(game, strm);
                break;
              case object_type_t::backdrop:
                backdrop = std::make_unique<backdrop_t>();
                backdrop->read(game, strm);
                break;
              default:
                common = std::make_unique<common_t>();
                common->read(game, strm);
                break;
            }
            break;

          case chunk_t::object_effect:
            effect = std::make_unique<effect_t>();
            result = effect->read(game, strm);
            break;

          case chunk_t::last:
            end    = std::make_unique<last_t>();
            result = end->read(game, strm);
          default: goto finished;
        }
      }
    finished:

      return result;
    }

    error_t item_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX %s '%s'##%zX",
                        (size_t)entry.ID,
                        GetObjectTypeString(type),
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

    std::unordered_map<uint32_t, std::vector<std::u16string>>
    item_t::image_handles() const
    {
      std::unordered_map<uint32_t, std::vector<std::u16string>> result;

      if (quickBackdrop)
        result[quickBackdrop->shape.handle].emplace_back(u"Backdrop");
      if (backdrop) result[backdrop->handle].emplace_back(u"Quick Backdrop");
      if (common && common->animations)
      {
        for (size_t animIndex = 0;
             animIndex < common->animations->animations.size();
             ++animIndex)
        {
          const auto animation = common->animations->animations[animIndex];
          for (int i = 0; i < 32; ++i)
          {
            if (animation.offsets[i] > 0)
            {
              for (size_t frame = 0;
                   frame < animation.directions[i].handles.size();
                   ++frame)
              {
                auto handle = animation.directions[i].handles[frame];
                result[handle].emplace_back(
                  u"Animation-"s + se::to_u16string(animIndex) +
                  u" Direction-"s + se::to_u16string(i) + u" Frame-"s +
                  se::to_u16string(frame));
              }
            }
          }
        }
      }

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
        if (result != error_t::ok) break;
        result         = item.read(game, strm);
        game.completed = (float)((double)strm.position / (double)strm.size());
      }

      if (strm.position < entry.end)
      {
        WARNING("There is still 0x" << entry.end - strm.position
                                    << " bytes left in the object bank");
      }
      else if (strm.position > entry.end)
      {
        ERROR("Object bank overshot entry by 0x" << strm.position - entry.end
                                                 << " bytes");
      }

      strm.position = entry.end;

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX Object Bank (%zu Items)##%zX",
                        (size_t)entry.ID,
                        items.size(),
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const item_t &item : items) item.view(srcexp);

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

      if (result == error_t::ok)
      {
        auto pstrm = entry.decode();
        unknown    = pstrm.read_u32();
        for (auto &color : colors)
        {
          color.r = pstrm.read_u8();
          color.g = pstrm.read_u8();
          color.b = pstrm.read_u8();
          /* color.a = */ pstrm.read_u8();
          color.a = 255u;
        }
      }

      return result;
    }

    error_t palette_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode(
            "0x%zX Frame Palette##%zX", (size_t)entry.ID, entry.position))
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
      for (int i = 0; i < 256; ++i) result[i] = colors[i];

      return result;
    }

    error_t object_instance_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Object Instance");

      info         = strm.read_u16();
      handle       = strm.read_u16();
      position.x   = game.oldGame ? strm.read_s16() : strm.read_s32();
      position.y   = game.oldGame ? strm.read_s16() : strm.read_s32();
      parentType   = (object_parent_type_t)strm.read_u16();
      parentHandle = strm.read_u16(); // object info (?)
      if (!game.oldGame)
      {
        layer   = strm.read_u16();
        unknown = strm.read_u16();
      }

      return error_t::ok;
    }

    error_t object_instance_t::view(source_explorer_t &srcexp) const
    {
      auto *obj = GetObject(srcexp.state, handle);
      std::u8string str;
      if (obj && obj->name) str += lak::to_u8string(obj->name->value);

      if (lak::TreeNode(
            "0x%zX %s##%zX", (size_t)handle, str.c_str(), (size_t)info))
      {
        ImGui::Separator();

        ImGui::Text("Handle: 0x%zX", (size_t)handle);
        ImGui::Text("Info: 0x%zX", (size_t)info);
        ImGui::Text(
          "Position: (%li, %li)", (long)position.x, (long)position.y);
        ImGui::Text("Parent Type: %s (0x%zX)",
                    GetObjectParentTypeString(parentType),
                    (size_t)parentType);
        ImGui::Text("Parent Handle: 0x%zX", (size_t)parentHandle);
        ImGui::Text("Layer: 0x%zX", (size_t)layer);
        ImGui::Text("Unknown: 0x%zX", (size_t)unknown);

        if (obj) obj->view(srcexp);

        switch (parentType)
        {
          case object_parent_type_t::frame_item:
            if (auto *parentObj = GetObject(srcexp.state, parentHandle);
                parentObj)
              parentObj->view(srcexp);
            break;
          case object_parent_type_t::frame:
            if (auto *parentObj = GetFrame(srcexp.state, parentHandle);
                parentObj)
              parentObj->view(srcexp);
            break;
          case object_parent_type_t::none:
          case object_parent_type_t::qualifier:
          default: break;
        }

        ImGui::Separator();
        ImGui::TreePop();
      }

      return error_t::ok;
    }

    error_t object_instances_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Object Instances");
      error_t result = entry.read(game, strm);

      if (result == error_t::ok)
      {
        auto hstrm = entry.decode();
        objects.clear();
        objects.resize(hstrm.read_u32());
        DEBUG("Objects: 0x" << objects.size());
        for (auto &object : objects) object.read(game, hstrm);
      }

      return result;
    }

    error_t object_instances_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode(
            "0x%zX Object Instances##%zX", (size_t)entry.ID, entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const auto &object : objects) object.view(srcexp);

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

    error_t play_header_r::view(source_explorer_t &srcexp) const
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
      error_t result = error_t::ok;

      if (lak::TreeNode(
            "0x%zX Random Seed##%zX", (size_t)entry.ID, entry.position))
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

      while (result == error_t::ok)
      {
        game.completed = (float)((double)strm.position / (double)strm.size());
        switch ((chunk_t)strm.peek_u16())
        {
          case chunk_t::frame_name:
            name   = std::make_unique<string_chunk_t>();
            result = name->read(game, strm);
            break;

          case chunk_t::frame_header:
            header = std::make_unique<header_t>();
            result = header->read(game, strm);
            break;

          case chunk_t::frame_password:
            password = std::make_unique<password_t>();
            result   = password->read(game, strm);
            break;

          case chunk_t::frame_palette:
            palette = std::make_unique<palette_t>();
            result  = palette->read(game, strm);
            break;

          case chunk_t::frame_object_instances:
            objectInstances = std::make_unique<object_instances_t>();
            result          = objectInstances->read(game, strm);
            break;

          case chunk_t::frame_fade_in_frame:
            fadeInFrame = std::make_unique<fade_in_frame_t>();
            result      = fadeInFrame->read(game, strm);
            break;

          case chunk_t::frame_fade_out_frame:
            fadeOutFrame = std::make_unique<fade_out_frame_t>();
            result       = fadeOutFrame->read(game, strm);
            break;

          case chunk_t::frame_fade_in:
            fadeIn = std::make_unique<fade_in_t>();
            result = fadeIn->read(game, strm);
            break;

          case chunk_t::frame_fade_out:
            fadeOut = std::make_unique<fade_out_t>();
            result  = fadeOut->read(game, strm);
            break;

          case chunk_t::frame_events:
            events = std::make_unique<events_t>();
            result = events->read(game, strm);
            break;

          case chunk_t::frame_play_header:
            playHead = std::make_unique<play_header_r>();
            result   = playHead->read(game, strm);
            break;

          case chunk_t::frame_additional_items:
            additionalItem = std::make_unique<additional_item_t>();
            result         = additionalItem->read(game, strm);
            break;

          case chunk_t::frame_additional_items_instances:
            additionalItemInstance =
              std::make_unique<additional_item_instance_t>();
            result = additionalItemInstance->read(game, strm);
            break;

          case chunk_t::frame_layers:
            layers = std::make_unique<layers_t>();
            result = layers->read(game, strm);
            break;

          case chunk_t::frame_virtual_size:
            virtualSize = std::make_unique<virtual_size_t>();
            result      = virtualSize->read(game, strm);
            break;

          case chunk_t::demo_file_path:
            demoFilePath = std::make_unique<demo_file_path_t>();
            result       = demoFilePath->read(game, strm);
            break;

          case chunk_t::random_seed:
            randomSeed = std::make_unique<random_seed_t>();
            result     = randomSeed->read(game, strm);
            break;

          case chunk_t::frame_layer_effect:
            layerEffect = std::make_unique<layer_effect_t>();
            result      = layerEffect->read(game, strm);
            break;

          case chunk_t::frame_bluray:
            blueray = std::make_unique<blueray_t>();
            result  = blueray->read(game, strm);
            break;

          case chunk_t::movement_timer_base:
            movementTimeBase = std::make_unique<movement_time_base_t>();
            result           = movementTimeBase->read(game, strm);
            break;

          case chunk_t::mosaic_image_table:
            mosaicImageTable = std::make_unique<mosaic_image_table_t>();
            result           = mosaicImageTable->read(game, strm);
            break;

          case chunk_t::frame_effects:
            effects = std::make_unique<effects_t>();
            result  = effects->read(game, strm);
            break;

          case chunk_t::frame_iphone_options:
            iphoneOptions = std::make_unique<iphone_options_t>();
            result        = iphoneOptions->read(game, strm);
            break;

          case chunk_t::frame_chunk334C:
            chunk334C = std::make_unique<chunk_334C_t>();
            result    = chunk334C->read(game, strm);
            break;

          case chunk_t::last:
            end    = std::make_unique<last_t>();
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
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX '%s'##%zX",
                        (size_t)entry.ID,
                        (name ? lak::strconv<char>(name->value).c_str() : ""),
                        entry.position))
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

    error_t handles_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Frame Handles");
      error_t result = entry.read(game, strm);

      if (result == error_t::ok)
      {
        auto hstrm = entry.decode();
        handles.resize(hstrm.size() / 2);
        for (auto &handle : handles) handle = hstrm.read_u16();
      }

      return result;
    }

    error_t handles_t::view(source_explorer_t &srcexp) const
    {
      return basic_view(srcexp, "Frame Handles");
    }

    error_t bank_t::read(game_t &game, lak::memory &strm)
    {
      DEBUG("Reading Frame Bank");
      error_t result = entry.read(game, strm);

      items.clear();
      while (result == error_t::ok &&
             (chunk_t)strm.peek_u16() == chunk_t::frame)
      {
        items.emplace_back();
        result         = items.back().read(game, strm);
        game.completed = (float)((double)strm.position / (double)strm.size());
      }

      if (strm.position < entry.end)
      {
        WARNING("There is still 0x" << entry.end - strm.position
                                    << " bytes left in the frame bank");
      }
      else if (strm.position > entry.end)
      {
        DEBUG("Frame bank overshot entry by 0x" << strm.position - entry.end
                                                << " bytes");
      }

      // strm.position = entry.end;

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX Frame Bank (%zu Items)##%zX",
                        (size_t)entry.ID,
                        items.size(),
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const item_t &item : items) item.view(srcexp);

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

      if (!game.oldGame && game.productBuild >= 284) --entry.handle;

      DEBUG("Reading Image (Handle: " << entry.handle << ")");

      if (result == error_t::ok)
      {
        lak::memory istrm = entry.decode(176 + (game.oldGame ? 16 : 80));
        if (game.oldGame)
          checksum = istrm.read_u16();
        else
          checksum = istrm.read_u32();
        reference    = istrm.read_u32();
        dataSize     = istrm.read_u32();
        size.x       = istrm.read_u16();
        size.y       = istrm.read_u16();
        graphicsMode = (graphics_mode_t)istrm.read_u8();
        flags        = (image_flag_t)istrm.read_u8();
#if 0
                if (graphicsMode <= graphics_mode_t::graphics3)
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
        action.x  = istrm.read_u16();
        action.y  = istrm.read_u16();
        if (!game.oldGame) transparent = ColorFrom32bitRGBA(istrm);
        dataPosition = istrm.position;
      }
      else
        ERROR("Error Reading Image (Handle: " << entry.handle << ")");

      return result;
    }

    error_t item_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode(
            "0x%zX Image##%zX", (size_t)entry.handle, entry.position))
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
        ImGui::Text(
          "Hotspot: (%zu, %zu)", (size_t)hotspot.x, (size_t)hotspot.y);
        ImGui::Text("Action: (%zu, %zu)", (size_t)action.x, (size_t)action.y);
        {
          lak::vec4f_t col = ((lak::vec4f_t)transparent) / 255.0f;
          ImGui::ColorEdit4("Transparent", &col.x);
        }
        ImGui::Text("Data Position: 0x%zX", dataPosition);

        if (ImGui::Button("View Image"))
        {
          srcexp.image = std::move(
            CreateTexture(image(srcexp.dumpColorTrans), srcexp.graphicsMode));
        }

        ImGui::Separator();
        ImGui::TreePop();
      }

      return result;
    }

    lak::memory item_t::image_data() const
    {
      lak::memory strm = entry.decode().seek(dataPosition);
      if ((flags & image_flag_t::LZX) != image_flag_t::none)
      {
        [[maybe_unused]] const uint32_t decomplen = strm.read_u32();
        return lak::memory(Inflate(strm.read(strm.read_u32())));
      }
      else
      {
        return strm;
      }
    }

    bool item_t::need_palette() const
    {
      return graphicsMode == graphics_mode_t::graphics2 ||
             graphicsMode == graphics_mode_t::graphics3;
    }

    lak::image4_t item_t::image(const bool colorTrans,
                                const lak::color4_t palette[256]) const
    {
      lak::image4_t result;
      result.resize((lak::vec2s_t)size);

      lak::memory strm = image_data();

      [[maybe_unused]] size_t bytesRead;
      if ((flags & (image_flag_t::RLE | image_flag_t::RLEW |
                    image_flag_t::RLET)) != image_flag_t::none)
      {
        bytesRead = ReadRLE(strm, result, graphicsMode, palette);
      }
      else
      {
        bytesRead = ReadRGB(strm, result, graphicsMode, palette);
      }

      if ((flags & image_flag_t::alpha) != image_flag_t::none)
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

      DEBUG("Image Bank Size: " << items.size());

      for (auto &item : items)
      {
        if (result != error_t::ok) break;
        result         = item.read(game, strm);
        game.completed = (float)((double)strm.position / (double)strm.size());
      }

      if (strm.position < entry.end)
      {
        WARNING("There is still 0x" << entry.end - strm.position
                                    << " bytes left in the image bank");
      }
      else if (strm.position > entry.end)
      {
        ERROR("Image bank overshot entry by 0x" << strm.position - entry.end
                                                << " bytes");
      }

      strm.position = entry.end;

      if ((chunk_t)strm.peek_u16() == chunk_t::image_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX Image Bank (%zu Items)##%zX",
                        (size_t)entry.ID,
                        items.size(),
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const item_t &item : items) item.view(srcexp);

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
        if (result != error_t::ok) break;
        result         = item.read(game, strm);
        game.completed = (float)((double)strm.position / (double)strm.size());
      }

      if (strm.position < entry.end)
      {
        WARNING("There is still 0x" << entry.end - strm.position
                                    << " bytes left in the font bank");
      }
      else if (strm.position > entry.end)
      {
        ERROR("Font bank overshot entry by 0x" << strm.position - entry.end
                                               << " bytes");
      }

      strm.position = entry.end;

      if ((chunk_t)strm.peek_u16() == chunk_t::font_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX Font Bank (%zu Items)##%zX",
                        (size_t)entry.ID,
                        items.size(),
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const item_t &item : items) item.view(srcexp);

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
        if (result != error_t::ok) break;
        result         = item.read(game, strm);
        game.completed = (float)((double)strm.position / (double)strm.size());
      }

      if (strm.position < entry.end)
      {
        WARNING("There is still 0x" << entry.end - strm.position
                                    << " bytes left in the sound bank");
      }
      else if (strm.position > entry.end)
      {
        ERROR("Sound bank overshot entry by 0x" << strm.position - entry.end
                                                << " bytes");
      }

      strm.position = entry.end;

      if ((chunk_t)strm.peek_u16() == chunk_t::sound_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX Sound Bank (%zu Items)##%zX",
                        (size_t)entry.ID,
                        items.size(),
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const item_t &item : items) item.view(srcexp);

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
        if (result != error_t::ok) break;
        result         = item.read(game, strm);
        game.completed = (float)((double)strm.position / (double)strm.size());
      }

      if (strm.position < entry.end)
      {
        WARNING("There is still 0x" << entry.end - strm.position
                                    << " bytes left in the music bank");
      }
      else if (strm.position > entry.end)
      {
        ERROR("Music bank overshot entry by 0x" << strm.position - entry.end
                                                << " bytes");
      }

      strm.position = entry.end;

      if ((chunk_t)strm.peek_u16() == chunk_t::music_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      error_t result = error_t::ok;

      if (lak::TreeNode("0x%zX Music Bank (%zu Items)##%zX",
                        (size_t)entry.ID,
                        items.size(),
                        entry.position))
      {
        ImGui::Separator();

        entry.view(srcexp);

        for (const item_t &item : items) item.view(srcexp);

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

    auto init_chunk = [&](auto &chunk) {
      chunk = std::make_unique<
        typename std::remove_reference_t<decltype(chunk)>::value_type>();
      return chunk->read(game, strm);
    };

    while (result == error_t::ok)
    {
      if (strm.size() > 0)
        game.completed = (float)((double)strm.position / (double)strm.size());
      chunk_t childID = (chunk_t)strm.peek_u16();
      switch (childID)
      {
        case chunk_t::title:
          DEBUG("Reading Title");
          result = init_chunk(title);
          break;

        case chunk_t::author:
          DEBUG("Reading Author");
          result = init_chunk(author);
          break;

        case chunk_t::copyright:
          DEBUG("Reading Copyright");
          result = init_chunk(copyright);
          break;

        case chunk_t::project_path:
          DEBUG("Reading Project Path");
          result = init_chunk(projectPath);
          break;

        case chunk_t::output_path:
          DEBUG("Reading Project Output Path");
          result = init_chunk(outputPath);
          break;

        case chunk_t::about:
          DEBUG("Reading About");
          result = init_chunk(about);
          break;

        case chunk_t::vitalise_preview:
          DEBUG("Reading Project Vitalise Preview");
          result = init_chunk(vitalisePreview);
          break;

        case chunk_t::menu:
          DEBUG("Reading Project Menu");
          result = init_chunk(menu);
          break;

        case chunk_t::extra_path:
          DEBUG("Reading Extension Path");
          result = init_chunk(extensionPath);
          break;

        case chunk_t::extensions:
          DEBUG("Reading Extensions");
          result = init_chunk(extensions);
          break;

        case chunk_t::extra_data:
          DEBUG("Reading Extension Data");
          result = init_chunk(extensionData);
          break;

        case chunk_t::additional_extensions:
          DEBUG("Reading Additional Extensions");
          result = init_chunk(additionalExtensions);
          break;

        case chunk_t::app_doc:
          DEBUG("Reading Application Doc");
          result = init_chunk(appDoc);
          break;

        case chunk_t::other_extension:
          DEBUG("Reading Other Extension");
          result = init_chunk(otherExtension);
          break;

        case chunk_t::extensions_list:
          DEBUG("Reading Extension List");
          result = init_chunk(extensionList);
          break;

        case chunk_t::icon:
          DEBUG("Reading Icon");
          result = init_chunk(icon);
          break;

        case chunk_t::demo_version:
          DEBUG("Reading Demo Version");
          result = init_chunk(demoVersion);
          break;

        case chunk_t::security_number:
          DEBUG("Reading Security Number");
          result = init_chunk(security);
          break;

        case chunk_t::binary_files:
          DEBUG("Reading Binary Files");
          result = init_chunk(binaryFiles);
          break;

        case chunk_t::menu_images:
          DEBUG("Reading Menu Images");
          result = init_chunk(menuImages);
          break;

        case chunk_t::movement_extensions:
          DEBUG("Reading Movement Extensions");
          result = init_chunk(movementExtensions);
          break;

        case chunk_t::exe_only:
          DEBUG("Reading EXE Only");
          result = init_chunk(exe);
          break;

        case chunk_t::protection:
          DEBUG("Reading Protection");
          result = init_chunk(protection);
          break;

        case chunk_t::shaders:
          DEBUG("Reading Shaders");
          result = init_chunk(shaders);
          break;

        case chunk_t::extended_header:
          DEBUG("Reading Extended Header");
          result = init_chunk(extendedHeader);
          break;

        case chunk_t::spacer:
          DEBUG("Reading Spacer");
          result = init_chunk(spacer);
          break;

        case chunk_t::chunk224F:
          DEBUG("Reading Chunk 224F");
          result = init_chunk(chunk224F);
          break;

        case chunk_t::title2:
          DEBUG("Reading Title 2");
          result = init_chunk(title2);
          break;

        case chunk_t::object_names:
          DEBUG("Reading Object Names");
          result = init_chunk(objectNames);
          break;

        case chunk_t::object_properties:
          DEBUG("Reading Object Properties");
          result = init_chunk(objectProperties);
          break;

        case chunk_t::font_meta:
          DEBUG("Reading TrueType Fonts Meta");
          result = init_chunk(truetypeFontsMeta);
          break;

        case chunk_t::font_chunk:
          DEBUG("Reading TrueType Fonts");
          result = init_chunk(truetypeFonts);
          break;

        case chunk_t::global_events:
          DEBUG("Reading Global Events");
          result = init_chunk(globalEvents);
          break;

        case chunk_t::global_strings:
          DEBUG("Reading Global Strings");
          result = init_chunk(globalStrings);
          break;

        case chunk_t::global_string_names:
          DEBUG("Reading Global String Names");
          result = init_chunk(globalStringNames);
          break;

        case chunk_t::global_values:
          DEBUG("Reading Global Values");
          result = init_chunk(globalValues);
          break;

        case chunk_t::global_value_names:
          DEBUG("Reading Global Value Names");
          result = init_chunk(globalValueNames);
          break;

        case chunk_t::frame_handles:
          DEBUG("Reading Frame Handles");
          result = init_chunk(frameHandles);
          break;

        case chunk_t::frame_bank:
          DEBUG("Reading Fame Bank");
          result = init_chunk(frameBank);
          break;

        case chunk_t::frame:
          DEBUG("Reading Frame (Missing Frame Bank)");
          if (frameBank) ERROR("Frame Bank Already Exists");
          frameBank = std::make_unique<frame::bank_t>();
          frameBank->items.clear();
          while (result == error_t::ok &&
                 (chunk_t)strm.peek_u16() == chunk_t::frame)
          {
            frameBank->items.emplace_back();
            result = frameBank->items.back().read(game, strm);
          }
          break;

          // case chunk_t::object_bank2:
          //     DEBUG("Reading Object Bank 2");
          //     result = init_chunk(objectBank2);
          //     break;

        case chunk_t::object_bank:
        case chunk_t::object_bank2:
          DEBUG("Reading Object Bank");
          result = init_chunk(objectBank);
          break;

        case chunk_t::image_bank:
          DEBUG("Reading Image Bank");
          result = init_chunk(imageBank);
          break;

        case chunk_t::sound_bank:
          DEBUG("Reading Sound Bank");
          result = init_chunk(soundBank);
          break;

        case chunk_t::music_bank:
          DEBUG("Reading Music Bank");
          result = init_chunk(musicBank);
          break;

        case chunk_t::font_bank:
          DEBUG("Reading Font Bank");
          result = init_chunk(fontBank);
          break;

        case chunk_t::last:
          DEBUG("Reading Last");
          result = init_chunk(last);
          goto finished;

        default:
          DEBUG("Invalid Chunk: 0x" << (size_t)childID);
          unknownChunks.emplace_back();
          result = unknownChunks.back().read(game, strm);
          break;
      }
    }

  finished:
    return result;
  }

  error_t header_t::view(source_explorer_t &srcexp) const
  {
    error_t result = error_t::ok;

    if (lak::TreeNode(
          "0x%zX Game Header##%zX", (size_t)entry.ID, entry.position))
    {
      ImGui::Separator();

      entry.view(srcexp);

      title.view(srcexp, "Title", true);
      author.view(srcexp, "Author", true);
      copyright.view(srcexp, "Copyright", true);
      outputPath.view(srcexp, "Output Path");
      projectPath.view(srcexp, "Project Path");
      about.view(srcexp, "About");

      vitalisePreview.view(srcexp);
      menu.view(srcexp);
      extensionPath.view(srcexp);
      extensions.view(srcexp);
      extensionData.view(srcexp);
      additionalExtensions.view(srcexp);
      appDoc.view(srcexp);
      otherExtension.view(srcexp);
      extensionList.view(srcexp);
      icon.view(srcexp);
      demoVersion.view(srcexp);
      security.view(srcexp);
      binaryFiles.view(srcexp);
      menuImages.view(srcexp);
      movementExtensions.view(srcexp);
      objectBank2.view(srcexp);
      exe.view(srcexp);
      protection.view(srcexp);
      shaders.view(srcexp);
      extendedHeader.view(srcexp);
      spacer.view(srcexp);
      chunk224F.view(srcexp);
      title2.view(srcexp);

      globalEvents.view(srcexp);
      globalStrings.view(srcexp);
      globalStringNames.view(srcexp);
      globalValues.view(srcexp);
      globalValueNames.view(srcexp);

      frameHandles.view(srcexp);
      frameBank.view(srcexp);
      objectBank.view(srcexp);
      imageBank.view(srcexp);
      soundBank.view(srcexp);
      musicBank.view(srcexp);
      fontBank.view(srcexp);

      objectNames.view(srcexp);
      objectProperties.view(srcexp);
      truetypeFontsMeta.view(srcexp);
      truetypeFonts.view(srcexp);

      for (auto &unk : unknownStrings) unk.view(srcexp);

      for (auto &unk : unknownCompressed) unk.view(srcexp);

      for (auto &unk : unknownChunks)
        unk.basic_view(
          srcexp,
          (std::string("Unknown ") + std::to_string(unk.entry.position))
            .c_str());

      last.view(srcexp);

      ImGui::Separator();

      ImGui::TreePop();
    }

    return result;
  }
}

#include "encryption.cpp"
#include "image.cpp"
