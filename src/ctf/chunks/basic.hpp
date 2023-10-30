#ifndef SRCEXP_CTF_CHUNKS_BASIC_HPP
#define SRCEXP_CTF_CHUNKS_BASIC_HPP

#include "../common.hpp"

namespace SourceExplorer
{
	struct basic_entry_t
	{
		union
		{
			uint32_t handle;
			chunk_t ID;
		};
		encoding_t mode;
		bool old;

		data_ref_span_t ref_span;
		data_point_t head;
		data_point_t body;

		size_t position() const
		{
			return lak::ok_or_err(ref_span.position().map_err([](auto &&) -> size_t
			                                                  { return SIZE_MAX; }));
		}

		const data_ref_span_t &raw_head() const;
		const data_ref_span_t &raw_body() const;
		result_t<data_ref_span_t> decode_head(size_t max_size = SIZE_MAX) const;
		result_t<data_ref_span_t> decode_body(size_t max_size = SIZE_MAX) const;
	};

	struct chunk_entry_t : public basic_entry_t
	{
		error_t read(game_t &game, data_reader_t &strm);
		void view(source_explorer_t &srcexp) const;
	};

	struct item_entry_t : public basic_entry_t
	{
		bool new_item = false;

		void read_init(game_t &game);

		error_t read_head(game_t &game,
		                  data_reader_t &strm,
		                  size_t size,
		                  bool has_handle = true);

		error_t read_body(game_t &game,
		                  data_reader_t &strm,
		                  bool compressed,
		                  lak::optional<size_t> size = lak::nullopt);

		// calls read_init, read_head and read_body automatically
		error_t read(game_t &game,
		             data_reader_t &strm,
		             bool compressed,
		             size_t headersize = 0,
		             bool has_handle   = true);
		void view(source_explorer_t &srcexp) const;
	};

	struct basic_chunk_t
	{
		chunk_entry_t entry;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(source_explorer_t &srcexp, const char *name) const;
	};

	struct basic_item_t
	{
		item_entry_t entry;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(source_explorer_t &srcexp, const char *name) const;
	};
}

#endif
