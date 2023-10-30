#ifndef SRCEXP_CTF_CHUNKS_FRAME_BANK_HPP
#define SRCEXP_CTF_CHUNKS_FRAME_BANK_HPP

#include "basic.hpp"

#include "last.hpp"
#include "string.hpp"

#include <lak/memory.hpp>

namespace SourceExplorer
{
	namespace frame
	{
		struct header_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct password_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct palette_t : public basic_chunk_t
		{
			uint32_t unknown;
			lak::array<lak::color4_t, 256> colors;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;

			lak::image4_t image() const;
		};

		struct object_instance_t
		{
			uint16_t handle;
			uint16_t info;
			lak::vec2i32_t position;
			object_parent_type_t parent_type;
			uint16_t parent_handle;
			uint16_t layer;
			uint16_t unknown;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct object_instances_t : public basic_chunk_t
		{
			lak::array<object_instance_t> objects;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_in_frame_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_out_frame_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_in_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_out_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct events_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct play_header_r : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct additional_item_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct additional_item_instance_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct layers_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct virtual_size_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct demo_file_path_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct random_seed_t : public basic_chunk_t
		{
			int16_t value;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct layer_effect_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct blueray_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct movement_time_base_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct mosaic_image_table_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct effects_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct iphone_options_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct chunk_334C_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct item_t : public basic_chunk_t
		{
			lak::unique_ptr<string_chunk_t> name;
			lak::unique_ptr<header_t> header;
			lak::unique_ptr<password_t> password;
			lak::unique_ptr<palette_t> palette;
			lak::unique_ptr<object_instances_t> object_instances;
			lak::unique_ptr<fade_in_frame_t> fade_in_frame;
			lak::unique_ptr<fade_out_frame_t> fade_out_frame;
			lak::unique_ptr<fade_in_t> fade_in;
			lak::unique_ptr<fade_out_t> fade_out;
			lak::unique_ptr<events_t> events;
			lak::unique_ptr<play_header_r> play_head;
			lak::unique_ptr<additional_item_t> additional_item;
			lak::unique_ptr<additional_item_instance_t> additional_item_instance;
			lak::unique_ptr<layers_t> layers;
			lak::unique_ptr<virtual_size_t> virtual_size;
			lak::unique_ptr<demo_file_path_t> demo_file_path;
			lak::unique_ptr<random_seed_t> random_seed;
			lak::unique_ptr<layer_effect_t> layer_effect;
			lak::unique_ptr<blueray_t> blueray;
			lak::unique_ptr<movement_time_base_t> movement_time_base;
			lak::unique_ptr<mosaic_image_table_t> mosaic_image_table;
			lak::unique_ptr<effects_t> effects;
			lak::unique_ptr<iphone_options_t> iphone_options;
			lak::unique_ptr<chunk_334C_t> chunk334C;
			lak::unique_ptr<last_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct handles_t : public basic_chunk_t
		{
			lak::array<uint16_t> handles;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}
}

#endif
