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
#	define NOMINMAX
#endif

#include "lak/compression/deflate.hpp"
#include "lak/compression/lz4.hpp"
#include "lak/math.hpp"
#include "lak/string.hpp"
#include "lak/string_utils.hpp"
#include "lak/string_view.hpp"

#include "explorer.h"
#include "tostring.hpp"

#ifdef GetObject
#	undef GetObject
#endif

#define TRACE_EXPECTED(EXPECTED, GOT)                                         \
	lak::streamify("expected '", EXPECTED, "', got '", GOT, "'")

namespace SourceExplorer
{
	bool force_compat = false;
	encryption_table decryptor;
	std::vector<uint8_t> _magic_key;
	game_mode_t _mode = game_mode_t::_OLD;
	uint8_t _magic_char;
	std::atomic<float> game_t::completed      = 0.0f;
	std::atomic<float> game_t::bank_completed = 0.0f;

	using namespace std::string_literals;

	std::vector<uint8_t> &operator+=(std::vector<uint8_t> &lhs,
	                                 const std::vector<uint8_t> &rhs)
	{
		lhs.insert(lhs.cend(), rhs.cbegin(), rhs.cend());
		return lhs;
	}

	error_t LoadGame(source_explorer_t &srcexp)
	{
		FUNCTION_CHECKPOINT();

		DEBUG("Attempting To Load ", srcexp.exe.path);

		srcexp.state.completed      = 0.0f;
		srcexp.state.bank_completed = 0.0f;

		srcexp.state        = game_t{};
		srcexp.state.compat = force_compat;

		RES_TRY_ASSIGN(auto bytes =,
		               lak::read_file(srcexp.exe.path).MAP_ERR("LoadGame"));

		srcexp.state.file = make_data_ref_ptr(lak::move(bytes));

		data_reader_t strm(srcexp.state.file);

		DEBUG("File Size: ", srcexp.state.file->size());

		if (auto err = ParsePEHeader(strm).MAP_SE_ERR(
		      "LoadGame: while parsing PE header at: ", strm.position());
		    err.is_err())
		{
			DEBUG(err.unsafe_unwrap_err());
			DEBUG("Assuming Separate .dat File Is Being Loaded");
			TRY(strm.seek(0x0));
		}
		else
		{
			DEBUG("Successfully Parsed PE Header");
		}

		RES_TRY(ParseGameHeader(strm, srcexp.state)
		          .MAP_SE_ERR("LoadGame: while parsing game header at: ",
		                      strm.position()));

		DEBUG("Successfully Parsed Game Header");

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

		RES_TRY(srcexp.state.game.read(srcexp.state, strm)
		          .MAP_SE_ERR("LoadGame: while parsing PE header at: ",
		                      strm.position()));

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

	bool DecodeChunk(lak::span<byte_t> chunk)
	{
		if (!decryptor.valid)
		{
			if (!decryptor.init(lak::span(_magic_key).first<0x100>(), _magic_char))
				return false;
		}

		return decryptor.decode(chunk);
	}

	error_t ParsePEHeader(data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT();

		uint16_t exe_sig = strm.read_u16().UNWRAP();
		DEBUG("EXE Signature: ", exe_sig);
		if (exe_sig != WIN_EXE_SIG)
		{
			return lak::err_t{error(LINE_TRACE,
			                        error::invalid_exe_signature,
			                        TRACE_EXPECTED(WIN_EXE_SIG, exe_sig),
			                        ", at ",
			                        (strm.position() - 2))};
		}

		strm.seek(WIN_EXE_PNT).UNWRAP();
		strm.seek(strm.read_u16().UNWRAP()).UNWRAP();
		DEBUG("EXE Pointer: ", strm.position(), ", At: ", (size_t)WIN_EXE_PNT);

		int32_t pe_sig = strm.read_s32().UNWRAP();
		DEBUG("PE Signature: ", pe_sig);
		if (pe_sig != WIN_PE_SIG)
		{
			return lak::err_t{error(LINE_TRACE,
			                        error::invalid_pe_signature,
			                        TRACE_EXPECTED(WIN_PE_SIG, pe_sig),
			                        ", at ",
			                        (strm.position() - 4))};
		}

		strm.skip(2).UNWRAP();

		uint16_t num_header_sections = strm.read_u16().UNWRAP();
		DEBUG("Number Of Header Sections: ", num_header_sections);

		strm.skip(16).UNWRAP();

		const uint16_t optional_header = 0x60;
		const uint16_t data_dir        = 0x80;
		strm.skip(optional_header + data_dir).UNWRAP();
		DEBUG("Pos: ", strm.position());

		uint64_t pos = 0;
		for (uint16_t i = 0; i < num_header_sections; ++i)
		{
			SCOPED_CHECKPOINT("Section ", i + 1, "/", num_header_sections);
			size_t start = strm.position();
			auto name    = strm.read_c_str<char>().UNWRAP();
			DEBUG("Name: ", name);
			if (name == ".extra")
			{
				strm.seek(start + 0x14).UNWRAP();
				pos = strm.read_s32().UNWRAP();
				break;
			}
			else if (i >= num_header_sections - 1)
			{
				strm.seek(start + 0x10).UNWRAP();
				uint32_t size = strm.read_u32().UNWRAP();
				uint32_t addr = strm.read_u32().UNWRAP();
				DEBUG("Size: ", size);
				DEBUG("Addr: ", addr);
				pos = size + addr;
				break;
			}
			strm.seek(start + 0x28).UNWRAP();
			DEBUG("Pos: ", strm.position());
		}

		CHECK_POSITION(strm, pos); // necessary check for cast to size_t
		TRY(strm.seek(static_cast<size_t>(pos)));

		return lak::ok_t{};
	}

	error_t ParseGameHeader(data_reader_t &strm, game_t &game_state)
	{
		FUNCTION_CHECKPOINT();

		size_t pos = strm.position();

		while (true)
		{
			DEBUG("Pos: ", pos);
			strm.seek(pos).UNWRAP();

			uint16_t first_short = strm.peek_u16().UNWRAP();
			DEBUG("First Short: ", first_short);

			uint32_t pame_magic = strm.peek_u32().UNWRAP();
			DEBUG("PAME Magic: ", pame_magic);

			uint64_t pack_magic = strm.peek_u64().UNWRAP();
			DEBUG("Pack Magic: ", pack_magic);

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
				RES_TRY_ASSIGN(
				  pos =,
				  ParsePackData(strm, game_state)
				    .map_err(APPEND_TRACE("while parsing game header at: ",
				                          strm.position())));
				break;
			}
			else if (pame_magic == HEADER_UNIC)
			{
				DEBUG("New Game (ccn)");
				game_state.old_game = false;
				game_state.ccn      = true;
				game_state.state    = {};
				game_state.state.push(chunk_t::_new);
				break;
			}
			else if (first_short == 0x222C)
			{
				strm.skip(4).UNWRAP();
				strm.skip(strm.read_u32().UNWRAP()).UNWRAP();
				pos = strm.position();
			}
			else if (first_short == 0x7F7F)
			{
				pos += 8;
			}
			else
			{
				return lak::err_t{error(LINE_TRACE,
				                        error::invalid_game_signature,
				                        "Expected ",
				                        (uint16_t)chunk_t::header,
				                        "u16, ",
				                        0x222C,
				                        "u16, ",
				                        0x7F7F,
				                        "u16, ",
				                        HEADER_GAME,
				                        "u32 or ",
				                        HEADER_PACK,
				                        "u64, Found ",
				                        first_short,
				                        "u16/",
				                        pame_magic,
				                        "u32/",
				                        pack_magic,
				                        "u64")};
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

		uint32_t header = strm.read_u32().UNWRAP();
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

		game_state.runtime_version = (product_code_t)strm.read_u16().UNWRAP();
		DEBUG("Runtime Version: ", (int)game_state.runtime_version);
		if (game_state.runtime_version == product_code_t::CNCV1VER)
		{
			DEBUG("CNCV1VER");
			// cnc = true;
			// readCNC(strm);
		}
		else
		{
			game_state.runtime_sub_version = strm.read_u16().UNWRAP();
			DEBUG("Runtime Sub-Version: ", game_state.runtime_sub_version);
			game_state.product_version = strm.read_u32().UNWRAP();
			DEBUG("Product Version: ", game_state.product_version);
			game_state.product_build = strm.read_u32().UNWRAP();
			DEBUG("Product Build: ", game_state.product_build);
		}

		return lak::ok_t{};
	}

	result_t<size_t> ParsePackData(data_reader_t &strm, game_t &game_state)
	{
		FUNCTION_CHECKPOINT();

		size_t start = strm.position();
		TRY_ASSIGN(uint64_t header =, strm.read_u64());
		DEBUG("Header: ", header);

		TRY_ASSIGN(uint32_t header_size =, strm.read_u32());
		DEBUG("Header Size: ", header_size);

		TRY_ASSIGN(uint32_t data_size =, strm.read_u32());
		DEBUG("Data Size: ", data_size);

		bool unicode = false;
		if (start + data_size - header_size < strm.size())
		{
			TRY(strm.seek(start + data_size - header_size));

			TRY_ASSIGN(header =, strm.read_u32());
			DEBUG("Head: ", header);
			unicode = header == HEADER_UNIC;
			if (unicode)
			{
				DEBUG("Unicode Game");
			}
			else
			{
				DEBUG("ASCII Game");
			}
		}
		else
			WARNING("Data Size Too Large: ",
			        start + data_size - 0x20,
			        " >= ",
			        strm.remaining().size());

		TRY(strm.seek(start + 0x10));

		TRY_ASSIGN([[maybe_unused]] uint32_t format_version =, strm.read_u32());
		DEBUG("Format Version: ", format_version);

		TRY(strm.skip(0x8));

		TRY_ASSIGN(int32_t count =, strm.read_s32());
		if (count < 0)
			return lak::err_t{error(LINE_TRACE, error::invalid_pack_count)};
		DEBUG("Pack Count: ", count);

		uint64_t off = strm.position();
		DEBUG("Offset: ", off);

		for (int32_t i = 0; i < count; ++i)
		{
			if ((strm.size() - strm.position()) < 2) break;
			TRY_ASSIGN(uint32_t val =, strm.read_u16());

			if ((strm.size() - strm.position()) < val) break;
			TRY(strm.skip(val));

			if ((strm.size() - strm.position()) < 4) break;
			TRY_ASSIGN(val =, strm.read_u32());

			if ((strm.size() - strm.position()) < val) break;
			TRY(strm.skip(val));
		}

		TRY_ASSIGN(header =, strm.read_u32());
		DEBUG("Header: ", header);

		bool has_bingo = (header != HEADER_GAME) && (header != HEADER_UNIC);
		DEBUG("Has Bingo: ", (has_bingo ? "true" : "false"));

		CHECK_POSITION(strm, off); // necessary check for cast to size_t
		TRY(strm.seek(static_cast<size_t>(off)));

		game_state.pack_files.resize(count);

		for (int32_t i = 0; i < count; ++i)
		{
			SCOPED_CHECKPOINT("Pack ", i + 1, "/", count);

			TRY_ASSIGN(uint32_t read =, strm.read_u16());
			// size_t strstart = strm.position();

			DEBUG("Filename Length: ", read);
			DEBUG("Position: ", strm.position());

			auto array_to_string = [](const auto &array)
			{
				return lak::string<
				  typename lak::remove_cvref_t<decltype(array)>::value_type>(
				  array.begin(), array.end());
			};

			if (unicode)
			{
				TRY_ASSIGN(game_state.pack_files[i].filename =,
				           strm.read<char16_t>(read).map(array_to_string));
			}
			else
			{
				TRY_ASSIGN(game_state.pack_files[i].filename =,
				           strm.read<char>(read)
				             .map(array_to_string)
				             .map((lak::u16string(*)(
				               const lak::string<char> &))lak::to_u16string<char>));
			}

			// strm.seek(strstart + (unicode ? read * 2 : read));

			// DEBUG("String Start: ", strstart);
			DEBUG("Packfile '", game_state.pack_files[i].filename, "'");

			if (has_bingo)
			{
				TRY_ASSIGN(game_state.pack_files[i].bingo =, strm.read_u32());
			}
			else
				game_state.pack_files[i].bingo = 0;

			DEBUG("Bingo: ", game_state.pack_files[i].bingo);

			// if (unicode)
			//     read = strm.read_u32();
			// else
			//     read = strm.read_u16();
			TRY_ASSIGN(read =, strm.read_u32());

			DEBUG("Pack File Data Size: ", read, ", Pos: ", strm.position());
			TRY_ASSIGN(game_state.pack_files[i].data =, strm.read<byte_t>(read));
		}

		TRY_ASSIGN(header =, strm.peek_u32()); // PAMU sometimes
		DEBUG("Header: ", header);

		if (header != HEADER_GAME && header != HEADER_UNIC)
			strm.skip(0x4).UNWRAP();

		return lak::ok_t{strm.position()};
	}

	lak::color4_t ColorFrom8bitRGB(uint8_t RGB) { return {RGB, RGB, RGB, 255}; }

	lak::color4_t ColorFrom8bitA(uint8_t A) { return {255, 255, 255, A}; }

	lak::color4_t ColorFrom15bitRGB(uint16_t RGB)
	{
		return {(uint8_t)((RGB & 0x7C00) >> 7), // 0111 1100 0000 0000
		        (uint8_t)((RGB & 0x03E0) >> 2), // 0000 0011 1110 0000
		        (uint8_t)((RGB & 0x001F) << 3), // 0000 0000 0001 1111
		        255};
	}

	lak::color4_t ColorFrom16bitRGB(uint16_t RGB)
	{
		return {(uint8_t)((RGB & 0xF800) >> 8), // 1111 1000 0000 0000
		        (uint8_t)((RGB & 0x07E0) >> 3), // 0000 0111 1110 0000
		        (uint8_t)((RGB & 0x001F) << 3), // 0000 0000 0001 1111
		        255};
	}

	result_t<lak::color4_t> ColorFrom8bitI(data_reader_t &strm,
	                                       const lak::color4_t palette[256])
	{
		TRY_ASSIGN(const auto val =, strm.read_u8());
		if (palette) return lak::ok_t{palette[val]};
		return lak::ok_t{ColorFrom8bitRGB(val)};
	}

	result_t<lak::color4_t> ColorFrom15bitRGB(data_reader_t &strm)
	{
		TRY_ASSIGN(const auto val =, strm.read_u16<lak::endian::little>());
		return lak::ok_t{ColorFrom15bitRGB(val)};
	}

	result_t<lak::color4_t> ColorFrom16bitRGB(data_reader_t &strm)
	{
		TRY_ASSIGN(const auto val =, strm.read_u16<lak::endian::little>());
		return lak::ok_t{ColorFrom16bitRGB(val)};
	}

	result_t<lak::color4_t> ColorFrom24bitBGR(data_reader_t &strm)
	{
		lak::color4_t rtn;
		TRY_ASSIGN(rtn.b =, strm.read_u8());
		TRY_ASSIGN(rtn.g =, strm.read_u8());
		TRY_ASSIGN(rtn.r =, strm.read_u8());
		rtn.a = 255;
		return lak::ok_t{rtn};
	}

	result_t<lak::color4_t> ColorFrom32bitBGRA(data_reader_t &strm)
	{
		lak::color4_t rtn;
		TRY_ASSIGN(rtn.b =, strm.read_u8());
		TRY_ASSIGN(rtn.g =, strm.read_u8());
		TRY_ASSIGN(rtn.r =, strm.read_u8());
		TRY_ASSIGN(rtn.a =, strm.read_u8());
		return lak::ok_t{rtn};
	}

	result_t<lak::color4_t> ColorFrom32bitBGR(data_reader_t &strm)
	{
		lak::color4_t rtn;
		TRY_ASSIGN(rtn.b =, strm.read_u8());
		TRY_ASSIGN(rtn.g =, strm.read_u8());
		TRY_ASSIGN(rtn.r =, strm.read_u8());
		rtn.a = 255;
		TRY(strm.skip(1));
		return lak::ok_t{rtn};
	}

	result_t<lak::color4_t> ColorFrom24bitRGB(data_reader_t &strm)
	{
		lak::color4_t rtn;
		TRY_ASSIGN(rtn.r =, strm.read_u8());
		TRY_ASSIGN(rtn.g =, strm.read_u8());
		TRY_ASSIGN(rtn.b =, strm.read_u8());
		rtn.a = 255;
		return lak::ok_t{rtn};
	}

	result_t<lak::color4_t> ColorFrom32bitRGBA(data_reader_t &strm)
	{
		lak::color4_t rtn;
		TRY_ASSIGN(rtn.r =, strm.read_u8());
		TRY_ASSIGN(rtn.g =, strm.read_u8());
		TRY_ASSIGN(rtn.b =, strm.read_u8());
		TRY_ASSIGN(rtn.a =, strm.read_u8());
		return lak::ok_t{rtn};
	}

	result_t<lak::color4_t> ColorFrom32bitRGB(data_reader_t &strm)
	{
		lak::color4_t rtn;
		TRY_ASSIGN(rtn.r =, strm.read_u8());
		TRY_ASSIGN(rtn.g =, strm.read_u8());
		TRY_ASSIGN(rtn.b =, strm.read_u8());
		rtn.a = 255;
		TRY(strm.skip(1));
		return lak::ok_t{rtn};
	}

	result_t<lak::color4_t> ColorFromMode(data_reader_t &strm,
	                                      const graphics_mode_t mode,
	                                      const lak::color4_t palette[256])
	{
		switch (mode)
		{
			case graphics_mode_t::RGBA32: return ColorFrom32bitRGBA(strm);
			case graphics_mode_t::BGRA32: return ColorFrom32bitBGRA(strm);

			case graphics_mode_t::RGB24: return ColorFrom24bitRGB(strm);
			case graphics_mode_t::BGR24: return ColorFrom24bitBGR(strm);

			case graphics_mode_t::RGB16: return ColorFrom16bitRGB(strm);
			case graphics_mode_t::RGB15: return ColorFrom15bitRGB(strm);

			case graphics_mode_t::RGB8: return ColorFrom8bitI(strm, palette);

			default: return ColorFrom24bitBGR(strm);
		}
	}

	void ColorsFrom8bitRGB(lak::span<lak::color4_t> colors,
	                       lak::span<const byte_t> RGB)
	{
		ASSERT_EQUAL(colors.size(), RGB.size());
		for (size_t i = 0; i < colors.size(); ++i)
			colors[i] = ColorFrom8bitRGB(uint8_t(RGB[i]));
	}

	void ColorsFrom8bitA(lak::span<lak::color4_t> colors,
	                     lak::span<const byte_t> A)
	{
		ASSERT_EQUAL(colors.size(), A.size());
		for (size_t i = 0; i < colors.size(); ++i)
			colors[i] = ColorFrom8bitA(uint8_t(A[i]));
	}

	void MaskFrom8bitA(lak::span<lak::color4_t> colors,
	                   lak::span<const byte_t> A)
	{
		ASSERT_EQUAL(colors.size(), A.size());
		for (size_t i = 0; i < colors.size(); ++i) colors[i].a = uint8_t(A[i]);
	}

	void ColorsFrom8bitI(lak::span<lak::color4_t> colors,
	                     lak::span<const byte_t> index,
	                     lak::span<const lak::color4_t, 256> palette)
	{
		ASSERT_EQUAL(colors.size(), index.size());
		for (size_t i = 0; i < colors.size(); ++i)
			colors[i] = palette[uint8_t(index[i])];
	}

	void ColorsFrom15bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB)
	{
		ASSERT_EQUAL(colors.size() * 2, RGB.size());
		for (size_t i = 0; i < colors.size(); ++i)
			colors[i] = ColorFrom15bitRGB(uint16_t(uint8_t(RGB[(i * 2) + 0])) |
			                              uint16_t(uint8_t(RGB[(i * 2) + 1]) << 8));
	}

	void ColorsFrom16bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB)
	{
		ASSERT_EQUAL(colors.size() * 2, RGB.size());
		for (size_t i = 0; i < colors.size(); ++i)
			colors[i] = ColorFrom16bitRGB(uint16_t(uint8_t(RGB[(i * 2) + 0])) |
			                              uint16_t(uint8_t(RGB[(i * 2) + 1]) << 8));
	}

	void ColorsFrom24bitBGR(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> BGR)
	{
		ASSERT_EQUAL(colors.size() * 3, BGR.size());
		for (size_t i = 0; i < colors.size(); ++i)
		{
			colors[i].b = uint8_t(BGR[(i * 3) + 0]);
			colors[i].g = uint8_t(BGR[(i * 3) + 1]);
			colors[i].r = uint8_t(BGR[(i * 3) + 2]);
			colors[i].a = 255;
		}
	}

	void ColorsFrom32bitBGR(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> BGR)
	{
		ASSERT_EQUAL(colors.size() * 4, BGR.size());
		for (size_t i = 0; i < colors.size(); ++i)
		{
			colors[i].b = uint8_t(BGR[(i * 4) + 0]);
			colors[i].g = uint8_t(BGR[(i * 4) + 1]);
			colors[i].r = uint8_t(BGR[(i * 4) + 2]);
			colors[i].a = 255;
		}
	}

	void ColorsFrom32bitBGRA(lak::span<lak::color4_t> colors,
	                         lak::span<const byte_t> BGR)
	{
		ASSERT_EQUAL(colors.size() * 4, BGR.size());
		for (size_t i = 0; i < colors.size(); ++i)
		{
			colors[i].b = uint8_t(BGR[(i * 4) + 0]);
			colors[i].g = uint8_t(BGR[(i * 4) + 1]);
			colors[i].r = uint8_t(BGR[(i * 4) + 2]);
			colors[i].a = uint8_t(BGR[(i * 4) + 3]);
		}
	}

	void ColorsFrom24bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB)
	{
		ASSERT_EQUAL(colors.size() * 3, RGB.size());
		for (size_t i = 0; i < colors.size(); ++i)
		{
			colors[i].r = uint8_t(RGB[(i * 3) + 0]);
			colors[i].g = uint8_t(RGB[(i * 3) + 1]);
			colors[i].b = uint8_t(RGB[(i * 3) + 2]);
			colors[i].a = 255;
		}
	}

	void ColorsFrom32bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB)
	{
		ASSERT_EQUAL(colors.size() * 4, RGB.size());
		for (size_t i = 0; i < colors.size(); ++i)
		{
			colors[i].r = uint8_t(RGB[(i * 4) + 0]);
			colors[i].g = uint8_t(RGB[(i * 4) + 1]);
			colors[i].b = uint8_t(RGB[(i * 4) + 2]);
			colors[i].a = 255;
		}
	}

	void ColorsFrom32bitRGBA(lak::span<lak::color4_t> colors,
	                         lak::span<const byte_t> RGB)
	{
		ASSERT_EQUAL(colors.size() * 4, RGB.size());
		for (size_t i = 0; i < colors.size(); ++i)
		{
			colors[i].r = uint8_t(RGB[(i * 4) + 0]);
			colors[i].g = uint8_t(RGB[(i * 4) + 1]);
			colors[i].b = uint8_t(RGB[(i * 4) + 2]);
			colors[i].a = uint8_t(RGB[(i * 4) + 3]);
		}
	}

	error_t ColorsFrom8bitRGB(lak::span<lak::color4_t> colors,
	                          data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size());
		ColorsFrom8bitRGB(colors, strm.read_bytes(colors.size()).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom8bitA(lak::span<lak::color4_t> colors, data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size());
		ColorsFrom8bitA(colors, strm.read_bytes(colors.size()).UNWRAP());
		return lak::ok_t{};
	}

	error_t MaskFrom8bitA(lak::span<lak::color4_t> colors, data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size());
		MaskFrom8bitA(colors, strm.read_bytes(colors.size()).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom8bitI(lak::span<lak::color4_t> colors,
	                        data_reader_t &strm,
	                        const lak::color4_t palette[256])
	{
		if (!palette) return ColorsFrom8bitRGB(colors, strm);

		CHECK_REMAINING(strm, colors.size());
		ColorsFrom8bitI(colors,
		                strm.read_bytes(colors.size()).UNWRAP(),
		                lak::span<const lak::color4_t, 256>::from_ptr(palette));
		return lak::ok_t{};
	}

	error_t ColorsFrom15bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 2);
		ColorsFrom15bitRGB(colors, strm.read_bytes(colors.size() * 2).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom16bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 2);
		ColorsFrom16bitRGB(colors, strm.read_bytes(colors.size()).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom24bitBGR(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 3);
		ColorsFrom24bitBGR(colors, strm.read_bytes(colors.size() * 3).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom32bitBGRA(lak::span<lak::color4_t> colors,
	                            data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 4);
		ColorsFrom32bitBGRA(colors, strm.read_bytes(colors.size() * 4).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom32bitBGR(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 4);
		ColorsFrom32bitBGR(colors, strm.read_bytes(colors.size() * 4).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom24bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 3);
		ColorsFrom24bitRGB(colors, strm.read_bytes(colors.size() * 3).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom32bitRGBA(lak::span<lak::color4_t> colors,
	                            data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 4);
		ColorsFrom32bitRGBA(colors, strm.read_bytes(colors.size() * 4).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFrom32bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm)
	{
		CHECK_REMAINING(strm, colors.size() * 4);
		ColorsFrom32bitRGB(colors, strm.read_bytes(colors.size() * 4).UNWRAP());
		return lak::ok_t{};
	}

	error_t ColorsFromMode(lak::span<lak::color4_t> colors,
	                       data_reader_t &strm,
	                       const graphics_mode_t mode,
	                       const lak::color4_t palette[256])
	{
		switch (mode)
		{
			case graphics_mode_t::RGBA32: return ColorsFrom32bitRGBA(colors, strm);
			case graphics_mode_t::BGRA32: return ColorsFrom32bitBGRA(colors, strm);

			case graphics_mode_t::RGB24: return ColorsFrom24bitRGB(colors, strm);
			case graphics_mode_t::BGR24: return ColorsFrom24bitBGR(colors, strm);

			case graphics_mode_t::RGB16: return ColorsFrom16bitRGB(colors, strm);
			case graphics_mode_t::RGB15: return ColorsFrom15bitRGB(colors, strm);

			case graphics_mode_t::RGB8:
				return ColorsFrom8bitI(colors, strm, palette);

			default: return ColorsFrom24bitBGR(colors, strm);
		}
	}

	uint8_t ColorModeSize(const graphics_mode_t mode)
	{
		switch (mode)
		{
			case graphics_mode_t::RGBA32: return 4;
			case graphics_mode_t::BGRA32: return 4;

			case graphics_mode_t::RGB24: return 3;
			case graphics_mode_t::BGR24: return 3;

			case graphics_mode_t::RGB16: return 2;
			case graphics_mode_t::RGB15: return 2;

			case graphics_mode_t::RGB8: return 1;

			default: return 3;
		}
	}

	result_t<size_t> ReadRLE(data_reader_t &strm,
	                         lak::image4_t &bitmap,
	                         graphics_mode_t mode,
	                         uint16_t padding,
	                         const lak::color4_t palette[256])
	{
		FUNCTION_CHECKPOINT();

		const size_t point_size = ColorModeSize(mode);

		DEBUG("Point Size: ", point_size);
		DEBUG("Padding: ", padding);

		size_t pos = 0;
		size_t i   = 0;

		size_t start = strm.position();

		while (true)
		{
			TRY_ASSIGN(uint8_t command =, strm.read_u8());

			if (command == 0) break;

			if (command > 128)
			{
				command -= 128;
				for (uint8_t n = 0; n < command; ++n)
				{
					if ((pos++) % (bitmap.size().x + padding) < bitmap.size().x)
					{
						RES_TRY_ASSIGN(bitmap[i++] =, ColorFromMode(strm, mode, palette));
					}
					else
					{
						TRY(strm.skip(point_size));
					}
				}
			}
			else
			{
				RES_TRY_ASSIGN(lak::color4_t col =,
				               ColorFromMode(strm, mode, palette));
				for (uint8_t n = 0; n < command; ++n)
				{
					if ((pos++) % (bitmap.size().x + padding) < bitmap.size().x)
						bitmap[i++] = col;
				}
			}
		}

		return lak::ok_t{strm.position() - start};
	}

	result_t<size_t> ReadRGB(data_reader_t &strm,
	                         lak::image4_t &bitmap,
	                         graphics_mode_t mode,
	                         uint16_t padding,
	                         const lak::color4_t palette[256])
	{
		FUNCTION_CHECKPOINT();

		const size_t point_size = ColorModeSize(mode);

		DEBUG("Point Size: ", point_size);
		DEBUG("Padding: ", padding);

		size_t start = strm.position();

		CHECK_REMAINING(strm, (bitmap.size().x + padding) * bitmap.size().y);

		if (padding == 0)
		{
			ColorsFromMode(
			  lak::span(bitmap.data(), bitmap.contig_size()), strm, mode, palette)
			  .UNWRAP();
		}
		else
		{
			for (size_t y = 0; y < bitmap.size().y; ++y)
			{
				ColorsFromMode(
				  lak::span(bitmap.data() + (y * bitmap.size().x), bitmap.size().x),
				  strm,
				  mode,
				  palette)
				  .UNWRAP();
				strm.skip(padding).UNWRAP();
			}
		}

		return lak::ok_t{strm.position() - start};
	}

	result_t<size_t> ReadAlpha(data_reader_t &strm,
	                           lak::image4_t &bitmap,
	                           uint16_t padding)
	{
		FUNCTION_CHECKPOINT();

		DEBUG("Padding: ", padding);

		size_t start = strm.position();

		CHECK_REMAINING(strm, (bitmap.size().x + padding) * bitmap.size().y);

		if (padding == 0)
		{
			MaskFrom8bitA(lak::span(bitmap.data(), bitmap.contig_size()), strm)
			  .UNWRAP();
		}
		else
		{
			for (size_t y = 0; y < bitmap.size().y; ++y)
			{
				MaskFrom8bitA(
				  lak::span(bitmap.data() + (y * bitmap.size().x), bitmap.size().x),
				  strm)
				  .UNWRAP();
				strm.skip(padding).UNWRAP();
			}
		}

		return lak::ok_t{strm.position() - start};
	}

	void ReadTransparent(const lak::color4_t &transparent, lak::image4_t &bitmap)
	{
		FUNCTION_CHECKPOINT();

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
		FUNCTION_CHECKPOINT();

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
			// return std::monostate{};
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
			case chunk_t::shaders2: return "Shaders 2";
			case chunk_t::extended_header: return "Extended Header";
			case chunk_t::spacer: return "Spacer";
			case chunk_t::frame_bank: return "Frame Bank";
			case chunk_t::chunk224F: return "Chunk 224F";
			case chunk_t::title2: return "Title2";

			case chunk_t::chunk2253: return "Chunk 2253";
			case chunk_t::object_names: return "Object Names";
			case chunk_t::chunk2255: return "Chunk 2255 (Empty?)";
			case chunk_t::two_five_plus_object_properties:
				return "Object Properties (2.5+)";
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

	result_t<data_ref_span_t> Decode(data_ref_span_t encoded,
	                                 chunk_t id,
	                                 encoding_t mode)
	{
		FUNCTION_CHECKPOINT();

		switch (mode)
		{
			case encoding_t::mode3:
			case encoding_t::mode2: return Decrypt(encoded, id, mode);
			case encoding_t::mode1:
				return lak::ok_t{
				  lak::ok_or_err(Inflate(encoded, false, false)
				                   .map_err([&](auto &&) { return encoded; }))};
			default:
				if (encoded.size() > 0 && uint8_t(encoded[0]) == 0x78)
					return lak::ok_t{
					  lak::ok_or_err(Inflate(encoded, false, false)
					                   .map_err([&](auto &&) { return encoded; }))};
				else
					return lak::ok_t{encoded};
		}
	}

	result_t<data_ref_span_t> Inflate(data_ref_span_t compressed,
	                                  bool skip_header,
	                                  bool anaconda,
	                                  size_t max_size)
	{
		FUNCTION_CHECKPOINT();

		lak::array<byte_t, 0x8000> buffer;
		auto inflater =
		  lak::deflate_iterator(compressed, buffer, !skip_header, anaconda);

		lak::array<byte_t> output;
		output.reserve(buffer.size());
		if (auto err = inflater.read(
		      [&](lak::span<byte_t> v)
		      {
			      bool hit_max = output.size() + v.size() > max_size;
			      if (hit_max) v = v.first(max_size - output.size());
			      output.reserve(output.size() + v.size());
			      for (const byte_t &b : v) output.push_back(b);
			      if (hit_max) DEBUG("Hit Max");
			      return !hit_max;
		      });
		    err.is_ok())
		{
			return lak::ok_t{make_data_ref_ptr(compressed, lak::move(output))};
		}
// #ifndef NDEBUG
#if 0
		else if (err.unsafe_unwrap_err() ==
		         lak::deflate_iterator::error_t::corrupt_stream)
		{
			auto [bytes_read, bits_read] = inflater.bytes_read();
			ERROR("Corrupt Stream. Bytes Read: ",
			      bytes_read,
			      ", Bits Read: ",
			      bits_read);
			return lak::ok_t{make_data_ref_ptr(compressed, lak::move(output))};
		}
#endif
		else if (err.unsafe_unwrap_err() == lak::deflate_iterator::error_t::ok)
		{
			// This is not (always) an error, we may intentionally stop the decode
			// early to not waste time and memory.

			CHECKPOINT();
			return lak::ok_t{make_data_ref_ptr(compressed, lak::move(output))};
		}
		else
		{
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

	result_t<data_ref_span_t> LZ4Decode(data_ref_span_t compressed,
	                                    unsigned int out_size)
	{
		FUNCTION_CHECKPOINT();

		lak::binary_reader reader(compressed);

		return lak::decode_lz4_block(reader, out_size)
		  .map_err(lak::lz4_error_name)
		  .map(
		    [&](auto &&decompressed)
		    {
			    return data_ref_span_t(
			      make_data_ref_ptr(compressed, lak::move(decompressed)));
		    })
		  .MAP_ERRCODE(error::inflate_failed);
	}

	result_t<data_ref_span_t> LZ4DecodeReadSize(data_ref_span_t compressed)
	{
		// intentionally doing this outside of the LZ4Decode call incase strm
		// copies first (?)
		data_reader_t strm(compressed);
		TRY_ASSIGN(const uint32_t out_size =, strm.read_u32());
		return LZ4Decode(strm.read_remaining_ref_span(), out_size);
	}

	result_t<data_ref_span_t> StreamDecompress(data_reader_t &strm,
	                                           unsigned int out_size)
	{
		FUNCTION_CHECKPOINT();

		lak::array<byte_t, 0x8000> buffer;
		auto inflater = lak::deflate_iterator(strm.remaining(),
		                                      buffer,
		                                      /* parse_header */ false,
		                                      /* anaconda */ true);

		lak::array<byte_t> output;
		if (auto err = inflater.read(
		      [&](lak::span<byte_t> v)
		      {
			      output.reserve(output.size() + v.size());
			      for (const byte_t &b : v) output.push_back(b);
			      return true;
		      });
		    err.is_ok())
		{
			const auto offset = strm.position();
			const auto bytes_read =
			  inflater.compressed().begin() - strm.remaining().begin();
			ASSERT_GREATER_OR_EQUAL(bytes_read, 0);
			strm.skip(bytes_read).UNWRAP();
			return lak::ok_t{make_data_ref_ptr(
			  strm._source, offset, bytes_read, lak::move(output))};
		}
		else
		{
			DEBUG("Out Size: ", out_size);
			DEBUG("Final? ", (inflater.is_final_block() ? "True" : "False"));
			return lak::err_t{
			  error(LINE_TRACE,
			        error::inflate_failed,
			        "Failed To Inflate (",
			        lak::deflate_iterator::error_name(err.unsafe_unwrap_err()),
			        ")")};
		}
	}

	result_t<data_ref_span_t> Decrypt(data_ref_span_t encrypted,
	                                  chunk_t ID,
	                                  encoding_t mode)
	{
		FUNCTION_CHECKPOINT();

		data_reader_t estrm(encrypted);

		if (mode == encoding_t::mode3)
		{
			if (encrypted.size() <= 4)
				return lak::err_t{
				  error(LINE_TRACE,
				        error::decrypt_failed,
				        "MODE 3 Decryption Failed: Encrypted Buffer Too Small")};
			// TODO: check endian
			// size_t dataLen = *reinterpret_cast<const uint32_t*>(&encrypted[0]);
			TRY(estrm.skip(4));

			auto mem_ptr  = estrm.copy_remaining();
			auto mem_span = lak::span<byte_t>(mem_ptr->get());

			data_reader_t mem_reader(mem_ptr);

			if ((_mode != game_mode_t::_284) && ((uint16_t)ID & 0x1) != 0)
				(uint8_t &)(mem_span[0]) ^=
				  ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

			if (DecodeChunk(mem_span))
			{
				if (mem_reader.remaining().size() <= 4)
					return lak::err_t{
					  error(LINE_TRACE,
					        error::decrypt_failed,
					        "MODE 3 Decryption Failed: Decoded Chunk Too Small")};
				// dataLen = *reinterpret_cast<uint32_t*>(&mem[0]);

				TRY(mem_reader.skip(4));

				auto rem_span = mem_reader.read_remaining_ref_span();

				// Try Inflate even if it doesn't need to be
				return lak::ok_t{lak::ok_or_err(
				  Inflate(rem_span, false, false)
				    .map_err([&](auto &&) { return rem_span; })
				    .if_err([](auto ref_span) { DEBUG("Size: ", ref_span.size()); }))};
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

			auto mem_ptr  = estrm.copy_remaining();
			auto mem_span = lak::span<byte_t>(mem_ptr->get());

			if ((_mode != game_mode_t::_284) && (uint16_t)ID & 0x1)
				(uint8_t &)(mem_span[0]) ^=
				  ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

			if (!DecodeChunk(mem_span))
			{
				if (mode == encoding_t::mode2)
				{
					return lak::err_t{error(
					  LINE_TRACE, error::decrypt_failed, "MODE 2 Decryption Failed")};
				}
			}

			return lak::ok_t{mem_ptr};
		}
	}

	result_t<frame::item_t &> GetFrame(game_t &game, uint16_t handle)
	{
		if (!game.game.frame_bank)
			return lak::err_t{error(LINE_TRACE, u8"No Frame Bank")};

		if (!game.game.frame_handles)
			return lak::err_t{error(LINE_TRACE, u8"No Frame Handles")};

		if (handle >= game.game.frame_handles->handles.size())
			return lak::err_t{error(LINE_TRACE, u8"Frame Handle Out Of Range")};

		handle = game.game.frame_handles->handles[handle];

		if (handle >= game.game.frame_bank->items.size())
			return lak::err_t{error(LINE_TRACE, u8"Frame Bank Handle Out Of Range")};

		return lak::ok_t{game.game.frame_bank->items[handle]};
	}

	result_t<object::item_t &> GetObject(game_t &game, uint16_t handle)
	{
		if (!game.game.object_bank)
			return lak::err_t{error(LINE_TRACE, u8"No Object Bank")};

		auto iter = game.object_handles.find(handle);

		if (iter == game.object_handles.end())
			return lak::err_t{error(LINE_TRACE, u8"Invalid Object Handle")};

		if (iter->second >= game.game.object_bank->items.size())
			return lak::err_t{
			  error(LINE_TRACE, u8"Object Bank Handle Out Of Range")};

		return lak::ok_t{game.game.object_bank->items[iter->second]};
	}

	result_t<image::item_t &> GetImage(game_t &game, uint32_t handle)
	{
		if (!game.game.image_bank)
			return lak::err_t{error(LINE_TRACE, u8"No Image Bank")};

		auto iter = game.image_handles.find(handle);

		if (iter == game.image_handles.end())
			return lak::err_t{error(LINE_TRACE, u8"Invalid Image Handle")};

		if (iter->second >= game.game.image_bank->items.size())
			return lak::err_t{error(LINE_TRACE, u8"Image Bank Handle Out Of Range")};

		return lak::ok_t{game.game.image_bank->items[iter->second]};
	}

	result_t<data_ref_span_t> data_point_t::decode(const chunk_t ID,
	                                               const encoding_t mode) const
	{
		return Decode(data, ID, mode);
	}

	error_t chunk_entry_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("chunk_entry_t::");

		const auto strm_ref_span = strm.peek_remaining_ref_span();

		const auto start = strm.position();

		old = game.old_game;
		TRY_ASSIGN(ID = (chunk_t), strm.read_u16());
		TRY_ASSIGN(mode = (encoding_t), strm.read_u16());

		DEBUG("Type: ", GetTypeString(ID), " (", uintmax_t(ID), ")");
		if (GetTypeString(ID) == lak::astring("INVALID"))
			WARNING("Invalid Type Detected (", uintmax_t(ID), ")");
		DEBUG("Mode: ", (uint16_t)mode);
		DEBUG("Position: ", start);
		DEBUG("Position: ", strm_ref_span.position().UNWRAP());
		DEBUG("Root Position: ", strm_ref_span.root_position().UNWRAP());

		if ((mode == encoding_t::mode2 || mode == encoding_t::mode3) &&
		    _magic_key.size() < 256)
			GetEncryptionKey(game);

		TRY_ASSIGN(const auto chunk_size =, strm.read_u32());
		const auto chunk_data_end = strm.position() + chunk_size;

		CHECK_REMAINING(strm, chunk_size);

		if (mode == encoding_t::mode1)
		{
			TRY_ASSIGN(body.expected_size =, strm.read_u32());

			if (game.old_game)
			{
				if (chunk_size > 4)
				{
					TRY_ASSIGN(body.data =, strm.read_ref_span(chunk_size - 4));
				}
				else
					body.data.reset();
			}
			else
			{
				TRY_ASSIGN(const auto data_size =, strm.read_u32());

				TRY_ASSIGN(body.data =, strm.read_ref_span(data_size));

				if (strm.position() > chunk_data_end)
				{
					ERROR("Read Too Much Data");
				}
				else if (strm.position() < chunk_data_end)
				{
					ERROR("Read Too Little Data");
				}

				TRY(strm.seek(chunk_data_end));
			}
		}
		else
		{
			body.expected_size = 0;
			TRY_ASSIGN(body.data =, strm.read_ref_span(chunk_size));
		}

		const auto size = strm.position() - start;
		strm.seek(start).UNWRAP();
		ref_span = strm.read_ref_span(size).UNWRAP();
		DEBUG("Ref Span Size: ", ref_span.size());

		return lak::ok_t{};
	}

	void chunk_entry_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("Entry Information##%zX", position())
		{
			if (old)
				ImGui::Text("Old Entry");
			else
				ImGui::Text("New Entry");
			ImGui::Text("Position: 0x%zX", position());
			ImGui::Text("Size: 0x%zX", ref_span.size());

			ImGui::Text("ID: 0x%zX", (size_t)ID);
			ImGui::Text("Mode: MODE%zu", (size_t)mode);

			ImGui::Text("Head Position: 0x%zX", head.position());
			ImGui::Text("Head Expected Size: 0x%zX", head.expected_size);
			ImGui::Text("Head Size: 0x%zX", head.data.size());

			ImGui::Text("Body Position: 0x%zX", body.position());
			ImGui::Text("Body Expected Size: 0x%zX", body.expected_size);
			ImGui::Text("Body Size: 0x%zX", body.data.size());
		}

		if (ImGui::Button("View Memory")) srcexp.view = this;
	}

	error_t item_entry_t::read(game_t &game,
	                           data_reader_t &strm,
	                           bool compressed,
	                           size_t header_size,
	                           bool has_handle)
	{
		FUNCTION_CHECKPOINT("item_entry_t::");

		DEBUG("Compressed: ", compressed);
		DEBUG("Header Size: ", header_size);

		const auto start = strm.position();

		old  = game.old_game;
		mode = encoding_t::mode0;
		if (has_handle)
		{
			TRY_ASSIGN(handle =, strm.read_u32());
		}
		else
			handle = 0xFF'FF'FF'FF;
		DEBUG("Handle: ", handle);

		TRY_ASSIGN(const uint32_t peekaboo =, strm.peek_u32());
		const bool new_item = peekaboo == 0xFF'FF'FF'FF;
		if (new_item)
		{
			DEBUG(LAK_BRIGHT_YELLOW "New Item" LAK_SGR_RESET);
			mode       = encoding_t::mode4;
			compressed = false;
		}

		if (!game.old_game && header_size > 0)
		{
			TRY_ASSIGN(head.data =, strm.read_ref_span(header_size));
			head.expected_size = 0;
		}

		if (game.old_game || compressed)
		{
			TRY_ASSIGN(body.expected_size =, strm.read_u32());
		}
		else
			body.expected_size = 0;
		DEBUG("Body Expected Size: ", body.expected_size);

		size_t data_size = 0;
		if (game.old_game)
		{
			const size_t old_start = strm.position();
			// Figure out exactly how long the compressed data is
			// :TODO: It should be possible to figure this out without
			// actually decompressing it... surely...
			RES_TRY_ASSIGN(
			  const auto raw =,
			  StreamDecompress(strm, static_cast<unsigned int>(body.expected_size))
			    .MAP_SE_ERR("item_entry_t::read"));
			if (raw.size() != body.expected_size)
			{
				WARNING("Actual decompressed size (",
				        raw.size(),
				        ") was not equal to the expected size (",
				        body.expected_size,
				        ").");
			}
			data_size = strm.position() - old_start;
			strm.seek(old_start).UNWRAP();
			DEBUG("Data Size: ", data_size);
		}
		else if (!new_item)
		{
			DEBUG("Pos: ", strm.position());
			TRY_ASSIGN(data_size =, strm.read_u32());
			DEBUG("Data Size: ", data_size);
		}

		TRY_ASSIGN(body.data =, strm.read_ref_span(data_size));
		DEBUG("Data Size: ", body.data.size());

		// hack because one of MMF1.5 or tinf_uncompress is a bitch
		if (game.old_game) mode = encoding_t::mode1;

		const auto size = strm.position() - start;
		strm.seek(start).UNWRAP();
		ref_span = strm.read_ref_span(size).UNWRAP();
		DEBUG("Ref Span Size: ", ref_span.size());

		return lak::ok_t{};
	}

	void item_entry_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("Entry Information##%zX", position())
		{
			if (old)
				ImGui::Text("Old Entry");
			else
				ImGui::Text("New Entry");
			ImGui::Text("Position: 0x%zX", position());
			ImGui::Text("Size: 0x%zX", ref_span.size());

			ImGui::Text("Handle: 0x%zX", (size_t)handle);

			ImGui::Text("Head Position: 0x%zX", head.position());
			ImGui::Text("Head Expected Size: 0x%zX", head.expected_size);
			ImGui::Text("Head Size: 0x%zX", head.data.size());

			ImGui::Text("Body Position: 0x%zX", body.position());
			ImGui::Text("Body Expected Size: 0x%zX", body.expected_size);
			ImGui::Text("Body Size: 0x%zX", body.data.size());
		}

		if (ImGui::Button("View Memory")) srcexp.view = this;
	}

	result_t<data_ref_span_t> basic_entry_t::decode_body(size_t max_size) const
	{
		FUNCTION_CHECKPOINT("basic_entry_t::");

		if (old)
		{
			switch (mode)
			{
				case encoding_t::mode0: return lak::ok_t{body.data};

				case encoding_t::mode1:
				{
					data_reader_t reader(body.data);
					TRY_ASSIGN(const uint8_t magic =, reader.read_u8());
					TRY_ASSIGN(const uint16_t len =, reader.read_u16());
					if (magic == 0x0F && (size_t)len == body.expected_size)
					{
						auto result = reader.read_remaining_ref_span(max_size);
						DEBUG("Size: ", result.size());
						return lak::ok_t{result};
					}
					else
					{
						return Inflate(body.data,
						               true,
						               true,
						               std::min(body.expected_size, max_size))
						  .MAP_SE_ERR("MODE1 Failed To Inflate")
						  .if_ok([](const auto &ref_span)
						         { DEBUG("Size: ", ref_span.size()); });
					}
				}

				case encoding_t::mode2:
					return lak::err_t{error(LINE_TRACE, error::no_mode2_decoder)};

				case encoding_t::mode3:
					return lak::err_t{error(LINE_TRACE, error::no_mode3_decoder)};

				default: return lak::err_t{error(LINE_TRACE, error::invalid_mode)};
			}
		}
		else
		{
			switch (mode)
			{
				case encoding_t::mode4:
				{
					return LZ4DecodeReadSize(body.data)
					  .MAP_SE_ERR("LZ4 Decode Failed")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode3: [[fallthrough]];
				case encoding_t::mode2:
				{
					return Decrypt(body.data, ID, mode)
					  .MAP_SE_ERR("MODE2/3 Failed To Decrypt")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode1:
				{
					return Inflate(body.data, false, false, max_size)
					  .MAP_SE_ERR("MODE1 Failed To Inflate")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode0: [[fallthrough]];
				default:
				{
					if (body.data.size() > 0 && uint8_t(body.data[0]) == 0x78)
					{
						return lak::ok_t{lak::ok_or_err(
						  Inflate(body.data, false, false, max_size)
						    .if_ok(
						      [](const auto &ref_span)
						      {
							      if (ref_span.size() == 0)
								      WARNING("Inflated Data Was Empty");
							      DEBUG("Size: ", ref_span.size());
						      })
						    .map_err(
						      [this](const auto &err)
						      {
							      WARNING("Guess MODE1 Failed To Inflate: ", err);
							      DEBUG("Size: ", body.data.size());
							      return body.data;
						      }))};
					}
					else
					{
						return lak::ok_t{body.data};
					}
				}
			}
		}
	}

	result_t<data_ref_span_t> basic_entry_t::decode_head(size_t max_size) const
	{
		FUNCTION_CHECKPOINT("basic_entry_t::");

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
				default: return lak::err_t{error(LINE_TRACE, error::invalid_mode)};
			}
		}
		else
		{
			switch (mode)
			{
				case encoding_t::mode3:
				case encoding_t::mode2:
				{
					// :TODO: this was originally body not head, check that this change
					// is correct.
					return Decrypt(head.data, ID, mode)
					  .MAP_SE_ERR("MODE2/3 Failed To Decrypt")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode1:
				{
					return Inflate(head.data, false, false, max_size)
					  .MAP_SE_ERR("MODE1 Failed To Inflate")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode4: [[fallthrough]];
				case encoding_t::mode0: [[fallthrough]];
				default:
				{
					if (head.data.size() > 0 && uint8_t(head.data[0]) == 0x78)
					{
						return lak::ok_t{lak::ok_or_err(
						  Inflate(head.data, false, false, max_size)
						    .if_ok(
						      [](const auto &ref_span)
						      {
							      if (ref_span.size() == 0)
								      WARNING("Inflated Data Was Empty");
							      DEBUG("Size: ", ref_span.size());
						      })
						    .map_err(
						      [this](const auto &err)
						      {
							      WARNING("Guess MODE1 Failed To Inflate: ", err);
							      DEBUG("Size: ", head.data.size());
							      return head.data;
						      }))};
					}
					else
					{
						return lak::ok_t{head.data};
					}
				}
			}
		}
	}

	const data_ref_span_t &basic_entry_t::raw_body() const { return body.data; }

	const data_ref_span_t &basic_entry_t::raw_head() const { return head.data; }

	result_t<std::u16string> ReadStringEntry(game_t &game,
	                                         const chunk_entry_t &entry)
	{
		if (game.old_game)
		{
			switch (entry.mode)
			{
				case encoding_t::mode0: [[fallthrough]];
				case encoding_t::mode1:
					return entry.decode_body()
					  .map(
					    [&](const data_ref_span_t &ref_span)
					    {
						    data_reader_t reader(ref_span);
						    return lak::to_u16string(reader.read_any_c_str<char>());
					    })
					  .MAP_SE_ERR("Invalid String Chunk: ",
					              (int)entry.mode,
					              ", Chunk: ",
					              (int)entry.ID);

				default:
					return lak::err_t{error(LINE_TRACE,
					                        error::invalid_mode,
					                        "Invalid String Mode: ",
					                        (int)entry.mode,
					                        ", Chunk: ",
					                        (int)entry.ID)};
			}
		}
		else
		{
			return entry.decode_body()
			  .map(
			    [&](const data_ref_span_t &ref_span)
			    {
				    data_reader_t reader(ref_span);
				    if (game.unicode)
					    return reader.read_any_c_str<char16_t>();
				    else
					    return lak::to_u16string(reader.read_any_c_str<char>());
			    })
			  .MAP_SE_ERR("Invalid String Chunk: ",
			              (int)entry.mode,
			              ", Chunk: ",
			              (int)entry.ID);
		}
	}

	error_t basic_chunk_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("basic_chunk_t::");
		error_t result = entry.read(game, strm);
		return result;
	}

	error_t basic_chunk_t::basic_view(source_explorer_t &srcexp,
	                                  const char *name) const
	{
		LAK_TREE_NODE("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position())
		{
			entry.view(srcexp);
		}

		return lak::ok_t{};
	}

	error_t basic_item_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("basic_item_t::");
		error_t result = entry.read(game, strm, true);
		return result;
	}

	error_t basic_item_t::basic_view(source_explorer_t &srcexp,
	                                 const char *name) const
	{
		LAK_TREE_NODE("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position())
		{
			entry.view(srcexp);
		}

		return lak::ok_t{};
	}

	error_t string_chunk_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("string_chunk_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("string_chunk_t::read"));

		TRY_SE_ASSIGN(value =, ReadStringEntry(game, entry));

		DEBUG(LAK_YELLOW "Value: \"", lak::to_astring(value), "\"" LAK_SGR_RESET);

		return lak::ok_t{};
	}

	error_t string_chunk_t::view(source_explorer_t &srcexp,
	                             const char *name,
	                             const bool preview) const
	{
		lak::astring str = "'" + astring() + "'";

		LAK_TREE_NODE("0x%zX %s %s##%zX",
		              (size_t)entry.ID,
		              name,
		              preview ? str.c_str() : "",
		              entry.position())
		{
			entry.view(srcexp);
			ImGui::Text("String: %s", str.c_str());
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

	error_t strings_chunk_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("strings_chunk_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("string_chunk_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().MAP_SE_ERR("string_chunk_t::read"));

		data_reader_t sstrm(span);

		while (!sstrm.empty())
		{
			auto str = sstrm.read_any_c_str<char16_t>();
			DEBUG("    Read String (Len ", str.size(), ")");
			if (!str.size()) break;
			values.emplace_back(lak::move(str));
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
		              entry.position())
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
		  "0x%zX Unknown Compressed##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			if (ImGui::Button("View Compressed"))
			{
				RES_TRY_ASSIGN(
				  auto span =,
				  entry.decode_body().MAP_SE_ERR("compressed_chunk_t::view"));

				data_reader_t strm(span);

				TRY(strm.skip(8));

				Inflate(strm.read_remaining_ref_span(), false, false)
				  .if_ok([&](auto ref_span) { srcexp.buffer = ref_span; })
				  .IF_ERR("Inflate Failed")
				  .discard();
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

	error_t icon_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("icon_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("icon_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().MAP_SE_ERR("icon_t::read"));

		data_reader_t dstrm(span);

		TRY_ASSIGN(const auto data_begin =, dstrm.peek_u32());
		TRY(dstrm.seek(data_begin));

		std::vector<lak::color4_t> palette;
		palette.resize(16 * 16);

		for (auto &point : palette)
		{
			TRY_ASSIGN(point.b =, dstrm.read_u8());
			TRY_ASSIGN(point.g =, dstrm.read_u8());
			TRY_ASSIGN(point.r =, dstrm.read_u8());
			// TRY_ASSIGN(point.a =, dstrm.read_u8());
			point.a = 0xFF;
			TRY(dstrm.skip(1));
		}

		bitmap.resize({16, 16});

		CHECK_REMAINING(dstrm, bitmap.contig_size());

		for (size_t y = 0; y < bitmap.size().y; ++y)
		{
			for (size_t x = 0; x < bitmap.size().x; ++x)
			{
				bitmap[{x, (bitmap.size().y - 1) - y}] =
				  palette[dstrm.read_u8().UNWRAP()];
			}
		}

		CHECK_REMAINING(dstrm, bitmap.contig_size() / 8);

		for (size_t i = 0; i < bitmap.contig_size() / 8; ++i)
		{
			uint8_t mask = dstrm.read_u8().UNWRAP();
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
		LAK_TREE_NODE("0x%zX Icon##%zX", (size_t)entry.ID, entry.position())
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

	error_t binary_file_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("binary_file_t::");

		TRY_ASSIGN(const auto str_len =, strm.read_u16());

		auto to_str = [](auto &&arr)
		{ return lak::to_u8string(lak::string_view(lak::span(arr))); };
		auto set_name = [&](auto &&str) { name = lak::move(str); };
		if (game.unicode)
		{
			TRY(strm.read<char16_t>(str_len).map(to_str).if_ok(set_name));
		}
		else
		{
			TRY(strm.read<char>(str_len).map(to_str).if_ok(set_name));
		}

		TRY_ASSIGN(const auto data_len =, strm.read_u32());

		TRY_ASSIGN(data =, strm.read_ref_span(data_len));

		return lak::ok_t{};
	}

	error_t binary_file_t::view(source_explorer_t &) const
	{
		auto str = lak::as_astring(name);

		LAK_TREE_NODE("%s", str.data())
		{
			ImGui::Text("Name: %s", str.data());
			ImGui::Text("Data Size: 0x%zX", data.size());
		}

		return lak::ok_t{};
	}

	error_t binary_files_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("binary_files_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("binary_files_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().MAP_SE_ERR("binary_files_t::read"));

		data_reader_t bstrm(span);

		TRY_ASSIGN(const auto item_count =, bstrm.read_u32());

		items.resize(item_count);
		for (auto &item : items)
		{
			RES_TRY(item.read(game, bstrm).MAP_SE_ERR("binary_files_t::read"));
		}

		return lak::ok_t{};
	}

	error_t binary_files_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE(
		  "0x%zX Binary Files##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			int index = 0;
			for (const auto &item : items)
			{
				ImGui::PushID(index++);
				DEFER(ImGui::PopID());
				RES_TRY(item.view(srcexp).MAP_SE_ERR("binary_files_t::view"));
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

	error_t extended_header_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("extended_header_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("extended_header_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().MAP_SE_ERR("extended_header_t::read"));

		data_reader_t estrm(span);

		TRY_ASSIGN(flags =, estrm.read_u32());
		TRY_ASSIGN(build_type =, estrm.read_u32());
		TRY_ASSIGN(build_flags =, estrm.read_u32());
		TRY_ASSIGN(screen_ratio_tolerance =, estrm.read_u16());
		TRY_ASSIGN(screen_angle =, estrm.read_u16());

		game.compat |= build_type >= 0x10000000;

		return lak::ok_t{};
	}

	error_t extended_header_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE(
		  "0x%zX Extended Header##%zX", (size_t)entry.ID, entry.position())
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

	error_t chunk_2253_item_t::read(game_t &, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("chunk2253_t::");

		CHECK_REMAINING(strm, 16U);

		position = strm.position();
		DEFER(strm.seek(position + 16).UNWRAP());

		TRY_ASSIGN(ID =, strm.read_u16());

		return lak::ok_t{};
	}

	error_t chunk_2253_item_t::view(source_explorer_t &) const
	{
		ImGui::Text("ID: 0x%zX", size_t(ID));

		return lak::ok_t{};
	}

	error_t chunk_2253_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("chunk2253_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("chunk_2253_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().MAP_SE_ERR("chunk_2253_t::read"));

		data_reader_t cstrm(span);

		while (cstrm.remaining().size() >= 16)
		{
			const size_t pos = cstrm.position();
			items.emplace_back();
			RES_TRY(items.back().read(game, cstrm).MAP_SE_ERR("chunk_2253_t::read"));
			if (cstrm.position() == pos) break;
		}

		if (!cstrm.empty()) WARNING("Data Left Over");

		return lak::ok_t{};
	}

	error_t chunk_2253_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Chunk 2253 (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(srcexp);

			size_t i = 0;
			for (const auto &item : items)
			{
				LAK_TREE_NODE("0x%zX Item##%zX", (size_t)item.ID, i++)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("chunk_2253_t::view"));
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

	error_t two_five_plus_object_properties_t::read(game_t &game,
	                                                data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("two_five_plus_object_properties_t::");

		game.two_five_plus_game = true;

		RES_TRY(entry.read(game, strm)
		          .MAP_SE_ERR("two_five_plus_object_properties_t::read"));

		data_reader_t reader(entry.raw_body());

		while (!reader.empty())
		{
			items.emplace_back();
			RES_TRY(items.back()
			          .read(game, reader, true, 0, false)
			          .MAP_SE_ERR("two_five_plus_object_properties_t::read"));
		}

		return lak::ok_t{};
	}

	error_t two_five_plus_object_properties_t::view(
	  source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Object Properties (2.5+) (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(srcexp);

			for (const auto &item : items)
			{
				LAK_TREE_NODE("Properties##%zX", item.position())
				{
					item.view(srcexp);
				}
			}
		}

		return lak::ok_t{};
	}

	error_t chunk_2257_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Chunk 2257");
	}

	error_t object_properties_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("object_properties_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("object_properties_t::read"));

		data_reader_t reader(entry.raw_body());

		while (!reader.empty())
		{
			items.emplace_back();
			RES_TRY(items.back()
			          .read(game, reader, false)
			          .MAP_SE_ERR("object_properties_t::read"));
		}

		return lak::ok_t{};
	}

	error_t object_properties_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Object Properties (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(srcexp);

			for (const auto &item : items)
			{
				LAK_TREE_NODE(
				  "0x%zX Properties##%zX", (size_t)item.ID, item.position())
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

	error_t truetype_fonts_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("truetype_fonts_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("truetype_fonts_t::read"));

		data_reader_t reader(entry.raw_body());

		for (size_t i = 0; !reader.empty(); ++i)
		{
			DEBUG("Font ", i);
			items.emplace_back();
			RES_TRY(items.back()
			          .read(game, reader, false)
			          .MAP_SE_ERR("truetype_fonts_t::read"));
		}

		return lak::ok_t{};
	}

	error_t truetype_fonts_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX TrueType Fonts (%zu Items)##%zX",
		              (size_t)entry.ID,
		              items.size(),
		              entry.position())
		{
			entry.view(srcexp);

			for (const auto &item : items)
			{
				LAK_TREE_NODE("0x%zX Font##%zX", (size_t)item.ID, item.position())
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

		error_t shape_t::read(game_t &, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::shape_t::");

			TRY_ASSIGN(border_size =, strm.read_u16());
			TRY_ASSIGN(border_color.r =, strm.read_u8());
			TRY_ASSIGN(border_color.g =, strm.read_u8());
			TRY_ASSIGN(border_color.b =, strm.read_u8());
			TRY_ASSIGN(border_color.a =, strm.read_u8());
			TRY_ASSIGN(shape = (shape_type_t), strm.read_u16());
			TRY_ASSIGN(fill = (fill_type_t), strm.read_u16());

			line     = line_flags_t::none;
			gradient = gradient_flags_t::horizontal;
			handle   = 0xFFFF;

			if (shape == shape_type_t::line)
			{
				TRY_ASSIGN(line = (line_flags_t), strm.read_u16());
			}
			else if (fill == fill_type_t::solid)
			{
				TRY_ASSIGN(color1.r =, strm.read_u8());
				TRY_ASSIGN(color1.g =, strm.read_u8());
				TRY_ASSIGN(color1.b =, strm.read_u8());
				TRY_ASSIGN(color1.a =, strm.read_u8());
			}
			else if (fill == fill_type_t::gradient)
			{
				TRY_ASSIGN(color1.r =, strm.read_u8());
				TRY_ASSIGN(color1.g =, strm.read_u8());
				TRY_ASSIGN(color1.b =, strm.read_u8());
				TRY_ASSIGN(color1.a =, strm.read_u8());
				TRY_ASSIGN(color2.r =, strm.read_u8());
				TRY_ASSIGN(color2.g =, strm.read_u8());
				TRY_ASSIGN(color2.b =, strm.read_u8());
				TRY_ASSIGN(color2.a =, strm.read_u8());
				TRY_ASSIGN(gradient = (gradient_flags_t), strm.read_u16());
			}
			else if (fill == fill_type_t::motif)
			{
				TRY_ASSIGN(handle =, strm.read_u16());
			}

			return lak::ok_t{};
		}

		error_t shape_t::view(source_explorer_t &) const
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

		error_t quick_backdrop_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::quick_backdrop_t::");

			RES_TRY(
			  entry.read(game, strm).MAP_SE_ERR("object::quick_backdrop_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().MAP_SE_ERR("object::quick_backdrop_t::read"));

			data_reader_t qstrm(span);

			TRY_ASSIGN(size =, qstrm.read_u32());
			TRY_ASSIGN(obstacle =, qstrm.read_u16());
			TRY_ASSIGN(collision =, qstrm.read_u16());
			if (game.old_game)
			{
				TRY_ASSIGN(dimension.x =, qstrm.read_u16());
				TRY_ASSIGN(dimension.y =, qstrm.read_u16());
			}
			else
			{
				TRY_ASSIGN(dimension.x =, qstrm.read_u32());
				TRY_ASSIGN(dimension.y =, qstrm.read_u32());
			}
			RES_TRY(
			  shape.read(game, qstrm).MAP_SE_ERR("object::quick_backdrop_t::read"));

			return lak::ok_t{};
		}

		error_t quick_backdrop_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Properties (Quick Backdrop)##%zX",
			              (size_t)entry.ID,
			              entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Size: 0x%zX", (size_t)size);
				ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
				ImGui::Text("Collision: 0x%zX", (size_t)collision);
				ImGui::Text(
				  "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);

				RES_TRY(
				  shape.view(srcexp).MAP_SE_ERR("object::quick_backdrop_t::view"));

				ImGui::Text("Handle: 0x%zX", (size_t)shape.handle);
				if (shape.handle < 0xFFFF)
				{
					RES_TRY(
					  GetImage(srcexp.state, shape.handle)
					    .MAP_SE_ERR("object::quick_backdrop_t::view: bad image")
					    .and_then([&](const auto &img) { return img.view(srcexp); })
					    .MAP_SE_ERR("object::quick_backdrop_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t backdrop_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::backdrop_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("object::backdrop_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().MAP_SE_ERR("object::backdrop_t::read"));

			data_reader_t bstrm(span);

			TRY_ASSIGN(size =, bstrm.read_u32());
			TRY_ASSIGN(obstacle =, bstrm.read_u16());
			TRY_ASSIGN(collision =, bstrm.read_u16());
			if (game.old_game && bstrm.remaining().size() >= 6)
			{
				TRY_ASSIGN(dimension.x =, bstrm.read_u16());
				TRY_ASSIGN(dimension.y =, bstrm.read_u16());
			}
			else if (!game.old_game)
			{
				TRY_ASSIGN(dimension.x =, bstrm.read_u32());
				TRY_ASSIGN(dimension.y =, bstrm.read_u32());
			}
			// if (!game.old_game) bstrm.skip(2);
			TRY_ASSIGN(handle =, bstrm.read_u16());

			return lak::ok_t{};
		}

		error_t backdrop_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Properties (Backdrop)##%zX", (size_t)entry.ID, entry.position())
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
					RES_TRY(
					  GetImage(srcexp.state, handle)
					    .MAP_SE_ERR("object::backdrop_t::view: bad image")
					    .and_then([&](const auto &img) { return img.view(srcexp); })
					    .MAP_SE_ERR("object::backdrop_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t animation_direction_t::read(game_t &, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::animation_direction_t::");

			DEBUG("Position: ", strm.position(), "/", strm.size());

			CHECK_REMAINING(strm, 8U);

			TRY_ASSIGN(min_speed =, strm.read_u8());
			TRY_ASSIGN(max_speed =, strm.read_u8());
			TRY_ASSIGN(repeat =, strm.read_u16());
			TRY_ASSIGN(back_to =, strm.read_u16());
			// handles.resize(strm.read_u16()); // :TODO: what's going on here? how
			// did anaconda do this?
			TRY_ASSIGN(const auto handle_count =, strm.read_u8());
			handles.resize(handle_count);
			TRY(strm.skip(1));

			DEBUG("Min Speed: ", min_speed);
			DEBUG("Max Speed: ", max_speed);
			DEBUG("Repeat: ", repeat);
			DEBUG("Back To: ", back_to);
			DEBUG("Handle Count: ", handles.size());

			DEBUG("Position: ", strm.position(), "/", strm.size());

			CHECK_REMAINING(strm, handles.size() * 2);
			for (auto &handle : handles) handle = strm.read_u16().UNWRAP();

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

				GetImage(srcexp.state, handle)
				  .MAP_SE_ERR("object::animation_direction_t::view: bad handle")
				  .and_then(
				    [&](auto &img)
				    {
					    return img.view(srcexp).MAP_SE_ERR(
					      "object::animation_direction_t::view: bad image");
				    })
				  .if_err(
				    [](const auto &err)
				    {
					    ImGui::Text("Invalid Image/Handle");
					    ImGui::Text(
					      "%s",
					      reinterpret_cast<const char *>(lak::streamify(err).c_str()));
				    })
				  .discard();
			}

			return lak::ok_t{};
		}

		error_t animation_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::animation_t::");

			DEBUG("Position: ", strm.position(), "/", strm.size());

			CHECK_REMAINING(strm, offsets.size() * 2);

			const size_t begin = strm.position();

			size_t index = 0;
			for (auto &offset : offsets)
			{
				TRY_ASSIGN(offset =, strm.read_u16());

				if (offset != 0)
				{
					CHECK_POSITION(strm, begin + offset);
					const size_t pos = strm.position();
					TRY(strm.seek(begin + offset));
					DEFER(strm.seek(pos).UNWRAP(););
					RES_TRY(directions[index]
					          .read(game, strm)
					          .MAP_SE_ERR("object::animation_t::read"));
				}
				++index;
			}

			TRY(strm.seek(begin + (offsets.size() * 2)));

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
						RES_TRY(
						  direction.view(srcexp).MAP_SE_ERR("object::animation_t::view"));
					}
				}
				++index;
			}

			return lak::ok_t{};
		}

		error_t animation_header_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::animation_header_t::");

			const size_t begin = strm.position();

			DEBUG("Position: ", strm.position());

			TRY_ASSIGN(size =, strm.read_u16());
			DEBUG("Size: ", size);

			TRY_ASSIGN(const auto offset_count =, strm.read_u16());
			offsets.resize(offset_count);
			animations.resize(offsets.size());

			DEBUG("Animation Count: ", animations.size());

			size_t index = 0;
			for (auto &offset : offsets)
			{
				DEBUG("Animation: ", index, "/", offsets.size());
				TRY_ASSIGN(offset =, strm.read_u16());

				if (offset != 0)
				{
					CHECK_POSITION(strm, begin + offset);
					const size_t pos = strm.position();
					TRY(strm.seek(begin + offset));
					DEFER(strm.seek(pos).UNWRAP());
					RES_TRY(animations[index]
					          .read(game, strm)
					          .MAP_SE_ERR("object::animation_header_t::read"));
				}
				++index;
			}

			TRY(strm.seek(begin + size));

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
							RES_TRY(animation.view(srcexp).MAP_SE_ERR(
							  "object::animation_header_t::view"));
						}
					}
					++index;
				}
			}

			return lak::ok_t{};
		}

		error_t common_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::common_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("object::common_t::read"));

			RES_TRY_ASSIGN(auto span =,
			               entry.decode_body().MAP_SE_ERR("object::common_t::read"));

			data_reader_t cstrm(span);

			// if (game.product_build >= 288 && !game.old_game)
			//     mode = game_mode_t::_288;
			// if (game.product_build >= 284 && !game.old_game && !game.compat)
			//     mode = game_mode_t::_284;
			// else
			//     mode = game_mode_t::_OLD;
			mode = _mode;

			// used for offsets.
			const size_t begin = cstrm.position();

			TRY_ASSIGN(size =, cstrm.read_u32());

			DEBUG("Size: ", size);

			CHECK_REMAINING(cstrm, size - 4);

			if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
			{
				// are these in the right order?
				TRY_ASSIGN(animations_offset =, cstrm.read_u16());
				TRY_ASSIGN(movements_offset =, cstrm.read_u16());
				TRY_ASSIGN(version =, cstrm.read_u16());
				TRY_ASSIGN(counter_offset =, cstrm.read_u16());
				TRY_ASSIGN(system_offset =, cstrm.read_u16());
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
				TRY_ASSIGN(movements_offset =, cstrm.read_u16());
				TRY_ASSIGN(animations_offset =, cstrm.read_u16());
				TRY_ASSIGN(version =, cstrm.read_u16());
				TRY_ASSIGN(counter_offset =, cstrm.read_u16());
				TRY_ASSIGN(system_offset =, cstrm.read_u16());
			}

			if (cstrm.empty()) return lak::ok_t{};
			TRY_ASSIGN(flags =, cstrm.read_u32());

#if 1
			TRY(cstrm.skip(8 * 2));
#else
			qualifiers.clear();
			qualifiers.reserve(8);
			for (size_t i = 0; i < 8; ++i)
			{
				TRY_ASSIGN(int16_t qualifier =, cstrm.read_s16());
				if (qualifier == -1) break;
				qualifiers.push_back(qualifier);
			}
#endif

			if (mode == game_mode_t::_284)
			{
				TRY_ASSIGN(system_offset =, cstrm.read_u16());
			}
			else
			{
				TRY_ASSIGN(extension_offset =, cstrm.read_u16());
			}

			TRY_ASSIGN(values_offset =, cstrm.read_u16());
			TRY_ASSIGN(strings_offset =, cstrm.read_u16());
			TRY_ASSIGN(new_flags =, cstrm.read_u32());
			TRY_ASSIGN(preferences =, cstrm.read_u32());
			TRY_ASSIGN(identifier =, cstrm.read_u32());
			TRY_ASSIGN(back_color.r =, cstrm.read_u8());
			TRY_ASSIGN(back_color.g =, cstrm.read_u8());
			TRY_ASSIGN(back_color.b =, cstrm.read_u8());
			TRY_ASSIGN(back_color.a =, cstrm.read_u8());
			TRY_ASSIGN(fade_in_offset =, cstrm.read_u32());
			TRY_ASSIGN(fade_out_offset =, cstrm.read_u32());

			if (animations_offset > 0)
			{
				DEBUG("Animations Offset: ", animations_offset);
				TRY(cstrm.seek(begin + animations_offset));
				animations = std::make_unique<animation_header_t>();
				RES_TRY(
				  animations->read(game, cstrm).MAP_SE_ERR("object::common_t::read"));
			}

			return lak::ok_t{};
		}

		error_t common_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Properties (Common)##%zX", (size_t)entry.ID, entry.position())
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
				// 	ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
				// 	ImGui::Text("Version: 0x%zX", (size_t)version);
				// 	ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
				// 	ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
				// 	ImGui::Text("Animations Offset: 0x%zX",
				// 	            (size_t)animations_offset);
				// 	ImGui::Text("Flags: 0x%zX", (size_t)flags);
				// 	ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
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
					RES_TRY(
					  animations->view(srcexp).MAP_SE_ERR("object::common_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::item_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("object::item_t::read"));

			RES_TRY_ASSIGN(auto span =,
			               entry.decode_body().MAP_SE_ERR("object::item_t::read"));

			data_reader_t dstrm(span);

			TRY_ASSIGN(handle =, dstrm.read_u16());
			TRY_ASSIGN(type = (object_type_t), dstrm.read_s16());
			TRY(dstrm.read_u16()); // flags
			TRY(dstrm.read_u16()); // "no longer used"
			TRY_ASSIGN(ink_effect =, dstrm.read_u32());
			TRY_ASSIGN(ink_effect_param =, dstrm.read_u32());

			// :TODO: refactor out this lambda

			[[maybe_unused]] auto err =
			  [&, this]() -> error_t
			{
				for (bool not_finished = true; not_finished;)
				{
					TRY_ASSIGN(const auto chunk_id = (chunk_t), strm.peek_u16());
					switch (chunk_id)
					{
						case chunk_t::object_name:
							name = std::make_unique<string_chunk_t>();
							RES_TRY(
							  name->read(game, strm).MAP_SE_ERR("object::item_t::read"));
							break;

						case chunk_t::object_properties:
							switch (type)
							{
								case object_type_t::quick_backdrop:
									quick_backdrop = std::make_unique<quick_backdrop_t>();
									RES_TRY(quick_backdrop->read(game, strm)
									          .MAP_SE_ERR("object::item_t::read"));
									break;

								case object_type_t::backdrop:
									backdrop = std::make_unique<backdrop_t>();
									RES_TRY(backdrop->read(game, strm)
									          .MAP_SE_ERR("object::item_t::read"));
									break;

								default:
									common = std::make_unique<common_t>();
									RES_TRY(common->read(game, strm)
									          .MAP_SE_ERR("object::item_t::read"));
									break;
							}
							break;

						case chunk_t::object_effect:
							effect = std::make_unique<effect_t>();
							RES_TRY(
							  effect->read(game, strm).MAP_SE_ERR("object::item_t::read"));
							break;

						case chunk_t::last:
							end = std::make_unique<last_t>();
							RES_TRY(
							  end->read(game, strm).MAP_SE_ERR("object::item_t::read"));
							[[fallthrough]];

						default: not_finished = false; break;
					}
				}

				return lak::ok_t{};
			}()
			                   .IF_ERR("Failed To Read Child Chunks");

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX %s '%s'##%zX",
			              (size_t)entry.ID,
			              GetObjectTypeString(type),
			              (name ? lak::strconv<char>(name->value).c_str() : ""),
			              entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Handle: 0x%zX", (size_t)handle);
				ImGui::Text("Type: 0x%zX", (size_t)type);
				ImGui::Text("Ink Effect: 0x%zX", (size_t)ink_effect);
				ImGui::Text("Ink Effect Parameter: 0x%zX", (size_t)ink_effect_param);

				if (name)
				{
					RES_TRY(name->view(srcexp, "Name", true)
					          .MAP_SE_ERR("object::item_t::view"));
				}

				if (quick_backdrop)
				{
					RES_TRY(
					  quick_backdrop->view(srcexp).MAP_SE_ERR("object::item_t::view"));
				}
				if (backdrop)
				{
					RES_TRY(backdrop->view(srcexp).MAP_SE_ERR("object::item_t::view"));
				}
				if (common)
				{
					RES_TRY(common->view(srcexp).MAP_SE_ERR("object::item_t::view"));
				}

				if (effect)
				{
					RES_TRY(effect->view(srcexp).MAP_SE_ERR("object::item_t::view"));
				}
				if (end)
				{
					RES_TRY(end->view(srcexp).MAP_SE_ERR("object::item_t::view"));
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
								auto h = animation.directions[i].handles[frame];
								result[h].emplace_back(
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

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("object::bank_t::");

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("object::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			for (auto &item : items)
			{
				RES_TRY(item.read(game, reader)
				          .MAP_SE_ERR("object::bank_t::read: Failed To Read Item ",
				                      (&item - items.data()),
				                      " Of ",
				                      items.size())
				          .if_ok(
				            [&](auto &&)
				            {
					            if (!item.end)
						            WARNING("Item ",
						                    (&item - items.data()),
						                    " Has No End Chunk");
				            }));

				game.bank_completed =
				  float(double(reader.position()) / double(reader.size()));
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the object bank");
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Object Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("object::bank_t::view"));
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

		error_t palette_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::palette_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("frame::palette_t::read"));

			RES_TRY_ASSIGN(auto span =,
			               entry.decode_body().MAP_SE_ERR("frame::palette_t::read"));

			data_reader_t pstrm(span);

			TRY_ASSIGN(unknown =, pstrm.read_u32());

			CHECK_REMAINING(pstrm, colors.size() * 4);
			for (auto &color : colors)
			{
				color.r = pstrm.read_u8().UNWRAP();
				color.g = pstrm.read_u8().UNWRAP();
				color.b = pstrm.read_u8().UNWRAP();
				/* color.a = */ pstrm.read_u8().UNWRAP();
				color.a = 255u;
			}

			return lak::ok_t{};
		}

		error_t palette_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Frame Palette##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);

				uint8_t index = 0;
				for (const auto &color : colors)
				{
					lak::vec3f_t col = ((lak::vec3f_t)color) / 256.0f;
					char str[3];
					snprintf(str, 3, "%hhX", index++);
					float f[] = {col.x, col.y, col.z};
					ImGui::ColorEdit3(str, f);
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

		error_t object_instance_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::object_instance_t::");

			TRY_ASSIGN(info =, strm.read_u16());
			TRY_ASSIGN(handle =, strm.read_u16());
			if (game.old_game)
			{
				TRY_ASSIGN(position.x =, strm.read_s16());
				TRY_ASSIGN(position.y =, strm.read_s16());
			}
			else
			{
				TRY_ASSIGN(position.x =, strm.read_s32());
				TRY_ASSIGN(position.y =, strm.read_s32());
			}
			TRY_ASSIGN(parent_type = (object_parent_type_t), strm.read_u16());
			TRY_ASSIGN(parent_handle =, strm.read_u16()); // object info (?)
			if (!game.old_game)
			{
				TRY_ASSIGN(layer =, strm.read_u16());
				TRY_ASSIGN(unknown =, strm.read_u16());
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
					RES_TRY(obj.unwrap().view(srcexp).MAP_SE_ERR(
					  "frame::object_instance_t::view"));
				}

				switch (parent_type)
				{
					case object_parent_type_t::frame_item:
						if (auto parent_obj = GetObject(srcexp.state, parent_handle);
						    parent_obj.is_ok())
						{
							RES_TRY(parent_obj.unwrap().view(srcexp).MAP_SE_ERR(
							  "frame::object_instance_t::view"));
						}
						break;

					case object_parent_type_t::frame:
						if (auto parent_obj = GetFrame(srcexp.state, parent_handle);
						    parent_obj.is_ok())
						{
							RES_TRY(parent_obj.unwrap().view(srcexp).MAP_SE_ERR(
							  "frame::object_instance_t::view"));
						}
						break;

					case object_parent_type_t::none: [[fallthrough]];
					case object_parent_type_t::qualifier: [[fallthrough]];
					default: break;
				}
			}

			return lak::ok_t{};
		}

		error_t object_instances_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::object_instances_t::");

			RES_TRY(
			  entry.read(game, strm).MAP_SE_ERR("frame::object_instances_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().MAP_SE_ERR("frame::object_instances_t::read"));

			data_reader_t hstrm(span);

			TRY_ASSIGN(const auto object_count =, hstrm.read_u32());

			objects.resize(object_count);

			DEBUG("Objects: ", objects.size());

			for (auto &object : objects)
			{
				RES_TRY(object.read(game, hstrm)
				          .MAP_SE_ERR("frame::object_instances_t::read"));
			}

			return lak::ok_t{};
		}

		error_t object_instances_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Object Instances##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);

				for (const auto &object : objects)
				{
					RES_TRY(
					  object.view(srcexp).MAP_SE_ERR("frame::object_instances_t::view"));
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

		error_t random_seed_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::random_seed_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("frame::random_seed_t::read"));

			TRY_SE_ASSIGN(value =,
			              entry.decode_body().and_then(
			                [](const auto &ref_span)
			                {
				                return data_reader_t(ref_span).read_s16().MAP_ERR(
				                  "frame::random_seed_t::read");
			                }));

			return lak::ok_t{};
		}

		error_t random_seed_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Random Seed##%zX", (size_t)entry.ID, entry.position())
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

		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::item_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("frame::item_t::read"));

			DEFER(game.bank_completed = 0.0f);

			data_reader_t reader(entry.raw_body());

			for (bool not_finished = true; not_finished;)
			{
				game.bank_completed =
				  float(double(reader.position()) / double(reader.size()));

				if (reader.remaining().size() < 2) break;

				switch ((chunk_t)reader.peek_u16().UNWRAP())
				{
					case chunk_t::frame_name:
						name = std::make_unique<string_chunk_t>();
						RES_TRY(
						  name->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_header:
						header = std::make_unique<header_t>();
						RES_TRY(
						  header->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_password:
						password = std::make_unique<password_t>();
						RES_TRY(
						  password->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_palette:
						palette = std::make_unique<palette_t>();
						RES_TRY(
						  palette->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_object_instances:
						object_instances = std::make_unique<object_instances_t>();
						RES_TRY(object_instances->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_in_frame:
						fade_in_frame = std::make_unique<fade_in_frame_t>();
						RES_TRY(fade_in_frame->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_out_frame:
						fade_out_frame = std::make_unique<fade_out_frame_t>();
						RES_TRY(fade_out_frame->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_in:
						fade_in = std::make_unique<fade_in_t>();
						RES_TRY(
						  fade_in->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_out:
						fade_out = std::make_unique<fade_out_t>();
						RES_TRY(
						  fade_out->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_events:
						events = std::make_unique<events_t>();
						RES_TRY(
						  events->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_play_header:
						play_head = std::make_unique<play_header_r>();
						RES_TRY(
						  play_head->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_additional_items:
						additional_item = std::make_unique<additional_item_t>();
						RES_TRY(additional_item->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_additional_items_instances:
						additional_item_instance =
						  std::make_unique<additional_item_instance_t>();
						RES_TRY(additional_item_instance->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_layers:
						layers = std::make_unique<layers_t>();
						RES_TRY(
						  layers->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_virtual_size:
						virtual_size = std::make_unique<virtual_size_t>();
						RES_TRY(virtual_size->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::demo_file_path:
						demo_file_path = std::make_unique<demo_file_path_t>();
						RES_TRY(demo_file_path->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::random_seed:
						random_seed = std::make_unique<random_seed_t>();
						RES_TRY(random_seed->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_layer_effect:
						layer_effect = std::make_unique<layer_effect_t>();
						RES_TRY(layer_effect->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_bluray:
						blueray = std::make_unique<blueray_t>();
						RES_TRY(
						  blueray->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::movement_timer_base:
						movement_time_base = std::make_unique<movement_time_base_t>();
						RES_TRY(movement_time_base->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::mosaic_image_table:
						mosaic_image_table = std::make_unique<mosaic_image_table_t>();
						RES_TRY(mosaic_image_table->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_effects:
						effects = std::make_unique<effects_t>();
						RES_TRY(
						  effects->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_iphone_options:
						iphone_options = std::make_unique<iphone_options_t>();
						RES_TRY(iphone_options->read(game, reader)
						          .MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::frame_chunk334C:
						chunk334C = std::make_unique<chunk_334C_t>();
						RES_TRY(
						  chunk334C->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						break;

					case chunk_t::last:
						end = std::make_unique<last_t>();
						RES_TRY(end->read(game, reader).MAP_SE_ERR("frame::item_t::read"));
						[[fallthrough]];

					default: not_finished = false; break;
				}
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the frame item");
			}

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX '%s'##%zX",
			              (size_t)entry.ID,
			              (name ? lak::strconv<char>(name->value).c_str() : ""),
			              entry.position())
			{
				entry.view(srcexp);

				if (name)
				{
					RES_TRY(name->view(srcexp, "Name", true)
					          .MAP_SE_ERR("frame::item_t::view"));
				}
				if (header)
				{
					RES_TRY(header->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (password)
				{
					RES_TRY(password->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (palette)
				{
					RES_TRY(palette->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (object_instances)
				{
					RES_TRY(
					  object_instances->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (fade_in_frame)
				{
					RES_TRY(
					  fade_in_frame->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (fade_out_frame)
				{
					RES_TRY(
					  fade_out_frame->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (fade_in)
				{
					RES_TRY(fade_in->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (fade_out)
				{
					RES_TRY(fade_out->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (events)
				{
					RES_TRY(events->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (play_head)
				{
					RES_TRY(play_head->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (additional_item)
				{
					RES_TRY(
					  additional_item->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (layers)
				{
					RES_TRY(layers->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (layer_effect)
				{
					RES_TRY(
					  layer_effect->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (virtual_size)
				{
					RES_TRY(
					  virtual_size->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (demo_file_path)
				{
					RES_TRY(
					  demo_file_path->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (random_seed)
				{
					RES_TRY(random_seed->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (blueray)
				{
					RES_TRY(blueray->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (movement_time_base)
				{
					RES_TRY(movement_time_base->view(srcexp).MAP_SE_ERR(
					  "frame::item_t::view"));
				}
				if (mosaic_image_table)
				{
					RES_TRY(mosaic_image_table->view(srcexp).MAP_SE_ERR(
					  "frame::item_t::view"));
				}
				if (effects)
				{
					RES_TRY(effects->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (iphone_options)
				{
					RES_TRY(
					  iphone_options->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (chunk334C)
				{
					RES_TRY(chunk334C->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
				if (end)
				{
					RES_TRY(end->view(srcexp).MAP_SE_ERR("frame::item_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t handles_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::handles_t::");

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("frame::handles_t::read"));

			RES_TRY_ASSIGN(auto span =,
			               entry.decode_body().MAP_SE_ERR("frame::handles_t::read"));

			data_reader_t hstrm(span);

			handles.resize(hstrm.size() / 2);
			for (auto &handle : handles) handle = hstrm.read_u16().UNWRAP();

			return lak::ok_t{};
		}

		error_t handles_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Frame Handles");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("frame::bank_t::");

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("frame::bank_t::read"));

			items.clear();

			size_t start_pos = strm.position();

			while (strm.remaining().size() >= 2 &&
			       (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::frame)
			{
				auto &item = items.emplace_back();
				RES_TRY(item.read(game, strm)
				          .IF_ERR("Failed To Read Item ", (&item - items.data()))
				          .MAP_SE_ERR("frame::bank_t::read"));

				game.bank_completed =
				  float(double(strm.position() - start_pos) / double(strm.size()));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Frame Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("frame::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}

	namespace image
	{
		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("image::item_t::");

			const auto strm_start = strm.position();
			if (game.ccn)
			{
				RES_TRY(
				  entry.read(game, strm, true, 10).MAP_SE_ERR("image::item_t::read"));

				const uint16_t gmode = entry.handle >> 16;
				switch (gmode)
				{
					case 0:
						graphics_mode = graphics_mode_t::RGBA32;
						flags         = image_flag_t::none;
						break;

					case 3:
						graphics_mode = graphics_mode_t::RGB24;
						flags         = image_flag_t::none;
						break;

					case 5:
						graphics_mode = graphics_mode_t::JPEG;
						flags         = image_flag_t::none;
						break;

					default: WARNING("Unknown Graphics Mode: ", gmode); break;
				}

				data_position = 0;

				if (!game.old_game && game.product_build >= 284) --entry.handle;

				auto hstrm = data_reader_t(entry.raw_head());

				TRY_ASSIGN([[maybe_unused]] const auto unk1 =, hstrm.read_u16());
				TRY_ASSIGN(size.x =, hstrm.read_u16());
				TRY_ASSIGN(size.y =, hstrm.read_u16());
				TRY_ASSIGN([[maybe_unused]] const auto unk2 =, hstrm.read_u16());
				TRY_ASSIGN([[maybe_unused]] const auto unk3 =, hstrm.read_u16());

				padding = uint16_t(
				  lak::slack<size_t>(size.x * ColorModeSize(graphics_mode), 4));
				alpha_padding = 0;
			}
			else
			{
				data_ref_span_t span;
				if (game.two_five_plus_game)
				{
					const size_t header_size = 36;
					RES_TRY(entry.read(game, strm, false, header_size)
					          .MAP_SE_ERR("image::item_t::read"));

					data_position = strm.position();

					RES_TRY_ASSIGN(
					  span =,
					  entry.decode_head(header_size).MAP_SE_ERR("image::item_t::read"));

					if (span.size() < header_size)
						return lak::err_t{error(LINE_TRACE, error::out_of_data)};
				}
				else
				{
					RES_TRY(
					  entry.read(game, strm, true).MAP_SE_ERR("image::item_t::read"));

					if (!game.old_game && game.product_build >= 284) --entry.handle;

					RES_TRY_ASSIGN(span =,
					               entry.decode_body(176 + (game.old_game ? 16 : 80))
					                 .MAP_SE_ERR("image::item_t::read"));
				}
				auto istrm = data_reader_t(span);

				DEBUG("Handle: ", entry.handle);

				if (game.old_game)
				{
					TRY_ASSIGN(checksum =, istrm.read_u16());
				}
				else
				{
					TRY_ASSIGN(checksum =, istrm.read_u32());
				}
				TRY_ASSIGN(reference =, istrm.read_u32());
				if (game.two_five_plus_game) TRY(istrm.skip(4));
				TRY_ASSIGN(data_size =, istrm.read_u32());
				TRY_ASSIGN(size.x =, istrm.read_u16());
				TRY_ASSIGN(size.y =, istrm.read_u16());
				TRY_ASSIGN(const uint8_t gmode =, istrm.read_u8());
				switch (gmode)
				{
					case 2: graphics_mode = graphics_mode_t::RGB8; break;
					case 3: graphics_mode = graphics_mode_t::RGB8; break;
					case 4: graphics_mode = graphics_mode_t::BGR24; break;
					case 6: graphics_mode = graphics_mode_t::RGB15; break;
					case 7: graphics_mode = graphics_mode_t::RGB16; break;
					case 8: graphics_mode = graphics_mode_t::BGRA32; break;
				}
				TRY_ASSIGN(flags = (image_flag_t), istrm.read_u8());
#if 0
				if (graphics_mode == graphics_mode_t::RGB8)
				{
					TRY_ASSIGN(palette_entries =, istrm.read_u8());
					for (size_t i = 0; i < palette.size();
					     ++i) // where is this size coming from???
						palette[i] = ColorFrom32bitRGBA(istrm); // not sure if RGBA or BGRA
					TRY_ASSIGN(count =, strm.read_u32());
				}
#endif
				if (!game.old_game)
				{
					TRY_ASSIGN(unknown =, istrm.read_u16());
				}
				TRY_ASSIGN(hotspot.x =, istrm.read_u16());
				TRY_ASSIGN(hotspot.y =, istrm.read_u16());
				TRY_ASSIGN(action.x =, istrm.read_u16());
				TRY_ASSIGN(action.y =, istrm.read_u16());

				if (!game.old_game) transparent = ColorFrom32bitRGBA(istrm).UNWRAP();

				if (game.two_five_plus_game)
				{
					ASSERT_EQUAL(strm.position(), data_position);
					TRY_ASSIGN(entry.body.data =, strm.read_ref_span(data_size));
					data_position = 0;

					const auto strm_end = strm.position();
					TRY(strm.seek(strm_start));
					TRY_ASSIGN(entry.ref_span =,
					           strm.read_ref_span(strm_end - strm_start));
					DEBUG("Corrected Ref Span Size: ", entry.ref_span.size());
				}
				else
				{
					data_position = istrm.position();
				}

				padding       = uint16_t(lak::slack<size_t>(size.x, 2) *
				                   ColorModeSize(graphics_mode));
				alpha_padding = uint16_t(lak::slack<size_t>(size.x, 2));
			}

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Image##%zX", (size_t)entry.handle, entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Checksum: 0x%zX", (size_t)checksum);
				ImGui::Text("Reference: 0x%zX", (size_t)reference);
				ImGui::Text("Data Size: 0x%zX", (size_t)data_size);
				ImGui::Text("Image Size: %zu * %zu", (size_t)size.x, (size_t)size.y);
				switch (graphics_mode)
				{
					case graphics_mode_t::RGBA32:
						ImGui::Text("Graphics Mode: RGBA32");
						break;
					case graphics_mode_t::BGRA32:
						ImGui::Text("Graphics Mode: BGRA32");
						break;
					case graphics_mode_t::RGB24:
						ImGui::Text("Graphics Mode: RGB24");
						break;
					case graphics_mode_t::BGR24:
						ImGui::Text("Graphics Mode: BGR24");
						break;
					case graphics_mode_t::RGB16:
						ImGui::Text("Graphics Mode: RGB16");
						break;
					case graphics_mode_t::RGB15:
						ImGui::Text("Graphics Mode: RGB15");
						break;
					case graphics_mode_t::RGB8:
						ImGui::Text("Graphics Mode: RGB8");
						break;
					case graphics_mode_t::JPEG:
						ImGui::Text("Graphics Mode: JPEG");
						break;
				}
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
				ImGui::Text("Padding: 0x%zX", size_t(padding));
				ImGui::Text("Alpha Padding: 0x%zX", size_t(alpha_padding));

				if (ImGui::Button("View Image"))
				{
					image(srcexp.dump_color_transparent)
					  .if_ok(
					    [&](lak::image4_t &&img)
					    { srcexp.image = CreateTexture(img, srcexp.graphics_mode); })
					  .IF_ERR("Failed To Read Image Data")
					  .discard();
				}
			}

			return lak::ok_t{};
		}

		result_t<data_ref_span_t> item_t::image_data() const
		{
			FUNCTION_CHECKPOINT("image::item_t::");

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().MAP_SE_ERR("image::item_t::image_data"));

			data_reader_t strm(span);

			TRY(strm.seek(data_position));

			if ((flags & image_flag_t::LZX) == image_flag_t::LZX)
			{
				if (entry.mode == encoding_t::mode4)
				{
					return lak::ok_t{strm.read_remaining_ref_span()};
				}
				else
				{
					TRY_ASSIGN([[maybe_unused]] const uint32_t decompressed_length =,
					           strm.read_u32());

					TRY_ASSIGN(const uint32_t compressed_length =, strm.read_u32());

					CHECK_REMAINING(strm, compressed_length);

					return strm.read_ref_span(compressed_length)
					  .MAP_ERR("item_t::image_data: read filed")
					  .map(
					    [](const data_ref_span_t &ref_span) -> data_ref_span_t
					    {
						    return lak::ok_or_err(
						      Inflate(ref_span, false, false)
						        .map_err([&](auto &&) { return ref_span; }));
					    });
				}
			}
			else
			{
				return lak::ok_t{strm.read_remaining_ref_span()};
			}
		}

		bool item_t::need_palette() const
		{
			return graphics_mode == graphics_mode_t::RGB8;
		}

		result_t<lak::image4_t> item_t::image(
		  const bool color_transparent, const lak::color4_t palette[256]) const
		{
			FUNCTION_CHECKPOINT("image::image_t::");

			lak::image4_t img = {};

			RES_TRY_ASSIGN(auto span =,
			               image_data().MAP_SE_ERR("image::item_t::image"));

			if (graphics_mode == graphics_mode_t::JPEG)
			{
				int x, y, n;
				uint8_t *data =
				  stbi_load_from_memory(reinterpret_cast<const uint8_t *>(span.data()),
				                        static_cast<int>(span.size()),
				                        &x,
				                        &y,
				                        &n,
				                        4);
				DEFER(stbi_image_free(data));

				if (x != size.x || y != size.y)
				{
					return lak::err_t{error(LINE_TRACE,
					                        error::str_err,
					                        "jpeg decode failed, expected size (",
					                        size.x,
					                        ", ",
					                        size.y,
					                        ") got (",
					                        x,
					                        ", ",
					                        y,
					                        ")")};
				}
				else
				{
					img = lak::image_from_rgba32(reinterpret_cast<const byte_t *>(data),
					                             lak::vec2s_t{size_t(x), size_t(y)});
				}
			}
			else
			{
				data_reader_t strm(span);

				img.resize(lak::vec2s_t(size));

				[[maybe_unused]] size_t bytes_read;
				if ((flags & (image_flag_t::RLE | image_flag_t::RLEW |
				              image_flag_t::RLET)) != image_flag_t::none)
				{
					RES_TRY_ASSIGN(bytes_read =,
					               ReadRLE(strm, img, graphics_mode, padding, palette));
				}
				else
				{
					RES_TRY_ASSIGN(bytes_read =,
					               ReadRGB(strm, img, graphics_mode, padding, palette));
				}

				if ((flags & image_flag_t::RGBA) != image_flag_t::none)
				{
					// we already read the alpha data with the colour data
				}
				else if ((flags & image_flag_t::alpha) != image_flag_t::none)
				{
					RES_TRY(ReadAlpha(strm, img, alpha_padding));
				}
				else if (color_transparent)
				{
					ReadTransparent(transparent, img);
				}
				else
				{
					for (size_t i = 0; i < img.contig_size(); ++i) img[i].a = 255;
				}

				if (!strm.empty())
					WARNING(strm.remaining().size(), " Bytes Left Over In Image Data");
			}

			return lak::ok_t{lak::move(img)};
		}

		error_t end_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Image Bank End");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("image::bank_t::");

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("image::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			uint32_t item_count = 0;
			if (game.ccn)
			{
				TRY_ASSIGN([[maybe_unused]] const auto unknown =, reader.read_u16());
				TRY_ASSIGN(item_count =, reader.read_u16());
			}
			else
			{
				TRY_ASSIGN(item_count =, reader.read_u32());
			}

			items.resize(item_count);

			DEBUG("Image Bank Size: ", items.size());

			size_t max_tries = 3;

			for (auto &item : items)
			{
				// clang-format off
				RES_TRY(
				  [&, this]() -> error_t
				  {
					  return item.read(game, reader)
					    .IF_ERR("Failed To Read Item ",
					            (&item - items.data()),
					            " Of ",
					            items.size())
					    .MAP_SE_ERR("image::bank_t::read");
				  }()
					  .or_else(
					    [&](const auto &err) -> error_t
					    {
						    if (max_tries == 0) return lak::err_t{err};
						    ERROR(err);
						    DEBUG("Continuing...");
						    --max_tries;
						    return lak::ok_t{};
					    }));
				// clang-format on

				game.bank_completed =
				  float(double(reader.position()) / double(reader.size()));
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the image bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::image_handles)
			{
				end = std::make_unique<end_t>();
				TRY_SE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Image Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("image::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).MAP_SE_ERR("image::bank_t::view"));
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

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("font::bank_t::");

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("font::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			for (auto &item : items)
			{
				RES_TRY(item.read(game, reader)
				          .IF_ERR("Failed To Read Item ",
				                  (&item - items.data()),
				                  " Of ",
				                  items.size())
				          .MAP_SE_ERR("font::bank_t::read"));

				game.bank_completed =
				  (float)((double)reader.position() / (double)reader.size());
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the font bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::font_handles)
			{
				end = std::make_unique<end_t>();
				TRY_SE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Font Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("font::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).MAP_SE_ERR("font::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}

	namespace sound
	{
		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("sound::item_t::");
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

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("sound::bank_t::");

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("sound::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			for (auto &item : items)
			{
				RES_TRY(item.read(game, reader)
				          .IF_ERR("Failed To Read Item ",
				                  (&item - items.data()),
				                  " Of ",
				                  items.size())
				          .MAP_SE_ERR("sound::bank_t::read"));

				game.bank_completed =
				  (float)((double)reader.position() / (double)reader.size());
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the sound bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::sound_handles)
			{
				end = std::make_unique<end_t>();
				TRY_SE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Sound Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("sound::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).MAP_SE_ERR("sound::bank_t::view"));
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

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			FUNCTION_CHECKPOINT("music::bank_t::");

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).MAP_SE_ERR("music::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			for (auto &item : items)
			{
				RES_TRY(item.read(game, reader)
				          .IF_ERR("Failed To Read Item ",
				                  (&item - items.data()),
				                  " Of ",
				                  items.size())
				          .MAP_SE_ERR("music::bank_t::read"));

				game.bank_completed =
				  (float)((double)reader.position() / (double)reader.size());
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the music bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::music_handles)
			{
				end = std::make_unique<end_t>();
				TRY_SE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Music Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).MAP_SE_ERR("music::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).MAP_SE_ERR("music::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}

	error_t last_t::view(source_explorer_t &srcexp) const
	{
		return basic_view(srcexp, "Last");
	}

	error_t header_t::read(game_t &game, data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT("header_t::");

		RES_TRY(entry.read(game, strm).MAP_SE_ERR("header_t::read"));

		auto init_chunk = [&](auto &chunk)
		{
			chunk = std::make_unique<
			  typename std::remove_reference_t<decltype(chunk)>::value_type>();
			return chunk->read(game, strm);
		};

		chunk_t childID  = (chunk_t)-1;
		size_t start_pos = SIZE_MAX;
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
			TRY_ASSIGN(childID = (chunk_t), strm.peek_u16());

			// the MAP_SE_ERRs are here so we get line information
			switch (childID)
			{
				case chunk_t::title:
					RES_TRY(init_chunk(title).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::author:
					RES_TRY(init_chunk(author).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::copyright:
					RES_TRY(init_chunk(copyright).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::project_path:
					RES_TRY(init_chunk(project_path).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::output_path:
					RES_TRY(init_chunk(output_path).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::about:
					RES_TRY(init_chunk(about).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::vitalise_preview:
					RES_TRY(init_chunk(vitalise_preview).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::menu:
					RES_TRY(init_chunk(menu).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::extra_path:
					RES_TRY(init_chunk(extension_path).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::extensions:
					RES_TRY(init_chunk(extensions).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::extra_data:
					RES_TRY(init_chunk(extension_data).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::additional_extensions:
					RES_TRY(
					  init_chunk(additional_extensions).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::app_doc:
					RES_TRY(init_chunk(app_doc).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::other_extension:
					RES_TRY(init_chunk(other_extension).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::extensions_list:
					RES_TRY(init_chunk(extension_list).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::icon:
					RES_TRY(init_chunk(icon).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::demo_version:
					RES_TRY(init_chunk(demo_version).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::security_number:
					RES_TRY(init_chunk(security).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::binary_files:
					RES_TRY(init_chunk(binary_files).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::menu_images:
					RES_TRY(init_chunk(menu_images).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::movement_extensions:
					RES_TRY(
					  init_chunk(movement_extensions).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::exe_only:
					RES_TRY(init_chunk(exe).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::protection:
					RES_TRY(init_chunk(protection).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::shaders:
					RES_TRY(init_chunk(shaders).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::extended_header:
					RES_TRY(init_chunk(extended_header).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::spacer:
					RES_TRY(init_chunk(spacer).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::chunk224F:
					RES_TRY(init_chunk(chunk224F).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::title2:
					RES_TRY(init_chunk(title2).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::chunk2253:
					// 16-bytes
					RES_TRY(init_chunk(chunk2253).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::object_names:
					RES_TRY(init_chunk(object_names).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::chunk2255:
					// blank???
					RES_TRY(init_chunk(chunk2255).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::two_five_plus_object_properties:
					// Appears to have sub chunks
					RES_TRY(init_chunk(two_five_plus_object_properties)
					          .MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::chunk2257:
					RES_TRY(init_chunk(chunk2257).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::object_properties:
					RES_TRY(init_chunk(object_properties).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::font_meta:
					RES_TRY(
					  init_chunk(truetype_fonts_meta).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::font_chunk:
					RES_TRY(init_chunk(truetype_fonts).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::shaders2:
					RES_TRY(init_chunk(shaders2).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::global_events:
					RES_TRY(init_chunk(global_events).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::global_strings:
					RES_TRY(init_chunk(global_strings).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::global_string_names:
					RES_TRY(
					  init_chunk(global_string_names).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::global_values:
					RES_TRY(init_chunk(global_values).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::global_value_names:
					RES_TRY(init_chunk(global_value_names).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::frame_handles:
					RES_TRY(init_chunk(frame_handles).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::frame_bank:
					RES_TRY(init_chunk(frame_bank).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::frame:
					if (frame_bank) ERROR("Frame Bank Already Exists");
					frame_bank = std::make_unique<frame::bank_t>();
					frame_bank->items.clear();
					while (strm.remaining().size() >= 2 &&
					       (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::frame)
					{
						if (frame_bank->items.emplace_back().read(game, strm).is_err())
							break;
					}
					break;

					// case chunk_t::object_bank2:
					//     RES_TRY(init_chunk(object_bank_2).MAP_SE_ERR("header_t::read"));
					//     break;

				case chunk_t::object_bank:
				case chunk_t::object_bank2:
					RES_TRY(init_chunk(object_bank).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::image_bank:
					RES_TRY(init_chunk(image_bank).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::sound_bank:
					RES_TRY(init_chunk(sound_bank).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::music_bank:
					RES_TRY(init_chunk(music_bank).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::font_bank:
					RES_TRY(init_chunk(font_bank).MAP_SE_ERR("header_t::read"));
					break;

				case chunk_t::last:
					RES_TRY(init_chunk(last).MAP_SE_ERR("header_t::read"));
					not_finished = false;
					break;

				default:
					DEBUG("Invalid Chunk: ", (size_t)childID);
					unknown_chunks.emplace_back();
					RES_TRY(unknown_chunks.back()
					          .read(game, strm)
					          .MAP_SE_ERR("header_t::read"));
					break;
			}
		}

		return lak::ok_t{};
	}

	error_t header_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Game Header##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			RES_TRY(title.view(srcexp, "Title", true).MAP_SE_ERR("header_t::view"));
			RES_TRY(
			  author.view(srcexp, "Author", true).MAP_SE_ERR("header_t::view"));
			RES_TRY(copyright.view(srcexp, "Copyright", true)
			          .MAP_SE_ERR("header_t::view"));
			RES_TRY(
			  output_path.view(srcexp, "Output Path").MAP_SE_ERR("header_t::view"));
			RES_TRY(project_path.view(srcexp, "Project Path")
			          .MAP_SE_ERR("header_t::view"));
			RES_TRY(about.view(srcexp, "About").MAP_SE_ERR("header_t::view"));

			RES_TRY(vitalise_preview.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(menu.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(extension_path.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(extensions.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(extension_data.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(additional_extensions.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(app_doc.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(other_extension.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(extension_list.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(icon.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(demo_version.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(security.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(binary_files.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(menu_images.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(movement_extensions.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(object_bank_2.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(exe.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(protection.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(shaders.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(shaders2.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(extended_header.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(spacer.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(chunk224F.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(title2.view(srcexp).MAP_SE_ERR("header_t::view"));

			RES_TRY(global_events.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(global_strings.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(global_string_names.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(global_values.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(global_value_names.view(srcexp).MAP_SE_ERR("header_t::view"));

			RES_TRY(frame_handles.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(frame_bank.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(object_bank.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(image_bank.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(sound_bank.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(music_bank.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(font_bank.view(srcexp).MAP_SE_ERR("header_t::view"));

			RES_TRY(chunk2253.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(object_names.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(chunk2255.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(two_five_plus_object_properties.view(srcexp).MAP_SE_ERR(
			  "header_t::view"));
			RES_TRY(chunk2257.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(object_properties.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(truetype_fonts_meta.view(srcexp).MAP_SE_ERR("header_t::view"));
			RES_TRY(truetype_fonts.view(srcexp).MAP_SE_ERR("header_t::view"));

			for (auto &unk : unknown_strings)
			{
				RES_TRY(unk.view(srcexp).MAP_SE_ERR("header_t::view"));
			}

			for (auto &unk : unknown_compressed)
			{
				RES_TRY(unk.view(srcexp).MAP_SE_ERR("header_t::view"));
			}

			for (auto &unk : unknown_chunks)
			{
				RES_TRY(unk
				          .basic_view(srcexp,
				                      (lak::astring("Unknown ") +
				                       std::to_string(unk.entry.position()))
				                        .c_str())
				          .MAP_SE_ERR("header_t::view"));
			}

			RES_TRY(last.view(srcexp).MAP_SE_ERR("header_t::view"));
		}

		return lak::ok_t{};
	}
}
