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
#include "lak/concepts.hpp"
#include "lak/math.hpp"
#include "lak/string.hpp"
#include "lak/string_utils.hpp"
#include "lak/string_view.hpp"

#include "../tostring.hpp"
#include "explorer.hpp"

#ifdef GetObject
#	undef GetObject
#endif

#define TRACE_EXPECTED(EXPECTED, GOT)                                         \
	lak::streamify("expected '", EXPECTED, "', got '", GOT, "'")

namespace SourceExplorer
{
	bool force_compat          = false;
	bool skip_broken_items     = false;
	bool open_broken_games     = true;
	size_t max_item_read_fails = 3;
	encryption_table decryptor;
	lak::array<uint8_t> _magic_key;
	game_mode_t _mode = game_mode_t::_OLD;
	uint8_t _magic_char;
	std::atomic<float> game_t::completed      = 0.0f;
	std::atomic<float> game_t::bank_completed = 0.0f;

	using namespace std::string_literals;

	lak::array<uint8_t> &operator+=(lak::array<uint8_t> &lhs,
	                                const lak::array<uint8_t> &rhs)
	{
		const size_t old_size = lhs.size();
		lhs.resize(lhs.size() + rhs.size());
		lak::copy(rhs.begin(), rhs.end(), lhs.begin() + old_size, lhs.end());
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

		RES_TRY_ASSIGN(
		  auto bytes =,
		  lak::read_file(srcexp.exe.path).RES_MAP_TO_TRACE("LoadGame"));

		srcexp.state.file = make_data_ref_ptr(lak::move(bytes));

		data_reader_t strm(srcexp.state.file);

		DEBUG("File Size: ", srcexp.state.file->size());

		if (srcexp.state.file->size() == 0)
		{
			ERROR("Empty File");
			return lak::err_t{error(error_type::out_of_data)};
		}

		if (auto err = ParsePEHeader(strm).RES_ADD_TRACE(
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
		          .RES_ADD_TRACE("LoadGame: while parsing game header at: ",
		                         strm.position()));

		DEBUG("Successfully Parsed Game Header");

		_magic_key.clear();

		if (srcexp.state.product_build < 284 || srcexp.state.old_game ||
		    srcexp.state.compat)
			_mode = game_mode_t::_OLD;
		else if (srcexp.state.product_build > 285)
			_mode = game_mode_t::_288;
		else
			_mode = game_mode_t::_284;

		if (_mode == game_mode_t::_OLD)
			_magic_char = 99; // '6';
		else
			_magic_char = 54; // 'c';

		RES_TRY(srcexp.state.game.read(srcexp.state, strm)
		          .RES_ADD_TRACE("LoadGame: while parsing PE header at: ",
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

		if (!game_state.old_game && game_state.product_build <= 285)
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
		_magic_key.resize(0x100, 0U);
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

	result_t<image_section_header_t> ParseImageSectionHeader(data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT();

		image_section_header_t result;
		size_t start = strm.position();
		TRY_ASSIGN(result.name =, strm.read_c_str<char>());
		TRY(strm.seek(start + 0x10));
		TRY_ASSIGN(result.size =, strm.read_u32());
		TRY_ASSIGN(result.addr =, strm.read_u32());
		TRY(strm.seek(start + 0x28));
		return lak::move_ok(result);
	}

	error_t ParsePEHeader(data_reader_t &strm)
	{
		FUNCTION_CHECKPOINT();

		TRY_ASSIGN(uint16_t exe_sig =, strm.read_u16());
		DEBUG("EXE Signature: ", exe_sig);
		if (exe_sig != WIN_EXE_SIG)
		{
			return lak::err_t{
			  error(error_type::invalid_exe_signature,
			        lak::streamify(TRACE_EXPECTED(WIN_EXE_SIG, exe_sig),
			                       ", at ",
			                       (strm.position() - 2)))};
		}

		strm.seek(WIN_EXE_PNT).UNWRAP();
		strm.seek(strm.read_u16().UNWRAP()).UNWRAP();
		DEBUG("EXE Pointer: ", strm.position(), ", At: ", (size_t)WIN_EXE_PNT);

		int32_t pe_sig = strm.read_s32().UNWRAP();
		DEBUG("PE Signature: ", pe_sig);
		if (pe_sig != WIN_PE_SIG)
		{
			return lak::err_t{
			  error(error_type::invalid_pe_signature,
			        lak::streamify(TRACE_EXPECTED(WIN_PE_SIG, pe_sig),
			                       ", at ",
			                       (strm.position() - 4)))};
		}

		strm.skip(2).UNWRAP();

		uint16_t num_header_sections = strm.read_u16().UNWRAP();
		DEBUG("Number Of Header Sections: ", num_header_sections);

		strm.skip(16).UNWRAP();

		const uint16_t optional_header = 0x60;
		const uint16_t data_dir        = 0x80;
		strm.skip(optional_header + data_dir).UNWRAP();
		DEBUG("Pos: ", strm.position());

		uint64_t game_start = 0;
		for (uint16_t i = 0; i < num_header_sections; ++i)
		{
			SCOPED_CHECKPOINT("Section ", i + 1, "/", num_header_sections);
			size_t start = strm.position();
			DEBUG("Start: ", start);
			RES_TRY_ASSIGN(auto image_section =,
			               ParseImageSectionHeader(strm).RES_ADD_TRACE(
			                 "while parsing PE header at: ", strm.position()));
			DEBUG("Name: ", image_section.name);
			DEBUG("Size: ", image_section.size);
			DEBUG("Addr: ", image_section.addr);
			if (image_section.addr == 0 && image_section.size != 0)
				game_start += image_section.size;
			else if (image_section.addr + image_section.size > game_start)
				game_start = image_section.addr + image_section.size;
		}

		DEBUG("Jumping To: ", game_start);
		CHECK_POSITION(strm, game_start); // necessary check for cast to size_t
		TRY(strm.seek(static_cast<size_t>(game_start)));

		return lak::ok_t{};
	}

	error_t ParseGameHeader(data_reader_t &strm, game_t &game_state)
	{
		FUNCTION_CHECKPOINT();

		size_t pos = strm.position();

		DEBUG("Pos: ", pos);
		strm.seek(pos).UNWRAP();

		if (strm.empty())
		{
			return lak::err_t{
			  error(error_type::invalid_game_signature,
			        u8"No game header. "
			        "If this game has an associated .DAT file, open that instead.")};
		}

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
				game_state.state.push_back(chunk_t::old);
				break;
			}
			else if (pack_magic == HEADER_PACK)
			{
				DEBUG("New Game");
				game_state.old_game = false;
				game_state.state    = {};
				game_state.state.push_back(chunk_t::_new);
				RES_TRY_ASSIGN(
				  pos =,
				  ParsePackData(strm, game_state)
				    .RES_ADD_TRACE("while parsing game header at: ", strm.position()));
				break;
			}
			else if (pame_magic == HEADER_UNIC)
			{
				DEBUG("New Game (ccn)");
				game_state.old_game = false;
				game_state.ccn      = true;
				game_state.state    = {};
				game_state.state.push_back(chunk_t::_new);
				break;
			}
			else if (pame_magic == HEADER_CRUF)
			{
				DEBUG("CRUF Game");
				game_state.old_game = false;
				game_state.ccn      = true;
				game_state.cruf     = true;
				game_state.state    = {};
				game_state.state.push_back(chunk_t::_new);
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
				return lak::err_t{error(error_type::invalid_game_signature,
				                        lak::streamify("Expected ",
				                                       (uint16_t)chunk_t::header,
				                                       "u16, ",
				                                       0x222C,
				                                       "u16, ",
				                                       0x7F7F,
				                                       "u16, ",
				                                       HEADER_GAME,
				                                       "u32, ",
				                                       HEADER_UNIC,
				                                       "u32, ",
				                                       HEADER_CRUF,
				                                       "u32, ",
				                                       HEADER_PACK,
				                                       "u64, Found ",
				                                       first_short,
				                                       "u16/",
				                                       pame_magic,
				                                       "u32/",
				                                       pack_magic,
				                                       "u64"))};
			}

			if (pos > strm.size())
			{
				return lak::err_t{
				  error(error_type::invalid_game_signature,
				        lak::streamify(
				          ": pos (", pos, ") > strm.size (", strm.size(), ")"))};
			}
		}

		uint32_t header = strm.read_u32().UNWRAP();
		DEBUG("Header: ", header);

		game_state.unicode = false;
		if (header == HEADER_UNIC || header == HEADER_CRUF)
		{
			game_state.unicode  = true;
			game_state.old_game = false;
		}
		else if (header != HEADER_GAME)
		{
			return lak::err_t{
			  error(error_type::invalid_game_signature,
			        lak::streamify(TRACE_EXPECTED(HEADER_GAME, header),
			                       ", at ",
			                       (strm.position() - 4)))};
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
		if (count < 0) return lak::err_t{error(error_type::invalid_pack_count)};
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
			case graphics_mode_t::RGBA32:
				return ColorFrom32bitRGBA(strm);
			case graphics_mode_t::BGRA32:
				return ColorFrom32bitBGRA(strm);

			case graphics_mode_t::RGB24:
				return ColorFrom24bitRGB(strm);
			case graphics_mode_t::BGR24:
				return ColorFrom24bitBGR(strm);

			case graphics_mode_t::RGB16:
				return ColorFrom16bitRGB(strm);
			case graphics_mode_t::RGB15:
				return ColorFrom15bitRGB(strm);

			case graphics_mode_t::RGB8:
				return ColorFrom8bitI(strm, palette);

			default:
				return ColorFrom24bitBGR(strm);
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
		ColorsFrom16bitRGB(colors, strm.read_bytes(colors.size() * 2).UNWRAP());
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
			case graphics_mode_t::RGBA32:
				return ColorsFrom32bitRGBA(colors, strm);
			case graphics_mode_t::BGRA32:
				return ColorsFrom32bitBGRA(colors, strm);

			case graphics_mode_t::RGB24:
				return ColorsFrom24bitRGB(colors, strm);
			case graphics_mode_t::BGR24:
				return ColorsFrom24bitBGR(colors, strm);

			case graphics_mode_t::RGB16:
				return ColorsFrom16bitRGB(colors, strm);
			case graphics_mode_t::RGB15:
				return ColorsFrom15bitRGB(colors, strm);

			case graphics_mode_t::RGB8:
				return ColorsFrom8bitI(colors, strm, palette);

			default:
				return ColorsFrom24bitBGR(colors, strm);
		}
	}

	uint8_t ColorModeSize(const graphics_mode_t mode)
	{
		switch (mode)
		{
			case graphics_mode_t::RGBA32:
				return 4;
			case graphics_mode_t::BGRA32:
				return 4;

			case graphics_mode_t::RGB24:
				return 3;
			case graphics_mode_t::BGR24:
				return 3;

			case graphics_mode_t::RGB16:
				return 2;
			case graphics_mode_t::RGB15:
				return 2;

			case graphics_mode_t::RGB8:
				return 1;

			default:
				return 3;
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

		DEBUG_EXPR(strm.remaining().size());
		DEBUG_EXPR(((bitmap.size().x * point_size) + padding) * bitmap.size().y);

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

		if (i != bitmap.contig_size())
			ERROR("Only Filled ", i, " Pixels Of ", bitmap.contig_size());

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

		DEBUG_EXPR(strm.remaining().size());
		DEBUG_EXPR((bitmap.size().x + padding) * bitmap.size().y);
		CHECK_REMAINING(
		  strm, ((bitmap.size().x * point_size) + padding) * bitmap.size().y);

		const lak::span bitmap_span{bitmap.data(), bitmap.contig_size()};

		if (padding == 0)
		{
			ColorsFromMode(bitmap_span, strm, mode, palette).UNWRAP();
		}
		else
		{
			for (size_t y = 0; y < bitmap.size().y; ++y)
			{
				ColorsFromMode(
				  bitmap_span.subspan(y * bitmap.size().x, bitmap.size().x),
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
		// FUNCTION_CHECKPOINT();

		if (mode == lak::graphics_mode::OpenGL)
		{
			// auto old_texture =
			//   lak::opengl::get_uint<1>(GL_TEXTURE_BINDING_2D).UNWRAP();

			lak::opengl::texture result(GL_TEXTURE_2D);
			result.bind()
			  .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
			  .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
			  .build(0,
			         GL_RGBA,
			         (lak::vec2<GLsizei>)bitmap.size(),
			         0,
			         GL_RGBA,
			         GL_UNSIGNED_BYTE,
			         bitmap.data());

			// glBindTexture(GL_TEXTURE_2D, old_texture);

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
			// return lak::monostate{};
		}
	}

	texture_t CreateTexture(const lak::image<float> &bitmap,
	                        const lak::graphics_mode mode)
	{
		// FUNCTION_CHECKPOINT();

		if (mode == lak::graphics_mode::OpenGL)
		{
			// auto old_texture =
			//   lak::opengl::get_uint<1>(GL_TEXTURE_BINDING_2D).UNWRAP();

			lak::opengl::texture result(GL_TEXTURE_2D);
			result.bind()
			  .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
			  .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
			  .build(0,
			         GL_RED,
			         (lak::vec2<GLsizei>)bitmap.size(),
			         0,
			         GL_RED,
			         GL_FLOAT,
			         bitmap.data());

			// glBindTexture(GL_TEXTURE_2D, old_texture);

			return result;
		}
		else if (mode == lak::graphics_mode::Software)
		{
			texture_color32_t result;
			result.init(bitmap.size().x, bitmap.size().y);
			for (size_t y = 0; y < bitmap.size().y; ++y)
				for (size_t x = 0; x < bitmap.size().x; ++x)
				{
					result.at(x, y).r = uint8_t(std::min<uint64_t>(
					  uint64_t(bitmap[lak::vec2s_t{x, y}] * 256), 255));
					result.at(x, y).g = 0;
					result.at(x, y).b = 0;
					result.at(x, y).a = 255;
				}
			return result;
		}
		else
		{
			FATAL("Unknown graphics mode: ", (uintmax_t)mode);
			// return lak::monostate{};
		}
	}

	void ViewImage(source_explorer_t &srcexp, const float scale)
	{
		// :TODO: Select palette
		if (const auto glimg = srcexp.image.template get<lak::opengl::texture>();
		    glimg)
		{
			if (!glimg->get() || srcexp.graphics_mode != lak::graphics_mode::OpenGL)
			{
				ImGui::Text("No image selected.");
			}
			else
			{
				ImGui::Image((ImTextureID)(uintptr_t)glimg->get(),
				             ImVec2(scale * (float)glimg->size().x,
				                    scale * (float)glimg->size().y));
			}
		}
		else if (const auto srimg = srcexp.image.template get<texture_color32_t>();
		         srimg)
		{
			if (!srimg->pixels ||
			    srcexp.graphics_mode != lak::graphics_mode::Software)
			{
				ImGui::Text("No image selected.");
			}
			else
			{
				ImGui::Image((ImTextureID)(uintptr_t)srimg,
				             ImVec2(scale * (float)srimg->w, scale * (float)srimg->h));
			}
		}
		else if (srcexp.image.template holds<lak::monostate>())
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
			case chunk_t::entry:
				return "Entry (ERROR)";

			case chunk_t::vitalise_preview:
				return "Vitalise Preview";

			case chunk_t::header:
				return "Header";
			case chunk_t::title:
				return "Title";
			case chunk_t::author:
				return "Author";
			case chunk_t::menu:
				return "Menu";
			case chunk_t::extra_path:
				return "Extra Path";
			case chunk_t::extensions:
				return "Extensions (deprecated)";
			case chunk_t::object_bank:
				return "Object Bank";

			case chunk_t::global_events:
				return "Global Events (deprecated)";
			case chunk_t::frame_handles:
				return "Frame Handles";
			case chunk_t::extra_data:
				return "Extra Data";
			case chunk_t::additional_extensions:
				return "Additional Extensions (deprecated)";
			case chunk_t::project_path:
				return "Project Path";
			case chunk_t::output_path:
				return "Output Path";
			case chunk_t::app_doc:
				return "App Doc";
			case chunk_t::other_extension:
				return "Other Extension(s)";
			case chunk_t::global_values:
				return "Global Values";
			case chunk_t::global_strings:
				return "Global Strings";
			case chunk_t::extensions_list:
				return "Extensions List";
			case chunk_t::icon:
				return "Icon";
			case chunk_t::demo_version:
				return "Demo Version";
			case chunk_t::security_number:
				return "Security Number";
			case chunk_t::binary_files:
				return "Binary Files";
			case chunk_t::menu_images:
				return "Menu Images";
			case chunk_t::about:
				return "About";
			case chunk_t::copyright:
				return "Copyright";
			case chunk_t::global_value_names:
				return "Global Value Names";
			case chunk_t::global_string_names:
				return "Global String Names";
			case chunk_t::movement_extensions:
				return "Movement Extensions";
			// case chunk_t::UNKNOWN8: return "UNKNOWN8";
			case chunk_t::object_bank2:
				return "Object Bank 2";
			case chunk_t::exe_only:
				return "EXE Only";
			case chunk_t::protection:
				return "Protection";
			case chunk_t::shaders:
				return "Shaders";
			case chunk_t::shaders2:
				return "Shaders 2";
			case chunk_t::extended_header:
				return "Extended Header";
			case chunk_t::spacer:
				return "Spacer";
			case chunk_t::frame_bank:
				return "Frame Bank";
			case chunk_t::chunk224F:
				return "Chunk 224F";
			case chunk_t::title2:
				return "Title2";

			case chunk_t::chunk2253:
				return "Chunk 2253";
			case chunk_t::object_names:
				return "Object Names";
			case chunk_t::chunk2255:
				return "Chunk 2255 (Empty?)";
			case chunk_t::two_five_plus_object_properties:
				return "Object Properties (2.5+)";
			case chunk_t::chunk2257:
				return "Chunk 2257 (4 bytes?)";
			case chunk_t::font_meta:
				return "TrueType Fonts Meta";
			case chunk_t::font_chunk:
				return "TrueType Fonts Chunk";

			case chunk_t::frame:
				return "Frame";
			case chunk_t::frame_header:
				return "Frame Header";
			case chunk_t::frame_name:
				return "Frame Name";
			case chunk_t::frame_password:
				return "Frame Password";
			case chunk_t::frame_palette:
				return "Frame Palette";
			case chunk_t::frame_object_instances:
				return "Frame Object Instances";
			case chunk_t::frame_fade_in_frame:
				return "Frame Fade In Frame";
			case chunk_t::frame_fade_out_frame:
				return "Frame Fade Out Frame";
			case chunk_t::frame_fade_in:
				return "Frame Fade In";
			case chunk_t::frame_fade_out:
				return "Frame Fade Out";
			case chunk_t::frame_events:
				return "Frame Events";
			case chunk_t::frame_play_header:
				return "Frame Play Header";
			case chunk_t::frame_additional_items:
				return "Frame Additional Item";
			case chunk_t::frame_additional_items_instances:
				return "Frame Additional Item Instance";
			case chunk_t::frame_layers:
				return "Frame Layers";
			case chunk_t::frame_virtual_size:
				return "Frame Virtical Size";
			case chunk_t::demo_file_path:
				return "Frame Demo File Path";
			case chunk_t::random_seed:
				return "Frame Random Seed";
			case chunk_t::frame_layer_effect:
				return "Frame Layer Effect";
			case chunk_t::frame_bluray:
				return "Frame BluRay Options";
			case chunk_t::movement_timer_base:
				return "Frame Movement Timer Base";
			case chunk_t::mosaic_image_table:
				return "Frame Mosaic Image Table";
			case chunk_t::frame_effects:
				return "Frame Effects";
			case chunk_t::frame_iphone_options:
				return "Frame iPhone Options";
			case chunk_t::frame_chunk334C:
				return "Frame CHUNK 334C";

			case chunk_t::pa_error:
				return "PA (ERROR)";

			case chunk_t::object_header:
				return "Object Header";
			case chunk_t::object_name:
				return "Object Name";
			case chunk_t::object_properties:
				return "Object Properties";
			case chunk_t::object_chunk4447:
				return "Object CHUNK 4447";
			case chunk_t::object_effect:
				return "Object Effect";

			case chunk_t::image_handles:
				return "Image Handles";
			case chunk_t::font_handles:
				return "Font Handles";
			case chunk_t::sound_handles:
				return "Sound Handles";
			case chunk_t::music_handles:
				return "Music Handles";

			case chunk_t::image_bank:
				return "Image Bank";
			case chunk_t::font_bank:
				return "Font Bank";
			case chunk_t::sound_bank:
				return "Sound Bank";
			case chunk_t::music_bank:
				return "Music Bank";

			case chunk_t::last:
				return "Last";

			case chunk_t::_default:
				[[fallthrough]];
			case chunk_t::vitalise:
				[[fallthrough]];
			case chunk_t::unicode:
				[[fallthrough]];
			case chunk_t::_new:
				[[fallthrough]];
			case chunk_t::old:
				[[fallthrough]];
			case chunk_t::frame_state:
				[[fallthrough]];
			case chunk_t::image_state:
				[[fallthrough]];
			case chunk_t::font_state:
				[[fallthrough]];
			case chunk_t::sound_state:
				[[fallthrough]];
			case chunk_t::music_state:
				[[fallthrough]];
			case chunk_t::no_child:
				[[fallthrough]];
			case chunk_t::skip:
				[[fallthrough]];
			default:
				return "INVALID";
		}
	}

	const char *GetObjectTypeString(object_type_t type)
	{
		switch (type)
		{
			case object_type_t::quick_backdrop:
				return "Quick Backdrop";
			case object_type_t::backdrop:
				return "Backdrop";
			case object_type_t::active:
				return "Active";
			case object_type_t::text:
				return "Text";
			case object_type_t::question:
				return "Question";
			case object_type_t::score:
				return "Score";
			case object_type_t::lives:
				return "Lives";
			case object_type_t::counter:
				return "Counter";
			case object_type_t::RTF:
				return "RTF";
			case object_type_t::sub_application:
				return "Sub Application";
			case object_type_t::player:
				return "Player";
			case object_type_t::keyboard:
				return "Keyboard";
			case object_type_t::create:
				return "Create";
			case object_type_t::timer:
				return "Timer";
			case object_type_t::game:
				return "Game";
			case object_type_t::speaker:
				return "Speaker";
			case object_type_t::system:
				return "System";
			default:
				return "Unknown/Invalid";
		}
	}

	const char *GetObjectParentTypeString(object_parent_type_t type)
	{
		switch (type)
		{
			case object_parent_type_t::none:
				return "None";
			case object_parent_type_t::frame:
				return "Frame";
			case object_parent_type_t::frame_item:
				return "Frame Item";
			case object_parent_type_t::qualifier:
				return "Qualifier";
			default:
				return "Invalid";
		}
	}

	lak::astring GetImageFlagString(image_flag_t flags)
	{
		lak::astring result;

		if (flags == image_flag_t::none)
			result = "none";
		else
		{
			if ((flags & image_flag_t::RLE) != image_flag_t::none)
				result += "RLE | ";
			if ((flags & image_flag_t::RLEW) != image_flag_t::none)
				result += "RLEW | ";
			if ((flags & image_flag_t::RLET) != image_flag_t::none)
				result += "RLET | ";
			if ((flags & image_flag_t::LZX) != image_flag_t::none)
				result += "LZX | ";
			if ((flags & image_flag_t::alpha) != image_flag_t::none)
				result += "alpha | ";
			if ((flags & image_flag_t::ace) != image_flag_t::none)
				result += "ace | ";
			if ((flags & image_flag_t::mac) != image_flag_t::none)
				result += "mac | ";
			if ((flags & image_flag_t::RGBA) != image_flag_t::none)
				result += "RGBA | ";

			result.resize(result.size() - 3);
		}

		return result;
	}

	lak::astring GetBuildFlagsString(build_flags_t flags)
	{
		lak::astring result;

		if (flags == build_flags_t::none)
			result = "none";
		else
		{
			if ((flags & build_flags_t::compression_level_max) !=
			    build_flags_t::none)
				result += "compression_level_max | ";
			if ((flags & build_flags_t::compress_sounds) != build_flags_t::none)
				result += "compress_sounds | ";
			if ((flags & build_flags_t::include_external_files) !=
			    build_flags_t::none)
				result += "include_external_files | ";
			if ((flags & build_flags_t::no_auto_image_filters) !=
			    build_flags_t::none)
				result += "no_auto_image_filters | ";
			if ((flags & build_flags_t::no_auto_sound_filters) !=
			    build_flags_t::none)
				result += "no_auto_sound_filters | ";
			if ((flags & build_flags_t::unknown1) != build_flags_t::none)
				result += "unknown1 | ";
			if ((flags & build_flags_t::unknown2) != build_flags_t::none)
				result += "unknown2 | ";
			if ((flags & build_flags_t::unknown3) != build_flags_t::none)
				result += "unknown3 | ";
			if ((flags & build_flags_t::dont_display_build_warnings) !=
			    build_flags_t::none)
				result += "dont_display_build_warnings | ";
			if ((flags & build_flags_t::optimize_image_size) != build_flags_t::none)
				result += "optimize_image_size | ";

			result.resize(result.size() - 3);
		}

		return result;
	}

	result_t<data_ref_span_t> Decode(data_ref_span_t encoded,
	                                 chunk_t id,
	                                 encoding_t mode)
	{
		FUNCTION_CHECKPOINT();

		switch (mode)
		{
			case encoding_t::mode3:
			case encoding_t::mode2:
				return Decrypt(encoded, id, mode);
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
		auto inflater = lak::deflate_iterator(
		  compressed,
		  buffer,
		  !skip_header ? lak::deflate_iterator::header_t::zlib
		               : lak::deflate_iterator::header_t::none,
		  anaconda);

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
			return lak::err_t{error(error_type::inflate_failed,
			                        lak::streamify("Failed To Inflate (",
			                                       lak::deflate_iterator::error_name(
			                                         err.unsafe_unwrap_err()),
			                                       ")"))};
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
		  .RES_MAP_TO_TRACE(error_type::inflate_failed);
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
		auto inflater =
		  lak::deflate_iterator(strm.remaining(),
		                        buffer,
		                        lak::deflate_iterator::header_t::none,
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
			const auto offset     = strm.position();
			const auto bytes_read = size_t(inflater.input_consumed());
			ASSERT_GREATER_OR_EQUAL(bytes_read, 0U);
			strm.skip(bytes_read).UNWRAP();
			return lak::ok_t{make_data_ref_ptr(
			  strm._source, offset, bytes_read, lak::move(output))};
		}
		else
		{
			DEBUG("Out Size: ", out_size);
			DEBUG("Final? ", (inflater.is_final_block() ? "True" : "False"));
			return lak::err_t{error(error_type::inflate_failed,
			                        lak::streamify("Failed To Inflate (",
			                                       lak::deflate_iterator::error_name(
			                                         err.unsafe_unwrap_err()),
			                                       ")"))};
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
				  error(error_type::decrypt_failed,
				        u8"MODE 3 Decryption Failed: Encrypted Buffer Too Small")};
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
					  error(error_type::decrypt_failed,
					        u8"MODE 3 Decryption Failed: Decoded Chunk Too Small")};
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
			  error(error_type::decrypt_failed, u8"MODE 3 Decryption Failed")};
		}
		else
		{
			if (encrypted.size() < 1)
				return lak::err_t{
				  error(error_type::decrypt_failed,
				        u8"MODE 2 Decryption Failed: Encrypted Buffer Too Small")};

			auto mem_ptr  = estrm.copy_remaining();
			auto mem_span = lak::span<byte_t>(mem_ptr->get());

			if ((_mode != game_mode_t::_284) && (uint16_t)ID & 0x1)
				(uint8_t &)(mem_span[0]) ^=
				  ((uint16_t)ID & 0xFF) ^ ((uint16_t)ID >> 0x8);

			if (!DecodeChunk(mem_span))
			{
				if (mode == encoding_t::mode2)
				{
					return lak::err_t{
					  error(error_type::decrypt_failed, u8"MODE 2 Decryption Failed")};
				}
			}

			return lak::ok_t{mem_ptr};
		}
	}

	result_t<frame::item_t &> GetFrame(game_t &game, uint16_t handle)
	{
		if (!game.game.frame_bank) return lak::err_t{error(u8"No Frame Bank")};

		if (!game.game.frame_handles)
			return lak::err_t{error(u8"No Frame Handles")};

		if (handle >= game.game.frame_handles->handles.size())
			return lak::err_t{error(u8"Frame Handle Out Of Range")};

		handle = game.game.frame_handles->handles[handle];

		if (handle >= game.game.frame_bank->items.size())
			return lak::err_t{error(u8"Frame Bank Handle Out Of Range")};

		return lak::ok_t{game.game.frame_bank->items[handle]};
	}

	result_t<object::item_t &> GetObject(game_t &game, uint16_t handle)
	{
		if (!game.game.object_bank) return lak::err_t{error(u8"No Object Bank")};

		auto iter = game.object_handles.find(handle);

		if (iter == game.object_handles.end())
			return lak::err_t{error(u8"Invalid Object Handle")};

		if (iter->second >= game.game.object_bank->items.size())
			return lak::err_t{error(u8"Object Bank Handle Out Of Range")};

		return lak::ok_t{game.game.object_bank->items[iter->second]};
	}

	result_t<image::item_t &> GetImage(game_t &game, uint32_t handle)
	{
		if (!game.game.image_bank) return lak::err_t{error(u8"No Image Bank")};

		auto iter = game.image_handles.find(handle);

		if (iter == game.image_handles.end())
			return lak::err_t{error(u8"Invalid Image Handle")};

		if (iter->second >= game.game.image_bank->items.size())
			return lak::err_t{error(u8"Image Bank Handle Out Of Range")};

		return lak::ok_t{game.game.image_bank->items[iter->second]};
	}

	result_t<std::u16string> ReadStringEntry(game_t &game,
	                                         const chunk_entry_t &entry)
	{
		if (game.old_game)
		{
			switch (entry.mode)
			{
				case encoding_t::mode0:
					[[fallthrough]];
				case encoding_t::mode1:
					return entry.decode_body()
					  .map(
					    [&](const data_ref_span_t &ref_span)
					    {
						    data_reader_t reader(ref_span);
						    return lak::to_u16string(reader.read_any_c_str<char>());
					    })
					  .RES_ADD_TRACE("Invalid String Chunk: ",
					                 (int)entry.mode,
					                 ", Chunk: ",
					                 (int)entry.ID);

				default:
					return lak::err_t{error(error_type::invalid_mode,
					                        lak::streamify("Invalid String Mode: ",
					                                       (int)entry.mode,
					                                       ", Chunk: ",
					                                       (int)entry.ID))};
			}
		}
		else
		{
			return entry.decode_body()
			  .map(
			    [&](const data_ref_span_t &ref_span)
			    {
				    data_reader_t reader(ref_span);
				    if (game.cruf && game.unicode)
					    return lak::to_u16string(reader.read_any_c_str<char8_t>());
				    else if (game.unicode)
					    return reader.read_any_c_str<char16_t>();
				    else
					    return lak::to_u16string(reader.read_any_c_str<char>());
			    })
			  .RES_ADD_TRACE("Invalid String Chunk: ",
			                 (int)entry.mode,
			                 ", Chunk: ",
			                 (int)entry.ID);
		}
	}
}
