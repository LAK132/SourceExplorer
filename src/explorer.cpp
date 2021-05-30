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

#include "lak/compression/deflate.hpp"
#include "lak/compression/lz4.hpp"

#include "explorer.h"
#include "tostring.hpp"

#ifdef GetObject
#  undef GetObject
#endif

#define TRACE_EXPECTED(EXPECTED, GOT)                                         \
  lak::streamify<char8_t>("expected '", EXPECTED, "', got '", GOT, "'")

namespace SourceExplorer
{
  bool force_compat = false;
  encryption_table decryptor;
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
    SCOPED_CHECKPOINT("LoadGame");

    srcexp.state        = game_t{};
    srcexp.state.compat = force_compat;

    RES_TRY_ASSIGN(srcexp.state.file =,
                   lak::read_file(srcexp.exe.path)
                     .map_err(MAP_TRACE(error::str_err, "LoadGame")));

    DEBUG("File Size: ", srcexp.state.file.size());

    RES_TRY_ERR(ParsePEHeader(srcexp.state.file, srcexp.state)
                  .map_err(APPEND_TRACE("while parsing PE header at: ",
                                        srcexp.state.file.position())));

    DEBUG("Successfully Parsed PE Header");

    _magic_key.clear();

    if (srcexp.state.product_build < 284 || srcexp.state.old_game ||
        srcexp.state.compat)
      _mode = game_mode_t::_OLD;
    else if (srcexp.state.product_build > 284)
      _mode = game_mode_t::_288;
    else
      _mode = game_mode_t::_284;

    if (_mode == game_mode_t::_OLD)
      _magic_char = 99; // '6';
    else
      _magic_char = 54; // 'c';

    RES_TRY_ERR(srcexp.state.game.read(srcexp.state, srcexp.state.file)
                  .map_err(APPEND_TRACE("while parsing PE header at: ",
                                        srcexp.state.file.position())));

    DEBUG("Successfully Read Game Entry");

    DEBUG("Unicode: ", (srcexp.state.unicode ? "true" : "false"));

    if (srcexp.state.game.project_path)
      srcexp.state.project = srcexp.state.game.project_path->value;

    if (srcexp.state.game.title)
      srcexp.state.title = srcexp.state.game.title->value;

    if (srcexp.state.game.copyright)
      srcexp.state.copyright = srcexp.state.game.copyright->value;

    DEBUG("Project Path: ", lak::strconv<char>(srcexp.state.project));
    DEBUG("Title: ", lak::strconv<char>(srcexp.state.title));
    DEBUG("Copyright: ", lak::strconv<char>(srcexp.state.copyright));

    if (srcexp.state.recompiled)
      WARNING("This Game May Have Been Recompiled!");

    if (srcexp.state.game.image_bank)
    {
      const auto &images = srcexp.state.game.image_bank->items;
      for (size_t i = 0; i < images.size(); ++i)
      {
        srcexp.state.image_handles[images[i].entry.handle] = i;
      }
    }

    if (srcexp.state.game.object_bank)
    {
      const auto &objects = srcexp.state.game.object_bank->items;
      for (size_t i = 0; i < objects.size(); ++i)
      {
        srcexp.state.object_handles[objects[i].handle] = i;
      }
    }

    return lak::ok_t{};
  }

  void GetEncryptionKey(game_t &game_state)
  {
    _magic_key.clear();
    _magic_key.reserve(256);

    if (_mode == game_mode_t::_284)
    {
      if (game_state.game.project_path)
        _magic_key += KeyString(game_state.game.project_path->value);
      if (_magic_key.size() < 0x80 && game_state.game.title)
        _magic_key += KeyString(game_state.game.title->value);
      if (_magic_key.size() < 0x80 && game_state.game.copyright)
        _magic_key += KeyString(game_state.game.copyright->value);
    }
    else
    {
      if (game_state.game.title)
        _magic_key += KeyString(game_state.game.title->value);
      if (_magic_key.size() < 0x80 && game_state.game.copyright)
        _magic_key += KeyString(game_state.game.copyright->value);
      if (_magic_key.size() < 0x80 && game_state.game.project_path)
        _magic_key += KeyString(game_state.game.project_path->value);
    }
    _magic_key.resize(0x100);
    std::memset(_magic_key.data() + 0x80, 0, 0x80);

    uint8_t *key_ptr = _magic_key.data();
    size_t len       = strlen((char *)key_ptr);
    uint8_t accum    = _magic_char;
    uint8_t hash     = _magic_char;
    for (size_t i = 0; i <= len; ++i)
    {
      hash = (hash << 7) + (hash >> 1);
      *key_ptr ^= hash;
      accum += *key_ptr * ((hash & 1) + 2);
      ++key_ptr;
    }
    *key_ptr = accum;

    decryptor.valid = false;
  }

  bool DecodeChunk(lak::span<uint8_t> chunk)
  {
    if (!decryptor.valid)
    {
      if (!decryptor.init(lak::span(_magic_key).first<0x100>(), _magic_char))
        return false;
    }

    return decryptor.decode(chunk);
  }

  error_t ParsePEHeader(lak::memory &strm, game_t &game_state)
  {
    SCOPED_CHECKPOINT("ParsePEHeader");

    uint16_t exe_sig = strm.read_u16();
    DEBUG("EXE Signature: ", exe_sig);
    if (exe_sig != WIN_EXE_SIG)
    {
      return lak::err_t{error(LINE_TRACE,
                              error::invalid_exe_signature,
                              TRACE_EXPECTED(WIN_EXE_SIG, exe_sig),
                              ", at ",
                              (strm.position() - 2))};
    }

    strm.seek(WIN_EXE_PNT);
    strm.seek(strm.read_u16());
    DEBUG("EXE Pointer: ", strm.position(), ", At: ", (size_t)WIN_EXE_PNT);

    int32_t pe_sig = strm.read_s32();
    DEBUG("PE Signature: ", pe_sig);
    if (pe_sig != WIN_PE_SIG)
    {
      return lak::err_t{error(LINE_TRACE,
                              error::invalid_pe_signature,
                              TRACE_EXPECTED(WIN_PE_SIG, pe_sig),
                              ", at ",
                              (strm.position() - 4))};
    }

    strm.skip(2);

    uint16_t num_header_sections = strm.read_u16();
    DEBUG("Number Of Header Sections: ", num_header_sections);

    strm.skip(16);

    const uint16_t optional_header = 0x60;
    const uint16_t data_dir        = 0x80;
    strm.skip(optional_header + data_dir);
    DEBUG("Pos: ", strm.position());

    uint64_t pos = 0;
    for (uint16_t i = 0; i < num_header_sections; ++i)
    {
      SCOPED_CHECKPOINT("Section ", i + 1, "/", num_header_sections);
      uint64_t start = strm.position();
      auto name      = strm.read_astring();
      DEBUG("Name: ", name);
      if (name == lak::string_view(".extra"))
      {
        strm.seek(start + 0x14);
        pos = strm.read_s32();
        break;
      }
      else if (i >= num_header_sections - 1)
      {
        strm.seek(start + 0x10);
        uint32_t size = strm.read_u32();
        uint32_t addr = strm.read_u32();
        DEBUG("Size: ", size);
        DEBUG("Addr: ", addr);
        pos = size + addr;
        break;
      }
      strm.seek(start + 0x28);
      DEBUG("Pos: ", strm.position());
    }

    while (true)
    {
      strm.seek(pos);
      uint16_t first_short = strm.read_u16();
      DEBUG("First Short: ", first_short);
      strm.seek(pos);
      uint32_t pame_magic = strm.read_u32();
      DEBUG("PAME Magic: ", pame_magic);
      strm.seek(pos);
      uint64_t pack_magic = strm.read_u64();
      DEBUG("Pack Magic: ", pack_magic);
      strm.seek(pos);
      DEBUG("Pos: ", pos);

      if (first_short == (uint16_t)chunk_t::header ||
          pame_magic == HEADER_GAME)
      {
        DEBUG("Old Game");
        game_state.old_game = true;
        game_state.state    = {};
        game_state.state.push(chunk_t::old);
        break;
      }
      else if (pack_magic == HEADER_PACK)
      {
        DEBUG("New Game");
        game_state.old_game = false;
        game_state.state    = {};
        game_state.state.push(chunk_t::_new);
        pos = ParsePackData(strm, game_state);
        break;
      }
      else if (first_short == 0x222C)
      {
        strm.skip(4);
        strm.skip(strm.read_u32());
        pos = strm.position();
      }
      else if (first_short == 0x7F7F)
      {
        pos += 8;
      }
      else
      {
        return lak::err_t{
          error(LINE_TRACE, error::invalid_game_signature, LINE_TRACE_STR)};
      }

      if (pos > strm.size())
      {
        return lak::err_t{error(LINE_TRACE,
                                error::invalid_game_signature,
                                ": pos (",
                                pos,
                                ") > strm.size (",
                                strm.size(),
                                ")")};
      }
    }

    uint32_t header = strm.read_u32();
    DEBUG("Header: ", header);

    game_state.unicode = false;
    if (header == HEADER_UNIC)
    {
      game_state.unicode  = true;
      game_state.old_game = false;
    }
    else if (header != HEADER_GAME)
    {
      return lak::err_t{error(LINE_TRACE,
                              error::invalid_game_signature,
                              TRACE_EXPECTED(HEADER_GAME, header),
                              ", at ",
                              (strm.position() - 4))};
    }

    game_state.runtime_version = (product_code_t)strm.read_u16();
    DEBUG("Runtime Version: ", (int)game_state.runtime_version);
    if (game_state.runtime_version == product_code_t::CNCV1VER)
    {
      DEBUG("CNCV1VER");
      // cnc = true;
      // readCNC(strm);
    }
    else
    {
      game_state.runtime_sub_version = strm.read_u16();
      DEBUG("Runtime Sub-Version: ", game_state.runtime_sub_version);
      game_state.product_version = strm.read_u32();
      DEBUG("Product Version: ", game_state.product_version);
      game_state.product_build = strm.read_u32();
      DEBUG("Product Build: ", game_state.product_build);
    }

    return lak::ok_t{};
  }

  uint64_t ParsePackData(lak::memory &strm, game_t &game_state)
  {
    DEBUG("Parsing pack data");

    uint64_t start  = strm.position();
    uint64_t header = strm.read_u64();
    DEBUG("Header: ", header);
    uint32_t header_size = strm.read_u32();
    (void)header_size;
    DEBUG("Header Size: ", header_size);
    uint32_t data_size = strm.read_u32();
    DEBUG("Data Size: ", data_size);

    strm.seek(start + data_size - 0x20);

    header = strm.read_u32();
    DEBUG("Head: ", header);
    bool unicode = header == HEADER_UNIC;
    if (unicode)
    {
      DEBUG("Unicode Game");
    }
    else
    {
      DEBUG("ASCII Game");
    }

    strm.seek(start + 0x10);

    uint32_t format_version = strm.read_u32();
    (void)format_version;
    DEBUG("Format Version: ", format_version);

    strm.skip(0x8);

    int32_t count = strm.read_s32();
    assert(count >= 0);
    DEBUG("Pack Count: ", count);

    uint64_t off = strm.position();
    DEBUG("Offset: ", off);

    for (int32_t i = 0; i < count; ++i)
    {
      if ((strm.size() - strm.position()) < 2) break;
      uint32_t val = strm.read_u16();

      if ((strm.size() - strm.position()) < val) break;
      strm.skip(val);

      if ((strm.size() - strm.position()) < 4) break;
      val = strm.read_u32();

      if ((strm.size() - strm.position()) < val) break;
      strm.skip(val);
    }

    header = strm.read_u32();
    DEBUG("Header: ", header);

    bool has_bingo = (header != HEADER_GAME) && (header != HEADER_UNIC);
    DEBUG("Has Bingo: ", (has_bingo ? "true" : "false"));

    strm.seek(off);

    game_state.pack_files.resize(count);

    for (int32_t i = 0; i < count; ++i)
    {
      SCOPED_CHECKPOINT("Pack ", i + 1, "/", count);

      uint32_t read = strm.read_u16();
      // size_t strstart = strm.position();

      DEBUG("Filename Length: ", read);
      DEBUG("Position: ", strm.position());

      if (unicode)
        game_state.pack_files[i].filename =
          strm.read_u16string_exact(read).to_string();
      else
        game_state.pack_files[i].filename =
          lak::to_u16string(strm.read_astring_exact(read));

      // strm.seek(strstart + (unicode ? read * 2 : read));

      // DEBUG("String Start: ", strstart);
      DEBUG("Packfile '", game_state.pack_files[i].filename, "'");

      if (has_bingo)
        game_state.pack_files[i].bingo = strm.read_u32();
      else
        game_state.pack_files[i].bingo = 0;

      DEBUG("Bingo: ", game_state.pack_files[i].bingo);

      // if (unicode)
      //     read = strm.read_u32();
      // else
      //     read = strm.read_u16();
      read = strm.read_u32();

      DEBUG("Pack File Data Size: ", read, ", Pos: ", strm.position());
      game_state.pack_files[i].data = strm.read(read);
    }

    header = strm.read_u32(); // PAMU sometimes
    DEBUG("Header: ", header);

    if (header == HEADER_GAME || header == HEADER_UNIC)
    {
      uint32_t pos = (uint32_t)strm.position();
      strm.unread(0x4);
      return pos;
    }
    return strm.position();
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

  result_t<lak::color4_t> ColorFrom8bit(lak::memory &strm,
                                        const lak::color4_t palette[256])
  {
    CHECK_REMAINING(strm, 1);

    if (palette) return lak::ok_t{palette[strm.read_u8()]};
    return lak::ok_t{ColorFrom8bit(strm.read_u8())};
  }

  result_t<lak::color4_t> ColorFrom15bit(lak::memory &strm)
  {
    CHECK_REMAINING(strm, 2);

    uint16_t val = strm.read_u8();
    val |= (uint16_t)strm.read_u8() << 8;
    return lak::ok_t{ColorFrom15bit(val)};
  }

  result_t<lak::color4_t> ColorFrom16bit(lak::memory &strm)
  {
    CHECK_REMAINING(strm, 2);

    uint16_t val = strm.read_u8();
    val |= (uint16_t)strm.read_u8() << 8;
    return lak::ok_t{ColorFrom16bit(val)};
  }

  result_t<lak::color4_t> ColorFrom24bitBGR(lak::memory &strm)
  {
    CHECK_REMAINING(strm, 3);
    lak::color4_t rtn;
    rtn.b = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.r = strm.read_u8();
    rtn.a = 255;
    return lak::ok_t{rtn};
  }

  result_t<lak::color4_t> ColorFrom32bitBGRA(lak::memory &strm)
  {
    CHECK_REMAINING(strm, 4);

    lak::color4_t rtn;
    rtn.b = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.r = strm.read_u8();
    rtn.a = strm.read_u8();
    return lak::ok_t{rtn};
  }

  result_t<lak::color4_t> ColorFrom32bitBGR(lak::memory &strm)
  {
    CHECK_REMAINING(strm, 4);

    lak::color4_t rtn;
    rtn.b = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.r = strm.read_u8();
    rtn.a = 255;
    strm.skip(1);
    return lak::ok_t{rtn};
  }

  result_t<lak::color4_t> ColorFrom24bitRGB(lak::memory &strm)
  {
    CHECK_REMAINING(strm, 3);

    lak::color4_t rtn;
    rtn.r = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.b = strm.read_u8();
    rtn.a = 255;
    return lak::ok_t{rtn};
  }

  template<typename STRM>
  result_t<lak::color4_t> ColorFrom32bitRGBA(STRM &strm)
  {
    CHECK_REMAINING(strm, 4);

    lak::color4_t rtn;
    rtn.r = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.b = strm.read_u8();
    rtn.a = strm.read_u8();
    return lak::ok_t{rtn};
  }

  template<typename STRM>
  result_t<lak::color4_t> ColorFrom32bitRGB(STRM &strm)
  {
    CHECK_REMAINING(strm, 4);

    lak::color4_t rtn;
    rtn.r = strm.read_u8();
    rtn.g = strm.read_u8();
    rtn.b = strm.read_u8();
    rtn.a = 255;
    strm.skip(1);
    return lak::ok_t{rtn};
  }

  result_t<lak::color4_t> ColorFromMode(lak::memory &strm,
                                        const graphics_mode_t mode,
                                        const lak::color4_t palette[256])
  {
    switch (mode)
    {
      case graphics_mode_t::graphics2: [[fallthrough]];
      case graphics_mode_t::graphics3: return ColorFrom8bit(strm, palette);

      case graphics_mode_t::graphics6: return ColorFrom15bit(strm);

      case graphics_mode_t::graphics7: return ColorFrom16bit(strm);

      case graphics_mode_t::graphics8: return ColorFrom32bitBGRA(strm);

      case graphics_mode_t::graphics4: [[fallthrough]];
      default: return ColorFrom24bitBGR(strm);
    }
  }

  uint8_t ColorModeSize(const graphics_mode_t mode)
  {
    switch (mode)
    {
      case graphics_mode_t::graphics2: [[fallthrough]];
      case graphics_mode_t::graphics3: return 1;

      case graphics_mode_t::graphics6: return 2;

      case graphics_mode_t::graphics7: return 2;

      case graphics_mode_t::graphics8: return 4;

      case graphics_mode_t::graphics4: [[fallthrough]];
      default: return 3;
    }
  }

  uint16_t BitmapPaddingSize(uint16_t width,
                             uint8_t col_size,
                             uint8_t bytes = 2)
  {
    uint16_t num = bytes - ((width * col_size) % bytes);
    return (uint16_t)std::ceil((double)(num == bytes ? 0 : num) /
                               (double)col_size);
  }

  result_t<size_t> ReadRLE(lak::memory &strm,
                           lak::image4_t &bitmap,
                           graphics_mode_t mode,
                           const lak::color4_t palette[256] = nullptr)
  {
    const size_t point_size = ColorModeSize(mode);
    const uint16_t pad      = BitmapPaddingSize(bitmap.size().x, point_size);
    size_t pos              = 0;
    size_t i                = 0;

    size_t start = strm.position();

    while (true)
    {
      CHECK_REMAINING(strm, 1);

      uint8_t command = strm.read_u8();

      if (command == 0) break;

      if (command > 128)
      {
        command -= 128;
        for (uint8_t n = 0; n < command; ++n)
        {
          if ((pos++) % (bitmap.size().x + pad) < bitmap.size().x)
          {
            RES_TRY_ASSIGN(bitmap[i++] =, ColorFromMode(strm, mode, palette));
          }
          else
          {
            CHECK_REMAINING(strm, point_size);
            strm.skip(point_size);
          }
        }
      }
      else
      {
        RES_TRY_ASSIGN(lak::color4_t col =,
                       ColorFromMode(strm, mode, palette));
        for (uint8_t n = 0; n < command; ++n)
        {
          if ((pos++) % (bitmap.size().x + pad) < bitmap.size().x)
            bitmap[i++] = col;
        }
      }
    }

    return lak::ok_t{strm.position() - start};
  }

  result_t<size_t> ReadRGB(lak::memory &strm,
                           lak::image4_t &bitmap,
                           graphics_mode_t mode,
                           const lak::color4_t palette[256] = nullptr)
  {
    const size_t point_size = ColorModeSize(mode);
    const uint16_t pad      = BitmapPaddingSize(bitmap.size().x, point_size);
    size_t i                = 0;

    size_t start = strm.position();

    CHECK_REMAINING(strm,
                    point_size * bitmap.size().y * (bitmap.size().x + pad));

    for (size_t y = 0; y < bitmap.size().y; ++y)
    {
      for (size_t x = 0; x < bitmap.size().x; ++x)
      {
        bitmap[i++] = ColorFromMode(strm, mode, palette).UNWRAP();
      }
      strm.skip(pad * point_size);
    }

    return lak::ok_t{strm.position() - start};
  }

  result_t<size_t> ReadAlpha(lak::memory &strm, lak::image4_t &bitmap)
  {
    const uint16_t pad = BitmapPaddingSize(bitmap.size().x, 1, 4);
    size_t i           = 0;

    size_t start = strm.position();

    CHECK_REMAINING(strm, bitmap.size().y * (bitmap.size().x + pad));

    for (size_t y = 0; y < bitmap.size().y; ++y)
    {
      for (size_t x = 0; x < bitmap.size().x; ++x)
      {
        bitmap[i++].a = strm.read_u8();
      }
      strm.skip(pad);
    }

    return lak::ok_t{strm.position() - start};
  }

  void ReadTransparent(const lak::color4_t &transparent, lak::image4_t &bitmap)
  {
    for (size_t i = 0; i < bitmap.contig_size(); ++i)
    {
      bitmap[i].a = lak::color3_t(bitmap[i]) == lak::color3_t(transparent)
                      ? transparent.a
                      : 255;
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
      FATAL("Unknown graphics mode: ", (uintmax_t)mode);
      return std::monostate{};
    }
  }

  void ViewImage(source_explorer_t &srcexp, const float scale)
  {
    // :TODO: Select palette
    if (std::holds_alternative<lak::opengl::texture>(srcexp.image))
    {
      const auto &img = std::get<lak::opengl::texture>(srcexp.image);
      if (!img.get() || srcexp.graphics_mode != lak::graphics_mode::OpenGL)
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
      if (!img.pixels || srcexp.graphics_mode != lak::graphics_mode::Software)
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

  const char *GetTypeString(chunk_t ID)
  {
    switch (ID)
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
      case chunk_t::chunk224F: return "Chunk 224F";
      case chunk_t::title2: return "Title2";

      case chunk_t::chunk2253: return "Chunk 2253";
      case chunk_t::object_names: return "Object Names";
      case chunk_t::chunk2255: return "Chunk 2255 (Empty?)";
      case chunk_t::recompiled_object_properties: return "Object Properties 2";
      case chunk_t::chunk2257: return "Chunk 2257 (4 bytes?)";
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

      case chunk_t::_default: [[fallthrough]];
      case chunk_t::vitalise: [[fallthrough]];
      case chunk_t::unicode: [[fallthrough]];
      case chunk_t::_new: [[fallthrough]];
      case chunk_t::old: [[fallthrough]];
      case chunk_t::frame_state: [[fallthrough]];
      case chunk_t::image_state: [[fallthrough]];
      case chunk_t::font_state: [[fallthrough]];
      case chunk_t::sound_state: [[fallthrough]];
      case chunk_t::music_state: [[fallthrough]];
      case chunk_t::no_child: [[fallthrough]];
      case chunk_t::skip: [[fallthrough]];
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

  result_t<std::vector<uint8_t>> Decode(const std::vector<uint8_t> &encoded,
                                        chunk_t id,
                                        encoding_t mode)
  {
    SCOPED_CHECKPOINT("Decode");

    switch (mode)
    {
      case encoding_t::mode3:
      case encoding_t::mode2: return Decrypt(encoded, id, mode);
      case encoding_t::mode1: return lak::ok_t{InflateOrCompressed(encoded)};
      default:
        if (encoded.size() > 0 && encoded[0] == 0x78)
          return lak::ok_t{InflateOrCompressed(encoded)};
        else
          return lak::ok_t{encoded};
    }
  }

  result_t<std::vector<uint8_t>> Inflate(
    const std::vector<uint8_t> &compressed,
    bool skip_header,
    bool anaconda,
    size_t max_size)
  {
    SCOPED_CHECKPOINT("Inflate");

    lak::array<uint8_t, 0x8000> buffer;
    auto inflater =
      lak::deflate_iterator(compressed, buffer, !skip_header, anaconda);

    std::vector<uint8_t> output;
    if (auto err = inflater.read([&](uint8_t v) {
          if (output.size() > max_size) return false;
          output.push_back(v);
          return true;
        });
        err.is_ok())
    {
      return lak::ok_t{lak::move(output)};
    }
    else if (err.unsafe_unwrap_err() == lak::deflate_iterator::error_t::ok)
    {
      // This is not (always) an error, we may intentionally stop the decode
      // early to not waste time and memory.

      return lak::ok_t{lak::move(output)};
    }
    else
    {
      DEBUG("Buffer Size: ", buffer.size());
      DEBUG("Max Size: ", max_size);
      DEBUG("Final? ", (inflater.is_final_block() ? "True" : "False"));
      return lak::err_t{
        error(LINE_TRACE,
              error::inflate_failed,
              "Failed To Inflate (",
              lak::deflate_iterator::error_name(err.unsafe_unwrap_err()),
              ")")};
    }
  }

  error_t Inflate(std::vector<uint8_t> &out,
                  const std::vector<uint8_t> &compressed,
                  bool skip_header,
                  bool anaconda,
                  size_t max_size)
  {
    SCOPED_CHECKPOINT("Inflate");

    RES_TRY_ASSIGN(out =,
                   Inflate(compressed, skip_header, anaconda, max_size)
                     .map_err(APPEND_TRACE("Inflate")));
    return lak::ok_t{};
  }

  error_t Inflate(lak::memory &out,
                  const std::vector<uint8_t> &compressed,
                  bool skip_header,
                  bool anaconda,
                  size_t max_size)
  {
    SCOPED_CHECKPOINT("Inflate");

    RES_TRY_ASSIGN(out =,
                   Inflate(compressed, skip_header, anaconda, max_size)
                     .map_err(APPEND_TRACE("Inflate")));
    return lak::ok_t{};
  }

  std::vector<uint8_t> InflateOrCompressed(
    const std::vector<uint8_t> &compressed)
  {
    SCOPED_CHECKPOINT("InflateOrCompressed");

    if (auto result = Inflate(compressed, false, false); result.is_ok())
      return lak::move(result).unsafe_unwrap();
    ERROR("Failed To Inflate");
    return compressed;
  }

  std::vector<uint8_t> DecompressOrCompressed(
    const std::vector<uint8_t> &compressed, unsigned int out_size)
  {
    SCOPED_CHECKPOINT("DecompressOrCompressed");

    if (auto result = Inflate(compressed, true, true); result.is_ok())
      return lak::move(result).unsafe_unwrap();
    ERROR("Failed To Decompress");
    return compressed;
  }

  result_t<std::vector<uint8_t>> LZ4Decode(lak::span_memory strm,
                                           unsigned int out_size)
  {
    SCOPED_CHECKPOINT("LZ4Decode");

    if (auto result = lak::decode_lz4_block(strm, out_size); result.is_ok())
      return lak::ok_t{lak::move(result).unsafe_unwrap().release()};

    ERROR("Failed To Decompress");
    return lak::err_t{
      error(LINE_TRACE, error::inflate_failed, "Failed To Decompress")};
  }

  result_t<std::vector<uint8_t>> LZ4DecodeReadSize(lak::span_memory strm)
  {
    CHECK_REMAINING(strm, 4);
    // intentionally doing this outside of the LZ4Decode call incase strm
    // copies first (?)
    const uint32_t out_size = strm.read_u32();
    return LZ4Decode(strm, out_size);
  }

  result_t<std::vector<uint8_t>> StreamDecompress(lak::memory &strm,
                                                  unsigned int out_size)
  {
    SCOPED_CHECKPOINT("StreamDecompress");

    lak::array<uint8_t, 0x8000> buffer;
    auto inflater = lak::deflate_iterator(lak::span(strm.cursor(), strm.end()),
                                          buffer,
                                          /* parse_header */ false,
                                          /* anaconda */ true);

    std::vector<uint8_t> output;
    if (auto err = inflater.read([&](uint8_t v) {
          output.push_back(v);
          return true;
        });
        err.is_ok())
    {
      if (inflater.compressed().begin() > strm.cursor())
        strm.skip(inflater.compressed().begin() - strm.cursor());
      return lak::ok_t{lak::move(output)};
    }
    else
    {
      DEBUG("Final? ", (inflater.is_final_block() ? "True" : "False"));
      return lak::err_t{
        error(LINE_TRACE,
              error::inflate_failed,
              "Failed To Inflate (",
              lak::deflate_iterator::error_name(err.unsafe_unwrap_err()),
              ")")};
    }
  }

  result_t<std::vector<uint8_t>> Decrypt(const std::vector<uint8_t> &encrypted,
                                         chunk_t ID,
                                         encoding_t mode)
  {
    SCOPED_CHECKPOINT("Decrypt");

    if (mode == encoding_t::mode3)
    {
      if (encrypted.size() <= 4)
        return lak::err_t{
          error(LINE_TRACE,
                error::decrypt_failed,
                "MODE 3 Decryption Failed: Encrypted Buffer Too Small")};
      // TODO: check endian
      // size_t dataLen = *reinterpret_cast<const uint32_t*>(&encrypted[0]);
      std::vector<uint8_t> mem(encrypted.begin() + 4, encrypted.end());

      if ((_mode != game_mode_t::_284) && ((uint16_t)ID & 0x1) != 0)
        mem[0] ^= ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

      if (DecodeChunk(mem))
      {
        if (mem.size() <= 4)
          return lak::err_t{
            error(LINE_TRACE,
                  error::decrypt_failed,
                  "MODE 3 Decryption Failed: Decoded Chunk Too Small")};
        // dataLen = *reinterpret_cast<uint32_t*>(&mem[0]);

        // Try Inflate even if it doesn't need to be
        return lak::ok_t{InflateOrCompressed(
          std::vector<uint8_t>(mem.begin() + 4, mem.end()))};
      }
      return lak::err_t{
        error(LINE_TRACE, error::decrypt_failed, "MODE 3 Decryption Failed")};
    }
    else
    {
      if (encrypted.size() < 1)
        return lak::err_t{
          error(LINE_TRACE,
                error::decrypt_failed,
                "MODE 2 Decryption Failed: Encrypted Buffer Too Small")};

      std::vector<uint8_t> mem = encrypted;

      if ((_mode != game_mode_t::_284) && (uint16_t)ID & 0x1)
        mem[0] ^= ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

      if (!DecodeChunk(mem))
      {
        if (mode == encoding_t::mode2)
        {
          return lak::err_t{error(
            LINE_TRACE, error::decrypt_failed, "MODE 2 Decryption Failed")};
        }
      }
      return lak::ok_t{lak::move(mem)};
    }
  }

  result_t<frame::item_t &> GetFrame(game_t &game, uint16_t handle)
  {
    if (!game.game.frame_bank)
      return lak::err_t{error(LINE_TRACE, "No Frame Bank")};

    if (!game.game.frame_handles)
      return lak::err_t{error(LINE_TRACE, "No Frame Handles")};

    if (handle >= game.game.frame_handles->handles.size())
      return lak::err_t{error(LINE_TRACE, "Frame Handle Out Of Range")};

    handle = game.game.frame_handles->handles[handle];

    if (handle >= game.game.frame_bank->items.size())
      lak::err_t{error(LINE_TRACE, "Frame Bank Handle Out Of Range")};

    return lak::ok_t{game.game.frame_bank->items[handle]};
  }

  result_t<object::item_t &> GetObject(game_t &game, uint16_t handle)
  {
    if (!game.game.object_bank)
      return lak::err_t{error(LINE_TRACE, "No Object Bank")};

    auto iter = game.object_handles.find(handle);

    if (iter == game.object_handles.end())
      return lak::err_t{error(LINE_TRACE, "Invalid Object Handle")};

    if (iter->second >= game.game.object_bank->items.size())
      return lak::err_t{error(LINE_TRACE, "Object Bank Handle Out Of Range")};

    return lak::ok_t{game.game.object_bank->items[iter->second]};
  }

  result_t<image::item_t &> GetImage(game_t &game, uint32_t handle)
  {
    if (!game.game.image_bank)
      return lak::err_t{error(LINE_TRACE, "No Image Bank")};

    auto iter = game.image_handles.find(handle);

    if (iter == game.image_handles.end())
      return lak::err_t{error(LINE_TRACE, "Invalid Image Handle")};

    if (iter->second >= game.game.image_bank->items.size())
      return lak::err_t{error(LINE_TRACE, "Image Bank Handle Out Of Range")};

    return lak::ok_t{game.game.image_bank->items[iter->second]};
  }

  result_t<lak::memory> data_point_t::decode(const chunk_t ID,
                                             const encoding_t mode) const
  {
    return Decode(data.get(), ID, mode).map([](std::vector<uint8_t> &&array) {
      return lak::memory(lak::move(array));
    });
  }

  error_t chunk_entry_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("chunk_entry_t::read");

    CHECK_REMAINING(strm, 8);

    position = strm.position();
    old      = game.old_game;
    ID       = (chunk_t)strm.read_u16();
    mode     = (encoding_t)strm.read_u16();

    DEBUG("Type: ", GetTypeString(ID), " (", uintmax_t(ID), ")");
    if (GetTypeString(ID) == lak::astring("INVALID"))
      WARNING("Invalid Type Detected");
    DEBUG("Mode: ", (uint16_t)mode);
    DEBUG("Position: ", position);

    if ((mode == encoding_t::mode2 || mode == encoding_t::mode3) &&
        _magic_key.size() < 256)
      GetEncryptionKey(game);

    const auto chunk_size     = strm.read_u32();
    const auto chunk_data_end = strm.position() + chunk_size;

    CHECK_REMAINING(strm, chunk_size);

    if (mode == encoding_t::mode1)
    {
      data.expected_size = strm.read_u32();

      if (game.old_game)
      {
        data.position = strm.position();

        if (chunk_size > 4)
          data.data = strm.read(chunk_size - 4);
        else
          data.data.clear();
      }
      else
      {
        const auto data_size = strm.read_u32();
        CHECK_REMAINING(strm, data_size);

        data.position = strm.position();
        data.data     = strm.read(data_size);

        if (strm.position() > chunk_data_end)
        {
          ERROR("Read Too Much Data");
        }
        else if (strm.position() < chunk_data_end)
        {
          ERROR("Read Too Little Data");
        }

        strm.seek(chunk_data_end);
      }
    }
    else
    {
      data.expected_size = 0;
      data.position      = strm.position();
      data.data          = strm.read(chunk_size);
    }
    end = strm.position();
    DEBUG("Size: ", end - position);

    return lak::ok_t{};
  }

  void chunk_entry_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("Entry Information##%zX", position)
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
      ImGui::Text("Header Expected Size: 0x%zX", header.expected_size);
      ImGui::Text("Header Size: 0x%zX", header.data.size());

      ImGui::Text("Data Position: 0x%zX", data.position);
      ImGui::Text("Data Expected Size: 0x%zX", data.expected_size);
      ImGui::Text("Data Size: 0x%zX", data.data.size());
    }

    if (ImGui::Button("View Memory")) srcexp.view = this;
  }

  error_t item_entry_t::read(game_t &game,
                             lak::memory &strm,
                             bool compressed,
                             size_t header_size)
  {
    SCOPED_CHECKPOINT("item_entry_t::read");

    if (!strm.remaining())
      return lak::err_t{error(LINE_TRACE, error::out_of_data)};

    position = strm.position();
    old      = game.old_game;
    mode     = encoding_t::mode0;
    handle   = strm.read_u32();

    const uint32_t peekaboo = strm.peek_u32();
    const bool new_item     = peekaboo == 0xFF'FF'FF'FF;
    if (new_item)
    {
      mode       = encoding_t::mode4;
      compressed = false;
    }

    if (!game.old_game && header_size > 0)
    {
      header.position      = strm.position();
      header.expected_size = 0;
      header.data          = strm.read(header_size);
    }

    data.expected_size = game.old_game || compressed ? strm.read_u32() : 0;

    size_t data_size = 0;
    if (game.old_game)
    {
      const size_t start = strm.position();
      // Figure out exactly how long the compressed data is
      // :TODO: It should be possible to figure this out without
      // actually decompressing it... surely...
      RES_TRY_ASSIGN(const auto raw =,
                     StreamDecompress(strm, data.expected_size)
                       .map_err(APPEND_TRACE("item_entry_t::read")));
      if (raw.size() != data.expected_size)
      {
        WARNING("Actual decompressed size (",
                raw.size(),
                ") was not equal to the expected size (",
                data.expected_size,
                ").");
      }
      data_size = strm.position() - start;
      strm.seek(start);
    }
    else if (!new_item)
    {
      data_size = strm.read_u32();
    }

    data.position = strm.position();
    CHECK_REMAINING(strm, data_size);
    data.data = strm.read(data_size);

    // hack because one of MMF1.5 or tinf_uncompress is a bitch
    if (game.old_game) mode = encoding_t::mode1;

    end = strm.position();

    return lak::ok_t{};
  }

  void item_entry_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("Entry Information##%zX", position)
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
      ImGui::Text("Header Expected Size: 0x%zX", header.expected_size);
      ImGui::Text("Header Size: 0x%zX", header.data.size());

      ImGui::Text("Data Position: 0x%zX", data.position);
      ImGui::Text("Data Expected Size: 0x%zX", data.expected_size);
      ImGui::Text("Data Size: 0x%zX", data.data.size());
    }

    if (ImGui::Button("View Memory")) srcexp.view = this;
  }

  result_t<lak::memory> basic_entry_t::decode(size_t max_size) const
  {
    SCOPED_CHECKPOINT("basic_entry_t::decode");

    auto array_to_memory = [](std::vector<uint8_t> &&array) {
      return lak::memory(lak::move(array));
    };

    if (old)
    {
      switch (mode)
      {
        case encoding_t::mode0: return lak::ok_t{lak::memory(data.data)};

        case encoding_t::mode1:
        {
          lak::memory result = data.data;
          uint8_t magic      = result.read_u8();
          uint16_t len       = result.read_u16();
          if (magic == 0x0F && (size_t)len == data.expected_size)
          {
            result =
              lak::memory(result.read(std::min(result.remaining(), max_size)));
            DEBUG("Size: ", result.size());
            return lak::ok_t{lak::move(result)};
          }
          else
          {
            result.unread(3);
            return Inflate(result,
                           result.get(),
                           true,
                           true,
                           std::min(data.expected_size, max_size))
              .map([&](const auto &) { return lak::move(result); })
              .map_err(APPEND_TRACE("MODE1 Failed To Inflate"))
              .if_ok([](const lak::memory &memory) {
                DEBUG("Size: ", memory.size());
              });
          }
        }

        case encoding_t::mode2:
          return lak::err_t{error(LINE_TRACE, error::no_mode2_decoder)};

        case encoding_t::mode3:
          return lak::err_t{error(LINE_TRACE, error::no_mode3_decoder)};

        default:
          ASSERT_NYI();
          return lak::err_t{error(LINE_TRACE, "Invalid Mode")};
      }
    }
    else
    {
      switch (mode)
      {
        case encoding_t::mode4:
          return LZ4DecodeReadSize(
                   lak::span_memory(lak::span<uint8_t>(
                     (uint8_t *)data.data.data(), data.data.size())))
            .map(array_to_memory)
            .map_err(APPEND_TRACE("LZ4 Decode Failed"))
            .if_ok([](const lak::memory &memory) {
              DEBUG("Size: ", memory.size())
            });

        case encoding_t::mode3: [[fallthrough]];
        case encoding_t::mode2:
          return Decrypt(data.data.get(), ID, mode)
            .map(array_to_memory)
            .map_err(APPEND_TRACE("MODE2/3 Failed To Decrypt"))
            .if_ok([](const lak::memory &memory) {
              DEBUG("Size: ", memory.size());
            });

        case encoding_t::mode1:
          return Inflate(data.data.get(), false, false, max_size)
            .map(array_to_memory)
            .map_err(APPEND_TRACE("MODE1 Failed To Inflate"))
            .if_ok([](const lak::memory &memory) {
              DEBUG("Size: ", memory.size());
            });

        case encoding_t::mode0: [[fallthrough]];
        default:
          if (data.data.size() > 0 && data.data.get()[0] == 0x78)
          {
            if (auto result = Inflate(data.data.get(), false, false, max_size)
                                .map(array_to_memory)
                                .if_ok([](const lak::memory &memory) {
                                  DEBUG("Size: ", memory.size());
                                });
                result.is_ok())
            {
              if (result.unwrap().size() == 0)
                WARNING("Inflated Data Was Empty");
              return result;
            }
            else
            {
              DEBUG("Size: ", data.data.size());
              WARNING("Guessed MODE1 Failed To Inflate: ",
                      result.unsafe_unwrap_err());
              return lak::ok_t{data.data};
            }
          }
          else
          {
            DEBUG("Size: ", data.data.size());
            return lak::ok_t{data.data};
          }
      }
    }
  }

  result_t<lak::memory> basic_entry_t::decodeHeader(size_t max_size) const
  {
    auto array_to_memory = [](std::vector<uint8_t> &&array) {
      return lak::memory(lak::move(array));
    };

    if (old)
    {
      switch (mode)
      {
        case encoding_t::mode0:
          return lak::err_t{error(LINE_TRACE, error::no_mode0_decoder)};
        case encoding_t::mode1:
          return lak::err_t{error(LINE_TRACE, error::no_mode1_decoder)};
        case encoding_t::mode2:
          return lak::err_t{error(LINE_TRACE, error::no_mode2_decoder)};
        case encoding_t::mode3:
          return lak::err_t{error(LINE_TRACE, error::no_mode3_decoder)};
        default:
          ASSERT_NYI();
          return lak::err_t{error(LINE_TRACE, "Invalid Mode")};
      }
    }
    else
    {
      switch (mode)
      {
        case encoding_t::mode3:
        case encoding_t::mode2:
          return Decrypt(data.data.get(), ID, mode)
            .map(array_to_memory)
            .map_err(APPEND_TRACE("MODE2/3 Failed To Decrypt"));

        case encoding_t::mode1:
          return Inflate(header.data.get(), false, false, max_size)
            .map(array_to_memory)
            .map_err(APPEND_TRACE("MODE1 Failed To Inflate"));

        case encoding_t::mode4: [[fallthrough]];
        case encoding_t::mode0: [[fallthrough]];
        default:
          if (header.data.size() > 0 && header.data.get()[0] == 0x78)
          {
            if (auto result =
                  Inflate(header.data.get(), false, false, max_size)
                    .map(array_to_memory);
                result.is_ok())
            {
              if (result.unwrap().size() == 0)
                WARNING("Inflated Data Was Empty");
              return result;
            }
            else
            {
              WARNING("Guessed MODE1 Failed To Inflate");
              return lak::ok_t{header.data};
            }
          }
          else
            return lak::ok_t{header.data};
      }
    }
  }

  const lak::memory &basic_entry_t::raw() const { return data.data; }

  const lak::memory &basic_entry_t::rawHeader() const { return header.data; }

  std::u16string ReadStringEntry(game_t &game, const chunk_entry_t &entry)
  {
    std::u16string result;

    if (game.old_game)
    {
      switch (entry.mode)
      {
        case encoding_t::mode0:
        case encoding_t::mode1:
        {
          if (auto decoded = entry.decode(); decoded.is_ok())
            result = lak::to_u16string(decoded.unsafe_unwrap().read_astring());
          else
            decoded
              .IF_ERR("Invalid String Chunk: ",
                      (int)entry.mode,
                      ", Chunk: ",
                      (int)entry.ID)
              .discard();
        }
        break;
        default:
        {
          ERROR("Invalid String Mode: ",
                (int)entry.mode,
                ", Chunk: ",
                (int)entry.ID);
        }
        break;
      }
    }
    else
    {
      if (auto decoded = entry.decode(); decoded.is_ok())
      {
        if (game.unicode)
          result = decoded.unsafe_unwrap().read_u16string().to_string();
        else
          result = lak::to_u16string(decoded.unsafe_unwrap().read_astring());
      }
      else
        decoded
          .IF_ERR("Invalid String Chunk: ",
                  (int)entry.mode,
                  ", Chunk: ",
                  (int)entry.ID)
          .discard();
    }

    return result;
  }

  error_t basic_chunk_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("basic_chunk_t::read");
    error_t result = entry.read(game, strm);
    return result;
  }

  error_t basic_chunk_t::basic_view(source_explorer_t &srcexp,
                                    const char *name) const
  {
    LAK_TREE_NODE("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position)
    {
      entry.view(srcexp);
    }

    return lak::ok_t{};
  }

  error_t basic_item_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("basic_item_t::read");
    error_t result = entry.read(game, strm, true);
    return result;
  }

  error_t basic_item_t::basic_view(source_explorer_t &srcexp,
                                   const char *name) const
  {
    LAK_TREE_NODE("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position)
    {
      entry.view(srcexp);
    }

    return lak::ok_t{};
  }

  error_t string_chunk_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("string_chunk_t::read");

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("string_chunk_t::read")));

    value = ReadStringEntry(game, entry);

    DEBUG(LAK_YELLOW "Value: \"", value, "\"" LAK_SGR_RESET);

    return lak::ok_t{};
  }

  error_t string_chunk_t::view(source_explorer_t &srcexp,
                               const char *name,
                               const bool preview) const
  {
    lak::astring str = astring();

    LAK_TREE_NODE("0x%zX %s '%s'##%zX",
                  (size_t)entry.ID,
                  name,
                  preview ? str.c_str() : "",
                  entry.position)
    {
      entry.view(srcexp);
      ImGui::Text("String: '%s'", str.c_str());
      ImGui::Text("String Length: 0x%zX", value.size());
    }

    return lak::ok_t{};
  }

  lak::u16string string_chunk_t::u16string() const { return value; }

  lak::u8string string_chunk_t::u8string() const
  {
    return lak::strconv<char8_t>(value);
  }

  lak::astring string_chunk_t::astring() const
  {
    return lak::strconv<char>(value);
  }

  error_t strings_chunk_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("strings_chunk_t::read");

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("string_chunk_t::read")));

    RES_TRY_ASSIGN(
      auto sstrm =,
      entry.decode().map_err(APPEND_TRACE("string_chunk_t::read")));

    while (sstrm.remaining())
    {
      auto str = sstrm.read_u16string(sstrm.remaining());
      DEBUG("    Read String (Len ", str.size(), ")");
      if (!str.size()) break;
      values.emplace_back(str.to_string());
    }

    return lak::ok_t{};
  }

  error_t strings_chunk_t::basic_view(source_explorer_t &srcexp,
                                      const char *name) const
  {
    LAK_TREE_NODE("0x%zX %s (%zu Items)##%zX",
                  (size_t)entry.ID,
                  name,
                  values.size(),
                  entry.position)
    {
      entry.view(srcexp);
      for (const auto &s : values)
        ImGui::Text("%s", (const char *)lak::to_u8string(s).c_str());
    }

    return lak::ok_t{};
  }

  error_t strings_chunk_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Unknown Strings");
  }

  error_t compressed_chunk_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE(
      "0x%zX Unknown Compressed##%zX", (size_t)entry.ID, entry.position)
    {
      entry.view(srcexp);

      if (ImGui::Button("View Compressed"))
      {
        RES_TRY_ASSIGN(
          auto strm =,
          entry.decode().map_err(APPEND_TRACE("compressed_chunk_t::view")));
        strm.skip(8);
        lak::array<uint8_t> buffer;
        tinf::error_t err =
          tinf::tinflate(strm.read(strm.remaining()), &buffer);
        if (err == tinf::error_t::OK)
          srcexp.buffer = std::vector(buffer.begin(), buffer.end());
        else
          ERROR("Failed To Decompress Chunk (", tinf::error_name(err), ")");
      }
    }

    return lak::ok_t{};
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
    SCOPED_CHECKPOINT("icon_t::read");

    RES_TRY_ERR(entry.read(game, strm).map_err(APPEND_TRACE("icon_t::read")));

    RES_TRY_ASSIGN(auto dstrm =,
                   entry.decode().map_err(APPEND_TRACE("icon_t::read")));
    dstrm.seek(dstrm.peek_u32());

    std::vector<lak::color4_t> palette;
    palette.resize(16 * 16);

    for (auto &point : palette)
    {
      point.b = dstrm.read_u8();
      point.g = dstrm.read_u8();
      point.r = dstrm.read_u8();
      // point.a = dstrm.read_u8();
      point.a = 0xFF;
      dstrm.skip(1);
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

    return lak::ok_t{};
  }

  error_t icon_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX Icon##%zX", (size_t)entry.ID, entry.position)
    {
      entry.view(srcexp);

      ImGui::Text("Image Size: %zu * %zu",
                  (size_t)bitmap.size().x,
                  (size_t)bitmap.size().y);

      if (ImGui::Button("View Image"))
      {
        srcexp.image = CreateTexture(bitmap, srcexp.graphics_mode);
      }
    }

    return lak::ok_t{};
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
    SCOPED_CHECKPOINT("binary_file_t::read");

    if (game.unicode)
      name = strm.read_u16string_exact(strm.read_u16()).to_string();
    else
      name = lak::to_u16string(strm.read_astring_exact(strm.read_u16()));

    data = strm.read(strm.read_u32());

    return lak::ok_t{};
  }

  error_t binary_file_t::view(source_explorer_t &srcexp) const
  {
    lak::astring str = lak::to_astring(name);

    LAK_TREE_NODE("%s", str.c_str())
    {
      ImGui::Text("Name: %s", str.c_str());
      ImGui::Text("Data Size: 0x%zX", data.size());
    }

    return lak::ok_t{};
  }

  error_t binary_files_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("binary_files_t::read");

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("binary_files_t::read")));

    RES_TRY_ASSIGN(
      lak::memory bstrm =,
      entry.decode().map_err(APPEND_TRACE("binary_files_t::read")));
    items.resize(bstrm.read_u32());
    for (auto &item : items)
      RES_TRY_ERR(
        item.read(game, bstrm).map_err(APPEND_TRACE("binary_files_t::read")));

    return lak::ok_t{};
  }

  error_t binary_files_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX Binary Files##%zX", (size_t)entry.ID, entry.position)
    {
      entry.view(srcexp);

      int index = 0;
      for (const auto &item : items)
      {
        ImGui::PushID(index++);
        DEFER(ImGui::PopID());
        RES_TRY_ERR(
          item.view(srcexp).map_err(APPEND_TRACE("binary_files_t::view")));
      }
    }

    return lak::ok_t{};
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
    SCOPED_CHECKPOINT("extended_header_t::read");

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("extended_header_t::read")));

    RES_TRY_ASSIGN(
      lak::memory estrm =,
      entry.decode().map_err(APPEND_TRACE("extended_header_t::read")));
    flags                  = estrm.read_u32();
    build_type             = estrm.read_u32();
    build_flags            = estrm.read_u32();
    screen_ratio_tolerance = estrm.read_u16();
    screen_angle           = estrm.read_u16();

    game.compat |= build_type >= 0x10000000;

    return lak::ok_t{};
  }

  error_t extended_header_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE(
      "0x%zX Extended Header##%zX", (size_t)entry.ID, entry.position)
    {
      entry.view(srcexp);

      ImGui::Text("Flags: 0x%zX", (size_t)flags);
      ImGui::Text("Build Type: 0x%zX", (size_t)build_type);
      ImGui::Text("Build Flags: 0x%zX", (size_t)build_flags);
      ImGui::Text("Screen Ratio Tolerance: 0x%zX",
                  (size_t)screen_ratio_tolerance);
      ImGui::Text("Screen Angle: 0x%zX", (size_t)screen_angle);
    }

    return lak::ok_t{};
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

  error_t chunk_2253_item_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("chunk2253_t::read");

    CHECK_REMAINING(strm, 16);

    position = strm.position();
    DEFER(strm.seek(position + 16));

    ID = strm.read_u16();

    return lak::ok_t{};
  }

  error_t chunk_2253_item_t::view(source_explorer_t &srcexp) const
  {
    ImGui::Text("ID: 0x%zX", ID);

    return lak::ok_t{};
  }

  error_t chunk_2253_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("chunk2253_t::read");

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("chunk_2253_t::read")));

    RES_TRY_ASSIGN(lak::memory cstrm =,
                   entry.decode().map_err(APPEND_TRACE("chunk_2253_t::read")));

    while (cstrm.remaining() >= 16)
    {
      const size_t pos = cstrm.position();
      items.emplace_back();
      RES_TRY_ERR(items.back()
                    .read(game, cstrm)
                    .map_err(APPEND_TRACE("chunk_2253_t::read")));
      if (cstrm.position() == pos) break;
    }

    if (cstrm.remaining()) WARNING("Data Left Over");

    return lak::ok_t{};
  }

  error_t chunk_2253_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX Chunk 2253 (%zu Items)##%zX",
                  (size_t)entry.ID,
                  items.size(),
                  entry.position)
    {
      entry.view(srcexp);

      size_t i = 0;
      for (const auto &item : items)
      {
        LAK_TREE_NODE("0x%zX Item##%zX", (size_t)item.ID, i++)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("chunk_2253_t::view")));
        }
      }
    }

    return lak::ok_t{};
  }

  error_t object_names_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Object Names");
  }

  error_t chunk_2255_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Chunk 2255");
  }

  error_t recompiled_object_properties_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("recompiled_object_properties_t::read");

    RES_TRY_ERR(
      entry.read(game, strm)
        .map_err(APPEND_TRACE("recompiled_object_properties_t::read")));

    const auto end = strm.position();
    DEFER(strm.seek(end));

    strm.seek(entry.data.position);

    while (strm.position() < end)
    {
      items.emplace_back();
      RES_TRY_ERR(
        items.back()
          .read(game, strm)
          .map_err(APPEND_TRACE("recompiled_object_properties_t::read")));
    }

    return lak::ok_t{};
  }

  error_t recompiled_object_properties_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX Object Properties 2 (%zu Items)##%zX",
                  (size_t)entry.ID,
                  items.size(),
                  entry.position)
    {
      entry.view(srcexp);

      for (const auto &item : items)
      {
        RES_TRY(
          item.basic_view(srcexp, "Properties")
            .map_err(APPEND_TRACE("recompiled_object_properties_t::view")));
      }
    }

    return lak::ok_t{};
  }

  error_t chunk_2257_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Chunk 2257");
  }

  error_t object_properties_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("object_properties_t::read");

    RES_TRY_ERR(entry.read(game, strm)
                  .map_err(APPEND_TRACE("object_properties_t::read")));

    const auto end = strm.position();
    DEFER(strm.seek(end));

    strm.seek(entry.data.position);

    while (strm.position() < end)
    {
      items.emplace_back();
      RES_TRY_ERR(items.back()
                    .read(game, strm, false)
                    .map_err(APPEND_TRACE("object_properties_t::read")));
    }

    return lak::ok_t{};
  }

  error_t object_properties_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX Object Properties (%zu Items)##%zX",
                  (size_t)entry.ID,
                  items.size(),
                  entry.position)
    {
      entry.view(srcexp);

      for (const auto &item : items)
      {
        LAK_TREE_NODE("0x%zX Properties##%zX", (size_t)item.ID, item.position)
        {
          item.view(srcexp);
        }
      }
    }

    return lak::ok_t{};
  }

  error_t truetype_fonts_meta_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "TrueType Fonts Meta");
  }

  error_t truetype_fonts_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("truetype_fonts_t::read");

    const size_t pos = strm.position();

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("truetype_fonts_t::read")));

    const auto end = strm.position();
    DEFER(strm.seek(end));
    strm.seek(entry.data.position);

    for (size_t i = 0; strm.position() < end; ++i)
    {
      DEBUG("Font ", i);
      items.emplace_back();
      RES_TRY_ERR(items.back()
                    .read(game, strm, false)
                    .map_err(APPEND_TRACE("truetype_fonts_t::read")));
    }

    return lak::ok_t{};
  }

  error_t truetype_fonts_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX TrueType Fonts (%zu Items)##%zX",
                  (size_t)entry.ID,
                  items.size(),
                  entry.position)
    {
      entry.view(srcexp);

      for (const auto &item : items)
      {
        LAK_TREE_NODE("0x%zX Font##%zX", (size_t)item.ID, item.position)
        {
          item.view(srcexp);
        }
      }
    }

    return lak::ok_t{};
  }

  namespace object
  {
    error_t effect_t::view(source_explorer_t &srcexp) const
    {
      return basic_view(srcexp, "Effect");
    }

    error_t shape_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::shape_t::read");

      border_size    = strm.read_u16();
      border_color.r = strm.read_u8();
      border_color.g = strm.read_u8();
      border_color.b = strm.read_u8();
      border_color.a = strm.read_u8();
      shape          = (shape_type_t)strm.read_u16();
      fill           = (fill_type_t)strm.read_u16();

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

      return lak::ok_t{};
    }

    error_t shape_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("Shape")
      {
        ImGui::Text("Border Size: 0x%zX", (size_t)border_size);
        lak::vec4f_t col = ((lak::vec4f_t)border_color) / 255.0f;
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
      }

      return lak::ok_t{};
    }

    error_t quick_backdrop_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::quick_backdrop_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("object::quick_backdrop_t::read")));

      RES_TRY_ASSIGN(lak::memory qstrm =,
                     entry.decode().map_err(
                       APPEND_TRACE("object::quick_backdrop_t::read")));
      size        = qstrm.read_u32();
      obstacle    = qstrm.read_u16();
      collision   = qstrm.read_u16();
      dimension.x = game.old_game ? qstrm.read_u16() : qstrm.read_u32();
      dimension.y = game.old_game ? qstrm.read_u16() : qstrm.read_u32();
      RES_TRY_ERR(shape.read(game, qstrm)
                    .map_err(APPEND_TRACE("object::quick_backdrop_t::read")));

      return lak::ok_t{};
    }

    error_t quick_backdrop_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Properties (Quick Backdrop)##%zX",
                    (size_t)entry.ID,
                    entry.position)
      {
        entry.view(srcexp);

        ImGui::Text("Size: 0x%zX", (size_t)size);
        ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
        ImGui::Text("Collision: 0x%zX", (size_t)collision);
        ImGui::Text(
          "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);

        RES_TRY_ERR(shape.view(srcexp).map_err(
          APPEND_TRACE("object::quick_backdrop_t::view")));

        ImGui::Text("Handle: 0x%zX", (size_t)shape.handle);
        if (shape.handle < 0xFFFF)
        {
          RES_TRY_ASSIGN(auto img =,
                         GetImage(srcexp.state, shape.handle)
                           .map_err(APPEND_TRACE(
                             "object::quick_backdrop_t::view: bad image")));
          RES_TRY(img.view(srcexp).map_err(
            APPEND_TRACE("object::quick_backdrop_t::view")));
        }
      }

      return lak::ok_t{};
    }

    error_t backdrop_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::backdrop_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("object::backdrop_t::read")));

      RES_TRY_ASSIGN(
        lak::memory bstrm =,
        entry.decode().map_err(APPEND_TRACE("object::backdrop_t::read")));
      size        = bstrm.read_u32();
      obstacle    = bstrm.read_u16();
      collision   = bstrm.read_u16();
      dimension.x = game.old_game ? bstrm.read_u16() : bstrm.read_u32();
      dimension.y = game.old_game ? bstrm.read_u16() : bstrm.read_u32();
      // if (!game.old_game) bstrm.skip(2);
      handle = bstrm.read_u16();

      return lak::ok_t{};
    }

    error_t backdrop_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE(
        "0x%zX Properties (Backdrop)##%zX", (size_t)entry.ID, entry.position)
      {
        entry.view(srcexp);

        ImGui::Text("Size: 0x%zX", (size_t)size);
        ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
        ImGui::Text("Collision: 0x%zX", (size_t)collision);
        ImGui::Text(
          "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);
        ImGui::Text("Handle: 0x%zX", (size_t)handle);
        if (handle < 0xFFFF)
        {
          RES_TRY_ASSIGN(
            auto img =,
            GetImage(srcexp.state, handle)
              .map_err(APPEND_TRACE("object::backdrop_t::view: bad image")));
          RES_TRY(img.view(srcexp).map_err(
            APPEND_TRACE("object::backdrop_t::view")));
        }
      }

      return lak::ok_t{};
    }

    error_t animation_direction_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::animation_direction_t::read");

      DEBUG("Position: ", strm.position(), "/", strm.size());

      CHECK_REMAINING(strm, 8);

      min_speed = strm.read_u8();
      max_speed = strm.read_u8();
      repeat    = strm.read_u16();
      back_to   = strm.read_u16();
      // handles.resize(strm.read_u16()); // :TODO: what's going on here? how
      // did anaconda do this?
      handles.resize(strm.read_u8());
      strm.skip(1);

      CHECK_REMAINING(strm, handles.size() * 2);

      for (auto &handle : handles) handle = strm.read_u16();

      return lak::ok_t{};
    }

    error_t animation_direction_t::view(source_explorer_t &srcexp) const
    {
      ImGui::Text("Min Speed: %d", (int)min_speed);
      ImGui::Text("Max Speed: %d", (int)max_speed);
      ImGui::Text("Repeat: 0x%zX", (size_t)repeat);
      ImGui::Text("Back To: 0x%zX", (size_t)back_to);
      ImGui::Text("Frames: 0x%zX", handles.size());

      int index = 0;
      for (const auto &handle : handles)
      {
        ImGui::PushID(index++);
        DEFER(ImGui::PopID());

        [&, this]() -> error_t {
          RES_TRY_ASSIGN(
            auto &img =,
            GetImage(srcexp.state, handle)
              .map_err(APPEND_TRACE(
                "object::animation_direction_t::view: bad image")));
          RES_TRY(img.view(srcexp).map_err(
            APPEND_TRACE("object::animation_direction_t::view")));
          return lak::ok_t{};
        }()
                         .if_err([](const auto &err) {
                           ImGui::Text("Invalid Image Handle");
                           ImGui::Text(lak::streamify<char>(err).c_str());
                         })
                         .discard();
      }

      return lak::ok_t{};
    }

    error_t animation_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::animation_t::read");

      DEBUG("Position: ", strm.position(), "/", strm.size());

      CHECK_REMAINING(strm, offsets.size() * 2);

      const size_t begin = strm.position();
      DEFER(strm.seek(begin + (offsets.size() * 2)););

      size_t index = 0;
      for (auto &offset : offsets)
      {
        offset = strm.read_u16();

        CHECK_POSITION(strm, begin + offset);

        if (offset != 0)
        {
          const size_t pos = strm.position();
          strm.seek(begin + offset);
          DEFER(strm.seek(pos););
          RES_TRY(directions[index]
                    .read(game, strm)
                    .map_err(APPEND_TRACE("object::animation_t::read")));
        }
        ++index;
      }

      return lak::ok_t{};
    }

    error_t animation_t::view(source_explorer_t &srcexp) const
    {
      size_t index = 0;
      for (const auto &direction : directions)
      {
        if (direction.handles.size() > 0)
        {
          LAK_TREE_NODE("Animation Direction 0x%zX", index)
          {
            RES_TRY(direction.view(srcexp).map_err(
              APPEND_TRACE("object::animation_t::view")));
          }
        }
        ++index;
      }

      return lak::ok_t{};
    }

    error_t animation_header_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::animation_header_t::read");

      const size_t begin = strm.position();

      DEBUG("Position: ", strm.position());

      size = strm.read_u16();
      DEBUG("Size: ", size);
      DEFER(strm.seek(begin + size));

      offsets.resize(strm.read_u16());
      animations.resize(offsets.size());

      DEBUG("Animation Count: ", animations.size());

      size_t index = 0;
      for (auto &offset : offsets)
      {
        DEBUG("Animation: ", index, "/", offsets.size());
        offset = strm.read_u16();

        CHECK_POSITION(strm, begin + offset);

        if (offset != 0)
        {
          const size_t pos = strm.position();
          strm.seek(begin + offset);
          DEFER(strm.seek(pos));
          RES_TRY(
            animations[index]
              .read(game, strm)
              .map_err(APPEND_TRACE("object::animation_header_t::read")));
        }
        ++index;
      }

      return lak::ok_t{};
    }

    error_t animation_header_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("Animations")
      {
        ImGui::Separator();
        ImGui::Text("Size: 0x%zX", (size_t)size);
        ImGui::Text("Animations: %zu", animations.size());
        size_t index = 0;
        for (const auto &animation : animations)
        {
          // TODO: figure out what this was meant to be checking
          if (/*animation.offsets[index] > 0 */ true)
          {
            LAK_TREE_NODE("Animation 0x%zX", index)
            {
              ImGui::Separator();
              RES_TRY(animation.view(srcexp).map_err(
                APPEND_TRACE("object::animation_header_t::view")));
            }
          }
          ++index;
        }
      }

      return lak::ok_t{};
    }

    error_t common_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::common_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("object::common_t::read")));

      RES_TRY_ASSIGN(
        lak::memory cstrm =,
        entry.decode().map_err(APPEND_TRACE("object::common_t::read")));

      // if (game.product_build >= 288 && !game.old_game)
      //     mode = game_mode_t::_288;
      // if (game.product_build >= 284 && !game.old_game && !game.compat)
      //     mode = game_mode_t::_284;
      // else
      //     mode = game_mode_t::_OLD;
      mode = _mode;

      const size_t begin = cstrm.position();

      size = cstrm.read_u32();

      CHECK_REMAINING(cstrm, size - 4);

      if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
      {
        if (!cstrm.remaining()) return lak::ok_t{};
        animations_offset = cstrm.read_u16();
        if (!cstrm.remaining()) return lak::ok_t{};
        movements_offset = cstrm.read_u16(); // are these in the right order?
        if (!cstrm.remaining()) return lak::ok_t{};
        version = cstrm.read_u16(); // are these in the right order?
        if (!cstrm.remaining()) return lak::ok_t{};
        counter_offset = cstrm.read_u16(); // are these in the right order?
        if (!cstrm.remaining()) return lak::ok_t{};
        system_offset = cstrm.read_u16(); // are these in the right order?
      }
      // else if (mode == game_mode_t::_284)
      // {
      //     counter_offset = cstrm.read_u16();
      //     version = cstrm.read_u16();
      //     cstrm.skip(2);
      //     movements_offset = cstrm.read_u16();
      //     extension_offset = cstrm.read_u16();
      //     animations_offset = cstrm.read_u16();
      // }
      else
      {
        if (!cstrm.remaining()) return lak::ok_t{};
        movements_offset = cstrm.read_u16();
        if (!cstrm.remaining()) return lak::ok_t{};
        animations_offset = cstrm.read_u16();
        if (!cstrm.remaining()) return lak::ok_t{};
        version = cstrm.read_u16();
        if (!cstrm.remaining()) return lak::ok_t{};
        counter_offset = cstrm.read_u16();
        if (!cstrm.remaining()) return lak::ok_t{};
        system_offset = cstrm.read_u16();
      }

      if (!cstrm.remaining()) return lak::ok_t{};
      flags = cstrm.read_u32();

#if 1
      cstrm.skip(8 * 2);
#else
      qualifiers.clear();
      qualifiers.reserve(8);
      for (size_t i = 0; i < 8; ++i)
      {
        int16_t qualifier = cstrm.read_s16();
        if (qualifier == -1) break;
        qualifiers.push_back(qualifier);
      }
#endif

      if (mode == game_mode_t::_284)
        system_offset = cstrm.read_u16();
      else
        extension_offset = cstrm.read_u16();

      if (!cstrm.remaining()) return lak::ok_t{};
      values_offset = cstrm.read_u16();
      if (!cstrm.remaining()) return lak::ok_t{};
      strings_offset = cstrm.read_u16();
      if (!cstrm.remaining()) return lak::ok_t{};
      new_flags = cstrm.read_u32();
      if (!cstrm.remaining()) return lak::ok_t{};
      preferences = cstrm.read_u32();
      if (!cstrm.remaining()) return lak::ok_t{};
      identifier = cstrm.read_u32();
      if (!cstrm.remaining()) return lak::ok_t{};
      back_color.r = cstrm.read_u8();
      if (!cstrm.remaining()) return lak::ok_t{};
      back_color.g = cstrm.read_u8();
      if (!cstrm.remaining()) return lak::ok_t{};
      back_color.b = cstrm.read_u8();
      if (!cstrm.remaining()) return lak::ok_t{};
      back_color.a = cstrm.read_u8();
      if (!cstrm.remaining()) return lak::ok_t{};
      fade_in_offset = cstrm.read_u32();
      if (!cstrm.remaining()) return lak::ok_t{};
      fade_out_offset = cstrm.read_u32();

      if (animations_offset > 0)
      {
        DEBUG("Animations Offset: ", animations_offset);
        cstrm.seek(begin + animations_offset);
        animations = std::make_unique<animation_header_t>();
        RES_TRY(animations->read(game, cstrm)
                  .map_err(APPEND_TRACE("object::common_t::read")));
      }

      return lak::ok_t{};
    }

    error_t common_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE(
        "0x%zX Properties (Common)##%zX", (size_t)entry.ID, entry.position)
      {
        entry.view(srcexp);

        if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
        {
          ImGui::Text("Animations Offset: 0x%zX", (size_t)animations_offset);
          ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
          ImGui::Text("Version: 0x%zX", (size_t)version);
          ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
          ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
          ImGui::Text("Flags: 0x%zX", (size_t)flags);
          ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
        }
        // else if (mode == game_mode_t::_284)
        // {
        //   ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
        //   ImGui::Text("Version: 0x%zX", (size_t)version);
        //   ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
        //   ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
        //   ImGui::Text("Animations Offset: 0x%zX",
        //               (size_t)animations_offset);
        //   ImGui::Text("Flags: 0x%zX", (size_t)flags);
        //   ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
        // }
        else
        {
          ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
          ImGui::Text("Animations Offset: 0x%zX", (size_t)animations_offset);
          ImGui::Text("Version: 0x%zX", (size_t)version);
          ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
          ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
          ImGui::Text("Flags: 0x%zX", (size_t)flags);
          ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
        }
        ImGui::Text("Values Offset: 0x%zX", (size_t)values_offset);
        ImGui::Text("Strings Offset: 0x%zX", (size_t)strings_offset);
        ImGui::Text("New Flags: 0x%zX", (size_t)new_flags);
        ImGui::Text("Preferences: 0x%zX", (size_t)preferences);
        ImGui::Text("Identifier: 0x%zX", (size_t)identifier);
        ImGui::Text("Fade In Offset: 0x%zX", (size_t)fade_in_offset);
        ImGui::Text("Fade Out Offset: 0x%zX", (size_t)fade_out_offset);

        lak::vec3f_t col = ((lak::vec3f_t)back_color) / 256.0f;
        ImGui::ColorEdit3("Background Color", &col.x);

        if (animations)
        {
          RES_TRY(animations->view(srcexp).map_err(
            APPEND_TRACE("object::common_t::view")));
        }
      }

      return lak::ok_t{};
    }

    error_t item_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::item_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("object::item_t::read")));

      RES_TRY_ASSIGN(
        auto dstrm =,
        entry.decode().map_err(APPEND_TRACE("object::item_t::read")));

      handle = dstrm.read_u16();
      type   = (object_type_t)dstrm.read_s16();
      dstrm.read_u16(); // flags
      dstrm.read_u16(); // "no longer used"
      ink_effect       = dstrm.read_u32();
      ink_effect_param = dstrm.read_u32();

      [&, this]() -> error_t {
        for (bool not_finished = true; not_finished;)
        {
          switch ((chunk_t)strm.peek_u16())
          {
            case chunk_t::object_name:
              name = std::make_unique<string_chunk_t>();
              RES_TRY_ERR(name->read(game, strm)
                            .map_err(APPEND_TRACE("object::item_t::read")));
              break;

            case chunk_t::object_properties:
              switch (type)
              {
                case object_type_t::quick_backdrop:
                  quick_backdrop = std::make_unique<quick_backdrop_t>();
                  RES_TRY_ERR(
                    quick_backdrop->read(game, strm)
                      .map_err(APPEND_TRACE("object::item_t::read")));
                  break;

                case object_type_t::backdrop:
                  backdrop = std::make_unique<backdrop_t>();
                  RES_TRY_ERR(
                    backdrop->read(game, strm)
                      .map_err(APPEND_TRACE("object::item_t::read")));
                  break;

                default:
                  common = std::make_unique<common_t>();
                  RES_TRY_ERR(
                    common->read(game, strm)
                      .map_err(APPEND_TRACE("object::item_t::read")));
                  break;
              }
              break;

            case chunk_t::object_effect:
              effect = std::make_unique<effect_t>();
              RES_TRY_ERR(effect->read(game, strm)
                            .map_err(APPEND_TRACE("object::item_t::read")));
              break;

            case chunk_t::last:
              end = std::make_unique<last_t>();
              RES_TRY_ERR(end->read(game, strm)
                            .map_err(APPEND_TRACE("object::item_t::read")));
              [[fallthrough]];

            default: not_finished = false; break;
          }
        }

        return lak::ok_t{};
      }()
                       .IF_ERR("Failed To Read Child Chunks")
                       .discard();

      return lak::ok_t{};
    }

    error_t item_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX %s '%s'##%zX",
                    (size_t)entry.ID,
                    GetObjectTypeString(type),
                    (name ? lak::strconv<char>(name->value).c_str() : ""),
                    entry.position)
      {
        entry.view(srcexp);

        ImGui::Text("Handle: 0x%zX", (size_t)handle);
        ImGui::Text("Type: 0x%zX", (size_t)type);
        ImGui::Text("Ink Effect: 0x%zX", (size_t)ink_effect);
        ImGui::Text("Ink Effect Parameter: 0x%zX", (size_t)ink_effect_param);

        if (name)
        {
          RES_TRY(name->view(srcexp, "Name", true)
                    .map_err(APPEND_TRACE("object::item_t::view")));
        }

        if (quick_backdrop)
        {
          RES_TRY(quick_backdrop->view(srcexp).map_err(
            APPEND_TRACE("object::item_t::view")));
        }
        if (backdrop)
        {
          RES_TRY(backdrop->view(srcexp).map_err(
            APPEND_TRACE("object::item_t::view")));
        }
        if (common)
        {
          RES_TRY(common->view(srcexp).map_err(
            APPEND_TRACE("object::item_t::view")));
        }

        if (effect)
        {
          RES_TRY(effect->view(srcexp).map_err(
            APPEND_TRACE("object::item_t::view")));
        }
        if (end)
        {
          RES_TRY(
            end->view(srcexp).map_err(APPEND_TRACE("object::item_t::view")));
        }
      }

      return lak::ok_t{};
    }

    std::unordered_map<uint32_t, std::vector<std::u16string>>
    item_t::image_handles() const
    {
      std::unordered_map<uint32_t, std::vector<std::u16string>> result;

      if (quick_backdrop)
        result[quick_backdrop->shape.handle].emplace_back(u"Backdrop");
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
                  u"Animation-"s + SourceExplorer::to_u16string(animIndex) +
                  u" Direction-"s + SourceExplorer::to_u16string(i) +
                  u" Frame-"s + SourceExplorer::to_u16string(frame));
              }
            }
          }
        }
      }

      return result;
    }

    error_t bank_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("object::bank_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("object::bank_t::read")));

      CHECK_POSITION(strm, entry.end);

      strm.seek(entry.data.position);
      DEFER(strm.seek(entry.end););

      items.resize(strm.read_u32());

      error_t result = lak::ok_t{};

      for (auto &item : items)
      {
        if (auto res = item.read(game, strm); res.is_err())
        {
          result = res
                     .IF_ERR("Failed To Read Item ",
                             (&item - items.data()),
                             " Of ",
                             items.size())
                     .map_err(APPEND_TRACE("object::bank_t::read"));
          if (strm.remaining() == 0) break;
        }
        if (!item.end)
          WARNING("Item ", (&item - items.data()), " Has No End Chunk");
        if (strm.position() > entry.end)
          WARNING("Reading Beyond Object Bank End");
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
      }

      if (strm.position() < entry.end)
      {
        WARNING("There is still ",
                entry.end - strm.position(),
                " bytes left in the object bank");
      }
      else if (strm.position() > entry.end)
      {
        ERROR("Object bank overshot entry by ",
              strm.position() - entry.end,
              " bytes");
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Object Bank (%zu Items)##%zX",
                    (size_t)entry.ID,
                    items.size(),
                    entry.position)
      {
        entry.view(srcexp);

        for (const item_t &item : items)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("object::bank_t::view")));
        }
      }

      return lak::ok_t{};
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
      SCOPED_CHECKPOINT("frame::palette_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("frame::palette_t::read")));

      RES_TRY_ASSIGN(
        auto pstrm =,
        entry.decode().map_err(APPEND_TRACE("frame::palette_t::read")));

      unknown = pstrm.read_u32();

      for (auto &color : colors)
      {
        color.r = pstrm.read_u8();
        color.g = pstrm.read_u8();
        color.b = pstrm.read_u8();
        /* color.a = */ pstrm.read_u8();
        color.a = 255u;
      }

      return lak::ok_t{};
    }

    error_t palette_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE(
        "0x%zX Frame Palette##%zX", (size_t)entry.ID, entry.position)
      {
        entry.view(srcexp);

        uint8_t index = 0;
        for (auto color : colors)
        {
          lak::vec3f_t col = ((lak::vec3f_t)color) / 256.0f;
          char str[3];
          snprintf(str, 3, "%hhX", index++);
          ImGui::ColorEdit3(str, &col.x);
        }
      }

      return lak::ok_t{};
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
      SCOPED_CHECKPOINT("frame::object_instance_t::read");

      info          = strm.read_u16();
      handle        = strm.read_u16();
      position.x    = game.old_game ? strm.read_s16() : strm.read_s32();
      position.y    = game.old_game ? strm.read_s16() : strm.read_s32();
      parent_type   = (object_parent_type_t)strm.read_u16();
      parent_handle = strm.read_u16(); // object info (?)
      if (!game.old_game)
      {
        layer   = strm.read_u16();
        unknown = strm.read_u16();
      }

      return lak::ok_t{};
    }

    error_t object_instance_t::view(source_explorer_t &srcexp) const
    {
      lak::u8string str;
      auto obj = GetObject(srcexp.state, handle);
      if (obj.is_ok() && obj.unwrap().name)
        str += lak::to_u8string(obj.unwrap().name->value);

      LAK_TREE_NODE("0x%zX %s##%zX", (size_t)handle, str.c_str(), (size_t)info)
      {
        ImGui::Text("Handle: 0x%zX", (size_t)handle);
        ImGui::Text("Info: 0x%zX", (size_t)info);
        ImGui::Text(
          "Position: (%li, %li)", (long)position.x, (long)position.y);
        ImGui::Text("Parent Type: %s (0x%zX)",
                    GetObjectParentTypeString(parent_type),
                    (size_t)parent_type);
        ImGui::Text("Parent Handle: 0x%zX", (size_t)parent_handle);
        ImGui::Text("Layer: 0x%zX", (size_t)layer);
        ImGui::Text("Unknown: 0x%zX", (size_t)unknown);

        if (obj.is_ok())
        {
          RES_TRY(obj.unwrap().view(srcexp).map_err(
            APPEND_TRACE("frame::object_instance_t::view")));
        }

        switch (parent_type)
        {
          case object_parent_type_t::frame_item:
            if (auto parent_obj = GetObject(srcexp.state, parent_handle);
                parent_obj.is_ok())
            {
              RES_TRY(parent_obj.unwrap().view(srcexp).map_err(
                APPEND_TRACE("frame::object_instance_t::view")));
            }
            break;

          case object_parent_type_t::frame:
            if (auto parent_obj = GetFrame(srcexp.state, parent_handle);
                parent_obj.is_ok())
            {
              RES_TRY(parent_obj.unwrap().view(srcexp).map_err(
                APPEND_TRACE("frame::object_instance_t::view")));
            }
            break;

          case object_parent_type_t::none: [[fallthrough]];
          case object_parent_type_t::qualifier: [[fallthrough]];
          default: break;
        }
      }

      return lak::ok_t{};
    }

    error_t object_instances_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("frame::object_instances_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("frame::object_instances_t::read")));

      RES_TRY_ASSIGN(auto hstrm =,
                     entry.decode().map_err(
                       APPEND_TRACE("frame::object_instances_t::read")));
      objects.clear();
      objects.resize(hstrm.read_u32());
      DEBUG("Objects: ", objects.size());
      for (auto &object : objects)
      {
        RES_TRY(object.read(game, hstrm)
                  .map_err(APPEND_TRACE("frame::object_instances_t::read")));
      }

      return lak::ok_t{};
    }

    error_t object_instances_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE(
        "0x%zX Object Instances##%zX", (size_t)entry.ID, entry.position)
      {
        entry.view(srcexp);

        for (const auto &object : objects)
        {
          RES_TRY(object.view(srcexp).map_err(
            APPEND_TRACE("frame::object_instances_t::view")));
        }
      }

      return lak::ok_t{};
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
      SCOPED_CHECKPOINT("frame::random_seed_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("frame::random_seed_t::read")));

      RES_TRY_ASSIGN(value =, entry.decode()).read_s16();

      return lak::ok_t{};
    }

    error_t random_seed_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Random Seed##%zX", (size_t)entry.ID, entry.position)
      {
        entry.view(srcexp);
        ImGui::Text("Value: %i", (int)value);
      }

      return lak::ok_t{};
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
      SCOPED_CHECKPOINT("frame::item_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("frame::item_t::read")));

      strm.seek(entry.position + 0x8);
      DEFER(strm.seek(entry.end););

      for (bool not_finished = true; not_finished;)
      {
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
        switch ((chunk_t)strm.peek_u16())
        {
          case chunk_t::frame_name:
            name = std::make_unique<string_chunk_t>();
            RES_TRY_ERR(name->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_header:
            header = std::make_unique<header_t>();
            RES_TRY_ERR(header->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_password:
            password = std::make_unique<password_t>();
            RES_TRY_ERR(password->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_palette:
            palette = std::make_unique<palette_t>();
            RES_TRY_ERR(palette->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_object_instances:
            object_instances = std::make_unique<object_instances_t>();
            RES_TRY_ERR(object_instances->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_fade_in_frame:
            fade_in_frame = std::make_unique<fade_in_frame_t>();
            RES_TRY_ERR(fade_in_frame->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_fade_out_frame:
            fade_out_frame = std::make_unique<fade_out_frame_t>();
            RES_TRY_ERR(fade_out_frame->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_fade_in:
            fade_in = std::make_unique<fade_in_t>();
            RES_TRY_ERR(fade_in->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_fade_out:
            fade_out = std::make_unique<fade_out_t>();
            RES_TRY_ERR(fade_out->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_events:
            events = std::make_unique<events_t>();
            RES_TRY_ERR(events->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_play_header:
            play_head = std::make_unique<play_header_r>();
            RES_TRY_ERR(play_head->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_additional_items:
            additional_item = std::make_unique<additional_item_t>();
            RES_TRY_ERR(additional_item->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_additional_items_instances:
            additional_item_instance =
              std::make_unique<additional_item_instance_t>();
            RES_TRY_ERR(additional_item_instance->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_layers:
            layers = std::make_unique<layers_t>();
            RES_TRY_ERR(layers->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_virtual_size:
            virtual_size = std::make_unique<virtual_size_t>();
            RES_TRY_ERR(virtual_size->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::demo_file_path:
            demo_file_path = std::make_unique<demo_file_path_t>();
            RES_TRY_ERR(demo_file_path->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::random_seed:
            random_seed = std::make_unique<random_seed_t>();
            RES_TRY_ERR(random_seed->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_layer_effect:
            layer_effect = std::make_unique<layer_effect_t>();
            RES_TRY_ERR(layer_effect->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_bluray:
            blueray = std::make_unique<blueray_t>();
            RES_TRY_ERR(blueray->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::movement_timer_base:
            movement_time_base = std::make_unique<movement_time_base_t>();
            RES_TRY_ERR(movement_time_base->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::mosaic_image_table:
            mosaic_image_table = std::make_unique<mosaic_image_table_t>();
            RES_TRY_ERR(mosaic_image_table->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_effects:
            effects = std::make_unique<effects_t>();
            RES_TRY_ERR(effects->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_iphone_options:
            iphone_options = std::make_unique<iphone_options_t>();
            RES_TRY_ERR(iphone_options->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::frame_chunk334C:
            chunk334C = std::make_unique<chunk_334C_t>();
            RES_TRY_ERR(chunk334C->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            break;

          case chunk_t::last:
            end = std::make_unique<last_t>();
            RES_TRY_ERR(end->read(game, strm)
                          .map_err(APPEND_TRACE("frame::item_t::read")));
            [[fallthrough]];

          default: not_finished = false; break;
        }
      }

      return lak::ok_t{};
    }

    error_t item_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX '%s'##%zX",
                    (size_t)entry.ID,
                    (name ? lak::strconv<char>(name->value).c_str() : ""),
                    entry.position)
      {
        entry.view(srcexp);

        if (name)
        {
          RES_TRY(name->view(srcexp, "Name", true)
                    .map_err(APPEND_TRACE("frame::item_t::view")));
        }
        if (header)
        {
          RES_TRY(
            header->view(srcexp).map_err(APPEND_TRACE("frame::item_t::view")));
        }
        if (password)
        {
          RES_TRY(password->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (palette)
        {
          RES_TRY(palette->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (object_instances)
        {
          RES_TRY(object_instances->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (fade_in_frame)
        {
          RES_TRY(fade_in_frame->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (fade_out_frame)
        {
          RES_TRY(fade_out_frame->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (fade_in)
        {
          RES_TRY(fade_in->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (fade_out)
        {
          RES_TRY(fade_out->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (events)
        {
          RES_TRY(
            events->view(srcexp).map_err(APPEND_TRACE("frame::item_t::view")));
        }
        if (play_head)
        {
          RES_TRY(play_head->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (additional_item)
        {
          RES_TRY(additional_item->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (layers)
        {
          RES_TRY(
            layers->view(srcexp).map_err(APPEND_TRACE("frame::item_t::view")));
        }
        if (layer_effect)
        {
          RES_TRY(layer_effect->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (virtual_size)
        {
          RES_TRY(virtual_size->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (demo_file_path)
        {
          RES_TRY(demo_file_path->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (random_seed)
        {
          RES_TRY(random_seed->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (blueray)
        {
          RES_TRY(blueray->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (movement_time_base)
        {
          RES_TRY(movement_time_base->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (mosaic_image_table)
        {
          RES_TRY(mosaic_image_table->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (effects)
        {
          RES_TRY(effects->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (iphone_options)
        {
          RES_TRY(iphone_options->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (chunk334C)
        {
          RES_TRY(chunk334C->view(srcexp).map_err(
            APPEND_TRACE("frame::item_t::view")));
        }
        if (end)
        {
          RES_TRY(
            end->view(srcexp).map_err(APPEND_TRACE("frame::item_t::view")));
        }
      }

      return lak::ok_t{};
    }

    error_t handles_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("frame::handles_t::read");

      RES_TRY_ERR(entry.read(game, strm)
                    .map_err(APPEND_TRACE("frame::handles_t::read")));

      RES_TRY_ASSIGN(
        auto hstrm =,
        entry.decode().map_err(APPEND_TRACE("frame::handles_t::read")));
      handles.resize(hstrm.size() / 2);
      for (auto &handle : handles) handle = hstrm.read_u16();

      return lak::ok_t{};
    }

    error_t handles_t::view(source_explorer_t &srcexp) const
    {
      return basic_view(srcexp, "Frame Handles");
    }

    error_t bank_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("frame::bank_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("frame::bank_t::read")));

      items.clear();
      error_t result = lak::ok_t{};
      while ((chunk_t)strm.peek_u16() == chunk_t::frame)
      {
        items.emplace_back();
        if (result = items.back().read(game, strm); result.is_err()) break;
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
      }

      if (strm.position() < entry.end)
      {
        WARNING("There is still ",
                entry.end - strm.position(),
                " bytes left in the frame bank");
      }
      else if (strm.position() > entry.end)
      {
        DEBUG("Frame bank overshot entry by ",
              strm.position() - entry.end,
              " bytes");
      }

      // strm.seek(entry.end);

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Frame Bank (%zu Items)##%zX",
                    (size_t)entry.ID,
                    items.size(),
                    entry.position)
      {
        entry.view(srcexp);

        for (const item_t &item : items)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("frame::bank_t::view")));
        }
      }

      return lak::ok_t{};
    }
  }

  namespace image
  {
    error_t item_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("image::item_t::read");

      lak::memory istrm;
      if (game.product_build > 288)
      {
        const size_t header_size = 36;
        RES_TRY_ERR(entry.read(game, strm, false, header_size)
                      .map_err(APPEND_TRACE("image::item_t::read")));

        RES_TRY_ASSIGN(istrm =,
                       entry.decodeHeader(header_size)
                         .map_err(APPEND_TRACE("image::item_t::read")));
        CHECK_REMAINING(istrm, header_size);
      }
      else
      {
        RES_TRY_ERR(entry.read(game, strm, true)
                      .map_err(APPEND_TRACE("image::item_t::read")));

        if (!game.old_game && game.product_build >= 284) --entry.handle;

        RES_TRY_ASSIGN(istrm =,
                       entry.decode(176 + (game.old_game ? 16 : 80))
                         .map_err(APPEND_TRACE("image::item_t::read")));
      }

      if (game.old_game)
        checksum = istrm.read_u16();
      else
        checksum = istrm.read_u32();
      reference = istrm.read_u32();
      if (game.product_build > 288) istrm.skip(4);
      data_size     = istrm.read_u32();
      size.x        = istrm.read_u16();
      size.y        = istrm.read_u16();
      graphics_mode = (graphics_mode_t)istrm.read_u8();
      flags         = (image_flag_t)istrm.read_u8();
#if 0
      if (graphics_mode <= graphics_mode_t::graphics3)
      {
        palette_entries = istrm.read_u8();
        for (size_t i = 0; i < palette.size();
              ++i) // where is this size coming from???
          palette[i] = ColorFrom32bitRGBA(strm); // not sure if RGBA or BGRA
        count = strm.read_u32();
      }
#endif
      if (!game.old_game) unknown = istrm.read_u16();
      hotspot.x = istrm.read_u16();
      hotspot.y = istrm.read_u16();
      action.x  = istrm.read_u16();
      action.y  = istrm.read_u16();
      if (!game.old_game) transparent = ColorFrom32bitRGBA(istrm).UNWRAP();
      if (game.product_build > 288)
      {
        data_position = 0;
        strm.seek(entry.data.position);
        entry.data.data = strm.read(data_size);
        entry.end       = strm.position();
      }
      else
      {
        data_position = istrm.position();
      }

      return lak::ok_t{};
    }

    error_t item_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Image##%zX", (size_t)entry.handle, entry.position)
      {
        entry.view(srcexp);

        ImGui::Text("Checksum: 0x%zX", (size_t)checksum);
        ImGui::Text("Reference: 0x%zX", (size_t)reference);
        ImGui::Text("Data Size: 0x%zX", (size_t)data_size);
        ImGui::Text("Image Size: %zu * %zu", (size_t)size.x, (size_t)size.y);
        ImGui::Text("Graphics Mode: 0x%zX", (size_t)graphics_mode);
        ImGui::Text("Image Flags: 0x%zX", (size_t)flags);
        ImGui::Text("Unknown: 0x%zX", (size_t)unknown);
        ImGui::Text(
          "Hotspot: (%zu, %zu)", (size_t)hotspot.x, (size_t)hotspot.y);
        ImGui::Text("Action: (%zu, %zu)", (size_t)action.x, (size_t)action.y);
        {
          lak::vec4f_t col = ((lak::vec4f_t)transparent) / 255.0f;
          ImGui::ColorEdit4("Transparent", &col.x);
        }
        ImGui::Text("Data Position: 0x%zX", data_position);

        if (ImGui::Button("View Image"))
        {
          auto img = image(srcexp.dump_color_transparent)
                       .IF_ERR("Failed To Read Image Data");
          if (img.is_ok())
            srcexp.image = CreateTexture(img.unwrap(), srcexp.graphics_mode);
        }
      }

      return lak::ok_t{};
    }

    result_t<lak::memory> item_t::image_data() const
    {
      SCOPED_CHECKPOINT("image::image_t::image_data");

      RES_TRY_ASSIGN(
        lak::memory strm =,
        entry.decode().map_err(APPEND_TRACE("image::item_t::image_data")));

      CHECK_POSITION(strm, data_position);

      strm.seek(data_position);

      if ((flags & image_flag_t::LZX) != image_flag_t::none)
      {
        if (entry.mode == encoding_t::mode4)
        {
          return lak::ok_t{lak::move(strm)};
        }
        else
        {
          CHECK_REMAINING(strm, 8);

          [[maybe_unused]] const uint32_t decompressed_length =
            strm.read_u32();
          const uint32_t compressed_length = strm.read_u32();

          CHECK_REMAINING(strm, compressed_length);

          return lak::ok_t{
            lak::memory(InflateOrCompressed(strm.read(compressed_length)))};
        }
      }
      else
      {
        return lak::ok_t{strm};
      }
    }

    bool item_t::need_palette() const
    {
      return graphics_mode == graphics_mode_t::graphics2 ||
             graphics_mode == graphics_mode_t::graphics3;
    }

    result_t<lak::image4_t> item_t::image(
      const bool color_transparent, const lak::color4_t palette[256]) const
    {
      SCOPED_CHECKPOINT("image::image_t::image");

      lak::image4_t result;
      result.resize((lak::vec2s_t)size);

      RES_TRY_ASSIGN(
        lak::memory strm =,
        image_data().map_err(APPEND_TRACE("image::item_t::image")));

      [[maybe_unused]] size_t bytes_read;
      if ((flags & (image_flag_t::RLE | image_flag_t::RLEW |
                    image_flag_t::RLET)) != image_flag_t::none)
      {
        RES_TRY_ASSIGN(bytes_read =,
                       ReadRLE(strm, result, graphics_mode, palette));
      }
      else
      {
        RES_TRY_ASSIGN(bytes_read =,
                       ReadRGB(strm, result, graphics_mode, palette));
      }

      if ((flags & image_flag_t::alpha) == image_flag_t::alpha)
      {
        if (entry.mode != encoding_t::mode4)
      {
        RES_TRY(ReadAlpha(strm, result));
      }
      }
      else if (color_transparent)
      {
        ReadTransparent(transparent, result);
      }
      else if (entry.mode == encoding_t::mode4)
      {
        for (size_t i = 0; i < result.contig_size(); ++i) result[i].a = 255;
      }

      if (strm.remaining() != 0)
        WARNING(strm.remaining(), " Bytes Left Over In Image Data");

      return lak::ok_t{result};
    }

    error_t end_t::view(source_explorer_t &srcexp) const
    {
      return basic_view(srcexp, "Image Bank End");
    }

    error_t bank_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("image::bank_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("image::bank_t::read")));

      strm.seek(entry.data.position);

      items.resize(strm.read_u32());

      DEBUG("Image Bank Size: ", items.size());

      error_t result = lak::ok_t{};
      for (auto &item : items)
      {
        if (result = item.read(game, strm); result.is_err()) break;
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
      }

      if (strm.position() < entry.end)
      {
        WARNING("There is still ",
                entry.end - strm.position(),
                " bytes left in the image bank");
      }
      else if (strm.position() > entry.end)
      {
        ERROR("Image bank overshot entry by ",
              strm.position() - entry.end,
              " bytes");
      }

      strm.seek(entry.end);

      if ((chunk_t)strm.peek_u16() == chunk_t::image_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Image Bank (%zu Items)##%zX",
                    (size_t)entry.ID,
                    items.size(),
                    entry.position)
      {
        entry.view(srcexp);

        for (const item_t &item : items)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("image::bank_t::view")));
        }

        if (end)
        {
          RES_TRY(
            end->view(srcexp).map_err(APPEND_TRACE("image::bank_t::view")));
        }
      }

      return lak::ok_t{};
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
      SCOPED_CHECKPOINT("font::bank_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("font::bank_t::read")));

      strm.seek(entry.data.position);

      items.resize(strm.read_u32());

      error_t result = lak::ok_t{};
      for (auto &item : items)
      {
        if (result = item.read(game, strm); result.is_err()) break;
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
      }

      if (strm.position() < entry.end)
      {
        WARNING("There is still ",
                entry.end - strm.position(),
                " bytes left in the font bank");
      }
      else if (strm.position() > entry.end)
      {
        ERROR("Font bank overshot entry by ",
              strm.position() - entry.end,
              " bytes");
      }

      strm.seek(entry.end);

      if ((chunk_t)strm.peek_u16() == chunk_t::font_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Font Bank (%zu Items)##%zX",
                    (size_t)entry.ID,
                    items.size(),
                    entry.position)
      {
        entry.view(srcexp);

        for (const item_t &item : items)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("font::bank_t::view")));
        }

        if (end)
        {
          RES_TRY(
            end->view(srcexp).map_err(APPEND_TRACE("font::bank_t::view")));
        }
      }

      return lak::ok_t{};
    }
  }

  namespace sound
  {
    error_t item_t::read(game_t &game, lak::memory &strm)
    {
      SCOPED_CHECKPOINT("sound::item_t::read");
      return entry.read(game, strm, false, 0x18);
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
      SCOPED_CHECKPOINT("sound::bank_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("sound::bank_t::read")));

      strm.seek(entry.data.position);

      items.resize(strm.read_u32());

      error_t result = lak::ok_t{};
      for (auto &item : items)
      {
        if (result = item.read(game, strm); result.is_err()) break;
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
      }

      if (strm.position() < entry.end)
      {
        WARNING("There is still ",
                entry.end - strm.position(),
                " bytes left in the sound bank");
      }
      else if (strm.position() > entry.end)
      {
        ERROR("Sound bank overshot entry by ",
              strm.position() - entry.end,
              " bytes");
      }

      strm.seek(entry.end);

      if ((chunk_t)strm.peek_u16() == chunk_t::sound_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Sound Bank (%zu Items)##%zX",
                    (size_t)entry.ID,
                    items.size(),
                    entry.position)
      {
        entry.view(srcexp);

        for (const item_t &item : items)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("sound::bank_t::view")));
        }

        if (end)
        {
          RES_TRY(
            end->view(srcexp).map_err(APPEND_TRACE("sound::bank_t::view")));
        }
      }

      return lak::ok_t{};
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
      SCOPED_CHECKPOINT("music::bank_t::read");

      RES_TRY_ERR(
        entry.read(game, strm).map_err(APPEND_TRACE("music::bank_t::read")));

      strm.seek(entry.data.position);

      items.resize(strm.read_u32());

      error_t result = lak::ok_t{};
      for (auto &item : items)
      {
        if (result = item.read(game, strm); result.is_err()) break;
        game.completed =
          (float)((double)strm.position() / (double)strm.size());
      }

      if (strm.position() < entry.end)
      {
        WARNING("There is still ",
                entry.end - strm.position(),
                " bytes left in the music bank");
      }
      else if (strm.position() > entry.end)
      {
        ERROR("Music bank overshot entry by ",
              strm.position() - entry.end,
              " bytes");
      }

      strm.seek(entry.end);

      if ((chunk_t)strm.peek_u16() == chunk_t::music_handles)
      {
        end = std::make_unique<end_t>();
        if (end) result = end->read(game, strm);
      }

      return result;
    }

    error_t bank_t::view(source_explorer_t &srcexp) const
    {
      LAK_TREE_NODE("0x%zX Music Bank (%zu Items)##%zX",
                    (size_t)entry.ID,
                    items.size(),
                    entry.position)
      {
        entry.view(srcexp);

        for (const item_t &item : items)
        {
          RES_TRY(
            item.view(srcexp).map_err(APPEND_TRACE("music::bank_t::view")));
        }

        if (end)
        {
          RES_TRY(
            end->view(srcexp).map_err(APPEND_TRACE("music::bank_t::view")));
        }
      }

      return lak::ok_t{};
    }
  }

  error_t last_t::view(source_explorer_t &srcexp) const
  {
    return basic_view(srcexp, "Last");
  }

  error_t header_t::read(game_t &game, lak::memory &strm)
  {
    SCOPED_CHECKPOINT("header_t::read");

    RES_TRY_ERR(
      entry.read(game, strm).map_err(APPEND_TRACE("header_t::read")));

    auto init_chunk = [&](auto &chunk) {
      chunk = std::make_unique<
        typename std::remove_reference_t<decltype(chunk)>::value_type>();
      return chunk->read(game, strm);
    };

    chunk_t childID  = (chunk_t)-1;
    size_t start_pos = -1;
    for (bool not_finished = true; not_finished;)
    {
      if (strm.size() > 0)
        game.completed =
          (float)((double)strm.position() / (double)strm.size());

      if (strm.position() == start_pos)
        return lak::err_t{error(LINE_TRACE,
                                error::str_err,
                                "last read chunk (",
                                GetTypeString(childID),
                                ") didn't move stream head")};

      start_pos = strm.position();
      childID   = (chunk_t)strm.peek_u16();

      switch (childID)
      {
        case chunk_t::title:
          RES_TRY_ERR(
            init_chunk(title).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::author:
          RES_TRY_ERR(
            init_chunk(author).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::copyright:
          RES_TRY_ERR(
            init_chunk(copyright).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::project_path:
          RES_TRY_ERR(
            init_chunk(project_path).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::output_path:
          RES_TRY_ERR(
            init_chunk(output_path).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::about:
          RES_TRY_ERR(
            init_chunk(about).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::vitalise_preview:
          RES_TRY_ERR(init_chunk(vitalise_preview)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::menu:
          RES_TRY_ERR(
            init_chunk(menu).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::extra_path:
          RES_TRY_ERR(init_chunk(extension_path)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::extensions:
          RES_TRY_ERR(
            init_chunk(extensions).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::extra_data:
          RES_TRY_ERR(init_chunk(extension_data)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::additional_extensions:
          RES_TRY_ERR(init_chunk(additional_extensions)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::app_doc:
          RES_TRY_ERR(
            init_chunk(app_doc).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::other_extension:
          RES_TRY_ERR(init_chunk(other_extension)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::extensions_list:
          RES_TRY_ERR(init_chunk(extension_list)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::icon:
          RES_TRY_ERR(
            init_chunk(icon).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::demo_version:
          RES_TRY_ERR(
            init_chunk(demo_version).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::security_number:
          RES_TRY_ERR(
            init_chunk(security).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::binary_files:
          RES_TRY_ERR(
            init_chunk(binary_files).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::menu_images:
          RES_TRY_ERR(
            init_chunk(menu_images).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::movement_extensions:
          RES_TRY_ERR(init_chunk(movement_extensions)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::exe_only:
          RES_TRY_ERR(init_chunk(exe).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::protection:
          RES_TRY_ERR(
            init_chunk(protection).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::shaders:
          RES_TRY_ERR(
            init_chunk(shaders).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::extended_header:
          RES_TRY_ERR(init_chunk(extended_header)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::spacer:
          RES_TRY_ERR(
            init_chunk(spacer).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::chunk224F:
          RES_TRY_ERR(
            init_chunk(chunk224F).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::title2:
          RES_TRY_ERR(
            init_chunk(title2).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::chunk2253:
          // 16-bytes
          RES_TRY_ERR(
            init_chunk(chunk2253).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::object_names:
          RES_TRY_ERR(
            init_chunk(object_names).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::chunk2255:
          // blank???
          RES_TRY_ERR(
            init_chunk(chunk2255).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::recompiled_object_properties:
          // Appears to have sub chunks
          RES_TRY_ERR(init_chunk(recompiled_object_properties)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::chunk2257:
          RES_TRY_ERR(
            init_chunk(chunk2257).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::object_properties:
          RES_TRY_ERR(init_chunk(object_properties)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::font_meta:
          RES_TRY_ERR(init_chunk(truetype_fonts_meta)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::font_chunk:
          RES_TRY_ERR(init_chunk(truetype_fonts)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::global_events:
          RES_TRY_ERR(
            init_chunk(global_events).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::global_strings:
          RES_TRY_ERR(init_chunk(global_strings)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::global_string_names:
          RES_TRY_ERR(init_chunk(global_string_names)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::global_values:
          RES_TRY_ERR(
            init_chunk(global_values).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::global_value_names:
          RES_TRY_ERR(init_chunk(global_value_names)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::frame_handles:
          RES_TRY_ERR(
            init_chunk(frame_handles).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::frame_bank:
          RES_TRY_ERR(
            init_chunk(frame_bank).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::frame:
          if (frame_bank) ERROR("Frame Bank Already Exists");
          frame_bank = std::make_unique<frame::bank_t>();
          frame_bank->items.clear();
          while ((chunk_t)strm.peek_u16() == chunk_t::frame)
          {
            frame_bank->items.emplace_back();
            if (frame_bank->items.back().read(game, strm).is_err()) break;
          }
          break;

          // case chunk_t::object_bank2:
          //     RES_TRY_ERR(init_chunk(object_bank_2).map_err(APPEND_TRACE("header_t::read")));
          //     break;

        case chunk_t::object_bank:
        case chunk_t::object_bank2:
          RES_TRY_ERR(
            init_chunk(object_bank).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::image_bank:
          RES_TRY_ERR(
            init_chunk(image_bank).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::sound_bank:
          RES_TRY_ERR(
            init_chunk(sound_bank).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::music_bank:
          RES_TRY_ERR(
            init_chunk(music_bank).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::font_bank:
          RES_TRY_ERR(
            init_chunk(font_bank).map_err(APPEND_TRACE("header_t::read")));
          break;

        case chunk_t::last:
          RES_TRY_ERR(
            init_chunk(last).map_err(APPEND_TRACE("header_t::read")));
          not_finished = false;
          break;

        default:
          DEBUG("Invalid Chunk: ", (size_t)childID);
          unknown_chunks.emplace_back();
          RES_TRY_ERR(unknown_chunks.back()
                        .read(game, strm)
                        .map_err(APPEND_TRACE("header_t::read")));
          break;
      }
    }

    return lak::ok_t{};
  }

  error_t header_t::view(source_explorer_t &srcexp) const
  {
    LAK_TREE_NODE("0x%zX Game Header##%zX", (size_t)entry.ID, entry.position)
    {
      entry.view(srcexp);

      RES_TRY(title.view(srcexp, "Title", true)
                .map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(author.view(srcexp, "Author", true)
                .map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(copyright.view(srcexp, "Copyright", true)
                .map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(output_path.view(srcexp, "Output Path")
                .map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(project_path.view(srcexp, "Project Path")
                .map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        about.view(srcexp, "About").map_err(APPEND_TRACE("header_t::view")));

      RES_TRY(
        vitalise_preview.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(menu.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        extension_path.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(extensions.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        extension_data.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(additional_extensions.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));
      RES_TRY(app_doc.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        other_extension.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        extension_list.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(icon.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        demo_version.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(security.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        binary_files.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        menu_images.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(movement_extensions.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));
      RES_TRY(
        object_bank_2.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(exe.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(protection.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(shaders.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        extended_header.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(spacer.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(chunk224F.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(title2.view(srcexp).map_err(APPEND_TRACE("header_t::view")));

      RES_TRY(
        global_events.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        global_strings.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(global_string_names.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));
      RES_TRY(
        global_values.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(global_value_names.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));

      RES_TRY(
        frame_handles.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(frame_bank.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        object_bank.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(image_bank.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(sound_bank.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(music_bank.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(font_bank.view(srcexp).map_err(APPEND_TRACE("header_t::view")));

      RES_TRY(chunk2253.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(
        object_names.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(chunk2255.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(recompiled_object_properties.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));
      RES_TRY(chunk2257.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      RES_TRY(object_properties.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));
      RES_TRY(truetype_fonts_meta.view(srcexp).map_err(
        APPEND_TRACE("header_t::view")));
      RES_TRY(
        truetype_fonts.view(srcexp).map_err(APPEND_TRACE("header_t::view")));

      for (auto &unk : unknown_strings)
      {
        RES_TRY(unk.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      }

      for (auto &unk : unknown_compressed)
      {
        RES_TRY(unk.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
      }

      for (auto &unk : unknown_chunks)
      {
        RES_TRY(unk
                  .basic_view(srcexp,
                              (lak::astring("Unknown ") +
                               std::to_string(unk.entry.position))
                                .c_str())
                  .map_err(APPEND_TRACE("header_t::view")));
      }

      RES_TRY(last.view(srcexp).map_err(APPEND_TRACE("header_t::view")));
    }

    return lak::ok_t{};
  }
}

#include "encryption.cpp"
