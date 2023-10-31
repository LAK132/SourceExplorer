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

#ifndef EXPLORER_H
#define EXPLORER_H

#include "../data_reader.hpp"
#include "../data_ref.hpp"

#include "common.hpp"

#include "chunks/header.hpp"

#include "../imgui_utils.hpp"
#include <imgui_memory_editor.h>
#include <misc/cpp/imgui_stdlib.h>
#include <misc/softraster/texture.h>

#include "stb_image.h"

#include <lak/binary_reader.hpp>
#include <lak/binary_writer.hpp>
#include <lak/debug.hpp>
#include <lak/file.hpp>
#include <lak/imgui/backend.hpp>
#include <lak/imgui/widgets.hpp>
#include <lak/lmdb/lmdb.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>
#include <lak/result.hpp>
#include <lak/stdint.hpp>
#include <lak/strconv.hpp>
#include <lak/string.hpp>
#include <lak/string_literals.hpp>
#include <lak/tinflate.hpp>
#include <lak/trace.hpp>
#include <lak/unicode.hpp>

#include <assert.h>
#include <atomic>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <stdint.h>
#include <unordered_map>

namespace SourceExplorer
{
	extern size_t max_item_read_fails;

	struct image_section_header_t
	{
		lak::astring name;
		uint32_t size;
		uint32_t addr;
	};

	struct game_t
	{
		static std::atomic<float> completed;
		static std::atomic<float> bank_completed;

		lak::astring game_path;
		lak::astring game_dir;

		data_ref_ptr_t file;

		lak::array<pack_file_t> pack_files;
		uint64_t data_pos;
		uint16_t num_header_sections;
		uint16_t num_sections;

		product_code_t runtime_version;
		uint16_t runtime_sub_version;
		uint32_t product_version;
		uint32_t product_build;

		lak::array<chunk_t> state;

		bool unicode            = false;
		bool old_game           = false;
		bool compat             = false;
		bool cnc                = false;
		bool recompiled         = false;
		bool two_five_plus_game = false;
		bool ccn                = false;
		bool cruf               = false;
		lak::array<uint8_t> protection;

		header_t game;

		lak::u16string project;
		lak::u16string title;
		lak::u16string copyright;

		std::unordered_map<uint32_t, size_t> image_handles;
		std::unordered_map<uint16_t, size_t> object_handles;
	};

	struct file_state_t
	{
		fs::path path;
		bool valid;
		bool attempt;

		// bad() = file is neither valid nor being attempted
		// !bad() = file is either valid or is being attempted
		bool bad() const { return !valid && !attempt; }

		// good() = file is valid and not being attempted
		// !good() = file is either invalid or being attempted
		bool good() const { return valid && !attempt; }

		void make_attempt()
		{
			valid   = false;
			attempt = true;
		}
	};

	struct source_explorer_t
	{
		lak::graphics_mode graphics_mode;

		game_t state;

		bool loaded                 = false;
		bool loaded_successfully    = false;
		bool baby_mode              = true;
		bool dump_color_transparent = true;
		bool allow_multithreading   = false;
		file_state_t exe;
		file_state_t images;
		file_state_t sorted_images;
		file_state_t sounds;
		file_state_t music;
		file_state_t shaders;
		file_state_t binary_files;
		file_state_t appicon;
		file_state_t error_log;
		file_state_t binary_block;
		file_state_t testing;
		file_state_t write;
		file_state_t database;

		MemoryEditor editor;

		lak::array<fs::path> testing_files;
		lak::optional<lak::lmdb::environment> db_env;

		const basic_entry_t *view = nullptr;
		texture_t image;
		data_ref_span_t buffer;
	};

	error_t LoadGame(source_explorer_t &srcexp);

	void GetEncryptionKey(game_t &game_state);

	result_t<image_section_header_t> ParseImageSectionHeader(
	  data_reader_t &strm);

	error_t ParsePEHeader(data_reader_t &strm);

	error_t ParseGameHeader(data_reader_t &strm, game_t &game_state);

	result_t<size_t> ParsePackData(data_reader_t &strm, game_t &game_state);

	lak::color4_t ColorFrom8bitRGB(uint8_t RGB);

	lak::color4_t ColorFrom8bitA(uint8_t A);

	lak::color4_t ColorFrom15bitRGB(uint16_t RGB);

	lak::color4_t ColorFrom16bitRGB(uint16_t RGB);

	result_t<lak::color4_t> ColorFrom8bitI(data_reader_t &strm,
	                                       const lak::color4_t palette[256]);

	result_t<lak::color4_t> ColorFrom15bitRGB(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom16bitRGB(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom24bitBGR(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom32bitBGRA(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom32bitBGR(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom24bitRGB(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom32bitRGBA(data_reader_t &strm);

	result_t<lak::color4_t> ColorFrom32bitRGB(data_reader_t &strm);

	result_t<lak::color4_t> ColorFromMode(data_reader_t &strm,
	                                      const graphics_mode_t mode,
	                                      const lak::color4_t palette[256]);

	void ColorsFrom8bitRGB(lak::span<lak::color4_t> colors,
	                       lak::span<const byte_t> RGB);

	void ColorsFrom8bitA(lak::span<lak::color4_t> colors,
	                     lak::span<const byte_t> A);

	void MaskFrom8bitA(lak::span<lak::color4_t> colors,
	                   lak::span<const byte_t> A);

	void ColorsFrom8bitI(lak::span<lak::color4_t> colors,
	                     lak::span<const byte_t> index,
	                     lak::span<const lak::color4_t, 256> palette);

	void ColorsFrom15bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB);

	void ColorsFrom16bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB);

	void ColorsFrom24bitBGR(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> BGR);

	void ColorsFrom32bitBGR(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> BGR);

	void ColorsFrom32bitBGRA(lak::span<lak::color4_t> colors,
	                         lak::span<const byte_t> BGR);

	void ColorsFrom24bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB);

	void ColorsFrom32bitRGB(lak::span<lak::color4_t> colors,
	                        lak::span<const byte_t> RGB);

	void ColorsFrom32bitRGBA(lak::span<lak::color4_t> colors,
	                         lak::span<const byte_t> RGB);

	error_t ColorsFrom8bitRGB(lak::span<lak::color4_t> colors,
	                          data_reader_t &strm);

	error_t ColorsFrom8bitA(lak::span<lak::color4_t> colors,
	                        data_reader_t &strm);

	error_t MaskFrom8bitA(lak::span<lak::color4_t> colors, data_reader_t &strm);

	error_t ColorsFrom8bitI(lak::span<lak::color4_t> colors,
	                        data_reader_t &strm,
	                        const lak::color4_t palette[256]);

	error_t ColorsFrom15bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm);

	error_t ColorsFrom16bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm);

	error_t ColorsFrom24bitBGR(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm);

	error_t ColorsFrom32bitBGRA(lak::span<lak::color4_t> colors,
	                            data_reader_t &strm);

	error_t ColorsFrom32bitBGR(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm);

	error_t ColorsFrom24bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm);

	error_t ColorsFrom32bitRGBA(lak::span<lak::color4_t> colors,
	                            data_reader_t &strm);

	error_t ColorsFrom32bitRGB(lak::span<lak::color4_t> colors,
	                           data_reader_t &strm);

	error_t ColorsFromMode(lak::span<lak::color4_t> colors,
	                       data_reader_t &strm,
	                       const graphics_mode_t mode,
	                       const lak::color4_t palette[256]);

	uint8_t ColorModeSize(const graphics_mode_t mode);

	// uint16_t BitmapPaddingSize(uint16_t width,
	//                            uint8_t col_size,
	//                            uint8_t bytes = 2);

	// uint16_t BitmapPaddingSize(uint16_t width,
	//                            uint8_t col_size,
	//                            uint8_t bytes = 4);

	result_t<size_t> ReadRLE(data_reader_t &strm,
	                         lak::image4_t &bitmap,
	                         graphics_mode_t mode,
	                         uint16_t padding,
	                         const lak::color4_t palette[256]);

	result_t<size_t> ReadRGB(data_reader_t &strm,
	                         lak::image4_t &bitmap,
	                         graphics_mode_t mode,
	                         uint16_t padding,
	                         const lak::color4_t palette[256]);

	result_t<size_t> ReadAlpha(data_reader_t &strm,
	                           lak::image4_t &bitmap,
	                           uint16_t padding);

	void ReadTransparent(const lak::color4_t &transparent,
	                     lak::image4_t &bitmap);

	texture_t CreateTexture(const lak::image4_t &bitmap,
	                        const lak::graphics_mode mode);

	texture_t CreateTexture(const lak::image<float> &bitmap,
	                        const lak::graphics_mode mode);

	void ViewImage(source_explorer_t &srcexp, const float scale = 1.0f);

	const char *GetTypeString(chunk_t ID);

	const char *GetObjectTypeString(object_type_t type);

	const char *GetObjectParentTypeString(object_parent_type_t type);

	lak::astring GetImageFlagString(image_flag_t flags);

	lak::astring GetBuildFlagsString(build_flags_t flags);

	result_t<data_ref_span_t> Decode(data_ref_span_t encoded,
	                                 chunk_t ID,
	                                 encoding_t mode);

	result_t<data_ref_span_t> Inflate(data_ref_span_t compressed,
	                                  bool skip_header,
	                                  bool anaconda,
	                                  size_t max_size = SIZE_MAX);

	result_t<data_ref_span_t> LZ4Decode(data_ref_span_t compressed,
	                                    unsigned int out_size);

	result_t<data_ref_span_t> LZ4DecodeReadSize(data_ref_span_t compressed);

	result_t<data_ref_span_t> StreamDecompress(data_reader_t &strm,
	                                           unsigned int out_size);

	result_t<data_ref_span_t> Decrypt(data_ref_span_t encrypted,
	                                  chunk_t ID,
	                                  encoding_t mode);

	result_t<frame::item_t &> GetFrame(game_t &game, uint16_t handle);

	result_t<object::item_t &> GetObject(game_t &game, uint16_t handle);

	result_t<image::item_t &> GetImage(game_t &game, uint32_t handle);

	result_t<std::u16string> ReadStringEntry(game_t &game,
	                                         const chunk_entry_t &entry);
}
#endif
