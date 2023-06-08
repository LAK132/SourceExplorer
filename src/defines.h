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

#ifndef DEFINES_H
#define DEFINES_H

#include <lak/string.hpp>

// Executable Signature
static const uint16_t WIN_EXE_SIG = 0x5A'4D;
// Position of Pointer to PE Header
static const uint8_t WIN_EXE_PNT = 0x3C;
// PE Signature
static const uint32_t WIN_PE_SIG = 0x00'00'45'50;

namespace SourceExplorer
{
	//
	// Game Headers
	//
	static const uint64_t HEADER_PACK = 0x12'47'87'49'77'77'77'77;
	// PAMU
	static const uint32_t HEADER_UNIC = 0x55'4D'41'50;
	// PAME
	static const uint32_t HEADER_GAME = 0x45'4D'41'50;
	// CRUF
	static const uint32_t HEADER_CRUF = 0x46'55'52'43;

	//
	// Product code
	//
	enum class product_code_t : uint16_t
	{
		CNCV1VER = 0x0207,
		MMF1     = 0x0300,
		MMF15    = 0x0301,
		MMF2     = 0x0302
	};

	//
	// Old Data Chunks
	//
	enum class old_chunk_type_t : uint16_t
	{
		old_headers     = 0x2223,
		old_frame_items = 0x2229,
		// Dummy in some games (PA2)
		old_frame_items_2 = 0x223F,
		old_frame         = 0x3333,
		old_frame_header  = 0x3334,
		// Object Instances
		old_obj_inst = 0x3338,
		// Frame Events
		old_frame_events = 0x333D,
		// Object Properties
		old_obj_prop = 0x4446
	};

	//
	// New Data Chunks
	//
	enum class chunk_t : uint16_t
	{
		//
		// States
		//
		_default    = 0x00,
		vitalise    = 0x11,
		unicode     = 0x23,
		_new        = 0x22,
		old         = 0x20,
		frame_state = 0x33,
		image_state = 0x66,
		font_state  = 0x67,
		sound_state = 0x68,
		music_state = 0x69,
		no_child    = 0x80,
		skip        = 0x81,

		entry = 0x0302,

		// Vitalise Chunks (0x11XX)
		vitalise_preview = 0x1122,

		// App Chunks (0x22XX)
		header                = 0x2223,
		title                 = 0x2224,
		author                = 0x2225,
		menu                  = 0x2226,
		extra_path            = 0x2227,
		extensions            = 0x2228, // Deprecated // Not in Anaconda
		object_bank           = 0x2229, // AKA FRAMEITEMS
		global_events         = 0x222A, // Deprecated // Not in Anaconda
		frame_handles         = 0x222B,
		extra_data            = 0x222C,
		additional_extensions = 0x222D, // Deprecated // Not in Anaconda
		project_path          = 0x222E, // Used for encryption
		output_path           = 0x222F,
		app_doc               = 0x2230,
		other_extension       = 0x2231,
		global_values         = 0x2232,
		global_strings        = 0x2233,
		extensions_list       = 0x2234,
		icon                  = 0x2235,
		demo_version          = 0x2236, // Not in Anaconda
		security_number       = 0x2237,
		binary_files          = 0x2238,
		menu_images           = 0x2239, // Not in Anaconda
		about                 = 0x223A,
		copyright             = 0x223B,
		global_value_names    = 0x223C, // Not in Anaconda
		global_string_names   = 0x223D, // Not in Anaconda
		movement_extensions   = 0x223E,
		// UNKNOWN8        = 0x223F,
		object_bank2 = 0x223F, // AKA FRAMEITEMS2
		exe_only     = 0x2240,
		// = 0x2241
		protection = 0x2242,
		shaders    = 0x2243,
		// = 0x2244
		extended_header = 0x2245,
		spacer          = 0x2246,
		frame_bank      = 0x224D, // Means FRAMEHANDLES might be broken. Actually
		                          // probably the Frame Bank
		chunk224F    = 0x224F,
		title2       = 0x2251,    // "StringChunk" ?
		chunk2253    = 0x2253,    // 16 bytes
		object_names = 0x2254,    // 2.5+ games only (?), array of null
		                          // terminated strings. "Empty"
		chunk2255                       = 0x2255,
		two_five_plus_object_properties = 0x2256, // 2.5+ games only (?)
		chunk2257                       = 0x2257, // 4 bytes
		font_meta                       = 0x2258, // 2.5+ games only (?)
		font_chunk                      = 0x2259, // 2.5+ games only (?)
		shaders2                        = 0x225A, // 2.5+ games only (?)

		// Frame Chunks (0x33XX)
		frame          = 0x3333,
		frame_header   = 0x3334,
		frame_name     = 0x3335,
		frame_password = 0x3336,
		frame_palette  = 0x3337,
		// OBJNAME2        = 0x3337, // "WTF"?
		frame_object_instances = 0x3338,
		frame_fade_in_frame    = 0x3339, // Not in Anaconda
		frame_fade_out_frame   = 0x333A, // Not in Anaconda
		frame_fade_in          = 0x333B,
		frame_fade_out         = 0x333C,

		frame_events                     = 0x333D,
		frame_play_header                = 0x333E,
		frame_additional_items           = 0x333F, // Not in Anaconda
		frame_additional_items_instances = 0x3340, // Not in Anaconda
		frame_layers                     = 0x3341,
		frame_virtual_size               = 0x3342,
		demo_file_path                   = 0x3343,
		random_seed                      = 0x3344, // Not in Anaconda
		frame_layer_effect               = 0x3345,
		frame_bluray                     = 0x3346, // Not in Anaconda
		movement_timer_base              = 0x3347,
		mosaic_image_table               = 0x3348, // Not in Anaconda
		frame_effects                    = 0x3349,
		frame_iphone_options             = 0x334A, // Not in Anaconda
		frame_chunk334C                  = 0x334C,

		// Object Chunks (0x44XX)
		pa_error          = 0x4150, // Not a data chunk
		object_header     = 0x4444,
		object_name       = 0x4445,
		object_properties = 0x4446,
		object_chunk4447  = 0x4447, // Not in Anaconda
		object_effect     = 0x4448,

		// Offset Chunks (0x55XX)
		image_handles = 0x5555,
		font_handles  = 0x5556,
		sound_handles = 0x5557,
		music_handles = 0x5558,

		// Bank Chunks (0x66XX)
		image_bank = 0x6666,
		font_bank  = 0x6667,
		sound_bank = 0x6668,
		music_bank = 0x6669,

		// Last Chunk
		last = 0x7F7F
	};

	//
	// Modes
	//
	enum class encoding_t : uint16_t
	{
		mode0 = 0, // Uncompressed / Unencrypted
		mode1 = 1, // Compressed / Unencrypted
		mode2 = 2, // Uncompressed / Encrypted
		mode3 = 3, // Compressed / Encrypted
		mode4 = 4, // LZ4 Compressed (not actually a value read from the binary)

		default_mode    = mode0,
		compressed_mode = mode1,
		encrypted_mode  = mode2,
	};

	//
	// Sound Modes
	//
	enum class sound_mode_t : uint32_t
	{
		wave = 1 << 0,
		midi = 1 << 1,
		oggs = 1 << 2,
		// 1 << 3,
		loc    = 1 << 4,
		pfd    = 1 << 5,
		loaded = 1 << 6,
		xm     = 1 << 30,
	};

	// static sound_mode_t operator | (const sound_mode_t A, const sound_mode_t
	// B)
	// {
	//     return (sound_mode_t)((uint32_t)A | (uint32_t)B);
	// }

	//
	// Frame Header Data Flags
	//
	enum class frame_header_flag_t : uint16_t
	{
		display_name         = 1 << 0,
		grab_desktop         = 1 << 1,
		keep_display         = 1 << 2,
		fade_in              = 1 << 3,
		fade_out             = 1 << 4,
		total_collision_mask = 1 << 5,
		password             = 1 << 6,
		resize_at_start      = 1 << 7,
		do_not_center        = 1 << 8,
		force_load_on_call   = 1 << 9,
		no_surface           = 1 << 10,
		reserved1            = 1 << 11,
		reserved2            = 1 << 12,
		record_demo          = 1 << 13,
		// 1 << 14,
		timed_movements = 1 << 15
	};

	// static frame_header_flag_t operator | (const frame_header_flag_t A, const
	// frame_header_flag_t B)
	// {
	//     return (frame_header_flag_t)((uint16_t)A | (uint16_t)B);
	// }

	//
	// Object Property Flags
	//
	enum class object_type_t : uint8_t // int8_t
	{
		quick_backdrop  = 0x00,
		backdrop        = 0x01,
		active          = 0x02,
		text            = 0x03,
		question        = 0x04,
		score           = 0x05,
		lives           = 0x06,
		counter         = 0x07,
		RTF             = 0x08,
		sub_application = 0x09,
		player          = 0xF9, // = -7,
		keyboard        = 0xFA, // = -6,
		create          = 0xFB, // = -5,
		timer           = 0xFC, // = -4,
		game            = 0xFD, // = -3,
		speaker         = 0xFE, // = -2,
		system          = 0xFF, // = -1,
	};

	enum class object_parent_type_t : uint16_t
	{
		none       = 0,
		frame      = 1,
		frame_item = 2,
		qualifier  = 3
	};

	enum class shape_type_t : uint16_t
	{
		line      = 1,
		rectangle = 2,
		ellipse   = 3
	};

	enum class fill_type_t : uint16_t
	{
		none     = 0,
		solid    = 1,
		gradient = 2,
		motif    = 3
	};

	enum class line_flags_t : uint16_t
	{
		none      = 0,
		inverse_X = 1 << 0,
		inverse_Y = 1 << 1
	};

	enum class gradient_flags_t : uint16_t
	{
		horizontal = 0,
		vertical   = 1,
	};

	//
	// Image Properties
	//
	enum class pixel_layout_t : uint8_t
	{
		mono,
		r,
		rg,
		rgb,
		bgr,
		rgbx,
		bgrx,
		xrgb,
		xbgr,
		bayer_rggb,
		bayer_bggr,
		bayer_grbg,
		bayer_gbrg,
	};

	enum class graphics_mode_t : uint8_t
	{
		RGBA32,
		BGRA32,
		RGB24,
		BGR24,
		RGB16,
		RGB15,
		RGB8,
		JPEG,
	};

	enum class image_flag_t : uint8_t
	{
		none  = 0,
		RLE   = 1 << 0, // 0x1
		RLEW  = 1 << 1, // 0x2
		RLET  = 1 << 2, // 0x4
		LZX   = 1 << 3, // 0x8
		alpha = 1 << 4, // 0x10
		ace   = 1 << 5, // 0x20
		mac   = 1 << 6, // 0x40
		RGBA  = 1 << 7, // 0x80
	};

	inline image_flag_t operator|(const image_flag_t A, const image_flag_t B)
	{
		return (image_flag_t)((uint8_t)A | (uint8_t)B);
	}

	inline image_flag_t operator&(const image_flag_t A, const image_flag_t B)
	{
		return (image_flag_t)((uint8_t)A & (uint8_t)B);
	}
}

#endif