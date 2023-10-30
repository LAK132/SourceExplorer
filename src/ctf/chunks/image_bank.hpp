#ifndef SRCEXP_CTF_CHUNKS_IMAGE_BANK_HPP
#define SRCEXP_CTF_CHUNKS_IMAGE_BANK_HPP

#include "basic.hpp"

namespace SourceExplorer
{
	namespace image
	{
		struct item_t : public basic_item_t
		{
			uint32_t checksum; // uint16_t for old
			uint32_t reference;
			uint32_t data_size;
			lak::vec2u16_t size;
			graphics_mode_t graphics_mode; // uint8_t
			image_flag_t flags;            // uint8_t
			uint16_t unknown;              // not for old
			lak::vec2u16_t hotspot;
			lak::vec2u16_t action;
			lak::color4_t transparent; // not for old
			size_t data_position;

			uint16_t padding;
			uint16_t alpha_padding;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;

			result_t<data_ref_span_t> image_data() const;
			bool need_palette() const;
			result_t<lak::image4_t> image(
			  const bool color_transparent,
			  const lak::color4_t palette[256] = nullptr) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			lak::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}
}

#endif
