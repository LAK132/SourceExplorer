#ifndef SRCEXP_CTF_CHUNKS_OBJECT_BANK_HPP
#define SRCEXP_CTF_CHUNKS_OBJECT_BANK_HPP

#include "basic.hpp"

#include "last.hpp"
#include "string.hpp"

#include <lak/string.hpp>

template<>
void lak::swap<lak::array<lak::u16string>>(
  lak::array<lak::u16string> &a, lak::array<lak::u16string> &b) = delete;

#include <unordered_map>

namespace SourceExplorer
{
	namespace object
	{
		struct effect_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct shape_t
		{
			fill_type_t fill;
			shape_type_t shape;
			line_flags_t line;
			gradient_flags_t gradient;
			uint16_t border_size;
			lak::color4_t border_color;
			lak::color4_t color1, color2;
			uint16_t handle;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct quick_backdrop_t : public basic_chunk_t
		{
			uint32_t size;
			uint16_t obstacle;
			uint16_t collision;
			lak::vec2u32_t dimension;
			shape_t shape;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct backdrop_t : public basic_chunk_t
		{
			uint32_t size;
			uint16_t obstacle;
			uint16_t collision;
			lak::vec2u32_t dimension;
			uint16_t handle;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct animation_direction_t
		{
			lak::array<uint16_t> handles;
			uint8_t min_speed;
			uint8_t max_speed;
			uint16_t repeat;
			uint16_t back_to;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct animation_t
		{
			lak::array<uint16_t, 32> offsets;
			lak::array<animation_direction_t, 32> directions;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct animation_header_t
		{
			uint16_t size;
			lak::array<uint16_t> offsets;
			lak::array<animation_t> animations;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct common_t : public basic_chunk_t
		{
			uint32_t size;

			uint16_t movements_offset;
			uint16_t animations_offset;
			uint16_t counter_offset;
			uint16_t system_offset;
			uint32_t fade_in_offset;
			uint32_t fade_out_offset;
			uint16_t values_offset;
			uint16_t strings_offset;
			uint16_t extension_offset;

			lak::unique_ptr<animation_header_t> animations;

			uint16_t version;
			uint32_t flags;
			uint32_t new_flags;
			uint32_t preferences;
			uint32_t identifier;
			lak::color4_t back_color;

			game_mode_t mode;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		// ObjectInfo + ObjectHeader
		struct item_t : public basic_chunk_t // OBJHEAD
		{
			lak::unique_ptr<string_chunk_t> name;
			lak::unique_ptr<effect_t> effect;
			lak::unique_ptr<last_t> end;

			uint16_t handle;
			object_type_t type;
			uint32_t ink_effect;
			uint32_t ink_effect_param;

			lak::unique_ptr<quick_backdrop_t> quick_backdrop;
			lak::unique_ptr<backdrop_t> backdrop;
			lak::unique_ptr<common_t> common;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;

			std::unordered_map<uint32_t, lak::array<lak::u16string>> image_handles()
			  const;
		};

		// aka FrameItems
		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

}

#endif
